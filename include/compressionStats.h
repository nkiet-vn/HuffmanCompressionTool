#pragma once

#include <cstdint>
#include <optional>
#include <ostream>
#include <string>

struct CompressionInfo {
    std::uintmax_t originalSize = 0;
    std::uintmax_t compressedSize = 0;
    std::optional<double> ratioPercent;
};

std::optional<std::uintmax_t> getFileSize(const std::string& filePath);

std::optional<double> calculateCompressionRatio(std::uintmax_t originalSize,
                                                std::uintmax_t compressedSize);

std::optional<CompressionInfo> getCompressionInfo(const std::string& originalPath,
                                                  const std::string& compressedPath);

std::string formatCompressionRatio(const std::optional<double>& ratioPercent);

void printCompressionInfo(const CompressionInfo& info, std::ostream& out);

bool printCompressionReport(const std::string& originalPath,
                            const std::string& compressedPath,
                            std::ostream& out);
