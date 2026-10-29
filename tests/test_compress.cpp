#undef NDEBUG
#include "compressor.h"
#include "crc32.h"
#include "fileFormat.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

static void writeFile(const std::string &path, const std::string &data)
{
    std::ofstream f(path, std::ios::binary);
    f << data;
}
static std::vector<unsigned char> readFile(const std::string &path)
{
    std::ifstream f(path, std::ios::binary);
    return std::vector<unsigned char>((std::istreambuf_iterator<char>(f)),
                                      std::istreambuf_iterator<char>());
}

// Khop tung byte voi vi du "aab" trong docs/fileFormat.md (mode STORED).
void test_aab_matches_spec()
{
    writeFile("aab.txt", "aab");
    assert(Compressor().compress("aab.txt", "aab.bin"));
    std::vector<unsigned char> expected = {
        0x48, 0x55, 0x46, 0x33,    // magic "HUF3"
        0x01,                      // mode = STORED
        0x03, 0, 0, 0, 0, 0, 0, 0, // originalLength = 3
        0x97, 0x22, 0x0E, 0x69,    // crc32("aab") = 0x690E2297
        0x61, 0x61, 0x62};         // "aab"
    assert(readFile("aab.bin") == expected);
}

// Du lieu lap lai nhieu -> HUF3 HUFFMAN luu do dai ma chuan tac.
void test_huffman_mode_layout()
{
    const std::string text(3000, 'a');
    writeFile("huf_layout.txt", text);
    assert(Compressor().compress("huf_layout.txt", "huf_layout.bin"));
    std::vector<unsigned char> bin = readFile("huf_layout.bin");

    // header + uniqueCount + (symbol, codeLength) + 3000 bit = 17 + 2 + 2 + 375.
    assert(bin.size() == HEADER_SIZE + 2 + 2 + 375);
    assert(bin[0] == 'H' && bin[1] == 'U' && bin[2] == 'F' && bin[3] == '3');
    assert(bin[4] == MODE_HUFFMAN);
    assert(bin[5] == 0xB8 && bin[6] == 0x0B); // 3000 = 0x0BB8 little-endian
    for (int i = 0; i < 4; ++i)
        assert(bin[13 + i] == (((crc32(text.data(), text.size()) >> (8 * i)) & 0xFF)));
    assert(bin[17] == 0x01 && bin[18] == 0x00); // uniqueCount = 1
    assert(bin[19] == 'a');                     // symbol
    assert(bin[20] == 0x01);                    // code length = 1
    // 3000 bit "0" -> 375 byte toàn 0
    for (size_t i = 21; i < bin.size(); ++i) assert(bin[i] == 0x00);
}

// DoD: file nen nho hon file goc voi file mau
void test_smaller_than_original()
{
    std::string text;
    for (int i = 0; i < 500; ++i)
        text += "the quick brown fox jumps over the lazy dog. ";
    writeFile("sample.txt", text);
    assert(Compressor().compress("sample.txt", "sample.bin"));
    std::vector<unsigned char> bin = readFile("sample.bin");
    assert(bin.size() < text.size());
    assert(bin[4] == MODE_HUFFMAN);
    std::cout << "original=" << text.size() << " compressed=" << bin.size() << "\n";
}

void test_empty_file()
{
    writeFile("empty.txt", "");
    assert(Compressor().compress("empty.txt", "empty.bin"));
    std::vector<unsigned char> bin = readFile("empty.bin");
    // HUFFMAN cần 17 + 2 = 19 byte (header + uniqueCount), STORED chỉ 17 byte -> chọn STORED
    assert(bin.size() == HEADER_SIZE);
    assert(bin[4] == MODE_STORED);
    for (size_t i = 5; i < 13; ++i) assert(bin[i] == 0);   // originalLength = 0
    assert(crc32("", 0) == 0);
}

// File nho: khong bao gio phinh qua HEADER_SIZE byte, va STORED dung bang HEADER_SIZE + goc.
void test_tiny_file_not_inflated()
{
    std::vector<std::string> samples = {"Xin chao Huffman!", std::string(17, 'x')};
    for (const std::string& text : samples)
    {
        writeFile("tiny.txt", text);
        assert(Compressor().compress("tiny.txt", "tiny.bin"));
        std::vector<unsigned char> bin = readFile("tiny.bin");
        assert(bin.size() <= text.size() + HEADER_SIZE);
        if (bin[4] == MODE_STORED)
        {
            assert(bin.size() == HEADER_SIZE + text.size());
        }
        else
        {
            // Neu chon HUFFMAN thi phai that su nho hon phuong an STORED.
            assert(bin.size() < HEADER_SIZE + text.size());
        }
    }
}

void test_missing_input()
{
    assert(!Compressor().compress("khong_ton_tai.txt", "x.bin"));
}

int main()
{
    test_aab_matches_spec();
    test_huffman_mode_layout();
    test_smaller_than_original();
    test_empty_file();
    test_tiny_file_not_inflated();
    test_missing_input();
    std::cout << "All compress tests passed\n";
    return 0;
}
