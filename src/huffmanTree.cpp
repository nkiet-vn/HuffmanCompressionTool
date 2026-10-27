#include "huffmanTree.h"
#include "minHeap.h"
#include <utility>
#include <memory>

// Chuyển đổi giữa MinHeapNode (raw pointer) và HuffmanNode (shared_ptr)
static std::shared_ptr<HuffmanNode> convertToHuffmanNode(MinHeapNode* node) {
    if (!node) return nullptr;
    if (node->isLeaf()) {
        return std::make_shared<HuffmanNode>(static_cast<uint8_t>(node->ch), static_cast<uint64_t>(node->freq));
    }
    auto left = convertToHuffmanNode(node->left);
    auto right = convertToHuffmanNode(node->right);
    uint8_t minSym = node->minSymbol;
    return std::make_shared<HuffmanNode>(static_cast<uint64_t>(node->freq), minSym, left, right);
}

static void deleteMinHeapTree(MinHeapNode* node) {
    if (!node) return;
    deleteMinHeapTree(node->left);
    deleteMinHeapTree(node->right);
    delete node;
}

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
        minHeap.push(new MinHeapNode(static_cast<char>(symbol), static_cast<int>(freqTable[symbol])));
        totalSymbols += freqTable[symbol];
    }
    if (minHeap.empty()) return nullptr;

    while (minHeap.size() > 1) {
        MinHeapNode* left = minHeap.pop();
        MinHeapNode* right = minHeap.pop();
        int combinedFreq = left->freq + right->freq;
        MinHeapNode* parent = new MinHeapNode('\0', combinedFreq, left, right);
        minHeap.push(parent);
    }
    
    MinHeapNode* root = minHeap.pop();
    auto huffmanRoot = convertToHuffmanNode(root);
    deleteMinHeapTree(root);
    return huffmanRoot;
}

void buildCodeTable(const std::shared_ptr<HuffmanNode>& root, HuffmanCodeTable& codeTable) {
    for (auto& code : codeTable) code.clear();
    visitCodes(root, "", codeTable);
}