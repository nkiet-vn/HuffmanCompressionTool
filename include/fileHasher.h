#pragma once

#include <optional>
#include <string>

std::optional<std::string> computeFileSha256(const std::string& filePath);

struct HashCompareResult {
    bool readable = false;
    bool match = false;
    std::string hashA;
    std::string hashB;
};

HashCompareResult compareFilesByHash(const std::string& filePathA,
                                     const std::string& filePathB);
