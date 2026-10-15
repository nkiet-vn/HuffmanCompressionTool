#ifndef HUFFMAN_NODE_H
#define HUFFMAN_NODE_H

// Nút của cây Huffman: ký tự, tần suất, con trái/phải.
struct HuffmanNode {
    char ch;
    int freq;
    HuffmanNode* left;
    HuffmanNode* right;

    HuffmanNode(char c, int f, HuffmanNode* l = nullptr, HuffmanNode* r = nullptr)
        : ch(c), freq(f), left(l), right(r) {}

    bool isLeaf() const { return left == nullptr && right == nullptr; }
};

#endif