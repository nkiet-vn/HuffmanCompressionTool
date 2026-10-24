#include "decompressor.h"
#include "huffmanTree.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace {

bool fail(const char* msg) {
    std::cerr << "Loi: " << msg << "\n";
    return false;
}

uint64_t readLE(const std::vector<uint8_t>& data, size_t pos, int bytes) {
    uint64_t value = 0;
    for (int i = 0; i < bytes; i++)
        value |= static_cast<uint64_t>(data[pos + i]) << (8 * i);
    return value;
}

} // namespace

bool Decompressor::decompress(const std::string& inputBinPath, const std::string& outputTxtPath) {
    std::ifstream in(inputBinPath, std::ios::binary);
    if (!in.is_open())
        return fail("Khong mo duoc file nen");

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)),
                              std::istreambuf_iterator<char>());
    in.close();

    // 1. Magic + uniqueCount + originalLength
    const size_t fixedHeader = 4 + 2 + 8;
    if (data.size() < fixedHeader)
        return fail("Header bi thieu du lieu");
    if (data[0] != 'H' || data[1] != 'U' || data[2] != 'F' || data[3] != '1')
        return fail("File khong dung dinh dang HUF1");

    uint64_t uniqueCount = readLE(data, 4, 2);
    uint64_t originalLength = readLE(data, 6, 8);
    if (uniqueCount > 256)
        return fail("uniqueCount vuot qua 256");

    // 2. Bảng tần suất
    const size_t tableBytes = static_cast<size_t>(uniqueCount) * 9;
    if (data.size() < fixedHeader + tableBytes)
        return fail("Bang tan suat bi thieu du lieu");

    uint64_t freq[256] = {};
    uint64_t sum = 0;
    int prevSymbol = -1;
    size_t pos = fixedHeader;
    for (uint64_t i = 0; i < uniqueCount; i++) {
        int symbol = data[pos];
        uint64_t f = readLE(data, pos + 1, 8);
        pos += 9;

        if (symbol <= prevSymbol)
            return fail("Bang tan suat khong sap xep tang dan hoac bi trung symbol");
        if (f == 0)
            return fail("Tan suat bang 0 trong bang tan suat");
        if (f > originalLength - sum)
            return fail("Tong tan suat khong khop originalLength");

        sum += f;
        freq[symbol] = f;
        prevSymbol = symbol;
    }
    if (sum != originalLength)
        return fail("Tong tan suat khac originalLength");

    std::ofstream out(outputTxtPath, std::ios::binary);
    if (!out.is_open())
        return fail("Khong tao duoc file giai nen");

    // 3. File rỗng
    if (uniqueCount == 0) {
        if (originalLength != 0)
            return fail("uniqueCount = 0 nhung originalLength > 0");
        out.close();
        return !out.fail();
    }

    // 4. Dựng cây cùng quy tắc với người nén
    uint64_t totalSymbols = 0;
    auto root = buildHuffmanTree(freq, totalSymbols);
    if (!root)
        return fail("Khong dung duoc cay Huffman");

    // 5. Giải mã bit (MSB-first). Bit đệm ở cuối bị bỏ qua.
    const size_t bitCount = (data.size() - pos) * 8;
    size_t bitIndex = 0;
    auto readBit = [&]() -> int {
        if (bitIndex >= bitCount) return -1;
        size_t byteIdx = pos + bitIndex / 8;
        int shift = 7 - static_cast<int>(bitIndex % 8);
        bitIndex++;
        return (data[byteIdx] >> shift) & 1;
    };

    for (uint64_t n = 0; n < originalLength; n++) {
        if (root->isLeaf) {
            // Chỉ có một symbol: mỗi symbol tốn đúng 1 bit (mã "0")
            if (readBit() == -1)
                return fail("Het du lieu bit truoc khi giai du");
            out.put(static_cast<char>(root->symbol));
            continue;
        }

        auto node = root;
        while (!node->isLeaf) {
            int bit = readBit();
            if (bit == -1)
                return fail("Het du lieu bit truoc khi giai du");
            node = (bit == 0) ? node->left : node->right;
        }
        out.put(static_cast<char>(node->symbol));
    }

    out.close();
    return !out.fail();
}
