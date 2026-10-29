#ifndef CRC32_H
#define CRC32_H

#include <array>
#include <cstddef>
#include <cstdint>

// CRC-32 theo chuẩn IEEE 802.3 (đa thức phản xạ 0xEDB88320, như PNG/zlib).
// Dùng bảng tra 256 phần tử tự sinh một lần (hàm static cục bộ), không cần thư viện ngoài.
// Xem docs/fileFormat.md mục 5.

// Bảng tra CRC-32 (256 phần tử), khởi tạo một lần, thread-safe từ C++11.
const std::array<std::uint32_t, 256>& crc32Table();

// Giá trị khởi tạo: 0xFFFFFFFF.
inline std::uint32_t crc32Begin() { return 0xFFFFFFFFu; }

// Cập nhật CRC với `length` byte. Có thể gọi nhiều lần để tính theo khối (vd 64 KB).
std::uint32_t crc32Update(std::uint32_t state, const void* data, std::size_t length);

// Chốt: lấy phần bù (^ 0xFFFFFFFF).
inline std::uint32_t crc32Finalize(std::uint32_t state) { return state ^ 0xFFFFFFFFu; }

// Tính CRC của một vùng dữ liệu trong một lần gọi.
std::uint32_t crc32(const void* data, std::size_t length);

#endif // CRC32_H
