// Test cho lõi Huffman: thứ tự pop của MinHeap, cây 1 ký tự, bảng mã không có tiền tố trùng.
// Mỗi test in một dòng [PASS]/[FAIL]; chương trình trả về 0 khi tất cả pass.
#include "huffmanTree.h"

#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <random>
#include <string>
#include <vector>

namespace {

int failedChecks = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            ++failedChecks;                                                      \
            std::cout << "    check failed: " #cond " (line " << __LINE__ << ")\n"; \
        }                                                                        \
    } while (0)

using NodePtr = std::shared_ptr<HuffmanNode>;

// Bộ so sánh MinHeap theo docs/fileFormat.md mục 3: (weight, minSymbol) tăng dần.
struct DocOrder {
    bool operator()(const NodePtr& a, const NodePtr& b) const {
        if (a->freq != b->freq) return a->freq > b->freq;
        return a->minSymbol > b->minSymbol;
    }
};

uint8_t sym(char c) { return static_cast<uint8_t>(c); }

HuffmanCodeTable buildCodes(const uint64_t freq[256], NodePtr* rootOut = nullptr,
                            uint64_t* totalOut = nullptr) {
    uint64_t total = 0;
    NodePtr root = buildHuffmanTree(freq, total);
    HuffmanCodeTable table;
    buildCodeTable(root, table);
    if (rootOut) *rootOut = root;
    if (totalOut) *totalOut = total;
    return table;
}

// Không mã nào là tiền tố của mã khác, mã của symbol có mặt không rỗng, symbol vắng mặt rỗng.
void checkPrefixFree(const uint64_t freq[256], const HuffmanCodeTable& table) {
    std::vector<int> present;
    for (int s = 0; s < 256; ++s) {
        if (freq[s] > 0) {
            present.push_back(s);
            CHECK(!table[s].empty());
        } else {
            CHECK(table[s].empty());
        }
    }
    for (size_t i = 0; i < present.size(); ++i) {
        for (size_t j = 0; j < present.size(); ++j) {
            if (i == j) continue;
            const std::string& a = table[present[i]];
            const std::string& b = table[present[j]];
            CHECK(a != b);
            CHECK(!(a.size() <= b.size() && b.compare(0, a.size(), a) == 0));
        }
    }
    // Bất đẳng thức Kraft: cây đầy đủ (>= 2 symbol) thì tổng 2^-len == 1.
    if (present.size() >= 2) {
        long double kraft = 0;
        for (int s : present) kraft += std::ldexp(1.0L, -static_cast<int>(table[s].size()));
        CHECK(kraft > 0.9999999L && kraft < 1.0000001L);
    }
}

// ---------- Test 1: thứ tự pop của MinHeap ----------

void testHeapPopOrderByFreqThenMinSymbol() {
    std::priority_queue<NodePtr, std::vector<NodePtr>, DocOrder> heap;
    // Đẩy vào theo thứ tự lộn xộn.
    heap.push(std::make_shared<HuffmanNode>(sym('d'), 5));
    heap.push(std::make_shared<HuffmanNode>(sym('b'), 1));
    heap.push(std::make_shared<HuffmanNode>(sym('c'), 5));
    heap.push(std::make_shared<HuffmanNode>(sym('a'), 3));
    heap.push(std::make_shared<HuffmanNode>(sym('e'), 1));

    const std::vector<std::pair<uint64_t, uint8_t>> expected = {
        {1, sym('b')}, {1, sym('e')}, {3, sym('a')}, {5, sym('c')}, {5, sym('d')}};
    for (const auto& want : expected) {
        CHECK(!heap.empty());
        if (heap.empty()) break;
        CHECK(heap.top()->freq == want.first);
        CHECK(heap.top()->minSymbol == want.second);
        heap.pop();
    }
    CHECK(heap.empty());
}

void testHeapPopOrderInternalNodeUsesMinSymbol() {
    // Nút trong (weight 2, minSymbol 'a') phải pop trước lá (2, 'z') và sau lá (1, 'z').
    auto leafA = std::make_shared<HuffmanNode>(sym('a'), 1);
    auto leafB = std::make_shared<HuffmanNode>(sym('b'), 1);
    auto parent = std::make_shared<HuffmanNode>(2, sym('a'), leafA, leafB);
    std::priority_queue<NodePtr, std::vector<NodePtr>, DocOrder> heap;
    heap.push(std::make_shared<HuffmanNode>(sym('z'), 2));
    heap.push(parent);
    heap.push(std::make_shared<HuffmanNode>(sym('y'), 1));

    CHECK(heap.top()->isLeaf && heap.top()->symbol == sym('y')); heap.pop();
    CHECK(!heap.top()->isLeaf && heap.top()->minSymbol == sym('a')); heap.pop();
    CHECK(heap.top()->isLeaf && heap.top()->symbol == sym('z')); heap.pop();
}

// Kiểm thứ tự pop của heap thật trong buildHuffmanTree qua mã sinh ra (giá trị tính tay theo docs).
void testBuildTreeTieBreakAllEqual() {
    // a,b,c,d đều freq 1: pop a,b -> P(2,a); pop c,d -> P(2,c); pop P(a), P(c).
    uint64_t freq[256] = {};
    for (char c : std::string("abcd")) freq[sym(c)] = 1;
    auto t = buildCodes(freq);
    CHECK(t[sym('a')] == "00");
    CHECK(t[sym('b')] == "01");
    CHECK(t[sym('c')] == "10");
    CHECK(t[sym('d')] == "11");
}

void testBuildTreeTieBreakInternalVsLeaf() {
    // a=3,b=1,c=1,d=2: pop b,c -> P(2,b); pop P(2,b) rồi d(2,d) -> Q(4,b); pop a(3), Q(4).
    uint64_t freq[256] = {};
    freq[sym('a')] = 3; freq[sym('b')] = 1; freq[sym('c')] = 1; freq[sym('d')] = 2;
    auto t = buildCodes(freq);
    CHECK(t[sym('a')] == "0");
    CHECK(t[sym('b')] == "100");
    CHECK(t[sym('c')] == "101");
    CHECK(t[sym('d')] == "11");
}

void testBuildTreeIndependentOfInsertionOrder() {
    // Heap được nạp theo symbol 0..255 nên kết quả phải xác định; lặp lại phải ra cùng bảng mã.
    uint64_t freq[256] = {};
    std::mt19937 rng(12345);
    for (int s = 0; s < 256; ++s) freq[s] = rng() % 7;  // nhiều freq trùng nhau
    auto first = buildCodes(freq);
    auto second = buildCodes(freq);
    CHECK(first == second);
}

void testSampleFromDoc() {
    // docs/fileFormat.md mục 4: "aab" -> b = 0, a = 1.
    uint64_t freq[256] = {};
    freq[sym('a')] = 2; freq[sym('b')] = 1;
    uint64_t total = 0;
    NodePtr root;
    auto t = buildCodes(freq, &root, &total);
    CHECK(total == 3);
    CHECK(root && !root->isLeaf);
    CHECK(t[sym('b')] == "0");
    CHECK(t[sym('a')] == "1");
}

// ---------- Test 2: cây 1 ký tự ----------

void testSingleSymbolTree() {
    for (int s = 0; s < 256; ++s) {
        uint64_t freq[256] = {};
        freq[s] = 7;
        NodePtr root;
        uint64_t total = 0;
        auto t = buildCodes(freq, &root, &total);
        CHECK(total == 7);
        CHECK(root != nullptr);
        if (!root) continue;
        CHECK(root->isLeaf);
        CHECK(root->symbol == s);
        CHECK(root->left == nullptr && root->right == nullptr);
        CHECK(t[s] == "0");
        for (int o = 0; o < 256; ++o) {
            if (o != s) CHECK(t[o].empty());
        }
    }
}

void testSingleSymbolHugeFreq() {
    uint64_t freq[256] = {};
    freq[sym('x')] = 1ULL << 40;
    uint64_t total = 0;
    NodePtr root;
    auto t = buildCodes(freq, &root, &total);
    CHECK(total == (1ULL << 40));
    CHECK(root && root->isLeaf);
    CHECK(t[sym('x')] == "0");
}

void testEmptyTable() {
    uint64_t freq[256] = {};
    NodePtr root;
    uint64_t total = 99;
    auto t = buildCodes(freq, &root, &total);
    CHECK(root == nullptr);
    CHECK(total == 0);
    for (const auto& c : t) CHECK(c.empty());
}

void testBuildCodeTableClearsOldCodes() {
    uint64_t two[256] = {};
    two[sym('a')] = 2; two[sym('b')] = 1;
    uint64_t one[256] = {};
    one[sym('x')] = 5;
    HuffmanCodeTable table;
    uint64_t total = 0;
    buildCodeTable(buildHuffmanTree(two, total), table);
    buildCodeTable(buildHuffmanTree(one, total), table);
    CHECK(table[sym('a')].empty());
    CHECK(table[sym('b')].empty());
    CHECK(table[sym('x')] == "0");
}

// ---------- Test 3: bảng mã không có tiền tố trùng ----------

void testPrefixFreeSmall() {
    uint64_t freq[256] = {};
    freq[sym('a')] = 45; freq[sym('b')] = 13; freq[sym('c')] = 12;
    freq[sym('d')] = 16; freq[sym('e')] = 9;  freq[sym('f')] = 5;
    auto t = buildCodes(freq);
    checkPrefixFree(freq, t);
}

void testPrefixFreeAll256Uniform() {
    uint64_t freq[256];
    for (auto& f : freq) f = 1;
    auto t = buildCodes(freq);
    checkPrefixFree(freq, t);
    for (const auto& c : t) CHECK(c.size() == 8);  // phân bố đều -> mọi mã dài 8 bit
}

void testPrefixFreeFibonacciSkewed() {
    // Fibonacci cho cây lệch tối đa (mã dài nhất ~ n-1 bit).
    uint64_t freq[256] = {};
    uint64_t a = 1, b = 1;
    for (int s = 0; s < 40; ++s) {
        freq[s] = a;
        uint64_t n = a + b; a = b; b = n;
    }
    auto t = buildCodes(freq);
    checkPrefixFree(freq, t);
}

void testPrefixFreeRandomTables() {
    std::mt19937 rng(2024);
    for (int iter = 0; iter < 200; ++iter) {
        uint64_t freq[256] = {};
        int distinct = 1 + static_cast<int>(rng() % 256);
        for (int k = 0; k < distinct; ++k) freq[rng() % 256] = 1 + rng() % 1000;
        auto t = buildCodes(freq);
        checkPrefixFree(freq, t);
    }
}

struct TestCase {
    const char* name;
    std::function<void()> fn;
};

}  // namespace

int main() {
    const std::vector<TestCase> tests = {
        {"MinHeap pop: freq tang dan, hoa thi minSymbol nho truoc", testHeapPopOrderByFreqThenMinSymbol},
        {"MinHeap pop: nut trong so voi la theo (weight, minSymbol)", testHeapPopOrderInternalNodeUsesMinSymbol},
        {"Cay: 4 symbol cung freq cho ma 00/01/10/11", testBuildTreeTieBreakAllEqual},
        {"Cay: hoa freq giua nut trong va la", testBuildTreeTieBreakInternalVsLeaf},
        {"Cay: ket qua xac dinh khi lap lai", testBuildTreeIndependentOfInsertionOrder},
        {"Cay: vi du \"aab\" trong fileFormat.md", testSampleFromDoc},
        {"Cay 1 ky tu: moi gia tri byte 0..255 -> la duy nhat, ma \"0\"", testSingleSymbolTree},
        {"Cay 1 ky tu: tan suat rat lon", testSingleSymbolHugeFreq},
        {"Bang tan suat rong -> root null, total 0", testEmptyTable},
        {"buildCodeTable xoa ma cu truoc khi gan", testBuildCodeTableClearsOldCodes},
        {"Prefix-free: vi du kinh dien 6 symbol", testPrefixFreeSmall},
        {"Prefix-free: 256 symbol deu nhau (moi ma 8 bit)", testPrefixFreeAll256Uniform},
        {"Prefix-free: cay lech kieu Fibonacci", testPrefixFreeFibonacciSkewed},
        {"Prefix-free: 200 bang tan suat ngau nhien (seed co dinh)", testPrefixFreeRandomTables},
    };

    int passed = 0;
    for (const auto& tc : tests) {
        const int before = failedChecks;
        tc.fn();
        const bool ok = failedChecks == before;
        std::cout << (ok ? "[PASS] " : "[FAIL] ") << tc.name << "\n";
        if (ok) ++passed;
    }
    std::cout << "\nKet qua: " << passed << "/" << tests.size() << " test pass\n";
    return passed == static_cast<int>(tests.size()) ? 0 : 1;
}