#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#ifdef _MSC_VER
#pragma comment(lib, "shell32.lib")
#endif
#endif

#include "compressor.h"
#include "consoleUI.h"
#include "decompressor.h"
#include "menuController.h"

namespace {

void printHelp() {
    std::cout << "Huffman Compression Tool - Cong cu nen du lieu Huffman (C++17)\n\n";
    std::cout << "Su dung:\n";
    std::cout << "  huffman -c <file_nguon> <file_nen.bin>   Nen file\n";
    std::cout << "  huffman -d <file_nen.bin> <file_dich>    Giai nen file\n";
    std::cout << "  huffman -v <file_nguon> <file_nen.bin>   Nen va xac minh khoi phuc SHA-256\n";
    std::cout << "  huffman -m, --menu                       Mo menu tuong tac\n";
    std::cout << "  huffman --help                           Hien thi huong dan nay\n\n";
    std::cout << "Chay khong tham so cung se mo menu tuong tac.\n\n";
    std::cout << "Vi du:\n";
    std::cout << "  huffman -c input.txt output.bin\n";
    std::cout << "  huffman -d output.bin restored.txt\n";
    std::cout << "  huffman -v input.txt output.bin\n";
}

// Trả về tham số dòng lệnh dạng UTF-8.
// Windows: đặt console sang UTF-8 (nhập/xuất chữ có dấu) và lấy tham số từ GetCommandLineW,
// vì argv của Windows đi qua bảng mã ANSI nên chữ có dấu (vd "mẫu.txt") bị biến thành '?'.
std::vector<std::string> utf8Arguments(int argc, char* argv[]) {
    std::vector<std::string> args(argv, argv + argc);
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    int wideCount = 0;
    LPWSTR* wideArgs = CommandLineToArgvW(GetCommandLineW(), &wideCount);
    if (wideArgs) {
        if (wideCount == argc) {
            for (int i = 0; i < argc; ++i) {
                const int bytes = WideCharToMultiByte(CP_UTF8, 0, wideArgs[i], -1, nullptr, 0, nullptr, nullptr);
                if (bytes <= 1) continue;
                std::string utf8(static_cast<size_t>(bytes), '\0');
                WideCharToMultiByte(CP_UTF8, 0, wideArgs[i], -1, &utf8[0], bytes, nullptr, nullptr);
                utf8.resize(static_cast<size_t>(bytes - 1));
                args[static_cast<size_t>(i)] = utf8;
            }
        }
        LocalFree(wideArgs);
    }
#endif
    return args;
}

int runCliMode(const std::string& mode, const std::string& inputPath,
               const std::string& outputPath) {
    ConsoleUI ui(std::cin, std::cout);
    Compressor compressor;
    Decompressor decompressor;
    MenuController controller(ui, compressor, decompressor);

    if (mode == "-c") return controller.compressFile(inputPath, outputPath) ? 0 : 1;
    if (mode == "-d") return controller.decompressFile(inputPath, outputPath) ? 0 : 1;
    if (mode == "-v") return controller.compressAndVerify(inputPath, outputPath) ? 0 : 1;
    return 1;
}

} // namespace

int main(int argc, char* argv[]) {
    const std::vector<std::string> args = utf8Arguments(argc, argv);
    if (argc == 2) {
        const std::string arg = args[1];
        if (arg == "--help" || arg == "-h") {
            printHelp();
            return 0;
        }
        if (arg == "--menu" || arg == "-m") {
            ConsoleUI ui(std::cin, std::cout);
            Compressor compressor;
            Decompressor decompressor;
            return MenuController(ui, compressor, decompressor).run();
        }
        std::cerr << "Loi: Che do khong hop le '" << arg << "'. Dung '--help' de xem huong dan.\n";
        return 1;
    }

    if (argc == 1) {
        ConsoleUI ui(std::cin, std::cout);
        Compressor compressor;
        Decompressor decompressor;
        return MenuController(ui, compressor, decompressor).run();
    }

    if (argc != 4) {
        std::cerr << "Loi: Sai so luong tham so. Dung '--help' de xem huong dan.\n";
        return 1;
    }

    const std::string mode = args[1];
    if (mode != "-c" && mode != "-d" && mode != "-v") {
        std::cerr << "Loi: Che do khong hop le '" << mode << "'. Dung '--help' de xem huong dan.\n";
        return 1;
    }
    return runCliMode(mode, args[2], args[3]);
}
