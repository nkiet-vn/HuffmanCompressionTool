#pragma once
#include <string>

// Nén inPath thành outPath theo định dạng HUF1 (docs/fileFormat.md).
// Trả về false và điền error nếu lỗi. Được cài đặt trong compressor.cpp.
bool compressFile(const std::string& inPath, const std::string& outPath, std::string& error);
