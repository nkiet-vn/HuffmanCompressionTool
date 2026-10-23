#pragma once

#include <string>

class ICompressor {
public:
    virtual ~ICompressor() = default;

    virtual bool compress(const std::string& inputPath,
                          const std::string& outputPath) = 0;
};

class IDecompressor {
public:
    virtual ~IDecompressor() = default;

    virtual bool decompress(const std::string& inputPath,
                            const std::string& outputPath) = 0;
};
