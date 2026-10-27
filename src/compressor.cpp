#include "compressor.h"
#include "bitWriter.h"
#include "huffmanTree.h"
#include <fstream>
#include <iostream>
#include <filesystem>

namespace {

void writeLE(std::ostream& out, uint64_t value, int bytes) {
    for (int i = 0; i < bytes; i++) {
        out.put(static_cast<char>(value & 0xFF));
        value >>= 8;
    }
}

} // namespace

bool Compressor::compress(const std::string& inputPath, const std::string& outputBinPath) {
    namespace fs = std::filesystem;
    std::error_code ec;

    // Kiểm tra trùng file bằng filesystem::equivalent
    if (fs::exists(inputPath, ec) && fs::exists(outputBinPath, ec) && 
        fs::equivalent(inputPath, outputBinPath, ec)) {
        std::cerr << "Loi: file nguon va file dich khong duoc trung nhau!\n";
        return false;
    }

    // Lượt 1: đếm tần suất
    std::ifstream in(inputPath, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Loi: Khong mo duoc file nguon " << inputPath << "\n";
        return false;
    }

    uint64_t freq[256] = {};
    uint64_t originalLength = 0;
    char buffer[65536];
    while (in.read(buffer, sizeof(buffer)) || in.gcount() > 0) {
        std::streamsize n = in.gcount();
        for (std::streamsize i = 0; i < n; i++)
            freq[static_cast<unsigned char>(buffer[i])]++;
        originalLength += static_cast<uint64_t>(n);
    }
    if (in.bad()) {
        std::cerr << "Loi: Doc file nguon that bai\n";
        return false;
    }

    uint16_t uniqueCount = 0;
    for (int s = 0; s < 256; s++)
        if (freq[s] > 0) uniqueCount++;

    // Ghi ra file tạm để tránh ghi đè file gốc
    std::string tempPath = outputBinPath + ".tmp";
    std::ofstream out(tempPath, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Loi: Khong tao duoc file " << tempPath << "\n";
        return false;
    }

    out.write("HUF1", 4);
    writeLE(out, uniqueCount, 2);
    writeLE(out, originalLength, 8);
    for (int s = 0; s < 256; s++) {
        if (freq[s] == 0) continue;
        out.put(static_cast<char>(s));
        writeLE(out, freq[s], 8);
    }

    // File rỗng: không có dữ liệu bit
    if (originalLength != 0) {
        // Lượt 2: mã hóa bit
        uint64_t totalSymbols = 0;
        auto root = buildHuffmanTree(freq, totalSymbols);
        HuffmanCodeTable codeTable;
        buildCodeTable(root, codeTable);

        in.clear();
        in.seekg(0, std::ios::beg);
        BitWriter writer(out);
        while (in.read(buffer, sizeof(buffer)) || in.gcount() > 0) {
            std::streamsize n = in.gcount();
            for (std::streamsize i = 0; i < n; i++) {
                const std::string& code = codeTable[static_cast<unsigned char>(buffer[i])];
                for (char c : code)
                    writer.writeBit(c == '1' ? 1 : 0);
            }
        }
        writer.flush();
    }

    out.close();
    in.close();
    
    if (out.fail()) {
        fs::remove(tempPath, ec);
        return false;
    }
    
    // Đổi tên file tạm thành file đích
    fs::remove(outputBinPath, ec);
    fs::rename(tempPath, outputBinPath, ec);
    if (ec) {
        std::cerr << "Loi: Khong doi ten duoc file tam: " << ec.message() << "\n";
        return false;
    }
    
    return true;
}


