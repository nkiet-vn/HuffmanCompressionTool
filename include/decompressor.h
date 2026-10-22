#ifndef DECOMPRESSOR_H
#define DECOMPRESSOR_H

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

struct HuffmanNode {
    uint8_t symbol;
    uint64_t freq;
    bool isLeaf;
    std::shared_ptr<HuffmanNode> left;
    std::shared_ptr<HuffmanNode> right;

    HuffmanNode(uint8_t sym, uint64_t f) 
        : symbol(sym), freq(f), isLeaf(true), left(nullptr), right(nullptr) {}

    HuffmanNode(uint64_t f, std::shared_ptr<HuffmanNode> l, std::shared_ptr<HuffmanNode> r) 
        : symbol(0), freq(f), isLeaf(false), left(l), right(r) {}
};

class Decompressor {
public:
    static bool decompress(const std::string& inputBinPath, const std::string& outputTxtPath);
};

#endif // DECOMPRESSOR_H
