#pragma once

#include <optional>
#include <ostream>
#include <string>

#include "CompressionStats.h"
#include "ICodec.h"

struct VerificationResult {
    bool compressOk = false;
    bool decompressOk = false;
    bool hashMatch = false;
    std::optional<CompressionInfo> info;
    std::string originalHash;
    std::string restoredHash;
    std::string errorMessage;

    bool isPassed() const { return compressOk && decompressOk && hashMatch; }
};

class CompressionVerifier {
public:
    CompressionVerifier(ICompressor& compressor, IDecompressor& decompressor);

    VerificationResult verifyRoundTrip(const std::string& originalPath,
                                       const std::string& compressedPath,
                                       const std::string& restoredPath,
                                       std::ostream& out);

private:
    ICompressor& compressor_;
    IDecompressor& decompressor_;
};
