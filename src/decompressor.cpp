#include "decompressor.h"
#include "pathUtil.h"
#include "crc32.h"
#include "fileFormat.h"
#include "huffmanTree.h"
#include "bitReader.h"
#include "canonicalHuffman.h"
#include <array>
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <vector>
#include <filesystem>

namespace
{

    struct OperationResult
    {
        bool success;
        MessageId error = MessageId::None;
        std::string details;
    };

    OperationResult successResult()
    {
        return {true, MessageId::None, {}};
    }

    OperationResult errorResult(MessageId error, const std::string &details = {})
    {
        return {false, error, details};
    }

    class OutputGuard
    {
    public:
        explicit OutputGuard(const std::string &path) : path_(toPath(path)) {}

        ~OutputGuard()
        {
            if (armed_ && !committed_)
            {
                std::error_code ec;
                std::filesystem::remove(path_, ec);
            }
        }

        void arm()
        {
            armed_ = true;
        }

        void commit()
        {
            committed_ = true;
        }

    private:
        std::filesystem::path path_;
        bool armed_ = false;
        bool committed_ = false;
    };

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

    bool readHuf1Frequency(std::istream &in, uint64_t &frequency)
    {
        std::array<uint8_t, 8> bytes{};
        if (!in.read(reinterpret_cast<char *>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size())))
            return false;
        frequency = readLittleEndian(bytes.data(), static_cast<int>(bytes.size()));
        return true;
    }

    bool readHuf2Frequency(std::istream &in, uint64_t &frequency)
    {
        return readLeb128(in, frequency);
    }

    using FrequencyReader = bool (*)(std::istream &, uint64_t &);

    OperationResult readFrequencyTable(std::istream &in, uint64_t uniqueCount,
                                       uint64_t originalLength, uint64_t freq[256],
                                       FrequencyReader readFrequency, MessageId frequencyError)
    {
        uint64_t sum = 0;
        int prevSymbol = -1;
        for (uint64_t i = 0; i < uniqueCount; i++)
        {
            const int next = in.get();
            if (next == std::char_traits<char>::eof())
                return errorResult(MessageId::CodecFrequencyTableShort);
            const int symbol = static_cast<unsigned char>(next);

            uint64_t f = 0;
            if (!readFrequency(in, f))
                return errorResult(frequencyError);

            if (symbol <= prevSymbol)
                return errorResult(MessageId::CodecFrequencyOrder);
            if (f == 0)
                return errorResult(MessageId::CodecZeroFrequency);
            if (f > originalLength - sum)
                return errorResult(MessageId::CodecFrequencyOverflow);

            sum += f;
            freq[symbol] = f;
            prevSymbol = symbol;
        }
        if (sum != originalLength)
            return errorResult(MessageId::CodecFrequencyTotalMismatch);
        return successResult();
    }

    struct HuffmanFrequencyTable
    {
        uint64_t frequencies[256] = {};
        std::streamoff dataOffset = 0;
    };

    OperationResult readHuffmanFrequencyTable(std::ifstream &in, uint64_t fileSize,
                                              uint64_t uniqueCount, uint64_t originalLength,
                                              bool hasCrc, HuffmanFrequencyTable &table)
    {
        const std::streamoff tableStart = in.tellg();
        if (tableStart < 0)
            return errorResult(MessageId::CodecHeaderShort);
        if (uniqueCount > 256)
            return errorResult(MessageId::CodecUniqueCountTooLarge);

        const OperationResult tableResult =
            readFrequencyTable(in, uniqueCount, originalLength, table.frequencies,
                               hasCrc ? readHuf2Frequency : readHuf1Frequency,
                               hasCrc ? MessageId::CodecInvalidFrequencyVarint
                                      : MessageId::CodecFrequencyTableShort);
        if (!tableResult.success)
            return tableResult;

        table.dataOffset = in.tellg();
        if (table.dataOffset < 0)
            return errorResult(MessageId::CodecFrequencyTableShort);
        const uint64_t pos = static_cast<uint64_t>(table.dataOffset);
        if (pos > fileSize || pos < static_cast<uint64_t>(tableStart))
            return errorResult(MessageId::CodecFrequencyTableShort);
        return successResult();
    }

    struct HuffmanLengthTable
    {
        std::uint8_t lengths[256] = {};
        std::vector<CanonicalCode> codes;
        std::streamoff dataOffset = 0;
    };

    OperationResult readHuffmanLengthTable(std::ifstream &in, uint64_t fileSize,
                                           uint64_t uniqueCount, uint64_t originalLength,
                                           HuffmanLengthTable &table)
    {
        if (uniqueCount > 256)
            return errorResult(MessageId::CodecUniqueCountTooLarge);
        if ((originalLength == 0) != (uniqueCount == 0))
            return errorResult(MessageId::CodecInvalidCodeTable);

        int previousSymbol = -1;
        for (uint64_t i = 0; i < uniqueCount; ++i)
        {
            const int nextSymbol = in.get();
            const int nextLength = in.get();
            if (nextSymbol == std::char_traits<char>::eof() ||
                nextLength == std::char_traits<char>::eof())
                return errorResult(MessageId::CodecFrequencyTableShort);

            const int symbol = static_cast<unsigned char>(nextSymbol);
            const int length = static_cast<unsigned char>(nextLength);
            if (symbol <= previousSymbol)
                return errorResult(MessageId::CodecFrequencyOrder);
            if (length == 0 || length > 32)
                return errorResult(MessageId::CodecInvalidCodeTable);
            table.lengths[symbol] = static_cast<std::uint8_t>(length);
            previousSymbol = symbol;
        }

        if (!buildCanonicalCodes(table.lengths, table.codes))
            return errorResult(MessageId::CodecInvalidCodeTable);
        table.dataOffset = in.tellg();
        if (table.dataOffset < 0)
            return errorResult(MessageId::CodecFrequencyTableShort);
        const uint64_t position = static_cast<uint64_t>(table.dataOffset);
        if (position > fileSize)
            return errorResult(MessageId::CodecFrequencyTableShort);
        return successResult();
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

    struct FlatNode
    {
        uint8_t symbol = 0;
        std::size_t left = std::numeric_limits<std::size_t>::max();
        std::size_t right = std::numeric_limits<std::size_t>::max();
        bool isLeaf = false;
    };

    std::size_t appendFlatNode(const std::shared_ptr<HuffmanNode> &node,
                               std::vector<FlatNode> &flatNodes)
    {
        const std::size_t index = flatNodes.size();
        flatNodes.push_back({node->symbol, std::numeric_limits<std::size_t>::max(),
                             std::numeric_limits<std::size_t>::max(), node->isLeaf});
        if (!node->isLeaf)
        {
            const std::size_t left = appendFlatNode(node->left, flatNodes);
            const std::size_t right = appendFlatNode(node->right, flatNodes);
            flatNodes[index].left = left;
            flatNodes[index].right = right;
        }
        return index;
    }

    std::vector<FlatNode> buildFlatTree(const std::shared_ptr<HuffmanNode> &root)
    {
        std::vector<FlatNode> flatNodes;
        flatNodes.reserve(511);
        appendFlatNode(root, flatNodes);
        return flatNodes;
    }

    bool buildCanonicalDecodeTree(const std::vector<CanonicalCode> &codes,
                                  std::vector<FlatNode> &flatNodes)
    {
        const std::size_t noChild = std::numeric_limits<std::size_t>::max();
        flatNodes.clear();
        flatNodes.push_back({});
        for (const CanonicalCode &code : codes)
        {
            std::size_t nodeIndex = 0;
            for (int bitIndex = static_cast<int>(code.length) - 1; bitIndex >= 0; --bitIndex)
            {
                const bool right = ((code.bits >> bitIndex) & 1U) != 0;
                std::size_t child = right ? flatNodes[nodeIndex].right : flatNodes[nodeIndex].left;
                const bool lastBit = bitIndex == 0;
                if (child == noChild)
                {
                    child = flatNodes.size();
                    if (right)
                        flatNodes[nodeIndex].right = child;
                    else
                        flatNodes[nodeIndex].left = child;
                    FlatNode node;
                    node.isLeaf = lastBit;
                    if (lastBit)
                        node.symbol = code.symbol;
                    flatNodes.push_back(node);
                }
                else if (lastBit || flatNodes[child].isLeaf)
                {
                    return false;
                }
                nodeIndex = child;
            }
        }
        return !codes.empty() || flatNodes.size() == 1;
    }

    OperationResult validateHuffmanBitSize(const std::shared_ptr<HuffmanNode> &root,
                                           const uint64_t freq[256], uint64_t originalLength,
                                           uint64_t fileSize, uint64_t dataOffset)
    {
        const uint64_t remainingBytes = fileSize - dataOffset;
        const uint64_t availableBits = remainingBytes * 8;
        uint64_t requiredBits = 0;
        if (!computeRequiredBits(root, freq, originalLength, requiredBits))
            return errorResult(MessageId::CodecInvalidBitSize);
        if (requiredBits > availableBits)
            return errorResult(MessageId::CodecBitDataShort);
        if (availableBits - requiredBits >= 8)
            return errorResult(MessageId::CodecTrailingData);
        return successResult();
    }

    struct DecodeResult
    {
        OperationResult result;
        uint32_t crc;
    };

    DecodeResult decodeBits(const std::string &inputBinPath, std::streamoff dataOffset,
                            const std::vector<FlatNode> &flatNodes, uint64_t originalLength,
                            std::ofstream &out, const ProgressCallback &progress,
                            bool canonical, bool validatePadding)
    {
        try
        {
            BitReader bitReader(inputBinPath, dataOffset);
            std::vector<char> buffer;
            buffer.reserve(IO_CHUNK_SIZE);
            uint32_t crcState = crc32Begin();
            uint64_t processed = 0;

            auto flushBuffer = [&]() -> bool
            {
                if (buffer.empty())
                    return true;
                crcState = crc32Update(crcState, buffer.data(), buffer.size());
                out.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
                buffer.clear();
                return !out.fail();
            };

            const std::size_t noChild = std::numeric_limits<std::size_t>::max();
            for (uint64_t n = 0; n < originalLength; n++)
            {
                std::size_t nodeIndex = 0;
                if (flatNodes[nodeIndex].isLeaf)
                {
                    // Chỉ có một symbol: mỗi symbol tốn đúng 1 bit (mã "0").
                    const int bit = bitReader.readBit();
                    if (bit == -1 || (canonical && bit != 0))
                        return {errorResult(MessageId::CodecInvalidBitData), 0};
                }
                else
                {
                    while (!flatNodes[nodeIndex].isLeaf)
                    {
                        const int bit = bitReader.readBit();
                        if (bit == -1)
                            return {errorResult(MessageId::CodecInvalidBitData), 0};
                        nodeIndex = bit == 0 ? flatNodes[nodeIndex].left : flatNodes[nodeIndex].right;
                        if (nodeIndex == noChild)
                            return {errorResult(MessageId::CodecInvalidBitData), 0};
                    }
                }

                buffer.push_back(static_cast<char>(flatNodes[nodeIndex].symbol));
                if (buffer.size() == IO_CHUNK_SIZE)
                {
                    if (!flushBuffer())
                        return {errorResult(MessageId::CodecWriteOutput), 0};
                    processed += buffer.capacity();
                    if (progress)
                        progress(processed, originalLength);
                }
            }
            const std::size_t remaining = buffer.size();
            if (!flushBuffer())
                return {errorResult(MessageId::CodecWriteOutput), 0};
            processed += remaining;
            if (progress && remaining > 0)
                progress(processed, originalLength);

            if (validatePadding)
            {
                unsigned int paddingBits = 0;
                int bit = 0;
                while ((bit = bitReader.readBit()) != -1)
                {
                    ++paddingBits;
                    if (paddingBits > 7 || bit != 0)
                        return {errorResult(MessageId::CodecTrailingData), 0};
                }
            }

            return {successResult(), crc32Finalize(crcState)};
        }
        catch (const std::exception &ex)
        {
            return {errorResult(MessageId::CodecInternalFailure, ex.what()), 0};
        }
    }

    OperationResult validateCrc(uint32_t actualCrc, uint32_t expectedCrc, bool hasCrc)
    {
        if (hasCrc && actualCrc != expectedCrc)
            return errorResult(MessageId::CodecChecksumMismatch);
        return successResult();
    }

    OperationResult createEmptyOutput(const std::string &outputTxtPath)
    {
        OutputGuard outputGuard(outputTxtPath);
        std::ofstream emptyOut(toPath(outputTxtPath), std::ios::binary);
        if (!emptyOut.is_open())
            return errorResult(MessageId::CodecCreateOutput);
        outputGuard.arm();
        emptyOut.close();
        if (emptyOut.fail())
            return errorResult(MessageId::CodecWriteOutput);
        outputGuard.commit();
        return successResult();
    }

    OperationResult decodeHuffmanOutput(const std::string &inputBinPath,
                                        const std::string &outputTxtPath,
                                        const HuffmanFrequencyTable &table,
                                        const std::shared_ptr<HuffmanNode> &root,
                                        uint64_t originalLength, uint32_t expectedCrc,
                                        bool hasCrc, const ProgressCallback &progress)
    {
        const std::vector<FlatNode> flatNodes = buildFlatTree(root);
        OutputGuard outputGuard(outputTxtPath);
        std::ofstream out(toPath(outputTxtPath), std::ios::binary);
        if (!out.is_open())
            return errorResult(MessageId::CodecCreateOutput);
        outputGuard.arm();

        DecodeResult decoded =
            decodeBits(inputBinPath, table.dataOffset, flatNodes, originalLength, out, progress,
                       false, false);
        if (!decoded.result.success)
        {
            out.close();
            return decoded.result;
        }

        out.close();
        if (out.fail())
            return errorResult(MessageId::CodecWriteOutput);

        const OperationResult crcResult = validateCrc(decoded.crc, expectedCrc, hasCrc);
        if (!crcResult.success)
            return crcResult;

        outputGuard.commit();
        return successResult();
    }

    OperationResult decodeCanonicalHuffmanOutput(const std::string &inputBinPath,
                                                 const std::string &outputTxtPath,
                                                 const HuffmanLengthTable &table,
                                                 uint64_t originalLength,
                                                 uint32_t expectedCrc,
                                                 const ProgressCallback &progress)
    {
        std::vector<FlatNode> flatNodes;
        if (!buildCanonicalDecodeTree(table.codes, flatNodes))
            return errorResult(MessageId::CodecInvalidCodeTable);

        OutputGuard outputGuard(outputTxtPath);
        std::ofstream out(toPath(outputTxtPath), std::ios::binary);
        if (!out.is_open())
            return errorResult(MessageId::CodecCreateOutput);
        outputGuard.arm();

        DecodeResult decoded =
            decodeBits(inputBinPath, table.dataOffset, flatNodes, originalLength,
                       out, progress, true, true);
        if (!decoded.result.success)
        {
            out.close();
            return decoded.result;
        }

        out.close();
        if (out.fail())
            return errorResult(MessageId::CodecWriteOutput);
        const OperationResult crcResult =
            validateCrc(decoded.crc, expectedCrc, true);
        if (!crcResult.success)
            return crcResult;

        outputGuard.commit();
        return successResult();
    }

    OperationResult decompressHuf3(const std::string &inputBinPath,
                                   const std::string &outputTxtPath,
                                   std::ifstream &in, uint64_t fileSize,
                                   uint64_t uniqueCount, uint64_t originalLength,
                                   uint32_t expectedCrc, const ProgressCallback &progress)
    {
        HuffmanLengthTable table;
        const OperationResult tableResult =
            readHuffmanLengthTable(in, fileSize, uniqueCount, originalLength, table);
        if (!tableResult.success)
            return tableResult;
        in.close();
        return decodeCanonicalHuffmanOutput(inputBinPath, outputTxtPath, table,
                                            originalLength, expectedCrc, progress);
    }

    // Giải nén mode HUFFMAN (dùng cho cả HUF1 và HUF2).
    OperationResult decompressHuffman(const std::string &inputBinPath,
                                      const std::string &outputTxtPath, std::ifstream &in,
                                      uint64_t fileSize, uint64_t uniqueCount,
                                      uint64_t originalLength, bool hasCrc, uint32_t expectedCrc,
                                      const ProgressCallback &progress)
    {
        HuffmanFrequencyTable table;
        OperationResult result = readHuffmanFrequencyTable(
            in, fileSize, uniqueCount, originalLength, hasCrc, table);
        if (!result.success)
            return result;
        in.close();

        if (uniqueCount == 0)
        {
            result = validateCrc(crc32Finalize(crc32Begin()), expectedCrc, hasCrc);
            if (!result.success)
                return result;
            return createEmptyOutput(outputTxtPath);
        }

        uint64_t totalSymbols = 0;
        const std::shared_ptr<HuffmanNode> root =
            buildHuffmanTree(table.frequencies, totalSymbols);
        if (!root)
            return errorResult(MessageId::CodecInvalidTree);

        result = validateHuffmanBitSize(root, table.frequencies, originalLength, fileSize,
                                        static_cast<uint64_t>(table.dataOffset));
        if (!result.success)
            return result;

        return decodeHuffmanOutput(inputBinPath, outputTxtPath, table, root, originalLength,
                                   expectedCrc, hasCrc, progress);
    }

    // Giải mã mode STORED (HUF2/HUF3): dữ liệu gốc nằm ngay sau header 17 byte.
    OperationResult decompressStored(const std::string &outputTxtPath, std::ifstream &in,
                                     uint64_t fileSize, uint64_t originalLength,
                                     uint32_t expectedCrc, const ProgressCallback &progress)
    {
        if (fileSize < HEADER_SIZE)
            return errorResult(MessageId::CodecHeaderShort);
        const uint64_t remaining = fileSize - HEADER_SIZE;
        if (remaining < originalLength)
            return errorResult(MessageId::CodecStoredDataShort);
        if (remaining > originalLength)
            return errorResult(MessageId::CodecTrailingData);

        OutputGuard outputGuard(outputTxtPath);
        std::ofstream out(toPath(outputTxtPath), std::ios::binary);
        if (!out.is_open())
            return errorResult(MessageId::CodecCreateOutput);
        outputGuard.arm();

        // Ghi theo khối 64 KB, tính CRC trên đúng các byte đọc được rồi ghi ra.
        std::vector<char> buffer(IO_CHUNK_SIZE);
        uint32_t crcState = crc32Begin();
        uint64_t left = originalLength;
        uint64_t processed = 0;
        while (left > 0)
        {
            const uint64_t step =
                left < IO_CHUNK_SIZE ? left : static_cast<uint64_t>(IO_CHUNK_SIZE);
            if (!readWithCrc(in, buffer, step, crcState))
                return errorResult(MessageId::CodecStoredDataShort);
            out.write(buffer.data(), static_cast<std::streamsize>(step));
            if (out.fail())
                return errorResult(MessageId::CodecWriteOutput);
            processed += step;
            if (progress)
                progress(processed, originalLength);
            left -= step;
        }

        out.close();
        in.close();
        if (out.fail())
            return errorResult(MessageId::CodecWriteOutput);
        if (crc32Finalize(crcState) != expectedCrc)
            return errorResult(MessageId::CodecChecksumMismatch);
        outputGuard.commit();
        return successResult();
    }

    struct InputHeader
    {
        int version = 0;
        uint8_t mode = MODE_HUFFMAN;
        uint64_t uniqueCount = 0;
        uint64_t originalLength = 0;
        uint32_t expectedCrc = 0;
    };

    OperationResult readInputHeader(std::ifstream &in, uint64_t fileSize, InputHeader &header)
    {
        if (fileSize < MAGIC_SIZE)
            return errorResult(MessageId::CodecHeaderShort);
        char magic[MAGIC_SIZE] = {};
        if (!in.read(magic, MAGIC_SIZE))
            return errorResult(MessageId::CodecHeaderShort);

        header.version = detectFormatVersion(magic);
        if (header.version == 2 || header.version == 3)
        {
            if (fileSize < HEADER_SIZE)
                return errorResult(MessageId::CodecHeaderShort);

            std::array<uint8_t, HEADER_SIZE> bytes{};
            std::memcpy(bytes.data(), magic, MAGIC_SIZE);
            if (!in.read(reinterpret_cast<char *>(bytes.data() + MAGIC_SIZE),
                         static_cast<std::streamsize>(HEADER_SIZE - MAGIC_SIZE)))
                return errorResult(MessageId::CodecHeaderShort);

            header.mode = bytes[4];
            header.originalLength = readLittleEndian(bytes.data() + 5, 8);
            header.expectedCrc =
                static_cast<uint32_t>(readLittleEndian(bytes.data() + 13, 4));
            if (header.mode != MODE_HUFFMAN && header.mode != MODE_STORED)
                return errorResult(MessageId::CodecInvalidMode);

            std::array<uint8_t, 2> countBytes{};
            if (header.mode == MODE_HUFFMAN)
            {
                if (!in.read(reinterpret_cast<char *>(countBytes.data()), 2))
                    return errorResult(MessageId::CodecHeaderShort);
                header.uniqueCount = readLittleEndian(countBytes.data(), 2);
            }
            return successResult();
        }

        if (header.version == 1)
        {
            constexpr uint64_t HUF1_HEADER = 4 + 2 + 8;
            if (fileSize < HUF1_HEADER)
                return errorResult(MessageId::CodecHeaderShort);
            std::array<uint8_t, 2> countBytes{};
            std::array<uint8_t, 8> lengthBytes{};
            if (!in.read(reinterpret_cast<char *>(countBytes.data()), 2) ||
                !in.read(reinterpret_cast<char *>(lengthBytes.data()), 8))
                return errorResult(MessageId::CodecHeaderShort);
            header.uniqueCount = readLittleEndian(countBytes.data(), 2);
            if (header.uniqueCount > 256)
                return errorResult(MessageId::CodecUniqueCountTooLarge);
            header.originalLength = readLittleEndian(lengthBytes.data(), 8);
            return successResult();
        }

        return errorResult(MessageId::CodecUnknownFormat);
    }

} // namespace

bool Decompressor::decompress(const std::string &inputBinPath, const std::string &outputTxtPath)
{
    return decompressWithProgress(inputBinPath, outputTxtPath, {});
}

bool Decompressor::decompressWithProgress(const std::string &inputBinPath,
                                         const std::string &outputTxtPath,
                                         const ProgressCallback &progress)
{
    lastError_ = MessageId::None;
    lastErrorDetails_.clear();
    const auto report = [this](const OperationResult &result)
    {
        if (result.success)
            return true;
        lastError_ = result.error == MessageId::None ? MessageId::CodecInternalFailure : result.error;
        lastErrorDetails_ = result.details;
        if (!lastErrorDetails_.empty() && lastErrorDetails_.compare(0, 2, ": ") != 0)
            lastErrorDetails_.insert(0, ": ");
        return false;
    };
    std::ifstream in(toPath(inputBinPath), std::ios::binary);
    if (!in.is_open())
    {
        lastError_ = MessageId::CodecReadArchive;
        lastErrorDetails_ = ": " + inputBinPath;
        return false;
    }

    std::error_code ec;
    if (areSameExistingFile(toPath(inputBinPath), toPath(outputTxtPath), ec))
    {
        lastError_ = MessageId::CodecSameSourceDestination;
        return false;
    }
    if (ec)
    {
        lastError_ = MessageId::CodecCheckDestination;
        lastErrorDetails_ = ": " + ec.message();
        return false;
    }

    // Chỉ đọc header + bảng tần suất vào RAM (tối đa vài KB).
    // Phần dữ liệu được đọc dần theo khối 64 KB, nên bộ nhớ không phụ thuộc kích thước file.
    in.seekg(0, std::ios::end);
    const std::streamoff fileSizeRaw = in.tellg();
    if (!in || fileSizeRaw < 0)
    {
        lastError_ = MessageId::CodecFileSizeUnknown;
        return false;
    }
    const uint64_t fileSize = static_cast<uint64_t>(fileSizeRaw);
    in.seekg(0, std::ios::beg);
    if (!in)
    {
        lastError_ = MessageId::CodecReadArchive;
        return false;
    }

    InputHeader header;
    const OperationResult headerResult = readInputHeader(in, fileSize, header);
    if (!report(headerResult))
        return false;
    const ProgressCallback activeProgress =
        header.originalLength > 10ULL * 1024ULL * 1024ULL ? progress : ProgressCallback{};

    if (header.version == 2 && header.mode == MODE_STORED)
        return report(decompressStored(outputTxtPath, in, fileSize, header.originalLength,
                                       header.expectedCrc, activeProgress));
    if (header.version == 3 && header.mode == MODE_STORED)
        return report(decompressStored(outputTxtPath, in, fileSize, header.originalLength,
                                       header.expectedCrc, activeProgress));
    if (header.version == 3)
        return report(decompressHuf3(inputBinPath, outputTxtPath, in, fileSize,
                                     header.uniqueCount, header.originalLength,
                                     header.expectedCrc, activeProgress));

    return report(decompressHuffman(
        inputBinPath, outputTxtPath, in, fileSize, header.uniqueCount,
        header.originalLength, header.version == 2, header.expectedCrc, activeProgress));
}
