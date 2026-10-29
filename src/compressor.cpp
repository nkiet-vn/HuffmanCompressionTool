#include "compressor.h"
#include "pathUtil.h"
#include "bitWriter.h"
#include "canonicalHuffman.h"
#include "crc32.h"
#include "fileFormat.h"
#include "huffmanTree.h"
#include <fstream>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace
{

    // Lượt 1: đếm tần suất, độ dài gốc và CRC32 (đọc theo khối 64 KB).
    bool scanSource(std::ifstream &in, uint64_t freq[256], uint64_t &originalLength, uint32_t &crc)
    {
        char buffer[IO_CHUNK_SIZE];
        uint32_t state = crc32Begin();
        while (in.read(buffer, sizeof(buffer)) || in.gcount() > 0)
        {
            const std::streamsize n = in.gcount();
            for (std::streamsize i = 0; i < n; i++)
                freq[static_cast<unsigned char>(buffer[i])]++;
            state = crc32Update(state, buffer, static_cast<std::size_t>(n));
            originalLength += static_cast<uint64_t>(n);
        }
        crc = crc32Finalize(state);
        return !in.bad();
    }

    // Ghi header HUF3 cố định: magic, mode, originalLength, crc32.
    void writeHeader(std::ostream &out, uint8_t mode, uint64_t originalLength, uint32_t crc)
    {
        out.write(MAGIC_HUF3, 4);
        out.put(static_cast<char>(mode));
        writeLittleEndian(out, originalLength, 8);
        writeLittleEndian(out, crc, 4);
    }

    void writeCodeLengthTable(std::ostream &out, const std::uint8_t lengths[256],
                              std::uint16_t uniqueCount)
    {
        writeLittleEndian(out, uniqueCount, 2);
        for (int s = 0; s < 256; s++)
        {
            if (lengths[s] == 0)
                continue;
            out.put(static_cast<char>(s));
            out.put(static_cast<char>(lengths[s]));
        }
    }

    // Mode HUFFMAN: header + bảng độ dài mã chuẩn tắc + dữ liệu bit.
    bool writeHuffman(std::ifstream &in, std::ostream &out, const uint64_t freq[256],
                      uint64_t originalLength, uint32_t crc, const ProgressCallback &progress)
    {
        uint16_t uniqueCount = 0;
        for (int s = 0; s < 256; s++)
            if (freq[s] > 0)
                uniqueCount++;

        uint8_t lengths[256] = {};
        std::vector<CanonicalCode> codes;
        if (originalLength != 0)
        {
            uint64_t totalSymbols = 0;
            const std::shared_ptr<HuffmanNode> root = buildHuffmanTree(freq, totalSymbols);
            if (!root || !getHuffmanCodeLengths(root, lengths) ||
                !buildCanonicalCodes(lengths, codes))
                return false;
        }

        writeHeader(out, MODE_HUFFMAN, originalLength, crc);
        writeCodeLengthTable(out, lengths, uniqueCount);
        if (originalLength == 0)
            return !out.fail();

        std::uint32_t codeBits[256] = {};
        std::uint8_t codeLengths[256] = {};
        for (const CanonicalCode& code : codes)
        {
            codeBits[code.symbol] = code.bits;
            codeLengths[code.symbol] = code.length;
        }

        char buffer[IO_CHUNK_SIZE];
        BitWriter writer(out);
        uint64_t processed = 0;
        while (in.read(buffer, sizeof(buffer)) || in.gcount() > 0)
        {
            const std::streamsize n = in.gcount();
            for (std::streamsize i = 0; i < n; i++)
            {
                const std::uint8_t symbol = static_cast<unsigned char>(buffer[i]);
                if (codeLengths[symbol] == 0)
                    return false;
                writer.writeBits(codeBits[symbol], codeLengths[symbol]);
            }
            processed += static_cast<uint64_t>(n);
            if (progress)
                progress(processed, originalLength);
        }
        writer.flush();
        return !in.bad() && !out.fail();
    }

    // Mode STORED HUF3: header + dữ liệu gốc nguyên vẹn.
    bool writeStored(std::ifstream &in, std::ostream &out, uint64_t originalLength, uint32_t crc,
                     const ProgressCallback &progress)
    {
        writeHeader(out, MODE_STORED, originalLength, crc);
        char buffer[IO_CHUNK_SIZE];
        uint64_t processed = 0;
        while (in.read(buffer, sizeof(buffer)) || in.gcount() > 0)
        {
            out.write(buffer, in.gcount());
            processed += static_cast<uint64_t>(in.gcount());
            if (progress)
                progress(processed, originalLength);
        }
        return !in.bad() && !out.fail();
    }

} // namespace

bool Compressor::compress(const std::string &inputPath, const std::string &outputBinPath)
{
    return compressWithProgress(inputPath, outputBinPath, {});
}

bool Compressor::compressWithProgress(const std::string &inputPath, const std::string &outputBinPath,
                                      const ProgressCallback &progress)
{
    lastError_ = MessageId::None;
    lastErrorDetails_.clear();
    const auto setError = [this](MessageId error, const std::string &details = std::string())
    {
        lastError_ = error;
        lastErrorDetails_ = details;
        return false;
    };
    namespace fs = std::filesystem;
    std::error_code ec;

    std::ifstream in(toPath(inputPath), std::ios::binary);
    if (!in.is_open())
        return setError(MessageId::CodecOpenSource, ": " + inputPath);

    // Kiểm tra trùng file bằng filesystem::equivalent, kể cả hard link và symlink.
    if (areSameExistingFile(toPath(inputPath), toPath(outputBinPath), ec))
        return setError(MessageId::CodecSameSourceDestination);
    if (ec)
        return setError(MessageId::CodecCheckDestination, ": " + ec.message());

    // Lượt 1: đếm tần suất + originalLength + CRC32.
    uint64_t freq[256] = {};
    uint64_t originalLength = 0;
    uint32_t crc = 0;
    if (!scanSource(in, freq, originalLength, crc))
        return setError(MessageId::CodecReadSource);
    // Về lại đầu file để đọc lượt 2 (clear() cần thiết vì lượt 1 kết thúc ở trạng thái eof).
    in.clear();
    in.seekg(0, std::ios::beg);
    if (!in)
        return setError(MessageId::CodecSeekSource);

    // Ghi ra file tạm để không đụng vào file đích cho tới khi thành công.
    std::string tempPath = outputBinPath + ".tmp";
    if (areSameExistingFile(toPath(inputPath), toPath(tempPath), ec))
        return setError(MessageId::CodecSameTempSource);
    if (ec)
        return setError(MessageId::CodecCheckTemp, ": " + ec.message());
    if (fs::exists(toPath(tempPath), ec))
        return setError(MessageId::CodecTempExists, ": " + tempPath);
    if (ec)
        return setError(MessageId::CodecCheckTemp, ": " + ec.message());

    std::ofstream out(toPath(tempPath), std::ios::binary);
    if (!out.is_open())
        return setError(MessageId::CodecCreateOutput, ": " + tempPath);

    // Dùng HUFFMAN nếu không lớn hơn phương án STORED; ngược lại lưu nguyên. Nhờ vậy kích
    // thước file nén luôn <= HEADER_SIZE + kích thước gốc, với mọi đầu vào.
    const bool huffmanOk = writeHuffman(in, out, freq, originalLength, crc, progress);
    const std::streamoff afterHuffman = out.tellp();
    const uint64_t huffmanSize = afterHuffman > 0 ? static_cast<uint64_t>(afterHuffman) : UINT64_MAX;
    const uint64_t storedSize = static_cast<uint64_t>(HEADER_SIZE) + originalLength;

    bool ok = huffmanOk;
    if (!ok || huffmanSize >= storedSize)
    {
        // Quay lui: mở lại file tạm ở chế độ cắt (truncate) rồi ghi theo mode STORED.
        out.close();
        out.open(toPath(tempPath), std::ios::binary | std::ios::trunc);
        in.clear();
        in.seekg(0, std::ios::beg);
        ok = out.is_open() && in && writeStored(in, out, originalLength, crc, progress);
    }

    out.close();
    in.close();

    if (!ok || out.fail())
    {
        fs::remove(toPath(tempPath), ec);
        return setError(MessageId::CodecInternalFailure);
    }

    // Đổi tên file tạm thành file đích
    fs::remove(toPath(outputBinPath), ec);
    fs::rename(toPath(tempPath), toPath(outputBinPath), ec);
    if (ec)
        return setError(MessageId::CodecRenameTemp, ": " + ec.message());

    return true;
}
