#ifndef FILE_FORMAT_H
#define FILE_FORMAT_H

#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <string>

// Hằng số và tiện ích chung cho định dạng HUF2 (xem docs/fileFormat.md — nguồn chuẩn).

// Header cố định: magic (4) + mode (1) + originalLength (8) + crc32 (4).
constexpr std::size_t HEADER_SIZE = 17;
constexpr std::size_t MAGIC_SIZE = 4;
constexpr std::size_t MAX_LEB128_BYTES = 10;  // đủ cho uint64

constexpr char MAGIC_HUF2[5] = "HUF2";
constexpr char MAGIC_HUF1[5] = "HUF1";

// Chế độ lưu dữ liệu.
constexpr std::uint8_t MODE_HUFFMAN = 0;
constexpr std::uint8_t MODE_STORED = 1;

// Kích thước khối đọc/ghi (64 KB): không bao giờ đọc cả file vào RAM.
constexpr std::size_t IO_CHUNK_SIZE = 64 * 1024;

// Nhận diện magic của file; trả về 2 nếu "HUF2", 1 nếu "HUF1", 0 nếu khác.
int detectFormatVersion(const char magic[MAGIC_SIZE]);

// Ghi số uint64 little-endian với `bytes` byte (1..8).
void writeLittleEndian(std::ostream& out, std::uint64_t value, int bytes);

// Đọc số little-endian với `bytes` byte (1..8) từ mảng byte.
std::uint64_t readLittleEndian(const std::uint8_t* data, int bytes);

// Ghi số nguyên không dấu theo LEB128 (varint).
void writeLeb128(std::ostream& out, std::uint64_t value);

// Đọc LEB128 từ stream. Trả về false nếu varint hỏng (quá 10 byte, tràn uint64, hoặc hết stream).
bool readLeb128(std::istream& in, std::uint64_t& value);

#endif // FILE_FORMAT_H
