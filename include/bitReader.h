#pragma once
#include <cstdint>
#include <istream>

// Đọc từng bit từ stream. Thứ tự bit: MSB trước trong mỗi byte.
class BitReader {
public:
    explicit BitReader(std::istream& in);
    int readBit();            // trả về 0 hoặc 1
    bool isEnd();             // true khi hết dữ liệu

private:
    std::istream& in_;
    uint8_t buffer_ = 0;
    int bitCount_ = 0;        // số bit còn lại trong buffer_
};