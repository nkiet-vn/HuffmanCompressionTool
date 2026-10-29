#include "menuController.h"
#include "pathUtil.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <system_error>

#include "compressionStats.h"
#include "fileHasher.h"
#include "fileFormat.h"
#include "compressor.h"
#include "decompressor.h"
#include "messages.h"

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
        {"1", ui_.text(MessageId::MenuCompress), ui_.text(MessageId::MenuCompressDescription)},
        {"2", ui_.text(MessageId::MenuDecompress), ui_.text(MessageId::MenuDecompressDescription)},
        {"3", ui_.text(MessageId::MenuVerify), ui_.text(MessageId::MenuVerifyDescription)},
        {"4", ui_.text(MessageId::MenuFileInfo), ui_.text(MessageId::MenuFileInfoDescription)},
        {"5", ui_.text(MessageId::MenuHelp), ui_.text(MessageId::MenuHelpDescription)},
        {"6", ui_.text(MessageId::MenuAbout), ui_.text(MessageId::MenuAboutDescription)},
        {"0", ui_.text(MessageId::MenuExit), ""},
    };
}

int MenuController::run()
{
    ui_.showHeader({kToolName, ui_.text(MessageId::ToolSubtitle), kVersion});

    while (running_)
    {
        ui_.showMenu(buildMenuItems());

        const std::optional<std::string> choice = ui_.readChoice();
        if (!choice)
        { // EOF: kết thúc phiên làm việc
            ui_.showInfo(ui_.text(MessageId::SessionEnded));
            break;
        }

        if (choice->empty())
        {
            ui_.showWarning(ui_.text(MessageId::NoChoice));
            continue;
        }
        if (*choice == "0" || *choice == "q" || *choice == "thoat")
        {
            running_ = false;
            ui_.showSuccess(ui_.text(MessageId::Goodbye));
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
            ui_.showError(std::string(ui_.text(MessageId::InvalidChoicePrefix)) + *choice +
                          "' " + ui_.text(MessageId::InvalidChoice));
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
    ui_.showInfo(std::string(ui_.text(MessageId::Compressing)) + inputPath + "  ->  " + outputPath);
    ProgressCallback progress;
    const std::optional<std::uintmax_t> sourceSize = getFileSize(inputPath);
    if (sourceSize && *sourceSize > 10ULL * 1024ULL * 1024ULL &&
        ConsoleUI::standardOutputIsTerminal())
    {
        progress = [this](std::uint64_t current, std::uint64_t total)
        {
            if (total != 0)
                ui_.showProgress(static_cast<unsigned int>(100.0 * current / total));
        };
    }
    if (!compressor_.compressWithProgress(inputPath, outputPath, progress))
    {
        const MessageId error = compressor_.lastError();
        const std::string detail = compressor_.lastErrorDetails();
        ui_.showError(error == MessageId::None
                          ? ui_.text(MessageId::CompressionFailed)
                          : std::string(ui_.text(error)) + detail);
        return false;
    }

    const std::optional<CompressionInfo> info = getCompressionInfo(inputPath, outputPath);
    if (info)
    {
        ui_.showSection(ui_.text(MessageId::CompressionStatsSection));
        ui_.showKeyValue(ui_.text(MessageId::OriginalSize), ConsoleUI::formatBytes(info->originalSize) +
                                               " (" + std::to_string(info->originalSize) +
                                               ui_.text(MessageId::ByteUnit) + ")");
        ui_.showKeyValue(ui_.text(MessageId::CompressedSize), ConsoleUI::formatBytes(info->compressedSize) +
                                                    " (" + std::to_string(info->compressedSize) +
                                                    ui_.text(MessageId::ByteUnit) + ")");
        ui_.showKeyValue(ui_.text(MessageId::CompressionRatio), formatCompressionRatio(info->ratioPercent));

        if (info->ratioPercent && *info->ratioPercent < 0.0)
        {
            ui_.showWarning(ui_.text(MessageId::LargerThanOriginal));
        }
        else if (info->ratioPercent)
        {
            ui_.showSuccess(std::string(ui_.text(MessageId::SavedSpace)) +
                            formatCompressionRatio(info->ratioPercent) +
                            ui_.text(MessageId::SavedSpaceSuffix));
        }
    }
    return true;
}

bool MenuController::decompressFile(const std::string &inputPath, const std::string &outputPath)
{
    ui_.showInfo(std::string(ui_.text(MessageId::Decompressing)) + inputPath + "  ->  " + outputPath);
    ProgressCallback progress;
    if (ConsoleUI::standardOutputIsTerminal())
    {
        progress = [this](std::uint64_t current, std::uint64_t total)
        {
            if (total != 0)
                ui_.showProgress(static_cast<unsigned int>(100.0 * current / total));
        };
    }
    if (!decompressor_.decompressWithProgress(inputPath, outputPath, progress))
    {
        const MessageId error = decompressor_.lastError();
        const std::string detail = decompressor_.lastErrorDetails();
        ui_.showError(error == MessageId::None
                          ? ui_.text(MessageId::DecompressionFailed)
                          : std::string(ui_.text(error)) + detail);
        return false;
    }

    const std::optional<std::uintmax_t> size = getFileSize(outputPath);
    if (size)
        ui_.showKeyValue(ui_.text(MessageId::OutputSize), ConsoleUI::formatBytes(*size));
    return true;
}

bool MenuController::compressAndVerify(const std::string &inputPath,
                                       const std::string &compressedPath)
{
    if (!compressFile(inputPath, compressedPath))
        return false;

    ui_.showSection(ui_.text(MessageId::VerifySection));

    const std::optional<std::string> originalHash = computeFileSha256(inputPath);
    if (!originalHash)
    {
        ui_.showError(ui_.text(MessageId::OriginalHashFailed));
        return false;
    }

    const std::string tempPath = compressedPath + ".verify_tmp";
    if (!decompressor_.decompress(compressedPath, tempPath))
    {
        ui_.showError(ui_.text(MessageId::VerifyDecompressionFailed));
        std::error_code ec;
        fs::remove(toPath(tempPath), ec);
        return false;
    }

    const std::optional<std::string> restoredHash = computeFileSha256(tempPath);
    std::error_code ec;
    fs::remove(toPath(tempPath), ec);

    if (!restoredHash)
    {
        ui_.showError(ui_.text(MessageId::RestoredHashFailed));
        return false;
    }

    ui_.showKeyValue(ui_.text(MessageId::OriginalHash), *originalHash);
    ui_.showKeyValue(ui_.text(MessageId::RestoredHash), *restoredHash);

    if (*originalHash != *restoredHash)
    {
        ui_.showError(ui_.text(MessageId::HashMismatch));
        return false;
    }
    ui_.showSuccess(ui_.text(MessageId::VerifySuccess));
    return true;
}

bool MenuController::showFileInfo(const std::string &path) const
{
    std::error_code ec;
    const std::uintmax_t size = fs::file_size(toPath(path), ec);
    if (ec)
    {
        ui_.showError(std::string(ui_.text(MessageId::FileReadFailed)) + path);
        return false;
    }

    std::ifstream probe(toPath(path), std::ios::binary);
    std::array<char, 17> header{};
    const bool hasMagic = static_cast<bool>(probe.read(header.data(), 4));
    const int version = hasMagic ? detectFormatVersion(header.data()) : 0;
    MessageId format = MessageId::FilePlain;
    std::optional<std::uint64_t> originalLength;
    std::optional<std::uint64_t> uniqueCount;
    if ((version == 2 || version == 3) && probe.read(header.data() + 4, 13))
    {
        const std::uint8_t mode = static_cast<std::uint8_t>(header[4]);
        if (mode == MODE_HUFFMAN || mode == MODE_STORED)
        {
            format = version == 3
                         ? (mode == MODE_HUFFMAN ? MessageId::FileHuf3Huffman
                                                 : MessageId::FileHuf3Stored)
                         : (mode == MODE_HUFFMAN ? MessageId::FileHuf2Huffman
                                                 : MessageId::FileHuf2Stored);
            originalLength = readLittleEndian(
                reinterpret_cast<const std::uint8_t *>(header.data() + 5), 8);
            if (mode == MODE_HUFFMAN)
            {
                std::array<std::uint8_t, 2> countBytes{};
                if (probe.read(reinterpret_cast<char *>(countBytes.data()), 2))
                    uniqueCount = readLittleEndian(countBytes.data(), 2);
            }
        }
    }
    else if (version == 1)
    {
        std::array<std::uint8_t, 10> legacyHeader{};
        if (probe.read(reinterpret_cast<char *>(legacyHeader.data()), 10))
        {
            format = MessageId::FileHuf1;
            uniqueCount = readLittleEndian(legacyHeader.data(), 2);
            originalLength = readLittleEndian(legacyHeader.data() + 2, 8);
        }
    }

    ui_.showSection(ui_.text(MessageId::FileInfoSection));
    ui_.showKeyValue(ui_.text(MessageId::FilePath), path);
    ui_.showKeyValue(ui_.text(MessageId::FileSize),
                     ConsoleUI::formatBytes(size) + " (" + std::to_string(size) +
                         ui_.text(MessageId::ByteUnit) + ")");
    ui_.showKeyValue(ui_.text(MessageId::FileFormat), ui_.text(format));
    ui_.showKeyValue(ui_.text(MessageId::OriginalLength),
                     originalLength
                         ? std::to_string(*originalLength) + ui_.text(MessageId::ByteUnit)
                         : ui_.text(MessageId::NotApplicable));
    const auto ratio = originalLength
                           ? calculateCompressionRatio(*originalLength, size)
                           : std::optional<double>{};
    ui_.showKeyValue(ui_.text(MessageId::CompressionRate),
                     originalLength ? formatCompressionRatio(ratio) : ui_.text(MessageId::NotApplicable));
    if (uniqueCount)
        ui_.showKeyValue(ui_.text(MessageId::UniqueCount), std::to_string(*uniqueCount));
    const std::optional<std::string> hash = computeFileSha256(path);
    if (!hash)
    {
        ui_.showError(std::string(ui_.text(MessageId::FileReadFailed)) + path);
        return false;
    }
    ui_.showKeyValue(ui_.text(MessageId::Sha256), *hash);
    return true;
}

// ---------------------------------------------------------------- handlers

std::optional<std::string> MenuController::askExistingFile(const std::string &prompt) const
{
    const std::string path = ui_.readNonEmptyLine(prompt);
    if (!isRegularFile(path))
    {
        ui_.showError(std::string(ui_.text(MessageId::FileNotFound)) + path);
        return std::nullopt;
    }
    return path;
}

bool MenuController::confirmOverwrite(const std::string &outputPath) const
{
    if (!isRegularFile(outputPath))
        return true;
    return ui_.confirm(std::string(ui_.text(MessageId::OverwriteQuestion)) + outputPath +
                           ui_.text(MessageId::OverwriteSuffix),
                       false);
}

void MenuController::handleCompress()
{
    ui_.showSection(ui_.text(MessageId::CompressSection));

    const std::optional<std::string> input = askExistingFile(ui_.text(MessageId::SourcePrompt));
    if (!input)
        return;

    const std::string output = ui_.readNonEmptyLine(ui_.text(MessageId::CompressionDestinationPrompt));
    if (!confirmOverwrite(output))
    {
        ui_.showInfo(ui_.text(MessageId::Cancelled));
        return;
    }
    compressFile(*input, output);
}

void MenuController::handleDecompress()
{
    ui_.showSection(ui_.text(MessageId::DecompressSection));

    const std::optional<std::string> input = askExistingFile(ui_.text(MessageId::CompressedFilePrompt));
    if (!input)
        return;

    const std::string output = ui_.readNonEmptyLine(ui_.text(MessageId::DestinationPrompt));
    if (!confirmOverwrite(output))
    {
        ui_.showInfo(ui_.text(MessageId::Cancelled));
        return;
    }
    decompressFile(*input, output);
}

void MenuController::handleVerify()
{
    ui_.showSection(ui_.text(MessageId::VerifyMenuSection));

    const std::optional<std::string> input = askExistingFile(ui_.text(MessageId::SourcePrompt));
    if (!input)
        return;

    const std::string output = ui_.readNonEmptyLine(ui_.text(MessageId::CompressionDestinationPrompt));
    if (!confirmOverwrite(output))
    {
        ui_.showInfo(ui_.text(MessageId::Cancelled));
        return;
    }
    compressAndVerify(*input, output);
}

void MenuController::handleFileInfo()
{
    ui_.showSection(ui_.text(MessageId::FileInfoSection));

    const std::string path = ui_.readNonEmptyLine(ui_.text(MessageId::InfoPathPrompt));
    showFileInfo(path);
}

void MenuController::handleHelp() const
{
    ui_.showSection(ui_.text(MessageId::HelpSection));
    const std::vector<std::vector<std::string>> rows = {
        {ui_.text(MessageId::HelpCmdCompress), ui_.text(MessageId::HelpCompress)},
        {ui_.text(MessageId::HelpCmdDecompress), ui_.text(MessageId::HelpDecompress)},
        {ui_.text(MessageId::HelpCmdVerify), ui_.text(MessageId::HelpVerify)},
        {ui_.text(MessageId::HelpCmdInfo), ui_.text(MessageId::HelpInfo)},
        {"-m, --menu", ui_.text(MessageId::HelpMenu)},
        {"-h, --help", ui_.text(MessageId::HelpHelp)},
    };
    ui_.showTable({ui_.text(MessageId::HelpCommand), ui_.text(MessageId::HelpDescription)}, rows);
    ui_.showInfo(ui_.text(MessageId::HelpMenuInfo));
}

void MenuController::handleAbout() const
{
    ui_.showSection(ui_.text(MessageId::AboutSection));
    ui_.showKeyValue(ui_.text(MessageId::Name), kToolName);
    ui_.showKeyValue(ui_.text(MessageId::Version), kVersion);
    ui_.showKeyValue(ui_.text(MessageId::Language), "C++17");
    ui_.showKeyValue(ui_.text(MessageId::Format), ui_.text(MessageId::AboutFormat));
    ui_.showKeyValue(ui_.text(MessageId::DataStructures), ui_.text(MessageId::AboutDataStructures));
    ui_.showKeyValue(ui_.text(MessageId::Integrity), ui_.text(MessageId::AboutIntegrity));
}
