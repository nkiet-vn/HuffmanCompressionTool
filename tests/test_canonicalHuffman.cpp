#undef NDEBUG
#include "canonicalHuffman.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

namespace
{

int failures = 0;

#define CHECK(condition)                                                        \
    do                                                                          \
    {                                                                           \
        if (!(condition))                                                       \
        {                                                                       \
            ++failures;                                                        \
            std::cerr << "FAIL line " << __LINE__ << ": " #condition "\n";    \
        }                                                                       \
    } while (false)

void testCanonicalAssignmentAndKraft()
{
    std::uint8_t lengths[256] = {};
    lengths[2] = 2;
    lengths[7] = 2;
    std::vector<CanonicalCode> codes;
    CHECK(buildCanonicalCodes(lengths, codes));
    CHECK(codes.size() == 2);
    CHECK(codes[0].symbol == 2 && codes[0].bits == 0 && codes[0].length == 2);
    CHECK(codes[1].symbol == 7 && codes[1].bits == 1 && codes[1].length == 2);

    lengths[10] = 1;
    lengths[11] = 1;
    CHECK(!buildCanonicalCodes(lengths, codes));
}

void testMaximumCodeLength()
{
    std::uint8_t lengths[256] = {};
    lengths[3] = 32;
    lengths[255] = 32;
    std::vector<CanonicalCode> codes;
    CHECK(buildCanonicalCodes(lengths, codes));
    CHECK(codes.size() == 2);
    CHECK(codes[0].bits == 0 && codes[1].bits == 1);

    lengths[3] = 33;
    CHECK(!buildCanonicalCodes(lengths, codes));

    std::shared_ptr<HuffmanNode> root =
        std::make_shared<HuffmanNode>(
            33, std::uint8_t{32},
            std::make_shared<HuffmanNode>(std::uint8_t{32}, 1),
            std::make_shared<HuffmanNode>(std::uint8_t{33}, 1));
    for (int symbol = 31; symbol >= 0; --symbol)
    {
        root = std::make_shared<HuffmanNode>(
            34, static_cast<std::uint8_t>(symbol),
            std::make_shared<HuffmanNode>(static_cast<std::uint8_t>(symbol), 1), root);
    }
    CHECK(!getHuffmanCodeLengths(root, lengths));
}

void testHuffmanTreeOverLimit()
{
    std::uint64_t frequencies[256] = {};
    frequencies[0] = 1;
    frequencies[1] = 1;
    for (int symbol = 2; symbol < 34; ++symbol)
        frequencies[symbol] = frequencies[symbol - 1] + frequencies[symbol - 2];

    std::uint64_t total = 0;
    const std::shared_ptr<HuffmanNode> root = buildHuffmanTree(frequencies, total);
    std::uint8_t lengths[256] = {};
    CHECK(root != nullptr);
    CHECK(!getHuffmanCodeLengths(root, lengths));
}

} // namespace

int main()
{
    testCanonicalAssignmentAndKraft();
    testMaximumCodeLength();
    testHuffmanTreeOverLimit();
    if (failures != 0)
        return 1;
    std::cout << "Canonical Huffman tests passed\n";
    return 0;
}
