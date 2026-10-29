#ifndef DECOMPRESSOR_H
#define DECOMPRESSOR_H

#include <string>
#include "iCodec.h"

// Giải nén file .bin: đọc HUF2 và vẫn đọc được HUF1 (tương thích ngược), xem docs/fileFormat.md.
// Với HUF2, kiểm tra CRC32 của dữ liệu gốc sau khi giải mã và xóa file đích nếu không khớp.
class Decompressor : public IDecompressor {
public:
    bool decompress(const std::string& inputBinPath, const std::string& outputTxtPath) override;
};

#endif // DECOMPRESSOR_H
