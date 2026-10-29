#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include <string>
#include "iCodec.h"

// Nén file thành định dạng HUF2 (xem docs/fileFormat.md):
// tự chọn HUFFMAN hoặc STORED sao cho file nén không lớn hơn HEADER_SIZE + kích thước gốc.
class Compressor : public ICompressor {
public:
    bool compress(const std::string& inputPath, const std::string& outputBinPath) override;
};

#endif // COMPRESSOR_H
