#include "decompressor.h"
#include "pathUtil.h"
#include "crc32.h"
#include "fileFormat.h"
#include "huffmanTree.h"
#include "bitReader.h"
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
#include <filesystem>

namespace
{

    bool fail(const char *msg)
    {
        std::cerr << "Loi: " << msg << "\n";
        return false;
    }

    // Cộng có kiểm tra tràn: trả về false nếu a + b vượt uint64.
    bool addNoOverflow(uint64_t a, uint64_t b, uint64_t &out)
    {
        if (b > UINT64_MAX - a)
            return false;
        out = a + b;
        return true;
    }

    // Đọc chính xác `length` byte theo khối 64 KB và cộng dồn CRC32. false nếu thiếu dữ liệu.
    bool readWithCrc(std::ifstream &in, std::vector<char> &buffer, uint64_t length,
                     uint32_t &crcState)
    {
        uint64_t left = length;
        while (left > 0)
        {
            const std::size_t want = static_cast<std::size_t>(
                left < IO_CHUNK_SIZE ? left : static_cast<uint64_t>(IO_CHUNK_SIZE));
            if (!in.read(buffer.data(), static_cast<std::streamsize>(want)))
                return false;
            crcState = crc32Update(crcState, buffer.data(), want);
            left -= want;
        }
        return true;
    }

    // Đọc bảng tần suất HUF1: uniqueCount mục × (symbol 1 byte + freq 8 byte LE).
    // Giữ nguyên mọi kiểm tra chặt của bản HUF1 trước đây.
    bool readFreqTableHuf1(std::ifstream &in, uint64_t uniqueCount, uint64_t originalLength,
                           uint64_t freq[256])
    {
        std::array<uint8_t, 9> entry{};
        uint64_t sum = 0;
        int prevSymbol = -1;
        for (uint64_t i = 0; i < uniqueCount; i++)
        {
            if (!in.read(reinterpret_cast<char *>(entry.data()), 9))
                return fail("Bang tan suat bi thieu du lieu");
            const int symbol = entry[0];
            const uint64_t f = readLittleEndian(entry.data() + 1, 8);

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
        return true;
    }

    // Đọc bảng tần suất HUF2: uniqueCount mục × (symbol 1 byte + freq LEB128).
    bool readFreqTableHuf2(std::ifstream &in, uint64_t uniqueCount, uint64_t originalLength,
                           uint64_t freq[256])
    {
        uint64_t sum = 0;
        int prevSymbol = -1;
        for (uint64_t i = 0; i < uniqueCount; i++)
        {
            const int next = in.get();
            if (next == std::char_traits<char>::eof())
                return fail("Bang tan suat bi thieu du lieu");
            const int symbol = static_cast<unsigned char>(next);

            uint64_t f = 0;
            if (!readLeb128(in, f))
                return fail("Varint tan suat khong hop le");

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
        return true;
    }

    // Số bit mã hóa cần cho `originalLength` ký tự theo cây `root`.
    // Trả về false nếu phép nhân/cộng bị tràn uint64.
    bool computeRequiredBits(const std::shared_ptr<HuffmanNode> &root, const uint64_t freq[256],
                             uint64_t originalLength, uint64_t &requiredBits)
    {
        requiredBits = 0;
        if (root->isLeaf)
        {
            requiredBits = originalLength;
            return true;
        }
        // Duyệt lặp để tránh đệ quy sâu.
        std::vector<std::pair<const HuffmanNode *, uint64_t>> stack;
        stack.push_back({root.get(), 0});
        while (!stack.empty())
        {
            auto entry = stack.back();
            stack.pop_back();
            const HuffmanNode *node = entry.first;
            const uint64_t depth = entry.second;
            if (node->isLeaf)
            {
                if (depth > UINT64_MAX / freq[node->symbol])
                    return false;
                uint64_t sum = 0;
                if (!addNoOverflow(depth * freq[node->symbol], requiredBits, sum))
                    return false;
                requiredBits = sum;
            }
            else
            {
                if (node->left)
                    stack.push_back({node->left.get(), depth + 1});
                if (node->right)
                    stack.push_back({node->right.get(), depth + 1});
            }
        }
        return true;
    }

    // Giải mã phần dữ liệu bit (MSB-first) và ghi ra `out` theo khối 64 KB,
    // tính CRC32 trên đúng các byte đã ghi ra. Trả về true nếu giải đủ originalLength symbol.
    bool decodeBits(const std::string &inputBinPath, std::streamoff dataOffset,
                    const std::shared_ptr<HuffmanNode> &root, uint64_t originalLength,
                    std::ofstream &out, uint32_t &crcOut)
    {
        BitReader bitReader(inputBinPath, dataOffset);
        std::vector<char> buffer;
        buffer.reserve(IO_CHUNK_SIZE);
        uint32_t crcState = crc32Begin();

        auto flushBuffer = [&]() -> bool
        {
            if (buffer.empty())
                return true;
            crcState = crc32Update(crcState, buffer.data(), buffer.size());
            out.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            buffer.clear();
            return !out.fail();
        };

        for (uint64_t n = 0; n < originalLength; n++)
        {
            int symbol = 0;
            if (root->isLeaf)
            {
                // Chỉ có một symbol: mỗi symbol tốn đúng 1 bit (mã "0").
                if (bitReader.readBit() == -1)
                    return false;
                symbol = root->symbol;
            }
            else
            {
                auto node = root;
                while (!node->isLeaf)
                {
                    const int bit = bitReader.readBit();
                    if (bit == -1)
                        return false;
                    node = (bit == 0) ? node->left : node->right;
                    if (!node)
                        return false;
                }
                symbol = node->symbol;
            }

            buffer.push_back(static_cast<char>(symbol));
            if (buffer.size() == IO_CHUNK_SIZE && !flushBuffer())
                return false;
        }
        if (!flushBuffer())
            return false;

        crcOut = crc32Finalize(crcState);
        return true;
    }

    // Giải mã mode HUFFMAN (dùng cho cả HUF1 và HUF2).
    // `in` phải đang ở ngay sau uniqueCount (2 byte), trước bảng tần suất.
    // `hasCrc` = true với HUF2, khi đó kiểm tra CRC32 sau khi giải xong.
    bool decompressHuffman(const std::string &inputBinPath, const std::string &outputTxtPath,
                           std::ifstream &in, uint64_t fileSize, uint64_t uniqueCount,
                           uint64_t originalLength, bool hasCrc, uint32_t expectedCrc)
    {
        // Đỉnh bảng tần suất: để phát hiện stream kết thúc sớm (uniqueCount là uint16).
        const std::streamoff tableStart = in.tellg();
        if (tableStart < 0)
            return fail("Header bi thieu du lieu");
        if (uniqueCount > 256)
            return fail("uniqueCount vuot qua 256");

        uint64_t freq[256] = {};
        const bool tableOk = hasCrc
                                 ? readFreqTableHuf2(in, uniqueCount, originalLength, freq)
                                 : readFreqTableHuf1(in, uniqueCount, originalLength, freq);
        if (!tableOk)
            return false;

        // `in` hiện ở ngay sau bảng tần suất = điểm bắt đầu dữ liệu bit.
        const std::streamoff dataStart = in.tellg();
        in.close();
        if (dataStart < 0)
            return fail("Bang tan suat bi thieu du lieu");
        const uint64_t pos = static_cast<uint64_t>(dataStart);
        if (pos > fileSize || pos < static_cast<uint64_t>(tableStart))
            return fail("Bang tan suat bi thieu du lieu");

        // File rỗng: chỉ có header, không có dữ liệu bit.
        if (uniqueCount == 0)
        {
            if (hasCrc && crc32Finalize(crc32Begin()) != expectedCrc)
                return fail("CRC32 khong khop: du lieu goc bi hong");
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
        {
            const uint64_t remainingBytes = fileSize - pos;
            const uint64_t availableBits = remainingBytes * 8;
            uint64_t requiredBits = 0;
            if (!computeRequiredBits(root, freq, originalLength, requiredBits))
                return fail("Kich thuoc du lieu bit khong hop le");
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
        uint32_t actualCrc = 0;
        try
        {
            decodeOk = decodeBits(inputBinPath, static_cast<std::streamoff>(pos), root,
                                  originalLength, out, actualCrc);
        } // BitReader đóng file ở đây
        catch (const std::exception &ex)
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
            cleanupOnError();
            return false;
        }

        // 6. HUF2: tự kiểm tra toàn vẹn dữ liệu gốc bằng CRC32.
        if (hasCrc && actualCrc != expectedCrc)
        {
            cleanupOnError();
            return fail("CRC32 khong khop: du lieu goc bi hong");
        }
        return true;
    }

    // Giải mã mode STORED (chỉ HUF2): dữ liệu gốc nằm nguyên vẹn ngay sau header 17 byte.
    bool decompressStored(const std::string &outputTxtPath, std::ifstream &in, uint64_t fileSize,
                          uint64_t originalLength, uint32_t expectedCrc)
    {
        if (fileSize < HEADER_SIZE)
            return fail("Header bi thieu du lieu");
        const uint64_t remaining = fileSize - HEADER_SIZE;
        if (remaining < originalLength)
            return fail("Du lieu STORED bi thieu");
        if (remaining > originalLength)
            return fail("File nen co du lieu thua o cuoi");

        std::ofstream out(toPath(outputTxtPath), std::ios::binary);
        if (!out.is_open())
            return fail("Khong tao duoc file giai nen");

        auto cleanupOnError = [&]()
        {
            out.close();
            std::error_code ec;
            std::filesystem::remove(toPath(outputTxtPath), ec);
        };

        // Ghi theo khối 64 KB, tính CRC trên đúng các byte đọc được rồi ghi ra.
        std::vector<char> buffer(IO_CHUNK_SIZE);
        uint32_t crcState = crc32Begin();
        uint64_t left = originalLength;
        while (left > 0)
        {
            const uint64_t step =
                left < IO_CHUNK_SIZE ? left : static_cast<uint64_t>(IO_CHUNK_SIZE);
            if (!readWithCrc(in, buffer, step, crcState))
            {
                cleanupOnError();
                return fail("Du lieu STORED bi thieu");
            }
            out.write(buffer.data(), static_cast<std::streamsize>(step));
            if (out.fail())
            {
                cleanupOnError();
                return fail("Khong ghi duoc file giai nen");
            }
            left -= step;
        }

        out.close();
        in.close();
        if (out.fail())
        {
            cleanupOnError();
            return false;
        }
        if (crc32Finalize(crcState) != expectedCrc)
        {
            cleanupOnError();
            return fail("CRC32 khong khop: du lieu goc bi hong");
        }
        return true;
    }

} // namespace

bool Decompressor::decompress(const std::string &inputBinPath, const std::string &outputTxtPath)
{
    std::ifstream in(toPath(inputBinPath), std::ios::binary);
    if (!in.is_open())
        return fail("Khong mo duoc file nen");

    std::error_code ec;
    if (areSameExistingFile(toPath(inputBinPath), toPath(outputTxtPath), ec))
        return fail("File nen va file dich khong duoc trung nhau");
    if (ec)
    {
        std::cerr << "Loi: Khong kiem tra duoc file dich: " << ec.message() << "\n";
        return false;
    }

    // Chỉ đọc header + bảng tần suất vào RAM (tối đa vài KB).
    // Phần dữ liệu được đọc dần theo khối 64 KB, nên bộ nhớ không phụ thuộc kích thước file.
    in.seekg(0, std::ios::end);
    const std::streamoff fileSizeRaw = in.tellg();
    if (!in || fileSizeRaw < 0)
        return fail("Khong xac dinh duoc kich thuoc file nen");
    const uint64_t fileSize = static_cast<uint64_t>(fileSizeRaw);
    in.seekg(0, std::ios::beg);
    if (!in)
        return fail("Khong doc duoc file nen");

    // 1. Magic (cần trước để biết HUF1 hay HUF2)
    if (fileSize < MAGIC_SIZE)
        return fail("Header bi thieu du lieu");
    char magic[MAGIC_SIZE] = {};
    if (!in.read(magic, MAGIC_SIZE))
        return fail("Header bi thieu du lieu");

    const int version = detectFormatVersion(magic);

    if (version == 2)
    {
        // HUF2: magic(4) + mode(1) + originalLength(8) + crc32(4)
        if (fileSize < HEADER_SIZE)
            return fail("Header bi thieu du lieu");

        std::array<uint8_t, HEADER_SIZE> header{};
        std::memcpy(header.data(), magic, MAGIC_SIZE);
        if (!in.read(reinterpret_cast<char *>(header.data() + MAGIC_SIZE),
                     static_cast<std::streamsize>(HEADER_SIZE - MAGIC_SIZE)))
            return fail("Header bi thieu du lieu");

        const uint8_t mode = header[4];
        const uint64_t originalLength = readLittleEndian(header.data() + 5, 8);
        const uint32_t expectedCrc =
            static_cast<uint32_t>(readLittleEndian(header.data() + 13, 4));

        if (mode == MODE_STORED)
            return decompressStored(outputTxtPath, in, fileSize, originalLength, expectedCrc);
        if (mode != MODE_HUFFMAN)
            return fail("Mode khong hop le (chi nhan 0 = Huffman, 1 = STORED)");

        // Header cố định 17 byte, sau đó là uniqueCount 2 byte.
        std::array<uint8_t, 2> countBytes{};
        if (!in.read(reinterpret_cast<char *>(countBytes.data()), 2))
            return fail("Header bi thieu du lieu");
        return decompressHuffman(inputBinPath, outputTxtPath, in, fileSize,
                                 readLittleEndian(countBytes.data(), 2), originalLength, true,
                                 expectedCrc);
    }

    if (version == 1)
    {
        // HUF1 (tương thích ngược): magic(4) + uniqueCount(2) + originalLength(8)
        constexpr uint64_t HUF1_HEADER = 4 + 2 + 8;
        if (fileSize < HUF1_HEADER)
            return fail("Header bi thieu du lieu");
        std::array<uint8_t, 2> countBytes{};
        std::array<uint8_t, 8> lengthBytes{};
        if (!in.read(reinterpret_cast<char *>(countBytes.data()), 2) ||
            !in.read(reinterpret_cast<char *>(lengthBytes.data()), 8))
            return fail("Header bi thieu du lieu");
        const uint64_t uniqueCount = readLittleEndian(countBytes.data(), 2);
        if (uniqueCount > 256)
            return fail("uniqueCount vuot qua 256");
        const uint64_t originalLength = readLittleEndian(lengthBytes.data(), 8);

        return decompressHuffman(inputBinPath, outputTxtPath, in, fileSize, uniqueCount,
                                 originalLength, false, 0);
    }

    return fail("File khong dung dinh dang HUF2 (magic khong hop le)");
}
