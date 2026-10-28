#include "huffmanTree.h"
#include "minHeap.h"
#include <utility>
#include <memory>

HuffmanNode::HuffmanNode(uint8_t symbolValue, uint64_t frequency)
    : symbol(symbolValue), freq(frequency), minSymbol(symbolValue), isLeaf(true),
      left(nullptr), right(nullptr) {}

HuffmanNode::HuffmanNode(uint64_t frequency, uint8_t smallestSymbol,
                         std::shared_ptr<HuffmanNode> leftNode,
                 std::shared_ptr<HuffmanNode> rightNode)
    : symbol(0), freq(frequency), minSymbol(smallestSymbol), isLeaf(false),
      left(std::move(leftNode)), right(std::move(rightNode)) {}

static void visitCodes(const std::shared_ptr<HuffmanNode>& node,
                const std::string& prefix, HuffmanCodeTable& codeTable) {
    if (!node) return;
    if (node->isLeaf) {
        codeTable[node->symbol] = prefix.empty() ? "0" : prefix;
        return;
    }
    visitCodes(node->left, prefix + '0', codeTable);
    visitCodes(node->right, prefix + '1', codeTable);
}

std::shared_ptr<HuffmanNode> buildHuffmanTree(const uint64_t freqTable[256], uint64_t& totalSymbols) {
    MinHeap minHeap;
    totalSymbols = 0;
    for (int symbol = 0; symbol < 256; ++symbol) {
        if (freqTable[symbol] == 0) continue;
        minHeap.push(std::make_shared<HuffmanNode>(static_cast<uint8_t>(symbol), freqTable[symbol]));
        totalSymbols += freqTable[symbol];
    }
    if (minHeap.empty()) return nullptr;

    // Lấy hai nút nhỏ nhất, gộp thành nút cha, đẩy lại vào heap cho đến khi còn một nút (gốc).
    while (minHeap.size() > 1) {
        auto left = minHeap.pop();
        auto right = minHeap.pop();
        const uint8_t minSym = left->minSymbol < right->minSymbol ? left->minSymbol : right->minSymbol;
        minHeap.push(std::make_shared<HuffmanNode>(left->freq + right->freq, minSym, left, right));
    }
    return minHeap.pop();
}

void buildCodeTable(const std::shared_ptr<HuffmanNode>& root, HuffmanCodeTable& codeTable) {
    for (auto& code : codeTable) code.clear();
    visitCodes(root, "", codeTable);
}