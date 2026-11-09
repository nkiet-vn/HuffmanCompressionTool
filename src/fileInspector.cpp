#include "fileInspector.h"
#include "fileFormat.h"
#include "pathUtil.h"
#include "canonicalHuffman.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

namespace {
std::string hexBytes(const std::vector<std::uint8_t>& bytes) {
    std::ostringstream out;
    out << std::uppercase << std::hex << std::setfill('0');
    for (auto byte : bytes) out << std::setw(2) << static_cast<int>(byte) << ' ';
    std::string result = out.str();
    if (!result.empty()) result.pop_back();
    return result;
}
}
InspectedFile inspectFile(const std::string& utf8Path) {
    InspectedFile result;
    std::error_code ec;
    const auto path = toPath(utf8Path);
    if (!std::filesystem::is_regular_file(path, ec) || ec) {
        result.error = "Cannot open a regular file";
        return result;
    }
    result.fileSize = std::filesystem::file_size(path, ec);
    if (ec) { result.error = ec.message(); return result; }
    std::ifstream in(path, std::ios::binary);
    if (!in) { result.error = "Cannot read file"; return result; }
    result.readable = true;
    if (result.fileSize < 4) return result;
    char magic[4];
    if (!in.read(magic, 4)) return result;
    result.version = detectFormatVersion(magic);
    if (!result.version) return result;
    if (result.fileSize < (result.version == 1 ? 14u : 17u)) {
        result.error = "Truncated header";
        return result;
    }
    in.seekg(0);
    const auto segment = [&](const std::string& name, std::size_t length, InspectedFile& info) -> std::vector<std::uint8_t> {
        const std::streampos pos = in.tellg();
        if (pos < 0 || static_cast<std::uint64_t>(pos) + length > info.fileSize) return {};
        std::vector<std::uint8_t> bytes(length);
        if (!in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(length))) return {};
        info.segments.push_back({name, static_cast<std::uint64_t>(pos), length, hexBytes(bytes)});
        return bytes;
    };
    segment("magic", 4, result);
    if (result.version >= 2) {
        auto mode = segment("mode", 1, result);
        auto length = segment("originalLength", 8, result);
        auto crc = segment("crc32", 4, result);
        if (mode.empty() || length.empty() || crc.empty()) { result.error = "Truncated header"; return result; }
        result.mode = mode[0];
        result.originalLength = readLittleEndian(length.data(), 8);
        result.crc32 = static_cast<std::uint32_t>(readLittleEndian(crc.data(), 4));
        if (result.mode > 1) { result.error = "Invalid storage mode"; return result; }
    } else {
        result.mode = 0; // HUF1 starts with uniqueCount, then originalLength
        auto count = segment("uniqueCount", 2, result);
        auto length = segment("originalLength", 8, result);
        if (length.empty() || count.empty()) { result.error = "Truncated header"; return result; }
        result.symbolCount = static_cast<std::uint16_t>(readLittleEndian(count.data(), 2));
        result.originalLength = readLittleEndian(length.data(), 8);
    }
    if (result.mode == 0) {
        std::vector<std::uint8_t> count;
        if (result.version != 1) count = segment("uniqueCount", 2, result);
        else { count.resize(2); count[0] = static_cast<std::uint8_t>(result.symbolCount & 255); count[1] = static_cast<std::uint8_t>(result.symbolCount >> 8); }
        if (count.empty()) { result.error = "Truncated symbol count"; return result; }
        result.symbolCount = static_cast<std::uint16_t>(readLittleEndian(count.data(), 2));
        if (result.symbolCount > 256 || (result.originalLength == 0) != (result.symbolCount == 0)) {
            result.error = "Invalid symbol count"; return result;
        }
        if (result.version == 3) {
            std::array<bool, 256> seen{};
            int lastSymbol = -1;
            for (unsigned i = 0; i < result.symbolCount; ++i) {
                const auto pair = segment("symbolLength", 2, result);
                if (pair.size() != 2 || seen[pair[0]] || int(pair[0]) <= lastSymbol || pair[1] == 0 || pair[1] > 32) {
                    result.error = "Invalid canonical code length table"; return result;
                }
                seen[pair[0]] = true;
                lastSymbol = pair[0];
                result.codeLengths[pair[0]] = pair[1];
            }
            std::vector<CanonicalCode> codes;
            if (!buildCanonicalCodes(result.codeLengths.data(), codes)) {
                result.error = "Invalid canonical code lengths"; return result;
            }
        }
        // HUF1/HUF2 contain a frequency table, not canonical code lengths.
    }
    result.validHeader = true;
    return result;
}
