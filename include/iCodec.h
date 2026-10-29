#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "messages.h"

using ProgressCallback = std::function<void(std::uint64_t, std::uint64_t)>;

class ICompressor {
public:
    virtual ~ICompressor() = default;

    virtual bool compress(const std::string& inputPath,
                          const std::string& outputPath) = 0;
    virtual bool compressWithProgress(const std::string& inputPath,
                                      const std::string& outputPath,
                                      const ProgressCallback& progress) {
        (void)progress;
        return compress(inputPath, outputPath);
    }
    virtual MessageId lastError() const { return MessageId::None; }
    virtual const std::string& lastErrorDetails() const {
        static const std::string empty;
        return empty;
    }
};

class IDecompressor {
public:
    virtual ~IDecompressor() = default;

    virtual bool decompress(const std::string& inputPath,
                            const std::string& outputPath) = 0;
    virtual bool decompressWithProgress(const std::string& inputPath,
                                        const std::string& outputPath,
                                        const ProgressCallback& progress) {
        (void)progress;
        return decompress(inputPath, outputPath);
    }
    virtual MessageId lastError() const { return MessageId::None; }
    virtual const std::string& lastErrorDetails() const {
        static const std::string empty;
        return empty;
    }
};
