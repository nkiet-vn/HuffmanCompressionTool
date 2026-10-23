#include "compressor.h"
#include "bitWriter.h"
#include "huffmanTree.h"
#include "codec.h"
#include <fstream>
#include <iostream>

namespace {

void writeLE(std::ostream& out, uint64_t value, int bytes) {
    for (int i = 0; i < bytes; i++) {
        out.put(static_cast<char>(value & 0xFF));
        value >>= 8;
    }
}

} // namespace

bool Compressor::compress(const std::string& inputPath, const std::string& outputBinPath) {
    if (inputPath == outputBinPath) {
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

    // Ghi header
    std::ofstream out(outputBinPath, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Loi: Khong tao duoc file " << outputBinPath << "\n";
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
    if (originalLength == 0) {
        out.close();
        return !out.fail();
    }

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

    out.close();
    return !out.fail();
}

// API tương thích với nhánh DSA-4 (codec.h): bọc Compressor để trả về thông báo lỗi.
bool compressFile(const std::string& inPath, const std::string& outPath, std::string& error) {
    {
        std::ifstream probe(inPath, std::ios::binary);
        if (!probe.is_open()) {
            error = "Khong mo duoc file nguon: " + inPath;
            return false;
        }
    }
    if (!Compressor().compress(inPath, outPath)) {
        error = "Nen that bai: " + inPath;
        return false;
    }
    return true;
}
