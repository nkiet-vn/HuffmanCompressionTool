#ifndef MIN_HEAP_NODE_H
#define MIN_HEAP_NODE_H

// Nút của cây Huffman: ký tự, tần suất, con trái/phải.
struct MinHeapNode {
    char ch;
    int freq;
    MinHeapNode* left;
    MinHeapNode* right;

    MinHeapNode(char c, int f, MinHeapNode* l = nullptr, MinHeapNode* r = nullptr)
        : ch(c), freq(f), left(l), right(r) {}

    bool isLeaf() const { return left == nullptr && right == nullptr; }
};

#endif