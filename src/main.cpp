#include <cstdlib>
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
#include "messages.h"

namespace
{

void printHelp(bool ascii)
{
    std::cout << message(MessageId::CliTitle, ascii) << "\n\n";
    std::cout << message(MessageId::Usage, ascii) << "\n";
    std::cout << message(MessageId::CliCompress, ascii) << "\n";
    std::cout << message(MessageId::CliDecompress, ascii) << "\n";
    std::cout << message(MessageId::CliVerify, ascii) << "\n";
    std::cout << message(MessageId::CliInfo, ascii) << "\n";
    std::cout << message(MessageId::CliMenu, ascii) << "\n";
    std::cout << message(MessageId::CliHelp, ascii) << "\n";
    std::cout << message(MessageId::CliAscii, ascii) << "\n\n";
    std::cout << message(MessageId::CliNoArgs, ascii) << "\n\n";
    std::cout << message(MessageId::CliExamples, ascii) << "\n";
    std::cout << message(MessageId::CliExampleCompress, ascii) << "\n";
    std::cout << message(MessageId::CliExampleDecompress, ascii) << "\n";
    std::cout << message(MessageId::CliExampleInfo, ascii) << "\n";
    std::cout << message(MessageId::CliExampleVerify, ascii) << "\n";
}

std::vector<std::string> utf8Arguments(int argc, char *argv[])
{
    std::vector<std::string> args(argv, argv + argc);
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    int wideCount = 0;
    LPWSTR *wideArgs = CommandLineToArgvW(GetCommandLineW(), &wideCount);
    if (wideArgs)
    {
        if (wideCount == argc)
        {
            for (int i = 0; i < argc; ++i)
            {
                const int bytes = WideCharToMultiByte(CP_UTF8, 0, wideArgs[i], -1, nullptr, 0, nullptr, nullptr);
                if (bytes <= 1)
                    continue;
                std::string utf8(static_cast<std::size_t>(bytes), '\0');
                WideCharToMultiByte(CP_UTF8, 0, wideArgs[i], -1, &utf8[0], bytes, nullptr, nullptr);
                utf8.resize(static_cast<std::size_t>(bytes - 1));
                args[static_cast<std::size_t>(i)] = utf8;
            }
        }
        LocalFree(wideArgs);
    }
#endif
    return args;
}

int runCliMode(const std::vector<std::string> &args, bool ascii)
{
    ConsoleUI ui(std::cin, std::cout, ascii);
    Compressor compressor;
    Decompressor decompressor;
    MenuController controller(ui, compressor, decompressor);

    if (args[0] == "-c")
        return controller.compressFile(args[1], args[2]) ? 0 : 1;
    if (args[0] == "-d")
        return controller.decompressFile(args[1], args[2]) ? 0 : 1;
    if (args[0] == "-v")
        return controller.compressAndVerify(args[1], args[2]) ? 0 : 1;
    if (args[0] == "-i")
        return controller.showFileInfo(args[1]) ? 0 : 1;
    std::cerr << message(MessageId::InvalidMode, ascii) << args[0]
              << message(MessageId::InvalidModeHelp, ascii) << "\n";
    return 1;
}

} // namespace

int main(int argc, char *argv[])
{
#ifdef _MSC_VER
    char *asciiEnvironment = nullptr;
    std::size_t environmentLength = 0;
    const bool hasAsciiEnvironment =
        _dupenv_s(&asciiEnvironment, &environmentLength, "HUFFMAN_ASCII") == 0 &&
        asciiEnvironment != nullptr;
    bool ascii = hasAsciiEnvironment && std::string(asciiEnvironment) == "1";
    std::free(asciiEnvironment);
#else
    const char *asciiEnvironment = std::getenv("HUFFMAN_ASCII");
    bool ascii = asciiEnvironment != nullptr && std::string(asciiEnvironment) == "1";
#endif
    std::vector<std::string> arguments = utf8Arguments(argc, argv);
    std::vector<std::string> args;
    args.reserve(arguments.size() > 0 ? arguments.size() - 1 : 0);
    for (std::size_t i = 1; i < arguments.size(); ++i)
    {
        if (arguments[i] == "--ascii")
            ascii = true;
        else
            args.push_back(arguments[i]);
    }

    if (args.empty())
    {
        ConsoleUI ui(std::cin, std::cout, ascii);
        Compressor compressor;
        Decompressor decompressor;
        return MenuController(ui, compressor, decompressor).run();
    }
    if (args.size() == 1 && (args[0] == "--help" || args[0] == "-h"))
    {
        printHelp(ascii);
        return 0;
    }
    if (args.size() == 1 && (args[0] == "--menu" || args[0] == "-m"))
    {
        ConsoleUI ui(std::cin, std::cout, ascii);
        Compressor compressor;
        Decompressor decompressor;
        return MenuController(ui, compressor, decompressor).run();
    }
    if (args.size() == 2 && args[0] == "-i")
        return runCliMode(args, ascii);
    if (args.size() == 3 &&
        (args[0] == "-c" || args[0] == "-d" || args[0] == "-v"))
        return runCliMode(args, ascii);

    if (!args.empty() && args[0] != "-c" && args[0] != "-d" &&
        args[0] != "-v" && args[0] != "-i")
        std::cerr << message(MessageId::InvalidMode, ascii) << args[0]
                  << message(MessageId::InvalidModeHelp, ascii) << "\n";
    else
        std::cerr << message(MessageId::InvalidArgumentCount, ascii) << "\n";
    return 1;
}
