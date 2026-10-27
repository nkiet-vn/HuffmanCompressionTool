#undef NDEBUG
#include "compressor.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

static void writeFile(const std::string& path, const std::string& data) {
    std::ofstream f(path, std::ios::binary);
    f << data;
}
static std::vector<unsigned char> readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return std::vector<unsigned char>((std::istreambuf_iterator<char>(f)),
                                      std::istreambuf_iterator<char>());
}

// Khop tung byte voi vi du "aab" trong docs/fileFormat.md
void test_aab_matches_spec() {
    writeFile("aab.txt", "aab");
    assert(Compressor().compress("aab.txt", "aab.bin"));
    std::vector<unsigned char> expected = {
        0x48, 0x55, 0x46, 0x31,
        0x02, 0x00,
        0x03, 0, 0, 0, 0, 0, 0, 0,
        0x61, 0x02, 0, 0, 0, 0, 0, 0, 0,
        0x62, 0x01, 0, 0, 0, 0, 0, 0, 0,
        0xC0};
    assert(readFile("aab.bin") == expected);
}

// DoD: file nen nho hon file goc voi file mau
void test_smaller_than_original() {
    std::string text;
    for (int i = 0; i < 500; ++i) text += "the quick brown fox jumps over the lazy dog. ";
    writeFile("sample.txt", text);
    assert(Compressor().compress("sample.txt", "sample.bin"));
    std::vector<unsigned char> bin = readFile("sample.bin");
    assert(bin.size() < text.size());
    std::cout << "original=" << text.size() << " compressed=" << bin.size() << "\n";
}

void test_empty_file() {
    writeFile("empty.txt", "");
    assert(Compressor().compress("empty.txt", "empty.bin"));
    assert(readFile("empty.bin").size() == 14);  // chi co header
}

void test_single_symbol() {
    writeFile("one.txt", std::string(1000, 'a'));
    assert(Compressor().compress("one.txt", "one.bin"));
    // header 14 + 9 = 23 byte, 1000 bit = 125 byte
    assert(readFile("one.bin").size() == 23 + 125);
}

void test_missing_input() {
    assert(!Compressor().compress("khong_ton_tai.txt", "x.bin"));
}

int main() {
    test_aab_matches_spec();
    test_smaller_than_original();
    test_empty_file();
    test_single_symbol();
    test_missing_input();
    std::cout << "All compress tests passed\n";
    return 0;
}