// Test cho menu console: ConsoleUI (định dạng, đọc input) và MenuController (luồng chọn chức năng).
#undef NDEBUG
#include <cassert>
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
    CHECK(out.str().find("khong duoc de trong") != std::string::npos);
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
    CHECK(text.find("[LOI] hong") != std::string::npos);
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
    CHECK(out.str().find("Tam biet") != std::string::npos);
}

void testMenuHandlesInvalidChoice() {
    std::istringstream in("99\n\n0\n");
    std::ostringstream out;
    ConsoleUI ui(in, out);
    Compressor compressor;
    Decompressor decompressor;
    MenuController menu(ui, compressor, decompressor);

    CHECK(menu.run() == 0);
    CHECK(out.str().find("khong hop le") != std::string::npos);
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
        CHECK(out.str().find("Ty le nen") != std::string::npos);
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
    CHECK(out.str().find("giong file goc 100%") != std::string::npos);
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
    CHECK(out.str().find("Khong tim thay file") != std::string::npos);
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
    CHECK(out.str().find("THONG TIN FILE") != std::string::npos);
    CHECK(out.str().find("Khong phai HUF1") != std::string::npos);

    removeIfExists(src);
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
    testCompressAndVerifyDetectsCleanFile();

    if (failures == 0)
        std::cout << "All menu tests passed\n";
    else
        std::cout << failures << " menu check(s) failed\n";
    return failures == 0 ? 0 : 1;
}
