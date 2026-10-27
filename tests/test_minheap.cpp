// Test cho Min-Heap tự viết (include/minHeap.h): thứ tự pop theo (freq, minSymbol).
#undef NDEBUG
#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <vector>

#include "minHeap.h"

// DoD: push 100 phần tử, pop luôn ra phần tử nhỏ nhất.
static void testHundredElements()
{
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> dist(1, 1000);

    MinHeap heap;
    std::vector<int> expected;
    std::vector<MinHeapNode *> nodes;

    for (int i = 0; i < 100; ++i)
    {
        int f = dist(rng);
        expected.push_back(f);
        nodes.push_back(new MinHeapNode(static_cast<char>('a' + i % 26), f));
        heap.push(nodes.back());
        assert(heap.top()->freq == *std::min_element(expected.begin(), expected.end()));
    }
    assert(heap.size() == 100);
    assert(!heap.empty());

    std::vector<int> sorted = expected;
    std::sort(sorted.begin(), sorted.end());
    for (int i = 0; i < 100; ++i)
    {
        assert(heap.top()->freq == sorted[i]);
        MinHeapNode *n = heap.pop();
        assert(n->freq == sorted[i]);
        assert(heap.size() == static_cast<std::size_t>(99 - i));
    }
    assert(heap.empty());

    for (MinHeapNode *n : nodes)
        delete n;
}

// Xen kẽ push/pop: pop vẫn luôn là min của tập hiện tại.
static void testInterleaved()
{
    std::mt19937 rng(777);
    std::uniform_int_distribution<int> dist(1, 50);

    MinHeap heap;
    std::vector<int> ref;
    std::vector<MinHeapNode *> nodes;

    for (int step = 0; step < 500; ++step)
    {
        if (ref.empty() || rng() % 3 != 0)
        {
            int f = dist(rng);
            nodes.push_back(new MinHeapNode('x', f));
            heap.push(nodes.back());
            ref.push_back(f);
        }
        else
        {
            auto it = std::min_element(ref.begin(), ref.end());
            MinHeapNode *n = heap.pop();
            assert(n->freq == *it);
            ref.erase(it);
        }
        assert(heap.size() == ref.size());
    }
    for (MinHeapNode *n : nodes)
        delete n;
}

// Tie-break theo minSymbol: cùng freq thì minSymbol nhỏ hơn pop trước.
static void testTieBreakByMinSymbol()
{
    MinHeapNode z('z', 3);
    MinHeapNode y('y', 1);
    MinHeapNode aLeaf('a', 1);
    MinHeapNode bLeaf('b', 1);
    MinHeapNode internal('\0', 2, &aLeaf, &bLeaf); // minSymbol = 'a'

    MinHeap heap;
    heap.push(&z);
    heap.push(&y);
    heap.push(&internal);

    // (1,'a') <= (1,'b') <= (1,'y') < (2,'a') < (3,'z')
    // Heap chứa: y(1), internal(2,'a'), z(3)
    assert(heap.pop() == &y);
    assert(heap.pop() == &internal);
    assert(heap.pop() == &z);
    assert(heap.empty());
    assert(internal.minSymbol == static_cast<unsigned char>('a'));
}

static void testEmptyAndErrors()
{
    MinHeap heap;
    assert(heap.empty());
    assert(heap.size() == 0);

    bool threw = false;
    try
    {
        heap.pop();
    }
    catch (const std::out_of_range &)
    {
        threw = true;
    }
    assert(threw);

    threw = false;
    try
    {
        heap.top();
    }
    catch (const std::out_of_range &)
    {
        threw = true;
    }
    assert(threw);

    threw = false;
    try
    {
        heap.push(nullptr);
    }
    catch (const std::invalid_argument &)
    {
        threw = true;
    }
    assert(threw);
}

static void testHuffmanNode()
{
    MinHeapNode a('a', 5), b('b', 9);
    MinHeapNode parent('\0', 14, &a, &b);
    assert(a.isLeaf() && b.isLeaf());
    assert(!parent.isLeaf());
    assert(parent.left == &a && parent.right == &b);
    assert(parent.freq == a.freq + b.freq);
    assert(parent.minSymbol == static_cast<unsigned char>('a'));
}

int main()
{
    testHundredElements();
    testInterleaved();
    testTieBreakByMinSymbol();
    testEmptyAndErrors();
    testHuffmanNode();
    std::cout << "All MinHeap tests passed\n";
    return 0;
}
