#include "decompressor.h"
#include "bitReader.h"
#include <fstream>
#include <iostream>

bool Decompressor::decompress(const std::string& inputBinPath, const std::string& outputTxtPath) {
    try {
        BitReader reader(inputBinPath);

        // TODO: Đọc Header từ file .bin theo quy định trong docs/fileFormat.md
        // Lấy số lượng ký tự ban đầu và dữ liệu tái tạo cây Huffman
        uint64_t totalSymbols = 0; 

        // Khôi phục cây Huffman từ thông tin Header
        std::shared_ptr<HuffmanNode> root = std::make_shared<HuffmanNode>();
        
        std::ofstream outFile(outputTxtPath, std::ios::binary);
        if (!outFile.is_open()) {
            std::cerr << "Error: Không thể mở file đầu ra " << outputTxtPath << std::endl;
            return false;
        }

        // Đọc từng bit để duyệt cây giải mã
        auto current = root;
        uint64_t decodedSymbols = 0;

        while (decodedSymbols < totalSymbols) {
            int bit = reader.readBit();
            if (bit == -1) break;

            current = (bit == 0) ? current->left : current->right;

            if (current && current->isLeaf) {
                outFile.put(static_cast<char>(current->symbol));
                decodedSymbols++;
                current = root; // Quay lại gốc cây cho ký tự tiếp theo
            }
        }

        outFile.close();
        return decodedSymbols == totalSymbols;
    } 
    catch (const std::exception& e) {
        std::cerr << "Lỗi giải nén: " << e.what() << std::endl;
        return false;
    }
}
