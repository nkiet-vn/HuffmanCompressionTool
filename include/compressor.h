#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include <string>
#include "iCodec.h"

// Nén file thành định dạng HUF3 (xem docs/fileFormat.md):
// tự chọn HUFFMAN hoặc STORED sao cho file nén không lớn hơn HEADER_SIZE + kích thước gốc.
class Compressor : public ICompressor {
public:
    bool compress(const std::string& inputPath, const std::string& outputBinPath) override;
    bool compressWithProgress(const std::string& inputPath, const std::string& outputBinPath,
                              const ProgressCallback& progress) override;
    MessageId lastError() const override { return lastError_; }
    const std::string& lastErrorDetails() const override { return lastErrorDetails_; }

private:
    MessageId lastError_ = MessageId::None;
    std::string lastErrorDetails_;
};

#endif // COMPRESSOR_H
