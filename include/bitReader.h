#ifndef BIT_READER_H
#define BIT_READER_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>

class BitReader {
private:
    static constexpr std::size_t BUFFER_SIZE = 64 * 1024;

    std::ifstream file;
    std::array<std::uint8_t, BUFFER_SIZE> byteBlock{};
    std::size_t blockPosition;
    std::size_t blockSize;
    bool reachedEnd;
    uint8_t buffer;
    int bitIndex; // Quản lý vị trí bit từ 7 (MSB) xuống 0 (LSB)

    void loadNextByte();

public:
    explicit BitReader(const std::string& filename);
    // Đọc bắt đầu từ offset byte (bỏ qua phần header).
    BitReader(const std::string& filename, std::streamoff startOffset);
    ~BitReader();

    BitReader(const BitReader&) = delete;
    BitReader& operator=(const BitReader&) = delete;

    int readBit();  // Đọc 1 bit (trả về 0 hoặc 1, trả về -1 nếu hết file)
    bool isEnd();   // Kiểm tra đã hết dữ liệu chưa
};

#endif // BIT_READER_H
