#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include "compressor.h"
#include "decompressor.h"
#include "fileFormat.h"
#include "pathUtil.h"

static int failures = 0;

#define CHECK(cond, msg) do { if (!(cond)) { std::cerr << "FAIL: " << msg << " (" << __LINE__ << ")\n"; failures++; } } while (0)

static void writeFile(const std::string& path, const std::string& content) {
    std::ofstream f(toPath(path), std::ios::binary);
    f.write(content.data(), static_cast<std::streamsize>(content.size()));
}

static std::string readFile(const std::string& path) {
    std::ifstream f(toPath(path), std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

static void roundTrip(const std::string& name, const std::string& content) {
    const std::string src = "rt_src.tmp", bin = "rt_out.bin", dst = "rt_dst.tmp";
    writeFile(src, content);
    CHECK(Compressor().compress(src, bin), name + ": nen that bai");
    CHECK(Decompressor().decompress(bin, dst), name + ": giai nen that bai");
    CHECK(readFile(dst) == content, name + ": du lieu khong khop tu dau den cuoi");
    std::remove(src.c_str()); std::remove(bin.c_str()); std::remove(dst.c_str());
}

// Đường dẫn có chữ có dấu (UTF-8) phải nén/giải nén được, kể cả trên Windows.
static void roundTripUnicodePath() {
    namespace fs = std::filesystem;
    const std::string dir = "thu_muc_trê", src = dir + "/mẫu_thử.txt",
                      bin = dir + "/kết_quả.bin", dst = dir + "/khôi_phục.txt";
    const std::string content = "Nén dữ liệu: ắằẳẵặ ôơư. " + std::string(200, 'x');
    std::error_code ec;
    fs::create_directories(toPath(dir), ec);
    writeFile(src, content);
    CHECK(Compressor().compress(src, bin), "duong dan Unicode: nen that bai");
    CHECK(fs::exists(toPath(bin)), "duong dan Unicode: khong thay file .bin");
    CHECK(Decompressor().decompress(bin, dst), "duong dan Unicode: giai nen that bai");
    CHECK(readFile(dst) == content, "duong dan Unicode: du lieu khong khop");
    fs::remove_all(toPath(dir), ec);
}

static void expectDecodeFails(const std::string& name, const std::string& bytes) {
    const std::string bin = "rt_bad.bin", dst = "rt_bad_out.tmp";
    writeFile(bin, bytes);
    CHECK(!Decompressor().decompress(bin, dst), name + ": le ra phai bao loi");
    std::remove(bin.c_str()); std::remove(dst.c_str());
}

int main() {
    roundTrip("rong", "");
    roundTrip("mot ky tu", std::string(20, 'a'));
    roundTrip("hai ky tu", "aab");
    roundTrip("tieng Viet", "Xin chào, đây là bài kiểm tra nén file Huffman. Ký tự có dấu: ắằẳẵặ ôơư.");
    {
        std::string all;
        for (int r = 0; r < 4; r++)
            for (int b = 0; b < 256; b++) all.push_back(static_cast<char>(b));
        roundTrip("nhi phan 0..255", all);
    }

    roundTripUnicodePath();

    // Header hỏng phải bị từ chối (HUF2 và HUF1)
    expectDecodeFails("sai magic", std::string("HF01") + std::string(20, '\0'));
    expectDecodeFails("header thieu", std::string("HUF2"));
    expectDecodeFails("HUF1 header thieu", std::string("HUF1"));
    expectDecodeFails("HUF2 mode la", std::string("HUF2") + std::string("\x02", 1) + std::string(20, '\0'));
    expectDecodeFails("HUF2 uniqueCount > 256", std::string("HUF2") + std::string("\x00", 1) +
                                                   std::string(8, '\0') + std::string(4, '\0') +
                                                   std::string("\x01\x01", 2));
    expectDecodeFails("HUF2 rong nhung co length", std::string("HUF2") + std::string("\x00", 1) +
                                                         std::string("\x05", 1) + std::string(7, '\0') +
                                                         std::string(4, '\0') + std::string("\x00\x00", 2));
    expectDecodeFails("HUF1 uniqueCount > 256", std::string("HUF1") + std::string("\x01\x01", 2) + std::string(8, '\0'));
    expectDecodeFails("HUF1 rong nhung co length", std::string("HUF1") + std::string("\x00\x00", 2) + std::string("\x05", 1) + std::string(7, '\0'));

    if (failures == 0) std::cout << "Tat ca test round-trip da PASS\n";
    return failures == 0 ? 0 : 1;
}
