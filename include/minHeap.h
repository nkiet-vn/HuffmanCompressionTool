#ifndef MIN_HEAP_H
#define MIN_HEAP_H

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "huffmanTree.h"

// Min-Heap tự viết chứa các nút cây Huffman (HuffmanNode).
// Khóa so sánh: (freq, minSymbol) tăng dần, đúng đặc tả trong docs/fileFormat.md mục 3.
// Dùng chung một kiểu nút với cây Huffman nên không cần chuyển đổi và không cần xóa thủ công:
// shared_ptr tự giải phóng cây khi không còn ai giữ.
class MinHeap {
public:
    using NodePtr = std::shared_ptr<HuffmanNode>;

    void push(NodePtr node) {                      // O(log n)
        if (!node) throw std::invalid_argument("MinHeap::push: node is null");
        data_.push_back(std::move(node));
        siftUp(data_.size() - 1);
    }

    NodePtr pop() {                                // O(log n)
        if (empty()) throw std::out_of_range("MinHeap::pop: heap is empty");
        NodePtr minNode = std::move(data_.front());
        data_.front() = std::move(data_.back());
        data_.pop_back();
        if (!data_.empty()) siftDown(0);
        return minNode;
    }

    const NodePtr& top() const {                   // O(1)
        if (empty()) throw std::out_of_range("MinHeap::top: heap is empty");
        return data_.front();
    }

    std::size_t size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }

private:
    std::vector<NodePtr> data_;

    static std::size_t parent(std::size_t i) { return (i - 1) / 2; }

    static bool less(const NodePtr& a, const NodePtr& b) {
        if (a->freq != b->freq) return a->freq < b->freq;
        return a->minSymbol < b->minSymbol;
    }

    void siftUp(std::size_t i) {
        while (i > 0) {
            std::size_t p = parent(i);
            if (less(data_[i], data_[p])) {
                std::swap(data_[i], data_[p]);
                i = p;
            } else {
                break;
            }
        }
    }

    void siftDown(std::size_t i) {
        const std::size_t n = data_.size();
        while (true) {
            std::size_t smallest = i, l = 2 * i + 1, r = 2 * i + 2;
            if (l < n && less(data_[l], data_[smallest])) smallest = l;
            if (r < n && less(data_[r], data_[smallest])) smallest = r;
            if (smallest == i) break;
            std::swap(data_[i], data_[smallest]);
            i = smallest;
        }
    }
};

#endif
