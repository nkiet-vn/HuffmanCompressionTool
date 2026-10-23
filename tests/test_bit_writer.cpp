#undef NDEBUG
#include "../include/bitWriter.h"
#include <cassert>
#include <iostream>
#include <sstream>

static unsigned char at(const std::string& s, size_t i) {
    return static_cast<unsigned char>(s[i]);
}

// DoD: ghi 10110 rồi kiểm tra byte ra đúng
void test_write_10110() {
    std::ostringstream oss;
    BitWriter w(oss);
    w.writeBit(1); w.writeBit(0); w.writeBit(1); w.writeBit(1); w.writeBit(0);
    assert(oss.str().empty());
    w.flush();
    assert(oss.str().size() == 1);
    assert(at(oss.str(), 0) == 0xB0);
}

void test_full_byte_written_immediately() {
    std::ostringstream oss;
    BitWriter w(oss);
    w.writeBits(0b11001010, 8);
    assert(oss.str().size() == 1);
    assert(at(oss.str(), 0) == 0xCA);
}

void test_flush_without_pending_bits() {
    std::ostringstream oss;
    BitWriter w(oss);
    w.flush();
    assert(oss.str().empty());
}

void test_multi_byte() {
    std::ostringstream oss;
    BitWriter w(oss);
    w.writeBits(0b1010101011, 10);
    w.flush();
    assert(oss.str().size() == 2);
    assert(at(oss.str(), 0) == 0xAA);
    assert(at(oss.str(), 1) == 0xC0);
}

int main() {
    test_write_10110();
    test_full_byte_written_immediately();
    test_flush_without_pending_bits();
    test_multi_byte();
    std::cout << "All BitWriter tests passed\n";
    return 0;
}