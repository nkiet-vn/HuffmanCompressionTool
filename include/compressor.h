#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include <string>
#include "iCodec.h"

// Nén file thành định dạng HUF1 (xem docs/fileFormat.md).
class Compressor : public ICompressor {
public:
    bool compress(const std::string& inputPath, const std::string& outputBinPath) override;
};

#endif // COMPRESSOR_H
