#include "compressionVerifier.h"

#include "fileHasher.h"

CompressionVerifier::CompressionVerifier(ICompressor& compressor,
                                         IDecompressor& decompressor)
    : compressor_(compressor), decompressor_(decompressor) {}

VerificationResult CompressionVerifier::verifyRoundTrip(const std::string& originalPath,
                                                        const std::string& compressedPath,
                                                        const std::string& restoredPath,
                                                        std::ostream& out) {
    VerificationResult result;

    std::optional<std::string> originalHash = computeFileSha256(originalPath);
    if (!originalHash) {
        result.errorMessage = "Khong doc duoc file goc: " + originalPath;
        out << "Loi: " << result.errorMessage << "\n";
        return result;
    }
    result.originalHash = *originalHash;

    result.compressOk = compressor_.compress(originalPath, compressedPath);
    if (!result.compressOk) {
        result.errorMessage = "Nen that bai.";
        out << "Loi: " << result.errorMessage << "\n";
        return result;
    }

    result.info = getCompressionInfo(originalPath, compressedPath);
    if (!result.info) {
        result.errorMessage = "Khong doc duoc kich thuoc file nen: " + compressedPath;
        out << "Loi: " << result.errorMessage << "\n";
        return result;
    }
    printCompressionInfo(*result.info, out);

    result.decompressOk = decompressor_.decompress(compressedPath, restoredPath);
    if (!result.decompressOk) {
        result.errorMessage = "Giai nen that bai.";
        out << "Loi: " << result.errorMessage << "\n";
        return result;
    }

    std::optional<std::string> restoredHash = computeFileSha256(restoredPath);
    if (!restoredHash) {
        result.errorMessage = "Khong doc duoc file giai nen: " + restoredPath;
        out << "Loi: " << result.errorMessage << "\n";
        return result;
    }
    result.restoredHash = *restoredHash;
    result.hashMatch = (result.originalHash == result.restoredHash);

    out << "SHA-256 file goc     : " << result.originalHash << "\n";
    out << "SHA-256 file giai nen: " << result.restoredHash << "\n";
    if (result.hashMatch) {
        out << "Ket qua: KHOP - file giai nen giong file goc 100%.\n";
    } else {
        result.errorMessage = "Hash khong khop.";
        out << "Ket qua: KHONG KHOP - file giai nen khac file goc.\n";
    }
    return result;
}
