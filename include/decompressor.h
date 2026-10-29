#ifndef DECOMPRESSOR_H
#define DECOMPRESSOR_H

#include <string>
#include "iCodec.h"

// Giải nén file .bin: đọc HUF3 và vẫn đọc được HUF2/HUF1 (tương thích ngược).
// Với HUF2/HUF3, kiểm tra CRC32 của dữ liệu gốc và xóa file đích nếu không khớp.
class Decompressor : public IDecompressor {
public:
    bool decompress(const std::string& inputBinPath, const std::string& outputTxtPath) override;
    bool decompressWithProgress(const std::string& inputBinPath, const std::string& outputTxtPath,
                                const ProgressCallback& progress) override;
    MessageId lastError() const override { return lastError_; }
    const std::string& lastErrorDetails() const override { return lastErrorDetails_; }

private:
    MessageId lastError_ = MessageId::None;
    std::string lastErrorDetails_;
};

#endif // DECOMPRESSOR_H
