#include "huffmanTree.h"
#include <queue>
#include <utility>

HuffmanNode::HuffmanNode(uint8_t symbolValue, uint64_t frequency)
    : symbol(symbolValue), freq(frequency), minSymbol(symbolValue), isLeaf(true),
      left(nullptr), right(nullptr) {}

HuffmanNode::HuffmanNode(uint64_t frequency, uint8_t smallestSymbol,
                         std::shared_ptr<HuffmanNode> leftNode,
                         std::shared_ptr<HuffmanNode> rightNode)
    : symbol(0), freq(frequency), minSymbol(smallestSymbol), isLeaf(false),
      left(std::move(leftNode)), right(std::move(rightNode)) {}

namespace {
struct CompareNode {
    bool operator()(const std::shared_ptr<HuffmanNode>& a,
                    const std::shared_ptr<HuffmanNode>& b) const {
        if (a->freq != b->freq) return a->freq > b->freq;
        return a->minSymbol > b->minSymbol;
    }
};

void visitCodes(const std::shared_ptr<HuffmanNode>& node,
                const std::string& prefix, HuffmanCodeTable& codeTable) {
    if (!node) return;
    if (node->isLeaf) {
        codeTable[node->symbol] = prefix.empty() ? "0" : prefix;
        return;
    }
    visitCodes(node->left, prefix + '0', codeTable);
    visitCodes(node->right, prefix + '1', codeTable);
}
}

std::shared_ptr<HuffmanNode> buildHuffmanTree(const uint64_t freqTable[256], uint64_t& totalSymbols) {
    std::priority_queue<std::shared_ptr<HuffmanNode>,
                        std::vector<std::shared_ptr<HuffmanNode>>, CompareNode> minHeap;
    totalSymbols = 0;
    for (int symbol = 0; symbol < 256; ++symbol) {
        if (freqTable[symbol] == 0) continue;
        minHeap.push(std::make_shared<HuffmanNode>(static_cast<uint8_t>(symbol), freqTable[symbol]));
        totalSymbols += freqTable[symbol];
    }
    if (minHeap.empty()) return nullptr;

    while (minHeap.size() > 1) {
        auto leftNode = minHeap.top();
        minHeap.pop();
        auto rightNode = minHeap.top();
        minHeap.pop();
        auto parent = std::make_shared<HuffmanNode>(
            leftNode->freq + rightNode->freq,
            (leftNode->minSymbol < rightNode->minSymbol) ? leftNode->minSymbol : rightNode->minSymbol,
            leftNode, rightNode);
        minHeap.push(parent);
    }
    return minHeap.top();
}

void buildCodeTable(const std::shared_ptr<HuffmanNode>& root, HuffmanCodeTable& codeTable) {
    for (auto& code : codeTable) code.clear();
    visitCodes(root, "", codeTable);
}