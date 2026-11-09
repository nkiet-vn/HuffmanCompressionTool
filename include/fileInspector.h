#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

struct HeaderSegment {
    std::string name;
    std::uint64_t offset = 0;
    std::uint64_t length = 0;
    std::string hex;
};

struct InspectedFile {
    bool readable = false;
    bool validHeader = false;
    int version = 0; // 0 = plain, 1, 2, 3 = HUF versions
    int mode = -1; // -1 unknown, 0 = HUFFMAN, 1 = STORED
    std::uint64_t fileSize = 0;
    std::uint64_t originalLength = 0;
    std::uint32_t crc32 = 0;
    std::uint16_t symbolCount = 0;
    std::array<std::uint8_t, 256> codeLengths{};
    std::vector<HeaderSegment> segments;
    std::string error;
};

// Inspects binary header fields only. Does NOT claim to validate entire payload/CRC.
InspectedFile inspectFile(const std::string& utf8Path);
