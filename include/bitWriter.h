#pragma once
#include <cstdint>
#include <ostream>

class BitWriter {
public:
    explicit BitWriter(std::ostream& out) : out_(out) {}

    // Ghi 1 bit (0 hoặc 1), MSB-first
    void writeBit(bool bit) {
        buffer_ = static_cast<uint8_t>((buffer_ << 1) | (bit ? 1 : 0));
        if (++count_ == 8) {
            writeByte();
        }
    }

    // Ghi `length` bit thấp nhất của `code`, từ bit cao đến bit thấp
    void writeBits(uint32_t code, int length) {
        for (int i = length - 1; i >= 0; --i) {
            writeBit((code >> i) & 1U);
        }
    }

    // Đệm bit 0 cho đủ 8 bit rồi ghi ra stream
    void flush() {
        if (count_ > 0) {
            buffer_ = static_cast<uint8_t>(buffer_ << (8 - count_));
            count_ = 8;
            writeByte();
        }
    }

private:
    void writeByte() {
        out_.put(static_cast<char>(buffer_));
        buffer_ = 0;
        count_ = 0;
    }

    std::ostream& out_;
    uint8_t buffer_ = 0;
    int count_ = 0;
};