#include "compressor.h"
#include "pathUtil.h"
#include "bitWriter.h"
#include "crc32.h"
#include "fileFormat.h"
#include "huffmanTree.h"
#include <fstream>
#include <iostream>
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

    // Ghi header HUF2 cố định: magic, mode, originalLength, crc32.
    void writeHeader(std::ostream &out, uint8_t mode, uint64_t originalLength, uint32_t crc)
    {
        out.write(MAGIC_HUF2, 4);
        out.put(static_cast<char>(mode));
        writeLittleEndian(out, originalLength, 8);
        writeLittleEndian(out, crc, 4);
    }

    // Ghi bảng tần suất: (symbol, freq LEB128) cho mọi symbol có mặt, tăng dần theo symbol.
    void writeFrequencyTable(std::ostream &out, const uint64_t freq[256], uint16_t uniqueCount)
    {
        writeLittleEndian(out, uniqueCount, 2);
        for (int s = 0; s < 256; s++)
        {
            if (freq[s] == 0)
                continue;
            out.put(static_cast<char>(s));
            writeLeb128(out, freq[s]);
        }
    }

    // Mode HUFFMAN: header + bảng tần suất + dữ liệu bit. File phải quay về đầu trước khi gọi.
    bool writeHuffman(std::ifstream &in, std::ostream &out, const uint64_t freq[256],
                      uint64_t originalLength, uint32_t crc)
    {
        uint16_t uniqueCount = 0;
        for (int s = 0; s < 256; s++)
            if (freq[s] > 0)
                uniqueCount++;

        writeHeader(out, MODE_HUFFMAN, originalLength, crc);
        writeFrequencyTable(out, freq, uniqueCount);

        // File rỗng: không có cây, không có dữ liệu bit.
        if (originalLength == 0)
            return !out.fail();

        uint64_t totalSymbols = 0;
        auto root = buildHuffmanTree(freq, totalSymbols);
        if (!root)
            return false; // không xảy ra: originalLength > 0 nghĩa là có ít nhất 1 symbol
        HuffmanCodeTable codeTable;
        buildCodeTable(root, codeTable);

        char buffer[IO_CHUNK_SIZE];
        BitWriter writer(out);
        while (in.read(buffer, sizeof(buffer)) || in.gcount() > 0)
        {
            const std::streamsize n = in.gcount();
            for (std::streamsize i = 0; i < n; i++)
            {
                const std::string &code = codeTable[static_cast<unsigned char>(buffer[i])];
                for (char c : code)
                    writer.writeBit(c == '1');
            }
        }
        writer.flush();
        return !in.bad() && !out.fail();
    }

    // Mode STORED: header + dữ liệu gốc nguyên vẹn. File phải quay về đầu trước khi gọi.
    bool writeStored(std::ifstream &in, std::ostream &out, uint64_t originalLength, uint32_t crc)
    {
        writeHeader(out, MODE_STORED, originalLength, crc);
        char buffer[IO_CHUNK_SIZE];
        while (in.read(buffer, sizeof(buffer)) || in.gcount() > 0)
            out.write(buffer, in.gcount());
        return !in.bad() && !out.fail();
    }

} // namespace

bool Compressor::compress(const std::string &inputPath, const std::string &outputBinPath)
{
    namespace fs = std::filesystem;
    std::error_code ec;

    // Kiểm tra trùng file bằng filesystem::equivalent
    if (fs::exists(toPath(inputPath), ec) && fs::exists(toPath(outputBinPath), ec) &&
        fs::equivalent(toPath(inputPath), toPath(outputBinPath), ec))
    {
        std::cerr << "Loi: file nguon va file dich khong duoc trung nhau!\n";
        return false;
    }

    std::ifstream in(toPath(inputPath), std::ios::binary);
    if (!in.is_open())
    {
        std::cerr << "Loi: Khong mo duoc file nguon " << inputPath << "\n";
        return false;
    }

    // Lượt 1: đếm tần suất + originalLength + CRC32.
    uint64_t freq[256] = {};
    uint64_t originalLength = 0;
    uint32_t crc = 0;
    if (!scanSource(in, freq, originalLength, crc))
    {
        std::cerr << "Loi: Doc file nguon that bai\n";
        return false;
    }
    // Về lại đầu file để đọc lượt 2 (clear() cần thiết vì lượt 1 kết thúc ở trạng thái eof).
    in.clear();
    in.seekg(0, std::ios::beg);
    if (!in)
    {
        std::cerr << "Loi: Khong quay lai duoc dau file nguon\n";
        return false;
    }

    // Ghi ra file tạm để không đụng vào file đích cho tới khi thành công.
    std::string tempPath = outputBinPath + ".tmp";
    std::ofstream out(toPath(tempPath), std::ios::binary);
    if (!out.is_open())
    {
        std::cerr << "Loi: Khong tao duoc file " << tempPath << "\n";
        return false;
    }

    // Dùng HUFFMAN nếu không lớn hơn phương án STORED; ngược lại lưu nguyên. Nhờ vậy kích
    // thước file nén luôn <= HEADER_SIZE + kích thước gốc, với mọi đầu vào.
    const bool huffmanOk = writeHuffman(in, out, freq, originalLength, crc);
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
        ok = out.is_open() && in && writeStored(in, out, originalLength, crc);
    }

    out.close();
    in.close();

    if (!ok || out.fail())
    {
        fs::remove(toPath(tempPath), ec);
        return false;
    }

    // Đổi tên file tạm thành file đích
    fs::remove(toPath(outputBinPath), ec);
    fs::rename(toPath(tempPath), toPath(outputBinPath), ec);
    if (ec)
    {
        std::cerr << "Loi: Khong doi ten duoc file tam: " << ec.message() << "\n";
        return false;
    }

    return true;
}
