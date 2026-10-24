#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "CompressionStats.h"
#include "CompressionVerifier.h"
#include "compressor.h"
#include "decompressor.h"

namespace fs = std::filesystem;

struct Sample {
    std::string name;
    std::string path;
    bool expectSmaller;  // file đủ lớn thì file nén phải nhỏ hơn file gốc
};

static void writeFile(const std::string& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary);
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
}

static std::string repeat(const std::string& text, int times) {
    std::string result;
    for (int i = 0; i < times; ++i) result += text;
    return result;
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

    add("ngan", "Xin chao Huffman!", false);          // file ngắn: có thể nén ra lớn hơn
    add("dai", repeat(english, 3000), true);          // file dài (~400 KB)
    add("tieng_viet_utf8", repeat(vietnamese, 200), true);

    for (int i = 1; i < argc; ++i) {                  // file thật do người dùng truyền vào
        samples.push_back({fs::path(argv[i]).filename().string(), argv[i], false});
    }

    Compressor compressor;
    Decompressor decompressor;
    CompressionVerifier verifier(compressor, decompressor);

    std::ostringstream table;
    table << "| File | Goc (byte) | Nen (byte) | Ty le nen | Hash | Ket qua |\n";
    table << "|---|---:|---:|---:|---|---|\n";

    int failures = 0;
    for (const Sample& s : samples) {
        std::ostringstream log;
        VerificationResult r = verifier.verifyRoundTrip(
            s.path, dir + "/" + s.name + ".bin", dir + "/" + s.name + ".out", log);

        bool ok = r.isPassed();
        if (ok && s.expectSmaller && r.info && r.info->compressedSize >= r.info->originalSize) {
            ok = false;
            log << "Loi: file nen khong nho hon file goc\n";
        }
        if (!ok) {
            ++failures;
            std::cout << "--- Chi tiet loi: " << s.name << " ---\n" << log.str();
        }

        std::string original = r.info ? std::to_string(r.info->originalSize) : "?";
        std::string compressed = r.info ? std::to_string(r.info->compressedSize) : "?";
        std::string ratio = r.info ? formatCompressionRatio(r.info->ratioPercent) : "?";
        table << "| " << s.name << " | " << original << " | " << compressed << " | "
              << ratio << " | " << (r.hashMatch ? "KHOP" : "KHONG KHOP") << " | "
              << (ok ? "PASS" : "FAIL") << " |\n";
    }

    std::cout << table.str();
    std::ofstream("ket_qua_nen.md") << table.str();

    fs::remove_all(dir);
    std::cout << (failures == 0 ? "\nTat ca file deu PASS\n" : "\nCo file bi FAIL\n");
    return failures == 0 ? 0 : 1;
}