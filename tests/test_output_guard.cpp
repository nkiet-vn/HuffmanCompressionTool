#undef NDEBUG
#include "compressor.h"
#include "decompressor.h"
#include "fileFormat.h"
#include "pathUtil.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static int failures = 0;

#define CHECK(condition, message)                                                \
    do                                                                           \
    {                                                                            \
        if (!(condition))                                                        \
        {                                                                        \
            std::cerr << "FAIL: " << (message) << " (line " << __LINE__ << ")\n"; \
            ++failures;                                                         \
        }                                                                        \
    } while (0)

using Bytes = std::vector<unsigned char>;

static void writeFile(const std::string &path, const std::string &data)
{
    std::ofstream out(toPath(path), std::ios::binary | std::ios::trunc);
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
}

static void writeBytes(const std::string &path, const Bytes &data)
{
    std::ofstream out(toPath(path), std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char *>(data.data()),
              static_cast<std::streamsize>(data.size()));
}

static Bytes readBytes(const std::string &path)
{
    std::ifstream in(toPath(path), std::ios::binary);
    return Bytes(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

static void checkOutputRemoved(const std::string &path, const char *caseName)
{
    CHECK(!fs::exists(toPath(path)), std::string(caseName) + ": output file remains");
    CHECK(!fs::exists(toPath(path + ".tmp")), std::string(caseName) + ": temporary file remains");
}

static void testHuffmanCrcFailureRemovesOutput()
{
    const std::string source = "guard_huffman_source.txt";
    const std::string archive = "guard_huffman.bin";
    const std::string output = "guard_huffman_output.txt";
    writeFile(source, std::string(20000, 'a') + std::string(5000, 'b') + std::string(500, 'z'));
    CHECK(Compressor().compress(source, archive), "Huffman source compression failed");

    Bytes packed = readBytes(archive);
    CHECK(packed.size() > HEADER_SIZE && packed[4] == MODE_HUFFMAN,
          "cleanup sample must use HUFFMAN");
    if (packed.size() > HEADER_SIZE && packed[4] == MODE_HUFFMAN)
    {
        packed[13] ^= 0x01;
        writeBytes(archive, packed);
        std::remove(output.c_str());
        CHECK(!Decompressor().decompress(archive, output), "Huffman CRC error was accepted");
        checkOutputRemoved(output, "Huffman CRC error");
    }

    std::remove(source.c_str());
    std::remove(archive.c_str());
    std::remove(output.c_str());
    std::remove((output + ".tmp").c_str());
}

static void testStoredCrcFailureRemovesOutput()
{
    const std::string source = "guard_stored_source.txt";
    const std::string archive = "guard_stored.bin";
    const std::string output = "guard_stored_output.txt";
    writeFile(source, "abc");
    CHECK(Compressor().compress(source, archive), "STORED source compression failed");

    Bytes packed = readBytes(archive);
    CHECK(packed.size() == HEADER_SIZE + 3 && packed[4] == MODE_STORED,
          "cleanup sample must use STORED");
    if (packed.size() == HEADER_SIZE + 3 && packed[4] == MODE_STORED)
    {
        packed[13] ^= 0x01;
        writeBytes(archive, packed);
        std::remove(output.c_str());
        CHECK(!Decompressor().decompress(archive, output), "STORED CRC error was accepted");
        checkOutputRemoved(output, "STORED CRC error");
    }

    std::remove(source.c_str());
    std::remove(archive.c_str());
    std::remove(output.c_str());
    std::remove((output + ".tmp").c_str());
}

static void testTruncatedInputLeavesNoOutput()
{
    const std::string source = "guard_truncated_source.txt";
    const std::string archive = "guard_truncated.bin";
    const std::string output = "guard_truncated_output.txt";
    writeFile(source, std::string(20000, 'a') + std::string(5000, 'b') + std::string(500, 'z'));
    CHECK(Compressor().compress(source, archive), "truncated source compression failed");

    Bytes packed = readBytes(archive);
    CHECK(packed.size() > HEADER_SIZE && packed[4] == MODE_HUFFMAN,
          "truncation sample must use HUFFMAN");
    if (packed.size() > HEADER_SIZE && packed[4] == MODE_HUFFMAN)
    {
        packed.resize(packed.size() - 1);
        writeBytes(archive, packed);
        std::remove(output.c_str());
        CHECK(!Decompressor().decompress(archive, output), "truncated input was accepted");
        checkOutputRemoved(output, "truncated input");
    }

    std::remove(source.c_str());
    std::remove(archive.c_str());
    std::remove(output.c_str());
    std::remove((output + ".tmp").c_str());
}

int main()
{
    testHuffmanCrcFailureRemovesOutput();
    testStoredCrcFailureRemovesOutput();
    testTruncatedInputLeavesNoOutput();
    if (failures == 0)
    {
        std::cout << "OutputGuard cleanup tests passed\n";
        return 0;
    }
    return 1;
}
