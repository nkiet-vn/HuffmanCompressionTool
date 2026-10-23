#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include <string>

// Nén file thành định dạng HUF1 (xem docs/fileFormat.md).
class Compressor {
public:
    static bool compress(const std::string& inputPath, const std::string& outputBinPath);
};

#endif // COMPRESSOR_H
