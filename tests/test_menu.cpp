// Test cho menu console: ConsoleUI (định dạng, đọc input) và MenuController (luồng chọn chức năng).
#undef NDEBUG
#include <cassert>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "compressor.h"
#include "consoleUI.h"
#include "decompressor.h"
#include "menuController.h"
#include "fileFormat.h"
#include "crc32.h"

namespace fs = std::filesystem;

namespace {

int failures = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            ++failures;                                                          \
            std::cerr << "FAIL line " << __LINE__ << ": " #cond "\n";            \
        }                                                                        \
    } while (0)

void writeFile(const std::string& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary);
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
}

void removeIfExists(const std::string& path) {
    std::error_code ec;
    fs::remove(path, ec);
}

std::size_t utf8Width(const std::string& text) {
    std::size_t width = 0;
    for (unsigned char byte : text) {
        if ((byte & 0xC0) != 0x80) ++width;
    }
    return width;
}

void writeLittleEndian(std::ostream& out, std::uint64_t value, int bytes) {
    for (int i = 0; i < bytes; ++i) {
        out.put(static_cast<char>(value & 0xFF));
        value >>= 8;
    }
}

void writeHuf1Sample(const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    out.write("HUF1", 4);
    writeLittleEndian(out, 1, 2);
    writeLittleEndian(out, 2, 8);
    out.put('A');
    writeLittleEndian(out, 2, 8);
    out.put('\0');
}

void writeHuf2HuffmanSample(const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    out.write("HUF2", 4);
    out.put(static_cast<char>(MODE_HUFFMAN));
    writeLittleEndian(out, 3, 8);
    writeLittleEndian(out, crc32("aab", 3), 4);
    writeLittleEndian(out, 2, 2);
    out.put('a');
    out.put(2);
    out.put('b');
    out.put(1);
    out.put(static_cast<char>(0xC0));
}

void writeHuf2StoredSample(const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    out.write("HUF2", 4);
    out.put(static_cast<char>(MODE_STORED));
    writeLittleEndian(out, 3, 8);
    writeLittleEndian(out, crc32("abc", 3), 4);
    out.write("abc", 3);
}

// ---------------------------------------------------------------- ConsoleUI

void testTrimAndEmptyInput() {
    std::istringstream in("   xin chao  \r\n");
    std::ostringstream out;
    ConsoleUI ui(in, out);

    CHECK(ui.readLine("> ") == "xin chao");
    // Hết dữ liệu vào -> trả chuỗi rỗng.
    CHECK(ui.readLine("> ").empty());
}

void testReadChoiceHandlesEof() {
    std::istringstream in("  2 \n");
    std::ostringstream out;
    ConsoleUI ui(in, out);

    auto first = ui.readChoice("chon: ");
    CHECK(first.has_value() && *first == "2");

    auto second = ui.readChoice("chon: ");
    CHECK(!second.has_value());
}

void testConfirmDefaults() {
    std::istringstream in("\nY\nyes\nn\n");
    std::ostringstream out;
    ConsoleUI ui(in, out);

    CHECK(ui.confirm("q1", true) == true);    // Enter -> mặc định true
    CHECK(ui.confirm("q2", false) == true);   // "Y"
    CHECK(ui.confirm("q3", false) == true);   // "yes"
    CHECK(ui.confirm("q4", true) == false);   // "n"
}

void testReadNonEmptyLineStripsQuotes() {
    std::istringstream in("\"C:\\a b\\file.txt\"\n  'x.bin'  \n\"\"\nok\n\"lech\n");
    std::ostringstream out;
    ConsoleUI ui(in, out);

    CHECK(ui.readNonEmptyLine("> ") == "C:\\a b\\file.txt");  // ngoặc kép kép
    CHECK(ui.readNonEmptyLine("> ") == "x.bin");               // ngoặc đơn + khoảng trắng
    CHECK(ui.readNonEmptyLine("> ") == "ok");                  // "" rỗng -> hỏi lại
    CHECK(ui.readNonEmptyLine("> ") == "\"lech");              // ngoặc không đủ cặp: giữ nguyên
}

void testReadNonEmptyLineRetries() {
    std::istringstream in("\n   \nfinal\n");
    std::ostringstream out;
    ConsoleUI ui(in, out);

    CHECK(ui.readNonEmptyLine("nhap: ") == "final");
    CHECK(out.str().find("không được để trống") != std::string::npos);
}

void testFormatBytes() {
    CHECK(ConsoleUI::formatBytes(0) == "0 B");
    CHECK(ConsoleUI::formatBytes(512) == "512 B");
    CHECK(ConsoleUI::formatBytes(2048) == "2.0 KB");
    CHECK(ConsoleUI::formatBytes(5ULL * 1024 * 1024) == "5.0 MB");
}

void testMenuRendersAllItems() {
    std::istringstream in("");
    std::ostringstream out;
    ConsoleUI ui(in, out);

    ui.showHeader({"Ten Cong Cu", "phu de", "9.9"});
    ui.showMenu({{"1", "Nen file", "mo ta 1"}, {"0", "Thoat", ""}});

    const std::string text = out.str();
    CHECK(text.find("Ten Cong Cu") != std::string::npos);
    CHECK(text.find("v9.9") != std::string::npos);
    CHECK(text.find("[1]") != std::string::npos);
    CHECK(text.find("Nen file") != std::string::npos);
    CHECK(text.find("mo ta 1") != std::string::npos);
    CHECK(text.find("[0]") != std::string::npos);
}

void testTableAlignment() {
    std::istringstream in("");
    std::ostringstream out;
    ConsoleUI ui(in, out);

    ui.showTable({"Lenh", "Mo ta"}, {{"-c", "Nen"}, {"-d", "Giai nen"}});
    const std::string text = out.str();
    CHECK(text.find("| Lenh |    Mo ta |") != std::string::npos);
    CHECK(text.find("|   -c |      Nen |") != std::string::npos);
}

void testMessageTags() {
    std::istringstream in("");
    std::ostringstream out;
    ConsoleUI ui(in, out);

    ui.showSuccess("xong");
    ui.showError("hong");
    ui.showWarning("canh bao");
    ui.showInfo("ghi chu");

    const std::string text = out.str();
    CHECK(text.find("[OK] xong") != std::string::npos);
    CHECK(text.find("[LỖI] hong") != std::string::npos);
    CHECK(text.find("[!] canh bao") != std::string::npos);
    CHECK(text.find("[i] ghi chu") != std::string::npos);
}

// ------------------------------------------------------------ MenuController

void testMenuExitImmediately() {
    std::istringstream in("0\n");
    std::ostringstream out;
    ConsoleUI ui(in, out);
    Compressor compressor;
    Decompressor decompressor;
    MenuController menu(ui, compressor, decompressor);

    CHECK(menu.run() == 0);
    CHECK(out.str().find("Tạm biệt") != std::string::npos);
}

void testMenuHandlesInvalidChoice() {
    std::istringstream in("99\n\n0\n");
    std::ostringstream out;
    ConsoleUI ui(in, out);
    Compressor compressor;
    Decompressor decompressor;
    MenuController menu(ui, compressor, decompressor);

    CHECK(menu.run() == 0);
    CHECK(out.str().find("không hợp lệ") != std::string::npos);
}

void testMenuCompressAndDecompressFlow() {
    const std::string src = "menu_src.txt";
    const std::string bin = "menu_src.bin";
    const std::string dst = "menu_out.txt";
    removeIfExists(src);
    removeIfExists(bin);
    removeIfExists(dst);
    writeFile(src, std::string(400, 'a') + "menu test");

    {
        // Chọn 1 (nen): file nguon, file dich, roi 0 de thoat.
        std::istringstream in("1\n" + src + "\n" + bin + "\n\n0\n");
        std::ostringstream out;
        ConsoleUI ui(in, out);
        Compressor compressor;
        Decompressor decompressor;
        MenuController menu(ui, compressor, decompressor);

        CHECK(menu.run() == 0);
        CHECK(fs::exists(bin));
        CHECK(out.str().find("Tỷ lệ nén") != std::string::npos);
    }

    {
        // Chọn 2 (giai nen).
        std::istringstream in("2\n" + bin + "\n" + dst + "\n\n0\n");
        std::ostringstream out;
        ConsoleUI ui(in, out);
        Compressor compressor;
        Decompressor decompressor;
        MenuController menu(ui, compressor, decompressor);

        CHECK(menu.run() == 0);
        CHECK(fs::exists(dst));
    }

    std::ifstream f(dst, std::ios::binary);
    std::string restored((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    CHECK(restored == std::string(400, 'a') + "menu test");

    removeIfExists(src);
    removeIfExists(bin);
    removeIfExists(dst);
}

void testMenuVerifyFlow() {
    const std::string src = "menu_verify.txt";
    const std::string bin = "menu_verify.bin";
    removeIfExists(src);
    removeIfExists(bin);
    writeFile(src, "noi dung kiem tra xac minh sha256");

    std::istringstream in("3\n" + src + "\n" + bin + "\n\n0\n");
    std::ostringstream out;
    ConsoleUI ui(in, out);
    Compressor compressor;
    Decompressor decompressor;
    MenuController menu(ui, compressor, decompressor);

    CHECK(menu.run() == 0);
    CHECK(out.str().find("SHA-256") != std::string::npos);
    CHECK(out.str().find("giống tệp gốc 100%") != std::string::npos);
    CHECK(!fs::exists(bin + ".verify_tmp"));

    removeIfExists(src);
    removeIfExists(bin);
}

void testMenuRejectsMissingFile() {
    std::istringstream in("1\nkhong_ton_tai_12345.txt\n\n0\n");
    std::ostringstream out;
    ConsoleUI ui(in, out);
    Compressor compressor;
    Decompressor decompressor;
    MenuController menu(ui, compressor, decompressor);

    CHECK(menu.run() == 0);
    CHECK(out.str().find("Không tìm thấy tệp") != std::string::npos);
}

void testMenuFileInfoFlow() {
    const std::string src = "menu_info.txt";
    removeIfExists(src);
    writeFile(src, "abc");

    std::istringstream in("4\n" + src + "\n\n0\n");
    std::ostringstream out;
    ConsoleUI ui(in, out);
    Compressor compressor;
    Decompressor decompressor;
    MenuController menu(ui, compressor, decompressor);

    CHECK(menu.run() == 0);
    CHECK(out.str().find("THÔNG TIN TỆP") != std::string::npos);
    CHECK(out.str().find("Tệp thường") != std::string::npos);

    removeIfExists(src);
}

void testFileInfoFormatsAndMissingFile() {
    const std::string huffmanInput = "menu_info_huffman_src.txt";
    const std::string huffmanArchive = "menu_info_huffman.huf";
    const std::string storedInput = "menu_info_stored_src.txt";
    const std::string storedArchive = "menu_info_stored.huf";
    const std::string huf2Huffman = "menu_info_huf2_huffman.huf";
    const std::string huf2Stored = "menu_info_huf2_stored.huf";
    const std::string huf1 = "menu_info_legacy.huf";
    const std::string plain = "menu_info_plain.txt";
    const std::string missing = "menu_info_missing_12345";
    for (const std::string& path : {huffmanInput, huffmanArchive, storedInput,
                                    storedArchive, huf2Huffman, huf2Stored, huf1, plain, missing})
        removeIfExists(path);
    writeFile(huffmanInput, std::string(2000, 'A'));
    writeFile(storedInput, "abc");
    writeHuf2HuffmanSample(huf2Huffman);
    writeHuf2StoredSample(huf2Stored);
    writeHuf1Sample(huf1);
    writeFile(plain, "plain file");

    Compressor compressor;
    CHECK(compressor.compress(huffmanInput, huffmanArchive));
    CHECK(compressor.compress(storedInput, storedArchive));

    auto infoFor = [](const std::string& path, bool& result) {
        std::istringstream in;
        std::ostringstream out;
        ConsoleUI ui(in, out);
        Compressor c;
        Decompressor d;
        MenuController menu(ui, c, d);
        result = menu.showFileInfo(path);
        return out.str();
    };
    bool result = false;
    const std::string huffmanInfo = infoFor(huffmanArchive, result);
    CHECK(result);
    CHECK(huffmanInfo.find("HUF3-HUFFMAN") != std::string::npos);
    CHECK(huffmanInfo.find("Kích thước gốc") != std::string::npos);
    CHECK(huffmanInfo.find("Số ký hiệu khác nhau") != std::string::npos);
    CHECK(huffmanInfo.find("SHA-256") != std::string::npos);

    const std::string storedInfo = infoFor(storedArchive, result);
    CHECK(result);
    CHECK(storedInfo.find("HUF3-STORED") != std::string::npos);
    CHECK(storedInfo.find("Tỷ lệ nén") != std::string::npos);

    const std::string huf2HuffmanInfo = infoFor(huf2Huffman, result);
    CHECK(result);
    CHECK(huf2HuffmanInfo.find("HUF2-HUFFMAN") != std::string::npos);

    const std::string huf2StoredInfo = infoFor(huf2Stored, result);
    CHECK(result);
    CHECK(huf2StoredInfo.find("HUF2-STORED") != std::string::npos);

    const std::string huf1Info = infoFor(huf1, result);
    CHECK(result);
    CHECK(huf1Info.find("HUF1") != std::string::npos);
    CHECK(huf1Info.find("Số ký hiệu khác nhau") != std::string::npos);

    const std::string plainInfo = infoFor(plain, result);
    CHECK(result);
    CHECK(plainInfo.find("Tệp thường") != std::string::npos);
    CHECK(plainInfo.find("Kích thước gốc") != std::string::npos);
    CHECK(plainInfo.find("Không áp dụng") != std::string::npos);

    const std::string missingInfo = infoFor(missing, result);
    CHECK(!result);
    CHECK(missingInfo.find("Không đọc được tệp") != std::string::npos);

    for (const std::string& path : {huffmanInput, huffmanArchive, storedInput,
                                    storedArchive, huf2Huffman, huf2Stored, huf1, plain})
        removeIfExists(path);
}

void testUtf8AsciiAndTableAlignment() {
    std::istringstream utf8Input;
    std::ostringstream utf8Output;
    ConsoleUI utf8Ui(utf8Input, utf8Output);
    utf8Ui.showMenu({{"1", utf8Ui.text(MessageId::MenuCompress),
                      utf8Ui.text(MessageId::MenuCompressDescription)}});
    CHECK(utf8Output.str().find("Nén tệp") != std::string::npos);

    std::istringstream asciiInput;
    std::ostringstream asciiOutput;
    ConsoleUI asciiUi(asciiInput, asciiOutput, true);
    asciiUi.showMenu({{"1", asciiUi.text(MessageId::MenuCompress),
                       asciiUi.text(MessageId::MenuCompressDescription)}});
    CHECK(asciiOutput.str().find("Nen tep") != std::string::npos);
    CHECK(asciiOutput.str().find("Nén") == std::string::npos);

    std::ostringstream tableOutput;
    ConsoleUI tableUi(utf8Input, tableOutput);
    tableUi.showTable({"Tên", "Mô tả"}, {{"Nén", "tốt"}, {"Giải nén", "khôi phục"}});
    std::istringstream lines(tableOutput.str());
    std::string line;
    std::size_t expectedWidth = 0;
    while (std::getline(lines, line)) {
        if (expectedWidth == 0) expectedWidth = utf8Width(line);
        CHECK(utf8Width(line) == expectedWidth);
    }
}

void testProgressDoesNotWriteToInjectedOutput() {
    std::istringstream input;
    std::ostringstream output;
    ConsoleUI ui(input, output);
    ui.showProgress(50);
    CHECK(output.str().empty());
}

void testCodecErrorsDoNotWriteToStderr() {
    const std::string missing = "codec_missing_12345";
    const std::string output = "codec_missing_output";
    removeIfExists(output);
    Compressor compressor;
    Decompressor decompressor;
    std::ostringstream captured;
    std::streambuf* previous = std::cerr.rdbuf(captured.rdbuf());
    CHECK(!compressor.compress(missing, output));
    CHECK(!decompressor.decompress(missing, output));
    std::cerr.rdbuf(previous);
    CHECK(captured.str().empty());
    CHECK(compressor.lastError() != MessageId::None);
    CHECK(decompressor.lastError() != MessageId::None);
}

void testCompressAndVerifyDetectsCleanFile() {
    const std::string src = "menu_roundtrip.txt";
    const std::string bin = "menu_roundtrip.bin";
    removeIfExists(src);
    removeIfExists(bin);
    writeFile(src, "abcabcabcabc");

    std::istringstream in("");
    std::ostringstream out;
    ConsoleUI ui(in, out);
    Compressor compressor;
    Decompressor decompressor;
    MenuController menu(ui, compressor, decompressor);

    CHECK(menu.compressAndVerify(src, bin));
    CHECK(!fs::exists(bin + ".verify_tmp"));

    removeIfExists(src);
    removeIfExists(bin);
}

} // namespace

int main() {
    testTrimAndEmptyInput();
    testReadChoiceHandlesEof();
    testConfirmDefaults();
    testReadNonEmptyLineRetries();
    testReadNonEmptyLineStripsQuotes();
    testFormatBytes();
    testMenuRendersAllItems();
    testTableAlignment();
    testMessageTags();
    testMenuExitImmediately();
    testMenuHandlesInvalidChoice();
    testMenuCompressAndDecompressFlow();
    testMenuVerifyFlow();
    testMenuRejectsMissingFile();
    testMenuFileInfoFlow();
    testFileInfoFormatsAndMissingFile();
    testUtf8AsciiAndTableAlignment();
    testProgressDoesNotWriteToInjectedOutput();
    testCodecErrorsDoNotWriteToStderr();
    testCompressAndVerifyDetectsCleanFile();

    if (failures == 0)
        std::cout << "All menu tests passed\n";
    else
        std::cout << failures << " menu check(s) failed\n";
    return failures == 0 ? 0 : 1;
}
