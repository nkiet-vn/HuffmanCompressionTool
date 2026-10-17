#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <vector>

#include "min_heap.h"

// DoD: push 100 phần tử, pop luôn ra phần tử nhỏ nhất.
static void testHundredElements() {
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> dist(1, 1000);

    MinHeap heap;
    std::vector<int> expected;
    std::vector<HuffmanNode*> nodes;

    for (int i = 0; i < 100; ++i) {
        int f = dist(rng);
        expected.push_back(f);
        nodes.push_back(new HuffmanNode(static_cast<char>('a' + i % 26), f));
        heap.push(nodes.back());
        // top() luôn là nhỏ nhất trong các phần tử đã push
        assert(heap.top()->freq == *std::min_element(expected.begin(), expected.end()));
    }
    assert(heap.size() == 100);
    assert(!heap.empty());

    std::sort(expected.begin(), expected.end());
    for (int i = 0; i < 100; ++i) {
        // pop luôn ra phần tử nhỏ nhất còn lại
        assert(heap.top()->freq == expected[i]);
        HuffmanNode* n = heap.pop();
        assert(n->freq == expected[i]);
        assert(heap.size() == static_cast<std::size_t>(99 - i));
    }
    assert(heap.empty());

    for (HuffmanNode* n : nodes) delete n;
}

// Xen kẽ push/pop: pop vẫn luôn là min của tập hiện tại.
static void testInterleaved() {
    std::mt19937 rng(777);
    std::uniform_int_distribution<int> dist(1, 50);  // nhiều giá trị trùng

    MinHeap heap;
    std::vector<int> ref;
    std::vector<HuffmanNode*> nodes;

    for (int step = 0; step < 500; ++step) {
        if (ref.empty() || rng() % 3 != 0) {
            int f = dist(rng);
            nodes.push_back(new HuffmanNode('x', f));
            heap.push(nodes.back());
            ref.push_back(f);
        } else {
            auto it = std::min_element(ref.begin(), ref.end());
            HuffmanNode* n = heap.pop();
            assert(n->freq == *it);
            ref.erase(it);
        }
        assert(heap.size() == ref.size());
    }
    for (HuffmanNode* n : nodes) delete n;
}

static void testEmptyAndErrors() {
    MinHeap heap;
    assert(heap.empty());
    assert(heap.size() == 0);

    bool threw = false;
    try { heap.pop(); } catch (const std::out_of_range&) { threw = true; }
    assert(threw);

    threw = false;
    try { heap.top(); } catch (const std::out_of_range&) { threw = true; }
    assert(threw);

    threw = false;
    try { heap.push(nullptr); } catch (const std::invalid_argument&) { threw = true; }
    assert(threw);
}

static void testHuffmanNode() {
    HuffmanNode a('a', 5), b('b', 9);
    HuffmanNode parent('\0', 14, &a, &b);
    assert(a.isLeaf() && b.isLeaf());
    assert(!parent.isLeaf());
    assert(parent.left == &a && parent.right == &b);
    assert(parent.freq == a.freq + b.freq);
}

int main() {
    testHundredElements();
    testInterleaved();
    testEmptyAndErrors();
    testHuffmanNode();
    std::cout << "All MinHeap tests passed\n";
    return 0;
}