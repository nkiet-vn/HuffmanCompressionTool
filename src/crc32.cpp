#include "crc32.h"

const std::array<std::uint32_t, 256>& crc32Table() {
    // Sinh bảng một lần theo đa thức phản xạ 0xEDB88320.
    static const std::array<std::uint32_t, 256> table = [] {
        std::array<std::uint32_t, 256> t{};
        for (std::uint32_t i = 0; i < 256; ++i) {
            std::uint32_t value = i;
            for (int bit = 0; bit < 8; ++bit) {
                // bit thấp nhất đang bật -> dịch phải rồi XOR đa thức, ngược lại chỉ dịch.
                value = (value & 1u) ? ((value >> 1) ^ 0xEDB88320u) : (value >> 1);
            }
            t[i] = value;
        }
        return t;
    }();
    return table;
}

std::uint32_t crc32Update(std::uint32_t state, const void* data, std::size_t length) {
    const std::array<std::uint32_t, 256>& table = crc32Table();
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    std::uint32_t crc = state;
    for (std::size_t i = 0; i < length; ++i) {
        // Chỉ số = (byte hiện trạng XOR byte dữ liệu) & 0xFF; phần còn lại dịch phải 8 bit.
        crc = (crc >> 8) ^ table[(crc ^ bytes[i]) & 0xFFu];
    }
    return crc;
}

std::uint32_t crc32(const void* data, std::size_t length) {
    return crc32Finalize(crc32Update(crc32Begin(), data, length));
}
