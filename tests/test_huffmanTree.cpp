#undef NDEBUG
#include "huffmanTree.h"
#include <cassert>
#include <cstdint>

int main() {
    uint64_t freqTable[256] = {};
    freqTable[static_cast<uint8_t>('a')] = 2;
    freqTable[static_cast<uint8_t>('b')] = 1;

    uint64_t totalSymbols = 0;
    auto root = buildHuffmanTree(freqTable, totalSymbols);
    HuffmanCodeTable codeTable;
    buildCodeTable(root, codeTable);

    assert(totalSymbols == 3);
    assert(root && !root->isLeaf);
    assert(codeTable[static_cast<uint8_t>('b')] == "0");
    assert(codeTable[static_cast<uint8_t>('a')] == "1");

    uint64_t singleFreqTable[256] = {};
    singleFreqTable[static_cast<uint8_t>('x')] = 5;
    root = buildHuffmanTree(singleFreqTable, totalSymbols);
    buildCodeTable(root, codeTable);

    assert(totalSymbols == 5);
    assert(root && root->isLeaf);
    assert(codeTable[static_cast<uint8_t>('x')] == "0");

    uint64_t emptyFreqTable[256] = {};
    root = buildHuffmanTree(emptyFreqTable, totalSymbols);
    buildCodeTable(root, codeTable);
    assert(!root);
    assert(totalSymbols == 0);

    return 0;
}