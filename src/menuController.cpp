#include "menuController.h"
#include "pathUtil.h"

#include <filesystem>
#include <fstream>
#include <system_error>

#include "compressionStats.h"
#include "fileHasher.h"
#include "fileFormat.h"
#include "compressor.h"
#include "decompressor.h"

namespace fs = std::filesystem;

namespace
{

    constexpr const char *kToolName = "Huffman Compression Tool";
    constexpr const char *kVersion = "1.1.0";

    bool isRegularFile(const std::string &path)
    {
        std::error_code ec;
        return fs::is_regular_file(toPath(path), ec) && !ec;
    }

} // namespace

MenuController::MenuController(ConsoleUI &ui, Compressor &compressor, Decompressor &decompressor)
    : ui_(ui), compressor_(compressor), decompressor_(decompressor) {}

std::vector<MenuItem> MenuController::buildMenuItems() const
{
    return {
        {"1", "Nen file", "nen file bat ky thanh dinh dang HUF2"},
        {"2", "Giai nen file", "khoi phuc file tu .bin"},
        {"3", "Nen va xac minh SHA-256", "nen + giai nen lai + so sanh hash"},
        {"4", "Xem thong tin file", "kich thuoc, dinh dang HUF2"},
        {"5", "Huong dan / tro giup", "cach dung dong lenh"},
        {"6", "Thong tin phan mem", "phien ban, cau truc du lieu chinh"},
        {"0", "Thoat", ""},
    };
}

int MenuController::run()
{
    ui_.showHeader({kToolName, "Cong cu nen va giai nen file bang thuat toan Huffman", kVersion});

    while (running_)
    {
        ui_.showMenu(buildMenuItems());

        const std::optional<std::string> choice = ui_.readChoice("Chon chuc nang [0-6]: ");
        if (!choice)
        { // EOF: kết thúc phiên làm việc
            ui_.showInfo("Ket thuc phien lam viec.");
            break;
        }

        if (choice->empty())
        {
            ui_.showWarning("Ban chua chon chuc nang nao.");
            continue;
        }
        if (*choice == "0" || *choice == "q" || *choice == "thoat")
        {
            running_ = false;
            ui_.showSuccess("Tam biet!");
            break;
        }
        if (*choice == "1")
        {
            handleCompress();
        }
        else if (*choice == "2")
        {
            handleDecompress();
        }
        else if (*choice == "3")
        {
            handleVerify();
        }
        else if (*choice == "4")
        {
            handleFileInfo();
        }
        else if (*choice == "5")
        {
            handleHelp();
        }
        else if (*choice == "6")
        {
            handleAbout();
        }
        else
        {
            ui_.showError("Lua chon '" + *choice + "' khong hop le. Chon tu 0 den 6.");
            continue;
        }

        if (running_)
            ui_.waitForEnter();
    }
    return 0;
}

// ---------------------------------------------------------------- nghiệp vụ

bool MenuController::compressFile(const std::string &inputPath, const std::string &outputPath)
{
    ui_.showInfo("Dang nen: " + inputPath + "  ->  " + outputPath);
    if (!compressor_.compress(inputPath, outputPath))
    {
        ui_.showError("Nen that bai. Kiem tra lai duong dan file nguon.");
        return false;
    }

    const std::optional<CompressionInfo> info = getCompressionInfo(inputPath, outputPath);
    if (info)
    {
        ui_.showSection("THONG KE NEN");
        ui_.showKeyValue("Kich thuoc goc", ConsoleUI::formatBytes(info->originalSize) +
                                               " (" + std::to_string(info->originalSize) + " byte)");
        ui_.showKeyValue("Kich thuoc file nen", ConsoleUI::formatBytes(info->compressedSize) +
                                                    " (" + std::to_string(info->compressedSize) + " byte)");
        ui_.showKeyValue("Ty le nen", formatCompressionRatio(info->ratioPercent));

        if (info->ratioPercent && *info->ratioPercent < 0.0)
        {
            ui_.showWarning("File nen LON HON file goc: header chiem nhieu hon phan du lieu tiet kiem.");
        }
        else if (info->ratioPercent)
        {
            ui_.showSuccess("Da tiet kiem " + formatCompressionRatio(info->ratioPercent) + " dung luong.");
        }
    }
    return true;
}

bool MenuController::decompressFile(const std::string &inputPath, const std::string &outputPath)
{
    ui_.showInfo("Dang giai nen: " + inputPath + "  ->  " + outputPath);
    if (!decompressor_.decompress(inputPath, outputPath))
    {
        ui_.showError("Giai nen that bai. File .bin co the bi hong hoac khong dung dinh dang HUF2/HUF1.");
        return false;
    }

    const std::optional<std::uintmax_t> size = getFileSize(outputPath);
    if (size)
        ui_.showKeyValue("Kich thuoc file dau ra", ConsoleUI::formatBytes(*size));
    return true;
}

bool MenuController::compressAndVerify(const std::string &inputPath,
                                       const std::string &compressedPath)
{
    if (!compressFile(inputPath, compressedPath))
        return false;

    ui_.showSection("XAC MINH KHOI PHUC (SHA-256)");

    const std::optional<std::string> originalHash = computeFileSha256(inputPath);
    if (!originalHash)
    {
        ui_.showError("Khong tinh duoc SHA-256 cua file goc.");
        return false;
    }

    const std::string tempPath = compressedPath + ".verify_tmp";
    if (!decompressor_.decompress(compressedPath, tempPath))
    {
        ui_.showError("Giai nen xac minh that bai.");
        std::error_code ec;
        fs::remove(toPath(tempPath), ec);
        return false;
    }

    const std::optional<std::string> restoredHash = computeFileSha256(tempPath);
    std::error_code ec;
    fs::remove(toPath(tempPath), ec);

    if (!restoredHash)
    {
        ui_.showError("Khong tinh duoc SHA-256 cua file phuc hoi.");
        return false;
    }

    ui_.showKeyValue("SHA-256 goc", *originalHash);
    ui_.showKeyValue("SHA-256 phuc hoi", *restoredHash);

    if (*originalHash != *restoredHash)
    {
        ui_.showError("Hash khong khop - file phuc hoi khac file goc.");
        return false;
    }
    ui_.showSuccess("Xac minh thanh cong: file phuc hoi giong file goc 100%.");
    return true;
}

void MenuController::showFileInfo(const std::string &path) const
{
    std::error_code ec;
    const std::uintmax_t size = fs::file_size(toPath(path), ec);
    if (ec)
    {
        ui_.showError("Khong doc duoc file: " + path);
        return;
    }

    ui_.showSection("THONG TIN FILE");
    ui_.showKeyValue("Duong dan", path);
    ui_.showKeyValue("Kich thuoc", ConsoleUI::formatBytes(size) + " (" + std::to_string(size) + " byte)");

    std::ifstream probe(toPath(path), std::ios::binary);
    char magic[4] = {};
    const bool hasMagic = static_cast<bool>(probe.read(magic, 4));
    probe.close();

    const int version = hasMagic ? detectFormatVersion(magic) : 0;
    const char *formatName = version == 2 ? "HUF2 (da nen)"
                            : version == 1 ? "HUF1 (da nen, dinh dang cu)"
                                           : "Khong phai HUF2/HUF1 (file thuong)";
    ui_.showKeyValue("Dinh dang", formatName);

    if (const std::optional<std::string> hash = computeFileSha256(path))
        ui_.showKeyValue("SHA-256", *hash);
}

// ---------------------------------------------------------------- handlers

std::optional<std::string> MenuController::askExistingFile(const std::string &prompt) const
{
    const std::string path = ui_.readNonEmptyLine(prompt);
    if (!isRegularFile(path))
    {
        ui_.showError("Khong tim thay file: " + path);
        return std::nullopt;
    }
    return path;
}

bool MenuController::confirmOverwrite(const std::string &outputPath) const
{
    if (!isRegularFile(outputPath))
        return true;
    return ui_.confirm("File dich '" + outputPath + "' da ton tai, ghi de?", false);
}

void MenuController::handleCompress()
{
    ui_.showSection("NEN FILE");

    const std::optional<std::string> input = askExistingFile("File nguon: ");
    if (!input)
        return;

    const std::string output = ui_.readNonEmptyLine("File nen dau ra (vd: data.bin): ");
    if (!confirmOverwrite(output))
    {
        ui_.showInfo("Da huy thao tac.");
        return;
    }
    compressFile(*input, output);
}

void MenuController::handleDecompress()
{
    ui_.showSection("GIAI NEN FILE");

    const std::optional<std::string> input = askExistingFile("File .bin can giai nen: ");
    if (!input)
        return;

    const std::string output = ui_.readNonEmptyLine("File dau ra: ");
    if (!confirmOverwrite(output))
    {
        ui_.showInfo("Da huy thao tac.");
        return;
    }
    decompressFile(*input, output);
}

void MenuController::handleVerify()
{
    ui_.showSection("NEN VA XAC MINH");

    const std::optional<std::string> input = askExistingFile("File nguon can nen: ");
    if (!input)
        return;

    const std::string output = ui_.readNonEmptyLine("File nen dau ra (vd: data.bin): ");
    if (!confirmOverwrite(output))
    {
        ui_.showInfo("Da huy thao tac.");
        return;
    }
    compressAndVerify(*input, output);
}

void MenuController::handleFileInfo()
{
    ui_.showSection("XEM THONG TIN FILE");

    const std::string path = ui_.readNonEmptyLine("Duong dan file: ");
    showFileInfo(path);
}

void MenuController::handleHelp() const
{
    ui_.showSection("HUONG DAN SU DUNG");
    const std::vector<std::vector<std::string>> rows = {
        {"-c <nguon> <dich.bin>", "Nen file"},
        {"-d <dich.bin> <ra>", "Giai nen file"},
        {"-v <nguon> <dich.bin>", "Nen va xac minh SHA-256"},
        {"-m, --menu", "Mo menu tuong tac nay"},
        {"-h, --help", "Hien thi tro giup"},
    };
    ui_.showTable({"Lenh", "Mo ta"}, rows);
    ui_.showInfo("Chay 'huffman' khong tham so cung se mo menu tuong tac.");
}

void MenuController::handleAbout() const
{
    ui_.showSection("THONG TIN PHAN MEM");
    ui_.showKeyValue("Ten", kToolName);
    ui_.showKeyValue("Phien ban", kVersion);
    ui_.showKeyValue("Ngon ngu", "C++17");
    ui_.showKeyValue("Dinh dang file", "HUF2, doc duoc ca HUF1 (xem docs/fileFormat.md)");
    ui_.showKeyValue("Cau truc du lieu", "Min-Heap tu viet (khoa: freq, minSymbol)");
    ui_.showKeyValue("Toan ven du lieu", "CRC32 cua du lieu goc (HUF2) + SHA-256 khi dung -v");
}
