#include "decompressor.h"
#include "pathUtil.h"
#include "huffmanTree.h"
#include "bitReader.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <filesystem>

namespace
{

    bool fail(const char *msg)
    {
        std::cerr << "Loi: " << msg << "\n";
        return false;
    }

    uint64_t readLE(const std::vector<uint8_t> &data, size_t pos, int bytes)
    {
        uint64_t value = 0;
        for (int i = 0; i < bytes; i++)
            value |= static_cast<uint64_t>(data[pos + i]) << (8 * i);
        return value;
    }

} // namespace

bool Decompressor::decompress(const std::string &inputBinPath, const std::string &outputTxtPath)
{
    std::ifstream in(toPath(inputBinPath), std::ios::binary);
    if (!in.is_open())
        return fail("Khong mo duoc file nen");

    // Chỉ đọc phần header + bảng tần suất vào RAM (tối đa 14 + 256*9 byte).
    // Phần dữ liệu bit được BitReader đọc dần từ file, nên bộ nhớ không phụ thuộc kích thước file.
    in.seekg(0, std::ios::end);
    const std::streamoff fileSizeRaw = in.tellg();
    if (!in || fileSizeRaw < 0)
        return fail("Khong xac dinh duoc kich thuoc file nen");
    const uint64_t fileSize = static_cast<uint64_t>(fileSizeRaw);
    in.seekg(0, std::ios::beg);

    // 1. Magic + uniqueCount + originalLength
    const size_t fixedHeader = 4 + 2 + 8;
    if (fileSize < fixedHeader)
        return fail("Header bi thieu du lieu");

    std::vector<uint8_t> data(fixedHeader);
    if (!in.read(reinterpret_cast<char *>(data.data()), static_cast<std::streamsize>(fixedHeader)))
        return fail("Header bi thieu du lieu");
    if (data[0] != 'H' || data[1] != 'U' || data[2] != 'F' || data[3] != '1')
        return fail("File khong dung dinh dang HUF1");

    uint64_t uniqueCount = readLE(data, 4, 2);
    uint64_t originalLength = readLE(data, 6, 8);
    if (uniqueCount > 256)
        return fail("uniqueCount vuot qua 256");

    // 2. Bảng tần suất
    const size_t tableBytes = static_cast<size_t>(uniqueCount) * 9;
    if (fileSize < fixedHeader + tableBytes)
        return fail("Bang tan suat bi thieu du lieu");

    data.resize(fixedHeader + tableBytes);
    if (tableBytes > 0 &&
        !in.read(reinterpret_cast<char *>(data.data() + fixedHeader), static_cast<std::streamsize>(tableBytes)))
        return fail("Bang tan suat bi thieu du lieu");
    in.close();

    uint64_t freq[256] = {};
    uint64_t sum = 0;
    int prevSymbol = -1;
    size_t pos = fixedHeader;
    for (uint64_t i = 0; i < uniqueCount; i++)
    {
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

    // File rỗng: chỉ có header, không có dữ liệu bit
    if (uniqueCount == 0)
    {
        std::ofstream emptyOut(toPath(outputTxtPath), std::ios::binary);
        if (!emptyOut.is_open())
            return fail("Khong tao duoc file giai nen");
        emptyOut.close();
        return !emptyOut.fail();
    }

    // 3. Dựng cây cùng quy tắc với người nén
    uint64_t totalSymbols = 0;
    auto root = buildHuffmanTree(freq, totalSymbols);
    if (!root)
        return fail("Khong dung duoc cay Huffman");

    // 4. Kiểm tra kích thước phần dữ liệu bit ngay từ đầu.
    //    Số bit tối thiểu cần để mã hóa originalLength ký tự phải khớp với số byte còn lại
    //    (chỉ cho phép tối đa 7 bit đệm ở byte cuối).
    const uint64_t remainingBytes = fileSize - pos;
    const uint64_t availableBits = remainingBytes * 8;
    {
        // Độ dài mã của từng symbol; cây 1 ký tự dùng đúng 1 bit/ký tự.
        uint64_t requiredBits = 0;
        if (root->isLeaf)
        {
            requiredBits = originalLength;
        }
        else
        {
            // Tính tổng số bit cần thiết dựa trên cây (duyệt lặp để tránh đệ quy sâu).
            std::vector<std::pair<const HuffmanNode *, int>> stack;
            stack.push_back({root.get(), 0});
            while (!stack.empty())
            {
                auto [node, depth] = stack.back();
                stack.pop_back();
                if (node->isLeaf)
                {
                    requiredBits += static_cast<uint64_t>(depth) * freq[node->symbol];
                }
                else
                {
                    if (node->left)
                        stack.push_back({node->left.get(), depth + 1});
                    if (node->right)
                        stack.push_back({node->right.get(), depth + 1});
                }
            }
        }
        if (requiredBits > availableBits)
            return fail("Du lieu bit bi thieu");
        if (availableBits - requiredBits >= 8)
            return fail("File nen co du lieu thua o cuoi");
    }

    // 5. Giải mã bit (MSB-first) dùng BitReader, bỏ qua phần header
    std::ofstream out(toPath(outputTxtPath), std::ios::binary);
    if (!out.is_open())
        return fail("Khong tao duoc file giai nen");

    auto cleanupOnError = [&]()
    {
        out.close();
        std::error_code ec;
        std::filesystem::remove(toPath(outputTxtPath), ec);
    };

    bool decodeOk = true;
    try
    {
        BitReader bitReader(inputBinPath, static_cast<std::streamoff>(pos));

        for (uint64_t n = 0; n < originalLength && decodeOk; n++)
        {
            if (root->isLeaf)
            {
                // Chỉ có một symbol: mỗi symbol tốn đúng 1 bit (mã "0")
                if (bitReader.readBit() == -1)
                {
                    decodeOk = false;
                    break;
                }
                out.put(static_cast<char>(root->symbol));
                continue;
            }

            auto node = root;
            while (!node->isLeaf)
            {
                int bit = bitReader.readBit();
                if (bit == -1)
                {
                    decodeOk = false;
                    break;
                }
                node = (bit == 0) ? node->left : node->right;
                if (!node)
                {
                    decodeOk = false;
                    break;
                }
            }
            if (decodeOk)
                out.put(static_cast<char>(node->symbol));
        }
    } // BitReader đóng file ở đây
    catch (const std::exception& ex)
    {
        cleanupOnError();
        std::cerr << "Loi: " << ex.what() << "\n";
        return false;
    }

    if (!decodeOk)
    {
        cleanupOnError();
        return fail("Du lieu bit khong hop le hoac bi thieu");
    }

    out.close();
    if (out.fail())
    {
        std::error_code ec;
        std::filesystem::remove(toPath(outputTxtPath), ec);
        return false;
    }
    return true;
}
