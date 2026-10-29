#pragma once

#include <cstdint>
#include <vector>

#include "huffmanTree.h"

struct CanonicalCode
{
    std::uint8_t symbol;
    std::uint8_t length;
    std::uint32_t bits;
};

bool getHuffmanCodeLengths(const std::shared_ptr<HuffmanNode>& root,
                           std::uint8_t lengths[256]);
bool buildCanonicalCodes(const std::uint8_t lengths[256],
                         std::vector<CanonicalCode>& codes);
