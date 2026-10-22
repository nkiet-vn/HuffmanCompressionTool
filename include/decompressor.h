#ifndef DECOMPRESSOR_H
#define DECOMPRESSOR_H

#include <string>
#include <memory>
#include <cstdint>

struct HuffmanNode {
    uint8_t symbol;
    bool isLeaf;
    std::shared_ptr<HuffmanNode> left;
    std::shared_ptr<HuffmanNode> right;

    HuffmanNode() : symbol(0), isLeaf(false), left(nullptr), right(nullptr) {}
    explicit HuffmanNode(uint8_t sym) : symbol(sym), isLeaf(true), left(nullptr), right(nullptr) {}
};

class Decompressor {
public:
    static bool decompress(const std::string& inputBinPath, const std::string& outputTxtPath);
};

#endif // DECOMPRESSOR_H
