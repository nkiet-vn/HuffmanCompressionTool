// Test cho Min-Heap tự viết (include/minHeap.h): thứ tự pop theo (freq, minSymbol).
// Heap chứa HuffmanNode qua shared_ptr (cùng kiểu nút với cây Huffman).
#undef NDEBUG
#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

#include "minHeap.h"

using NodePtr = std::shared_ptr<HuffmanNode>;

static NodePtr leaf(char c, uint64_t freq)
{
    return std::make_shared<HuffmanNode>(static_cast<uint8_t>(c), freq);
}

static NodePtr internalNode(uint64_t freq, const NodePtr &l, const NodePtr &r)
{
    const uint8_t minSym = std::min(l->minSymbol, r->minSymbol);
    return std::make_shared<HuffmanNode>(freq, minSym, l, r);
}

// DoD: push 100 phần tử, pop luôn ra phần tử nhỏ nhất.
static void testHundredElements()
{
    std::mt19937 rng(12345);
    std::uniform_int_distribution<uint64_t> dist(1, 1000);

    MinHeap heap;
    std::vector<uint64_t> expected;

    for (int i = 0; i < 100; ++i)
    {
        uint64_t f = dist(rng);
        expected.push_back(f);
        heap.push(leaf(static_cast<char>('a' + i % 26), f));
        assert(heap.top()->freq == *std::min_element(expected.begin(), expected.end()));
    }
    assert(heap.size() == 100);
    assert(!heap.empty());

    std::vector<uint64_t> sorted = expected;
    std::sort(sorted.begin(), sorted.end());
    for (int i = 0; i < 100; ++i)
    {
        assert(heap.top()->freq == sorted[i]);
        NodePtr n = heap.pop();
        assert(n->freq == sorted[i]);
        assert(heap.size() == static_cast<std::size_t>(99 - i));
    }
    assert(heap.empty());
}

// Xen kẽ push/pop: pop vẫn luôn là min của tập hiện tại.
static void testInterleaved()
{
    std::mt19937 rng(777);
    std::uniform_int_distribution<uint64_t> dist(1, 50);

    MinHeap heap;
    std::vector<uint64_t> ref;

    for (int step = 0; step < 500; ++step)
    {
        if (ref.empty() || rng() % 3 != 0)
        {
            uint64_t f = dist(rng);
            heap.push(leaf('x', f));
            ref.push_back(f);
        }
        else
        {
            auto it = std::min_element(ref.begin(), ref.end());
            NodePtr n = heap.pop();
            assert(n->freq == *it);
            ref.erase(it);
        }
        assert(heap.size() == ref.size());
    }
}

// Tie-break theo minSymbol: cùng freq thì minSymbol nhỏ hơn pop trước.
static void testTieBreakByMinSymbol()
{
    NodePtr z = leaf('z', 3);
    NodePtr y = leaf('y', 1);
    NodePtr aLeaf = leaf('a', 1);
    NodePtr bLeaf = leaf('b', 1);
    NodePtr internal = internalNode(2, aLeaf, bLeaf); // minSymbol = 'a'

    MinHeap heap;
    heap.push(z);
    heap.push(y);
    heap.push(internal);

    // Heap chứa: y(1), internal(2,'a'), z(3)
    assert(heap.pop() == y);
    assert(heap.pop() == internal);
    assert(heap.pop() == z);
    assert(heap.empty());
    assert(internal->minSymbol == static_cast<uint8_t>('a'));

    // Cùng freq: (1,'a') pop trước (1,'b') trước (1,'y').
    MinHeap tie;
    tie.push(y);
    tie.push(bLeaf);
    tie.push(aLeaf);
    assert(tie.pop() == aLeaf);
    assert(tie.pop() == bLeaf);
    assert(tie.pop() == y);
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
    NodePtr a = leaf('a', 5), b = leaf('b', 9);
    NodePtr parent = internalNode(14, a, b);
    assert(a->isLeaf && b->isLeaf);
    assert(!parent->isLeaf);
    assert(parent->left == a && parent->right == b);
    assert(parent->freq == a->freq + b->freq);
    assert(parent->minSymbol == static_cast<uint8_t>('a'));
}

// Tần suất lớn hơn 2^31 không bị tràn số (freq là uint64_t).
static void testHugeFrequencies()
{
    MinHeap heap;
    heap.push(leaf('a', 5000000000ULL));
    heap.push(leaf('b', 3000000000ULL));
    heap.push(leaf('c', 4000000000ULL));
    assert(heap.pop()->freq == 3000000000ULL);
    assert(heap.pop()->freq == 4000000000ULL);
    assert(heap.pop()->freq == 5000000000ULL);
}

int main()
{
    testHundredElements();
    testInterleaved();
    testTieBreakByMinSymbol();
    testEmptyAndErrors();
    testHuffmanNode();
    testHugeFrequencies();
    std::cout << "All MinHeap tests passed\n";
    return 0;
}
