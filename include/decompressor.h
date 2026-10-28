#ifndef DECOMPRESSOR_H
#define DECOMPRESSOR_H

#include <string>
#include "ICodec.h"

// Giải nén file .bin (định dạng HUF1, xem docs/fileFormat.md).
class Decompressor : public IDecompressor {
public:
    bool decompress(const std::string& inputBinPath, const std::string& outputTxtPath) override;
};

#endif // DECOMPRESSOR_H
