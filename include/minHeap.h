#ifndef MIN_HEAP_H
#define MIN_HEAP_H

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

#include "huffmanNode.h"

// MinHeap các MinHeapNode* theo tần suất (freq nhỏ nhất ở đỉnh).
class MinHeap {
public:
    void push(MinHeapNode* node) {                 // O(log n)
        if (node == nullptr) throw std::invalid_argument("MinHeap::push: node is null");
        data_.push_back(node);
        siftUp(data_.size() - 1);
    }

    MinHeapNode* pop() {                           // O(log n)
        if (empty()) throw std::out_of_range("MinHeap::pop: heap is empty");
        MinHeapNode* minNode = data_.front();
        data_.front() = data_.back();
        data_.pop_back();
        if (!data_.empty()) siftDown(0);
        return minNode;
    }

    MinHeapNode* top() const {                     // O(1)
        if (empty()) throw std::out_of_range("MinHeap::top: heap is empty");
        return data_.front();
    }

    std::size_t size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }

private:
    std::vector<MinHeapNode*> data_;

    static std::size_t parent(std::size_t i) { return (i - 1) / 2; }

    static bool less(const MinHeapNode* a, const MinHeapNode* b) {
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