#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "compressionStats.h"
#include "compressionVerifier.h"
#include "compressor.h"
#include "decompressor.h"

namespace fs = std::filesystem;

struct Sample {
    std::string name;
    std::string path;
    bool expectSmaller;  // file đủ lớn thì file nén phải nhỏ hơn file gốc
};

// Đọc byte thứ 5 của file nén (mode HUF3): 0 = HUFFMAN, 1 = STORED.
static std::string modeLabel(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    char header[5] = {};
    if (!in.read(header, 5) || header[0] != 'H' || header[1] != 'U' ||
        header[2] != 'F' || header[3] != '3')
        return "?";
    return header[4] == 0 ? "HUFFMAN" : (header[4] == 1 ? "STORED" : "?");
}

static void writeFile(const std::string& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary);
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
}

static std::string repeat(const std::string& text, int times) {
    std::string result;
    for (int i = 0; i < times; ++i) result += text;
    return result;
}

// Du lieu ngau nhien (khong nen duoc) de kiem tra mode STORED tren file lon.
static std::string randomBytes(std::size_t size, std::uint32_t seed) {
    std::mt19937 rng(seed);
    std::string out(size, '\0');
    for (std::size_t i = 0; i < size; ++i) out[i] = static_cast<char>(rng() & 0xFFu);
    return out;
}

static std::size_t leb128Size(std::uint64_t value) {
    std::size_t size = 1;
    while (value >= 0x80u) {
        value >>= 7;
        ++size;
    }
    return size;
}

static std::pair<std::size_t, std::size_t> huffmanHeaderSizes(const std::string& path) {
    std::uint64_t frequencies[256] = {};
    std::ifstream input(path, std::ios::binary);
    char byte = 0;
    while (input.get(byte))
        ++frequencies[static_cast<unsigned char>(byte)];

    std::size_t uniqueCount = 0;
    std::size_t huf2Size = 17 + 2;
    for (std::uint64_t frequency : frequencies) {
        if (frequency == 0)
            continue;
        ++uniqueCount;
        huf2Size += 1 + leb128Size(frequency);
    }
    const std::size_t huf3Size = 17 + 2 + uniqueCount * 2;
    return {huf2Size, huf3Size};
}

int main(int argc, char** argv) {
    // Lưu ý: file nguồn này phải lưu ở dạng UTF-8
    const std::string vietnamese =
        "Nén dữ liệu bằng thuật toán Huffman là phương pháp không mất mát. "
        "Ký tự xuất hiện nhiều được gán mã ngắn, ký tự hiếm gặp được gán mã dài. "
        "Tiếng Việt có nhiều dấu: ắ ằ ẳ ẵ ặ ấ ầ ẩ ẫ ậ ế ề ể ễ ệ ố ồ ổ ỗ ộ ớ ờ ở ỡ ợ ứ ừ ử ữ ự.\r\n";
    const std::string english =
        "The quick brown fox jumps over the lazy dog. "
        "Huffman coding assigns shorter codes to more frequent symbols.\n";

    const std::string dir = "verify_tmp";
    fs::create_directories(dir);

    std::vector<Sample> samples;
    auto add = [&](const std::string& name, const std::string& content, bool expectSmaller) {
        std::string path = dir + "/" + name + ".txt";
        writeFile(path, content);
        samples.push_back({name, path, expectSmaller});
    };

    // File ngắn giờ KHÔNG còn bị phình: chọn STORED, chỉ tốn thêm HEADER_SIZE byte.
    add("ngan", "Xin chao Huffman!", false);
    add("dai", repeat(english, 3000), true);          // file dài (~400 KB)
    add("tieng_viet_utf8", repeat(vietnamese, 200), true);
    // Dữ liệu ngẫu nhiên: không nén được -> STORED, kích thước = gốc + 17.
    add("ngau_nhien", randomBytes(5000000u, 20261029u), false);

    for (int i = 1; i < argc; ++i) {                  // file thật do người dùng truyền vào
        samples.push_back({fs::path(argv[i]).filename().string(), argv[i], false});
    }

    Compressor compressor;
    Decompressor decompressor;
    CompressionVerifier verifier(compressor, decompressor);

    std::ostringstream table;
    table << "| File | Goc (byte) | Nen (byte) | Ty le nen | Mode | Hash | Ket qua |\n";
    table << "|---|---:|---:|---:|---|---|---|\n";
    std::ostringstream headerTable;
    headerTable << "\n| File mẫu | Header HUF2-HUFFMAN (byte) | Header HUF3-HUFFMAN (byte) | Giảm (byte) |\n";
    headerTable << "|---|---:|---:|---:|\n";
    for (const std::string& name : {"dai", "tieng_viet_utf8", "ngau_nhien"}) {
        const auto sizes = huffmanHeaderSizes(dir + "/" + name + ".txt");
        headerTable << "| " << name << " | " << sizes.first << " | " << sizes.second
                    << " | " << sizes.first - sizes.second << " |\n";
    }

    int failures = 0;
    if (formatCompressionRatio(calculateCompressionRatio(1000000, 999999)) != "0.00%" ||
        formatCompressionRatio(calculateCompressionRatio(5000000, 5000017)) != "-0.0003%") {
        std::cerr << "Loi: formatCompressionRatio lam tron sai gia tri nho\n";
        ++failures;
    }

    for (const Sample& s : samples) {
        std::ostringstream log;
        VerificationResult r = verifier.verifyRoundTrip(
            s.path, dir + "/" + s.name + ".bin", dir + "/" + s.name + ".out", log);

        bool ok = r.isPassed();
        if (ok && s.expectSmaller && r.info && r.info->compressedSize >= r.info->originalSize) {
            ok = false;
            log << "Loi: file nen khong nho hon file goc\n";
        }
        // Với định dạng HUF3, file nén không bao giờ lớn hơn gốc quá HEADER_SIZE (17) byte.
        if (ok && r.info && r.info->compressedSize > r.info->originalSize + 17) {
            ok = false;
            log << "Loi: file nen vuot qua goc + 17 byte\n";
        }
        if (!ok) {
            ++failures;
            std::cout << "--- Chi tiet loi: " << s.name << " ---\n" << log.str();
        }

        std::string original = r.info ? std::to_string(r.info->originalSize) : "?";
        std::string compressed = r.info ? std::to_string(r.info->compressedSize) : "?";
        std::string ratio = r.info ? formatCompressionRatio(r.info->ratioPercent) : "?";
        const std::string bin = dir + "/" + s.name + ".bin";
        table << "| " << s.name << " | " << original << " | " << compressed << " | "
              << ratio << " | " << modeLabel(bin) << " | " << (r.hashMatch ? "KHOP" : "KHONG KHOP")
              << " | " << (ok ? "PASS" : "FAIL") << " |\n";
    }

    std::cout << table.str();
    std::cout << headerTable.str();
    std::ofstream("ket_qua_nen.md") << table.str() << headerTable.str();

    fs::remove_all(dir);
    std::cout << (failures == 0 ? "\nTat ca file deu PASS\n" : "\nCo file bi FAIL\n");
    return failures == 0 ? 0 : 1;
}