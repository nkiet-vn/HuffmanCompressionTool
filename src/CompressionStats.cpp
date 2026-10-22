#include "CompressionStats.h"

#include <filesystem>
#include <iomanip>
#include <sstream>
#include <system_error>

std::optional<std::uintmax_t> getFileSize(const std::string& filePath) {
    std::error_code errorCode;
    if (!std::filesystem::is_regular_file(filePath, errorCode) || errorCode) {
        return std::nullopt;
    }
    std::uintmax_t size = std::filesystem::file_size(filePath, errorCode);
    if (errorCode) {
        return std::nullopt;
    }
    return size;
}

std::optional<double> calculateCompressionRatio(std::uintmax_t originalSize,
                                                std::uintmax_t compressedSize) {
    if (originalSize == 0) {
        return std::nullopt;
    }
    double sizeRatio = static_cast<double>(compressedSize) / static_cast<double>(originalSize);
    return (1.0 - sizeRatio) * 100.0;
}

std::optional<CompressionInfo> getCompressionInfo(const std::string& originalPath,
                                                  const std::string& compressedPath) {
    std::optional<std::uintmax_t> originalSize = getFileSize(originalPath);
    std::optional<std::uintmax_t> compressedSize = getFileSize(compressedPath);
    if (!originalSize || !compressedSize) {
        return std::nullopt;
    }
    CompressionInfo info;
    info.originalSize = *originalSize;
    info.compressedSize = *compressedSize;
    info.ratioPercent = calculateCompressionRatio(info.originalSize, info.compressedSize);
    return info;
}

std::string formatCompressionRatio(const std::optional<double>& ratioPercent) {
    if (!ratioPercent) {
        return "N/A";
    }
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << *ratioPercent << "%";
    return stream.str();
}

void printCompressionInfo(const CompressionInfo& info, std::ostream& out) {
    out << "Kich thuoc goc : " << info.originalSize << " byte\n";
    out << "Kich thuoc nen : " << info.compressedSize << " byte\n";
    out << "Ty le nen      : " << formatCompressionRatio(info.ratioPercent) << "\n";

    if (!info.ratioPercent) {
        out << "Luu y: file goc rong nen khong tinh duoc ty le nen.\n";
    } else if (*info.ratioPercent < 0.0) {
        out << "Luu y: file nen LON HON file goc (header va bang tan suat "
               "chiem nhieu hon phan du lieu tiet kiem duoc).\n";
    }
}

bool printCompressionReport(const std::string& originalPath,
                            const std::string& compressedPath,
                            std::ostream& out) {
    std::optional<CompressionInfo> info = getCompressionInfo(originalPath, compressedPath);
    if (!info) {
        out << "Loi: khong doc duoc kich thuoc file goc hoac file nen.\n";
        return false;
    }
    printCompressionInfo(*info, out);
    return true;
}
