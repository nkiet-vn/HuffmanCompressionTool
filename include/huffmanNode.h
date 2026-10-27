#ifndef MIN_HEAP_NODE_H
#define MIN_HEAP_NODE_H

// Nút của cây Huffman: ký tự, tần suất, ký tự nhỏ nhất trong cây con, con trái/phải.
struct MinHeapNode {
    char ch;
    int freq;
    unsigned char minSymbol;   // ký tự nhỏ nhất trong cây con (dùng để tie-break)
    MinHeapNode* left;
    MinHeapNode* right;

    MinHeapNode(char c, int f, MinHeapNode* l = nullptr, MinHeapNode* r = nullptr)
        : ch(c), freq(f), minSymbol(static_cast<unsigned char>(c)), left(l), right(r) {
        if (l && r) {
            minSymbol = (l->minSymbol < r->minSymbol) ? l->minSymbol : r->minSymbol;
        }
    }

    bool isLeaf() const { return left == nullptr && right == nullptr; }
};

#endif
