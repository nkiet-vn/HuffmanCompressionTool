#ifndef HUFFMAN_TREE_H
#define HUFFMAN_TREE_H

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct HuffmanNode {
    uint8_t symbol;
    uint64_t freq;
    uint8_t minSymbol;
    bool isLeaf;
    std::shared_ptr<HuffmanNode> left;
    std::shared_ptr<HuffmanNode> right;

    HuffmanNode(uint8_t symbol, uint64_t freq);
    HuffmanNode(uint64_t freq, uint8_t minSymbol,
                std::shared_ptr<HuffmanNode> left,
                std::shared_ptr<HuffmanNode> right);
};

using HuffmanCodeTable = std::array<std::string, 256>;

std::shared_ptr<HuffmanNode> buildHuffmanTree(const uint64_t freqTable[256], uint64_t& totalSymbols);
void buildCodeTable(const std::shared_ptr<HuffmanNode>& root, HuffmanCodeTable& codeTable);

#endif