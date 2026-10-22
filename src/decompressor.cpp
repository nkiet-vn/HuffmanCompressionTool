#include "decompressor.h"
#include "bitReader.h"
#include <fstream>
#include <iostream>
#include <queue>

// Hàm đọc uint64_t theo dạng Little-Endian
static bool readUint64LE(std::ifstream& file, uint64_t& value) {
    value = 0;
    for (int i = 0; i < 8; i++) {
        char ch;
        if (!file.get(ch)) return false;
        uint64_t byte = static_cast<unsigned char>(ch);
        value |= (byte << (i * 8));
    }
    return true;
}

// Struct so sánh cho priority_queue xây cây Huffman
struct CompareNode {
    bool operator()(const std::shared_ptr<HuffmanNode>& a, const std::shared_ptr<HuffmanNode>& b) const {
        return a->freq > b->freq;
    }
};

// Hàm dựng cây Huffman từ bảng tần số
static std::shared_ptr<HuffmanNode> buildHuffmanTree(const uint64_t freq[256], uint64_t& totalSymbols) {
    std::priority_queue<std::shared_ptr<HuffmanNode>, 
                        std::vector<std::shared_ptr<HuffmanNode>>, 
                        CompareNode> pq;

    totalSymbols = 0;
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            pq.push(std::make_shared<HuffmanNode>(static_cast<uint8_t>(i), freq[i]));
            totalSymbols += freq[i];
        }
    }

    if (pq.empty()) return nullptr;

    // Trường hợp file chỉ có 1 loại ký tự duy nhất
    if (pq.size() == 1) {
        auto singleLeaf = pq.top();
        auto root = std::make_shared<HuffmanNode>(singleLeaf->freq, singleLeaf, nullptr);
        return root;
    }

    while (pq.size() > 1) {
        auto left = pq.top(); pq.pop();
        auto right = pq.top(); pq.pop();

        auto parent = std::make_shared<HuffmanNode>(left->freq + right->freq, left, right);
        pq.push(parent);
    }

    return pq.top();
}

bool Decompressor::decompress(const std::string& inputBinPath, const std::string& outputTxtPath) {
    std::ifstream binFile(inputBinPath, std::ios::binary);
    if (!binFile.is_open()) {
        std::cerr << "Lỗi: Không thể mở file nén " << inputBinPath << std::endl;
        return false;
    }

    // 1. Kiểm tra Magic Number "HF01"
    char magic[4];
    if (!binFile.read(magic, 4) || magic[0] != 'H' || magic[1] != 'F' || magic[2] != '0' || magic[3] != '1') {
        std::cerr << "Lỗi: File không đúng định dạng nén HF01!" << std::endl;
        return false;
    }

    // 2. Đọc Bảng Tần Số 256 phần tử
    uint64_t freq[256];
    for (int i = 0; i < 256; i++) {
        if (!readUint64LE(binFile, freq[i])) {
            std::cerr << "Lỗi: Header bị thiếu dữ liệu!" << std::endl;
            return false;
        }
    }

    // 3. Tái tạo cây Huffman
    uint64_t totalSymbols = 0;
    auto root = buildHuffmanTree(freq, totalSymbols);

    if (!root || totalSymbols == 0) {
        // File rỗng
        std::ofstream outFile(outputTxtPath, std::ios::binary);
        return true;
    }

    std::ofstream outFile(outputTxtPath, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "Lỗi: Không thể tạo file giải nén " << outputTxtPath << std::endl;
        return false;
    }

    // 4. Giải nén luồng Bit bằng BitReader[cite: 4, 5]
    binFile.close(); // Đóng file stream để BitReader mở lại từ đầu đường dẫn
    BitReader reader(inputBinPath);

    // Bỏ qua 4 bytes Magic + 256 * 8 bytes Frequency Table trong BitReader
    for (size_t i = 0; i < 4 + 256 * 8; i++) {
        for (int b = 0; b < 8; b++) {
            reader.readBit();
        }
    }

    auto current = root;
    uint64_t decodedSymbols = 0;

    while (decodedSymbols < totalSymbols) {
        int bit = reader.readBit(); //[cite: 4, 5]
        if (bit == -1) break;

        current = (bit == 0) ? current->left : current->right;

        if (current && current->isLeaf) {
            outFile.put(static_cast<char>(current->symbol));
            decodedSymbols++;
            current = root; // Quay về gốc cho ký tự tiếp theo
        }
    }

    outFile.close();
    return decodedSymbols == totalSymbols;
}
