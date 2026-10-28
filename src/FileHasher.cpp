#include "FileHasher.h"
#include "pathUtil.h"

#include <cstdint>
#include <fstream>
#include <vector>

#include "sha256.h"

std::optional<std::string> computeFileSha256(const std::string& filePath) {
    std::ifstream input(toPath(filePath), std::ios::binary);
    if (!input.is_open()) {
        return std::nullopt;
    }

    Sha256 hasher;
    std::vector<char> chunk(64 * 1024);
    while (input) {
        input.read(chunk.data(), static_cast<std::streamsize>(chunk.size()));
        std::streamsize bytesRead = input.gcount();
        if (bytesRead > 0) {
            hasher.update(reinterpret_cast<const std::uint8_t*>(chunk.data()),
                          static_cast<std::size_t>(bytesRead));
        }
    }
    if (input.bad()) {
        return std::nullopt;
    }
    return hasher.finalizeHex();
}

HashCompareResult compareFilesByHash(const std::string& filePathA,
                                     const std::string& filePathB) {
    HashCompareResult result;
    std::optional<std::string> hashA = computeFileSha256(filePathA);
    std::optional<std::string> hashB = computeFileSha256(filePathB);
    if (!hashA || !hashB) {
        return result;
    }
    result.readable = true;
    result.hashA = *hashA;
    result.hashB = *hashB;
    result.match = (result.hashA == result.hashB);
    return result;
}
