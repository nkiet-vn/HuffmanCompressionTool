#pragma once
#include <cstdint>
#include <ostream>

// Ghi từng bit ra stream. Thứ tự bit: MSB trước trong mỗi byte.
class BitWriter {
public:
    explicit BitWriter(std::ostream& out);
    void writeBit(int bit);   // ghi 1 bit (0 hoặc 1)
    void flush();             // đệm 0 cho đủ byte rồi ghi ra

private:
    std::ostream& out_;
    uint8_t buffer_ = 0;
    int bitCount_ = 0;
};