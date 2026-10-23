#ifndef DECOMPRESSOR_H
#define DECOMPRESSOR_H

#include <string>

// Giải nén file .bin (định dạng HUF1, xem docs/fileFormat.md).
class Decompressor {
public:
    static bool decompress(const std::string& inputBinPath, const std::string& outputTxtPath);
};

#endif // DECOMPRESSOR_H
