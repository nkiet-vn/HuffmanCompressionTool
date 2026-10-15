#ifndef MIN_HEAP_H
#define MIN_HEAP_H

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

#include "huffman_node.h"

// MinHeap các HuffmanNode* theo tần suất (freq nhỏ nhất ở đỉnh).
class MinHeap {
public:
    void push(HuffmanNode* node) {                 // O(log n)
        if (node == nullptr) throw std::invalid_argument("MinHeap::push: node is null");
        data_.push_back(node);
        siftUp(data_.size() - 1);
    }

    HuffmanNode* pop() {                           // O(log n)
        if (empty()) throw std::out_of_range("MinHeap::pop: heap is empty");
        HuffmanNode* minNode = data_.front();
        data_.front() = data_.back();
        data_.pop_back();
        if (!data_.empty()) siftDown(0);
        return minNode;
    }

    HuffmanNode* top() const {                     // O(1)
        if (empty()) throw std::out_of_range("MinHeap::top: heap is empty");
        return data_.front();
    }

    std::size_t size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }

private:
    std::vector<HuffmanNode*> data_;

    static std::size_t parent(std::size_t i) { return (i - 1) / 2; }

    void siftUp(std::size_t i) {
        while (i > 0 && data_[i]->freq < data_[parent(i)]->freq) {
            std::swap(data_[i], data_[parent(i)]);
            i = parent(i);
        }
    }

    void siftDown(std::size_t i) {
        const std::size_t n = data_.size();
        while (true) {
            std::size_t smallest = i, l = 2 * i + 1, r = 2 * i + 2;
            if (l < n && data_[l]->freq < data_[smallest]->freq) smallest = l;
            if (r < n && data_[r]->freq < data_[smallest]->freq) smallest = r;
            if (smallest == i) break;
            std::swap(data_[i], data_[smallest]);
            i = smallest;
        }
    }
};

#endif