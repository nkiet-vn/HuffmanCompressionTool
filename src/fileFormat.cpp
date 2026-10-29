#include "fileFormat.h"

int detectFormatVersion(const char magic[MAGIC_SIZE]) {
    if (magic[0] != 'H' || magic[1] != 'U' || magic[2] != 'F') return 0;
    if (magic[3] == '3') return 3;
    if (magic[3] == '2') return 2;
    if (magic[3] == '1') return 1;
    return 0;
}

void writeLittleEndian(std::ostream& out, std::uint64_t value, int bytes) {
    for (int i = 0; i < bytes; ++i) {
        out.put(static_cast<char>(value & 0xFFu));
        value >>= 8;
    }
}

std::uint64_t readLittleEndian(const std::uint8_t* data, int bytes) {
    std::uint64_t value = 0;
    for (int i = 0; i < bytes; ++i)
        value |= static_cast<std::uint64_t>(data[i]) << (8 * i);
    return value;
}

void writeLeb128(std::ostream& out, std::uint64_t value) {
    // Nhóm thấp 7 bit trước; cờ bit 0 (bit cao) = 1 nghĩa là còn byte tiếp.
    while (value >= 0x80u) {
        out.put(static_cast<char>((value & 0x7Fu) | 0x80u));
        value >>= 7;
    }
    out.put(static_cast<char>(value & 0x7Fu));
}

bool readLeb128(std::istream& in, std::uint64_t& value) {
    value = 0;
    for (std::size_t i = 0; i < MAX_LEB128_BYTES; ++i) {
        const int next = in.get();
        if (next == std::char_traits<char>::eof()) return false;  // hết stream giữa varint
        const std::uint64_t byte = static_cast<std::uint64_t>(static_cast<unsigned char>(next));
        const std::uint64_t payload = byte & 0x7Fu;
        // Byte thứ 10 (i == 9) chỉ được giữ 1 bit dữ liệu, nếu không sẽ tràn uint64.
        if (i == MAX_LEB128_BYTES - 1 && payload > 1u) return false;
        value |= payload << (7 * i);
        if ((byte & 0x80u) == 0) return true;
    }
    return false;  // varint dài hơn 10 byte
}
