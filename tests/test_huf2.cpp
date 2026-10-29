// Test HUF3 round-trip/corruption/fuzz và tương thích ngược HUF2/HUF1.
// Mỗi test đếm lỗi thay vì assert để fuzz không dừng ở lần hỏng đầu tiên; main trả 1 nếu có lỗi.
#undef NDEBUG
#include "compressor.h"
#include "canonicalHuffman.h"
#include "compressionStats.h"
#include "crc32.h"
#include "decompressor.h"
#include "fileFormat.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static int gFailures = 0;

#define CHECK(cond, msg)                                                      \
    do                                                                        \
    {                                                                         \
        if (!(cond))                                                          \
        {                                                                     \
            std::cerr << "FAIL: " << (msg) << " (dong " << __LINE__ << ")\n"; \
            ++gFailures;                                                      \
        }                                                                     \
    } while (0)

using Bytes = std::vector<unsigned char>;

static void writeFile(const std::string &path, const std::string &data)
{
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    f.write(data.data(), static_cast<std::streamsize>(data.size()));
}

// Ghi mảng byte (dùng cho file .bin sinh ra nằm trong bộ nhớ).
static void writeBytes(const std::string &path, const Bytes &data)
{
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    f.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
}

static Bytes readBytes(const std::string &path)
{
    std::ifstream f(path, std::ios::binary);
    return Bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

static std::string bytesToString(const Bytes &data)
{
    return std::string(reinterpret_cast<const char *>(data.data()), data.size());
}

// ------------------------------------------------------------------ dữ liệu mẫu

// Mode của file nén (0 = HUFFMAN, 1 = STORED); trả 0xFF nếu file quá ngắn.
static int modeOf(const Bytes& packed) {
    return packed.size() > 4 ? packed[4] : 0xFF;
}

static std::string ratio(std::size_t compressed, std::size_t original) {
    if (original == 0) return "N/A";
    const double value =
        (1.0 - static_cast<double>(compressed) / static_cast<double>(original)) * 100.0;
    return formatCompressionRatio(value);
}

static std::string randomBytes(std::size_t size, std::uint32_t seed)
{
    std::mt19937 rng(seed);
    std::string out(size, '\0');
    for (std::size_t i = 0; i < size; ++i)
        out[i] = static_cast<char>(rng() & 0xFFu);
    return out;
}

struct Case
{
    std::string name;
    std::string content;
};

static std::vector<Case> allCases()
{
    std::vector<Case> cases;
    cases.push_back({"rong", ""});
    cases.push_back({"1 byte", std::string(1, 'Q')});
    cases.push_back({"17 byte", "0123456789abcdefg"});
    cases.push_back({"1 MB byte 0", std::string(1024 * 1024, '\0')});

    std::string all;
    for (int b = 0; b < 256; ++b)
        all.push_back(static_cast<char>(b));
    cases.push_back({"256 gia tri byte", all});

    cases.push_back({"tieng Viet UTF-8",
                     "Nén dữ liệu: ắằẳẵặ ấầẩẫậ ếềểễệ ốồổỗộ ớờởỡợ ứừửữự. "
                     "Xin chào Việt Nam! " +
                         std::string(50, 'a') + "\n"});

    cases.push_back({"5 MB ngau nhien", randomBytes(5u * 1024u * 1024u, 20261029u)});
    return cases;
}

// ------------------------------------------------------------------ 1 + 2

static void testRoundTripAndSizeBound()
{
    std::cout << "[HUF3] Round-trip tung byte + gioi han kich thuoc\n";
    const std::string src = "h2_src.tmp", bin = "h2.bin", dst = "h2_dst.tmp";

    for (const Case &c : allCases())
    {
        writeFile(src, c.content);
        CHECK(Compressor().compress(src, bin), c.name + ": nen that bai");
        const Bytes packed = readBytes(bin);
        CHECK(packed.size() >= HEADER_SIZE, c.name + ": file nen ngan hon header");
        if (packed.size() >= HEADER_SIZE)
        {
            CHECK(packed[0] == 'H' && packed[1] == 'U' && packed[2] == 'F' && packed[3] == '3',
                  c.name + ": magic khong phai HUF3");
            const std::uint8_t mode = packed[4];
            CHECK(mode == MODE_HUFFMAN || mode == MODE_STORED, c.name + ": mode khong hop le");
        }
        // (2) Không bao giờ lớn hơn gốc quá HEADER_SIZE byte.
        CHECK(packed.size() <= c.content.size() + HEADER_SIZE, c.name + ": vuot qua goc + 17");
        // Với file không nén được, phải chọn STORED và đúng kích thước.
        if (modeOf(packed) == MODE_STORED)
            CHECK(packed.size() == HEADER_SIZE + c.content.size(), c.name + ": STORED sai kich thuoc");

        CHECK(Decompressor().decompress(bin, dst), c.name + ": giai nen that bai");
        CHECK(bytesToString(readBytes(dst)) == c.content, c.name + ": du lieu khong khop tung byte");

        std::cout << "  " << c.name << ": goc=" << c.content.size() << " nen=" << packed.size()
                  << " mode=" << (modeOf(packed) == MODE_STORED ? "STORED" : "HUFFMAN")
                  << " ty le=" << ratio(packed.size(), c.content.size()) << "\n";
    }
    std::remove(src.c_str());
    std::remove(bin.c_str());
    std::remove(dst.c_str());
}

// ------------------------------------------------------------------ 3 + 4

// Bit nằm trong phần dữ liệu (sau header + bảng tần suất) của file HUFFMAN.
static void flipDataBit(Bytes &packed)
{
    // Dữ liệu bit nằm ở cuối file: lật bit thấp nhất của byte cuối.
    packed.back() ^= 0x01;
}

static void testCorruptionDetected()
{
    std::cout << "[HUF3] Phat hien hong (CRC32, magic, cat file, byte thua, mode)\n";
    const std::string src = "h2c_src.tmp", bin = "h2c.bin", dst = "h2c_dst.tmp";

    // Dùng dữ liệu đủ lớn để chắc chắn chọn HUFFMAN (có phần dữ liệu bit).
    const std::string text = std::string(20000, 'a') + std::string(5000, 'b') + std::string(500, 'z');

    // (3) Sửa 1 bit trong phần dữ liệu -> lỗi CRC.
    writeFile(src, text);
    CHECK(Compressor().compress(src, bin), "corrupt: nen that bai");
    Bytes packed = readBytes(bin);
    CHECK(modeOf(packed) == MODE_HUFFMAN, "corrupt: du lieu mau phai o mode HUFFMAN");
    if (modeOf(packed) == MODE_HUFFMAN)
    {
        flipDataBit(packed);
        writeBytes(bin, packed);
        CHECK(!Decompressor().decompress(bin, dst), "lat 1 bit du lieu: le ra phai bao loi CRC");
        CHECK(!fs::exists(dst), "lat 1 bit du lieu: file dich do dang phai bi xoa");
    }

    // (3b) Lat 1 bit ở giữa dữ liệu (không chỉ byte cuối).
    packed = readBytes(bin);
    writeFile(src, text);
    CHECK(Compressor().compress(src, bin), "corrupt2: nen that bai");
    packed = readBytes(bin);
    packed[packed.size() / 2] ^= 0x10;
    writeBytes(bin, packed);
    CHECK(!Decompressor().decompress(bin, dst), "lat 1 bit giua du lieu: le ra phai bao loi");

    // (3c) Sửa CRC trong header nhưng dữ liệu còn nguyên -> vẫn phải lỗi.
    packed = readBytes(bin);
    writeFile(src, text);
    CHECK(Compressor().compress(src, bin), "corrupt3: nen that bai");
    packed = readBytes(bin);
    packed[13] ^= 0xFF;
    writeBytes(bin, packed);
    CHECK(!Decompressor().decompress(bin, dst), "sua crc32 trong header: le ra phai bao loi");

    // (4) Sửa magic.
    writeFile(src, text);
    CHECK(Compressor().compress(src, bin), "corrupt4: nen that bai");
    packed = readBytes(bin);
    packed[0] = 'X';
    writeBytes(bin, packed);
    CHECK(!Decompressor().decompress(bin, dst), "sai magic: le ra phai bao loi");

    // (4) Cắt file.
    writeFile(src, text);
    CHECK(Compressor().compress(src, bin), "corrupt5: nen that bai");
    packed = readBytes(bin);
    packed.resize(packed.size() - 100);
    writeBytes(bin, packed);
    CHECK(!Decompressor().decompress(bin, dst), "cat file: le ra phai bao loi");
    packed.resize(HEADER_SIZE / 2);
    writeBytes(bin, packed);
    CHECK(!Decompressor().decompress(bin, dst), "cat duoi header: le ra phai bao loi");

    // (4) Thêm byte thừa.
    writeFile(src, text);
    CHECK(Compressor().compress(src, bin), "corrupt6: nen that bai");
    packed = readBytes(bin);
    packed.push_back(0x00);
    writeBytes(bin, packed);
    CHECK(!Decompressor().decompress(bin, dst), "them 1 byte thua: le ra phai bao loi");

    // (4) Mode không hợp lệ.
    writeFile(src, text);
    CHECK(Compressor().compress(src, bin), "corrupt7: nen that bai");
    packed = readBytes(bin);
    packed[4] = 0x02;
    writeBytes(bin, packed);
    CHECK(!Decompressor().decompress(bin, dst), "mode = 2: le ra phai bao loi");

    // STORED: cắt / thêm byte / lật bit cũng phải bị bắt.
    writeFile(src, "abc");
    CHECK(Compressor().compress(src, bin), "corrupt8: nen that bai");
    packed = readBytes(bin);
    CHECK(modeOf(packed) == MODE_STORED, "abc phai o mode STORED");
    Bytes truncated(packed.begin(), packed.end() - 1);
    writeBytes(bin, truncated);
    CHECK(!Decompressor().decompress(bin, dst), "STORED thieu du lieu: le ra phai bao loi");
    packed.push_back('d');
    writeBytes(bin, packed);
    CHECK(!Decompressor().decompress(bin, dst), "STORED thua du lieu: le ra phai bao loi");
    packed.pop_back();
    packed[HEADER_SIZE] = 'X';
    writeBytes(bin, packed);
    CHECK(!Decompressor().decompress(bin, dst), "STORED lat 1 byte: le ra phai bao loi CRC");

    std::remove(src.c_str());
    std::remove(bin.c_str());
    std::remove(dst.c_str());
}

// (5) File HUF1 cũ, nhúng sẵn dạng mảng byte.
static void testHuf1BackwardCompatibility()
{
    std::cout << "[HUF1] Tuong thich nguoc\n";
    const std::string bin = "huf1_old.bin", dst = "huf1_old.txt";
    // "aab" theo đặc tả HUF1 (xem docs/fileFormat.md mục 9): 33 byte.
    const Bytes huf1Aab = {
        0x48, 0x55, 0x46, 0x31,          // magic "HUF1"
        0x02, 0x00,                      // uniqueCount = 2
        0x03, 0, 0, 0, 0, 0, 0, 0,       // originalLength = 3
        0x61, 0x02, 0, 0, 0, 0, 0, 0, 0, // 'a' freq 2
        0x62, 0x01, 0, 0, 0, 0, 0, 0, 0, // 'b' freq 1
        0xC0};                           // bit "110" + đệm
    writeBytes(bin, huf1Aab);
    CHECK(Decompressor().decompress(bin, dst), "HUF1 'aab': giai nen that bai");
    CHECK(bytesToString(readBytes(dst)) == "aab", "HUF1 'aab': du lieu sai");

    // File rỗng HUF1: chỉ có header 14 byte.
    const Bytes huf1Empty = {'H', 'U', 'F', '1', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    writeBytes(bin, huf1Empty);
    CHECK(Decompressor().decompress(bin, dst), "HUF1 rong: giai nen that bai");
    CHECK(readBytes(dst).empty(), "HUF1 rong: file dau ra phai rong");

    // File HUF1 lớn hơn, dựng thủ công: 3000 ký tự 'a' (cây 1 ký tự -> mã 1 bit "0").
    Bytes big = {'H', 'U', 'F', '1', 0x01, 0x00};  // magic + uniqueCount = 1
    const std::uint64_t length = 3000;
    for (int i = 0; i < 8; ++i)                    // originalLength = 3000 (0x0BB8), LE
        big.push_back(static_cast<unsigned char>((length >> (8 * i)) & 0xFF));
    big.push_back('a');
    const std::uint64_t freq = static_cast<std::uint64_t>(length);
    for (int i = 0; i < 8; ++i)
        big.push_back(static_cast<unsigned char>((freq >> (8 * i)) & 0xFF));
    big.insert(big.end(), static_cast<std::size_t>((length + 7) / 8), 0x00);
    writeBytes(bin, big);
    CHECK(Decompressor().decompress(bin, dst), "HUF1 3000 chu 'a': giai nen that bai");
    CHECK(bytesToString(readBytes(dst)) == std::string(length, 'a'),
          "HUF1 3000 chu 'a': du lieu sai");

    // HUF1 hỏng: bảng tần suất sai thứ tự -> phải lỗi.
    Bytes badOrder = huf1Aab;
    badOrder[14] = 'b';
    badOrder[15] = 2;
    badOrder[23] = 'a';
    badOrder[24] = 1;
    writeBytes(bin, badOrder);
    CHECK(!Decompressor().decompress(bin, dst), "HUF1 bang tan suat sai thu tu: le ra phai bao loi");

    std::remove(bin.c_str());
    std::remove(dst.c_str());
}

static void appendLittleEndian(Bytes& bytes, std::uint64_t value, int count)
{
    for (int i = 0; i < count; ++i)
    {
        bytes.push_back(static_cast<unsigned char>(value & 0xFFu));
        value >>= 8;
    }
}

static void testHuf2BackwardCompatibility()
{
    std::cout << "[HUF2] Tuong thich nguoc HUF2\n";
    const std::string source = "aab";
    Bytes archive = {'H', 'U', 'F', '2', MODE_HUFFMAN};
    appendLittleEndian(archive, source.size(), 8);
    appendLittleEndian(archive, crc32(source.data(), source.size()), 4);
    appendLittleEndian(archive, 2, 2);
    archive.push_back('a');
    archive.push_back(2); // LEB128 frequency
    archive.push_back('b');
    archive.push_back(1);
    archive.push_back(0xC0); // Huffman bits 110, then zero padding

    const std::string bin = "huf2_legacy.bin";
    const std::string dst = "huf2_legacy.txt";
    writeBytes(bin, archive);
    CHECK(Decompressor().decompress(bin, dst), "HUF2 'aab': giai nen that bai");
    CHECK(bytesToString(readBytes(dst)) == source, "HUF2 'aab': du lieu sai");
    std::remove(bin.c_str());
    std::remove(dst.c_str());
}

static void testInvalidHuf3LengthTable()
{
    std::cout << "[HUF3] Tu choi bang do dai ma sai Kraft\n";
    Bytes archive = {'H', 'U', 'F', '3', MODE_HUFFMAN};
    appendLittleEndian(archive, 1, 8);
    appendLittleEndian(archive, 0, 4);
    appendLittleEndian(archive, 3, 2);
    archive.insert(archive.end(), {'a', 1, 'b', 1, 'c', 1});

    const std::string bin = "huf3_bad_kraft.bin";
    const std::string dst = "huf3_bad_kraft.txt";
    writeBytes(bin, archive);
    CHECK(!Decompressor().decompress(bin, dst), "HUF3 bang Kraft oversubscribed duoc chap nhan");
    CHECK(!fs::exists(dst), "HUF3 bang Kraft hong de lai file dich");
    std::remove(bin.c_str());
    std::remove(dst.c_str());
}

static void testSourceDestinationSafety()
{
    std::cout << "[Codec] Bao ve file nguon va file nen\n";
    const std::string output = "safety_output.bin";
    const std::string collidingSource = output + ".tmp";
    const std::string reservedOutput = "safety_reserved.bin";
    const std::string reservedTemp = reservedOutput + ".tmp";
    const std::string reservedSource = "safety_reserved_source.txt";
    const std::string compressed = "safety_compressed.bin";
    const std::string content = std::string(5000, 'x');

    writeFile(collidingSource, content);
    CHECK(!Compressor().compress(collidingSource, output),
          "file nguon trung voi file tam: le ra phai bao loi");
    CHECK(bytesToString(readBytes(collidingSource)) == content,
          "file nguon trung voi file tam: noi dung nguon bi thay doi");
    CHECK(!fs::exists(output), "file nguon trung voi file tam: khong duoc tao file dich");

    writeFile(reservedSource, content);
    writeFile(reservedTemp, "keep existing temp file");
    CHECK(!Compressor().compress(reservedSource, reservedOutput),
          "file tam da ton tai: le ra phai bao loi");
    CHECK(bytesToString(readBytes(reservedTemp)) == "keep existing temp file",
          "file tam da ton tai: noi dung bi ghi de");
    CHECK(!fs::exists(reservedOutput), "file tam da ton tai: khong duoc tao file dich");

    const std::string original = "safety_original.txt";
    writeFile(original, content);
    CHECK(Compressor().compress(original, compressed), "safety: nen that bai");
    const Bytes before = readBytes(compressed);
    CHECK(!Decompressor().decompress(compressed, compressed),
          "file nen va file giai nen trung nhau: le ra phai bao loi");
    CHECK(readBytes(compressed) == before,
          "file nen va file giai nen trung nhau: file nen bi thay doi");

    std::remove(collidingSource.c_str());
    std::remove(output.c_str());
    std::remove(reservedTemp.c_str());
    std::remove(reservedOutput.c_str());
    std::remove(reservedSource.c_str());
    std::remove(compressed.c_str());
    std::remove(original.c_str());
}

// (6) Fuzz: 500 lần đột biến ngẫu nhiên, không crash / không treo / không chấp nhận dữ liệu khác gốc.
static void testFuzz()
{
    std::cout << "[HUF3] Fuzz 500 lan dot bien ngau nhien\n";
    const std::string src = "fz_src.tmp", bin = "fz.bin", dst = "fz_out.tmp";

    struct Seed
    {
        std::string content;
        std::string name;
    };
    const std::vector<Seed> seeds = {
        {"abcabcabcabcabc", "STORED"},
        {std::string(5000, 'a') + std::string(3000, 'b'), "HUFFMAN"},
        {"Huffman: ky tu xuat hien nhieu ma ngan, hiem ma dai. " + std::string(200, 'x'), "HUFFMAN"},
        {"", "rong"},
    };

    // Fuzz sinh rất nhiều thông báo lỗi hợp lệ; tạm chuyển stderr sang bộ đệm để log đọc được.
    std::ostringstream quiet;
    std::streambuf* previousErr = std::cerr.rdbuf(quiet.rdbuf());

    std::mt19937 rng(12345);
    int accepted = 0;
    int rejected = 0;

    for (const Seed &seed : seeds)
    {
        writeFile(src, seed.content);
        CHECK(Compressor().compress(src, bin), "fuzz " + seed.name + ": nen that bai");
        const Bytes original = readBytes(bin);
        CHECK(!original.empty(), "fuzz " + seed.name + ": file nen rong");

        for (int iter = 0; iter < 500 / static_cast<int>(seeds.size()); ++iter)
        {
            Bytes mutated = original;
            const int kind = static_cast<int>(rng() % 4u);
            switch (kind)
            {
            case 0: // sửa 1..3 byte
                for (int k = 0; k < 1 + static_cast<int>(rng() % 3u); ++k)
                    mutated[rng() % mutated.size()] ^= static_cast<unsigned char>(1u << (rng() % 8u));
                break;
            case 1: // cắt
                mutated.resize(rng() % mutated.size());
                break;
            case 2: // thêm byte
                mutated.push_back(static_cast<unsigned char>(rng() & 0xFFu));
                break;
            default: // ghi đè một đoạn
            {
                const std::size_t at = rng() % mutated.size();
                const std::size_t len = 1 + rng() % (mutated.size() - at);
                for (std::size_t i = 0; i < len; ++i)
                    mutated[at + i] = static_cast<unsigned char>(rng() & 0xFFu);
                break;
            }
            }

            writeBytes(bin, mutated);
            std::remove(dst.c_str());
            const bool ok = Decompressor().decompress(bin, dst);
            if (ok)
            {
                ++accepted;
                // KHÔNG BAO GIỜ chấp nhận dữ liệu hỏng: nếu giải nén thành công thì phải
                // khớp từng byte với file gốc.
                const bool same = bytesToString(readBytes(dst)) == seed.content;
                CHECK(same, "fuzz " + seed.name + ": chap nhan du lieu hong (ket qua khac goc)");
                if (!same)
                {
                    // Lưu lại ca lỗi để tái hiện.
                    writeBytes("fz_fail.bin", mutated);
                }
            }
            else
            {
                ++rejected;
                CHECK(!fs::exists(dst) || bytesToString(readBytes(dst)) == seed.content,
                      "fuzz " + seed.name + ": bao loi nhung van de lai file dich do dang");
            }
        }
    }

    std::cerr.rdbuf(previousErr);
    std::cout << "  chap nhan=" << accepted << " tu choi=" << rejected << "\n";

    std::remove(src.c_str());
    std::remove(bin.c_str());
    std::remove(dst.c_str());
}

// ------------------------------------------------------------------ CRC32

static void testCrc32()
{
    std::cout << "[CRC32]\n";
    CHECK(crc32("123456789", 9) == 0xCBF43926u, "CRC32(\"123456789\") sai");
    CHECK(crc32("", 0) == 0x00000000u, "CRC32(\"\") sai");
    CHECK(crc32("aab", 3) == 0x690E2297u, "CRC32(\"aab\") sai");
    CHECK(crc32Table()[1] == 0x77073096u, "bang tra CRC32: phan tu 1 sai");
    // Cập nhật theo khối phải cho cùng kết quả với một lần.
    const std::string text = "chia du lieu thanh nhieu khoi nho";
    std::uint32_t state = crc32Begin();
    for (char c : text)
        state = crc32Update(state, &c, 1);
    CHECK(crc32Finalize(state) == crc32(text.data(), text.size()), "CRC32 theo khoi khac CRC32 mot lan");
}

int main()
{
    testCrc32();
    testRoundTripAndSizeBound();
    testCorruptionDetected();
    testInvalidHuf3LengthTable();
    testHuf2BackwardCompatibility();
    testHuf1BackwardCompatibility();
    testSourceDestinationSafety();
    testFuzz();

    if (gFailures == 0)
    {
        std::cout << "Tat ca test HUF3/HUF2/HUF1 da PASS\n";
        return 0;
    }
    std::cerr << "Co " << gFailures << " loi\n";
    return 1;
}
