#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "CompressionStats.h"
#include "CompressionVerifier.h"
#include "FileHasher.h"
#include "ICodec.h"
#include "sha256.h"

namespace fs = std::filesystem;

static int gPassCount = 0;
static int gFailCount = 0;

#define CHECK(condition)                                                        \
    do {                                                                        \
        if (condition) {                                                        \
            ++gPassCount;                                                       \
        } else {                                                                \
            ++gFailCount;                                                       \
            std::cout << "  [FAIL] " << #condition << " (dong " << __LINE__     \
                      << ")\n";                                                 \
        }                                                                       \
    } while (0)

static void writeFile(const std::string& path, const std::string& content) {
    std::ofstream output(path, std::ios::binary);
    output.write(content.data(), static_cast<std::streamsize>(content.size()));
}

static std::string readFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

static std::string sha256OfString(const std::string& text) {
    Sha256 hasher;
    hasher.update(reinterpret_cast<const std::uint8_t*>(text.data()), text.size());
    return hasher.finalizeHex();
}

class IdentityCodec : public ICompressor, public IDecompressor {
public:
    bool compress(const std::string& inputPath, const std::string& outputPath) override {
        return copyFile(inputPath, outputPath);
    }
    bool decompress(const std::string& inputPath, const std::string& outputPath) override {
        return copyFile(inputPath, outputPath);
    }

private:
    static bool copyFile(const std::string& from, const std::string& to) {
        std::error_code errorCode;
        return fs::copy_file(from, to, fs::copy_options::overwrite_existing, errorCode) &&
               !errorCode;
    }
};

class CorruptingDecompressor : public IDecompressor {
public:
    bool decompress(const std::string& inputPath, const std::string& outputPath) override {
        std::string data = readFile(inputPath);
        if (!data.empty()) data.back() = static_cast<char>(data.back() ^ 0x01);
        writeFile(outputPath, data);
        return true;
    }
};

class FailingCodec : public ICompressor, public IDecompressor {
public:
    bool compress(const std::string&, const std::string&) override { return false; }
    bool decompress(const std::string&, const std::string&) override { return false; }
};

static void testCompressionRatio() {
    std::cout << "[T10] Ty le nen\n";
    CHECK(std::fabs(*calculateCompressionRatio(1000, 600) - 40.0) < 1e-9);
    CHECK(std::fabs(*calculateCompressionRatio(1000, 1000) - 0.0) < 1e-9);
    CHECK(std::fabs(*calculateCompressionRatio(1000, 1200) - (-20.0)) < 1e-9);
    CHECK(std::fabs(*calculateCompressionRatio(1000, 0) - 100.0) < 1e-9);
    CHECK(!calculateCompressionRatio(0, 0).has_value());
    CHECK(!calculateCompressionRatio(0, 50).has_value());

    CHECK(formatCompressionRatio(calculateCompressionRatio(1000, 600)) == "40.00%");
    CHECK(formatCompressionRatio(calculateCompressionRatio(3, 2)) == "33.33%");
    CHECK(formatCompressionRatio(std::nullopt) == "N/A");
}

static void testFileSize() {
    std::cout << "[T10] Kich thuoc file\n";
    const std::string path = "t10_size.tmp";
    writeFile(path, "12345");
    CHECK(getFileSize(path).value_or(0) == 5);
    writeFile(path, "");
    CHECK(getFileSize(path).has_value() && *getFileSize(path) == 0);
    fs::remove(path);
    CHECK(!getFileSize("khong_ton_tai_t10.tmp").has_value());
}

static void testSha256() {
    std::cout << "[T10] SHA-256\n";
    CHECK(sha256OfString("") ==
          "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    CHECK(sha256OfString("abc") ==
          "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CHECK(sha256OfString("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") ==
          "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    CHECK(sha256OfString(std::string(55, 'a')) ==
          "9f4390f8d30c2dd92ec9f095b65e2b9ae9b0a925a5258e241c9f1e910f734318");
    CHECK(sha256OfString(std::string(56, 'a')) ==
          "b35439a4ac6f0948b6d6f9e3c6af0f5f590ce20f1bde7090ef7970686ec6738a");
    CHECK(sha256OfString(std::string(64, 'a')) ==
          "ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb");
    CHECK(sha256OfString(std::string(1000000, 'a')) ==
          "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
}

static void testFileHasher() {
    std::cout << "[T10] Hash file\n";
    writeFile("t10_a.tmp", "Xin chao Huffman");
    writeFile("t10_b.tmp", "Xin chao Huffman");
    writeFile("t10_c.tmp", "Xin chao Huffmam");

    CHECK(computeFileSha256("t10_a.tmp") == sha256OfString("Xin chao Huffman"));
    CHECK(compareFilesByHash("t10_a.tmp", "t10_b.tmp").match);
    CHECK(!compareFilesByHash("t10_a.tmp", "t10_c.tmp").match);
    CHECK(compareFilesByHash("t10_a.tmp", "t10_c.tmp").readable);
    CHECK(!compareFilesByHash("t10_a.tmp", "khong_ton_tai_t10.tmp").readable);
    CHECK(!computeFileSha256("khong_ton_tai_t10.tmp").has_value());

    std::string bigContent(200000, 'x');
    writeFile("t10_big.tmp", bigContent);
    CHECK(computeFileSha256("t10_big.tmp") == sha256OfString(bigContent));

    fs::remove("t10_a.tmp");
    fs::remove("t10_b.tmp");
    fs::remove("t10_c.tmp");
    fs::remove("t10_big.tmp");
}

static void testVerifierSuccess() {
    std::cout << "[T10] Verifier: nen - giai nen khop\n";
    writeFile("t10_orig.txt", "Nén dữ liệu bằng Huffman\r\nTiếng Việt UTF-8: ăâêôơư\r\n");

    IdentityCodec codec;
    CompressionVerifier verifier(codec, codec);
    std::ostringstream log;
    VerificationResult result =
        verifier.verifyRoundTrip("t10_orig.txt", "t10_orig.bin", "t10_restored.txt", log);

    CHECK(result.isPassed());
    CHECK(result.compressOk && result.decompressOk && result.hashMatch);
    CHECK(result.info.has_value());
    CHECK(result.info->originalSize == result.info->compressedSize);
    CHECK(log.str().find("Ty le nen") != std::string::npos);
    CHECK(log.str().find("KHOP") != std::string::npos);
    CHECK(log.str().find("KHONG KHOP") == std::string::npos);

    fs::remove("t10_orig.txt");
    fs::remove("t10_orig.bin");
    fs::remove("t10_restored.txt");
}

static void testVerifierMismatch() {
    std::cout << "[T10] Verifier: phat hien file giai nen sai\n";
    writeFile("t10_orig.txt", "du lieu goc can duoc bao toan");

    IdentityCodec compressor;
    CorruptingDecompressor decompressor;
    CompressionVerifier verifier(compressor, decompressor);
    std::ostringstream log;
    VerificationResult result =
        verifier.verifyRoundTrip("t10_orig.txt", "t10_orig.bin", "t10_restored.txt", log);

    CHECK(!result.isPassed());
    CHECK(result.compressOk && result.decompressOk);
    CHECK(!result.hashMatch);
    CHECK(result.originalHash != result.restoredHash);
    CHECK(log.str().find("KHONG KHOP") != std::string::npos);

    fs::remove("t10_orig.txt");
    fs::remove("t10_orig.bin");
    fs::remove("t10_restored.txt");
}

static void testVerifierErrors() {
    std::cout << "[T10] Verifier: cac truong hop loi\n";
    IdentityCodec identity;
    FailingCodec failing;
    std::ostringstream log;

    {
        CompressionVerifier verifier(identity, identity);
        VerificationResult result = verifier.verifyRoundTrip(
            "khong_ton_tai_t10.txt", "t10_x.bin", "t10_x.txt", log);
        CHECK(!result.isPassed());
        CHECK(!result.errorMessage.empty());
        CHECK(!result.compressOk);
    }

    writeFile("t10_orig.txt", "abc");

    {
        CompressionVerifier verifier(failing, identity);
        VerificationResult result =
            verifier.verifyRoundTrip("t10_orig.txt", "t10_orig.bin", "t10_restored.txt", log);
        CHECK(!result.compressOk);
        CHECK(!result.isPassed());
    }

    {
        CompressionVerifier verifier(identity, failing);
        VerificationResult result =
            verifier.verifyRoundTrip("t10_orig.txt", "t10_orig.bin", "t10_restored.txt", log);
        CHECK(result.compressOk);
        CHECK(!result.decompressOk);
        CHECK(!result.isPassed());
    }

    fs::remove("t10_orig.txt");
    fs::remove("t10_orig.bin");
    fs::remove("t10_restored.txt");
}

static void testVerifierEmptyFile() {
    std::cout << "[T10] Verifier: file goc rong\n";
    writeFile("t10_empty.txt", "");

    IdentityCodec codec;
    CompressionVerifier verifier(codec, codec);
    std::ostringstream log;
    VerificationResult result =
        verifier.verifyRoundTrip("t10_empty.txt", "t10_empty.bin", "t10_empty_out.txt", log);

    CHECK(result.isPassed());
    CHECK(result.info.has_value() && !result.info->ratioPercent.has_value());
    CHECK(log.str().find("N/A") != std::string::npos);

    fs::remove("t10_empty.txt");
    fs::remove("t10_empty.bin");
    fs::remove("t10_empty_out.txt");
}

static void testPrintReport() {
    std::cout << "[T10] printCompressionReport (dung cho menu T11)\n";
    writeFile("t10_r_orig.txt", std::string(1000, 'a'));
    writeFile("t10_r_comp.bin", std::string(250, 'b'));

    std::ostringstream log;
    CHECK(printCompressionReport("t10_r_orig.txt", "t10_r_comp.bin", log));
    CHECK(log.str().find("75.00%") != std::string::npos);

    writeFile("t10_r_comp.bin", std::string(1500, 'b'));
    std::ostringstream logNegative;
    CHECK(printCompressionReport("t10_r_orig.txt", "t10_r_comp.bin", logNegative));
    CHECK(logNegative.str().find("-50.00%") != std::string::npos);
    CHECK(logNegative.str().find("LON HON") != std::string::npos);

    std::ostringstream logMissing;
    CHECK(!printCompressionReport("t10_r_orig.txt", "khong_ton_tai_t10.bin", logMissing));

    fs::remove("t10_r_orig.txt");
    fs::remove("t10_r_comp.bin");
}

int main() {
    testCompressionRatio();
    testFileSize();
    testSha256();
    testFileHasher();
    testVerifierSuccess();
    testVerifierMismatch();
    testVerifierErrors();
    testVerifierEmptyFile();
    testPrintReport();

    std::cout << "\nKet qua: " << gPassCount << " pass, " << gFailCount << " fail\n";
    return gFailCount == 0 ? 0 : 1;
}
