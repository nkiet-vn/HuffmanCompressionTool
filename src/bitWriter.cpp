#include "bitWriter.h"

BitWriter::BitWriter(std::ostream& out) : out_(out) {}

void BitWriter::writeBit(int bit) {
    buffer_ = static_cast<uint8_t>((buffer_ << 1) | (bit & 1));
    ++bitCount_;
    if (bitCount_ == 8) {
        out_.put(static_cast<char>(buffer_));
        buffer_ = 0;
        bitCount_ = 0;
    }
}

void BitWriter::flush() {
    if (bitCount_ > 0) {
        buffer_ = static_cast<uint8_t>(buffer_ << (8 - bitCount_));
        out_.put(static_cast<char>(buffer_));
        buffer_ = 0;
        bitCount_ = 0;
    }
    out_.flush();
}