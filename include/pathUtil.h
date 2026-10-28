#ifndef PATH_UTIL_H
#define PATH_UTIL_H

#include <filesystem>
#include <string>

// Quy ước: mọi đường dẫn trong chương trình là chuỗi UTF-8.
// Trên Windows, std::ifstream/ofstream nhận `const char*` sẽ hiểu theo bảng mã ANSI (không phải UTF-8),
// nên đường dẫn có chữ có dấu (vd "Huffman-trê", "mẫu.txt") không mở được. Chuyển sang
// std::filesystem::path trước khi mở file để Windows dùng API Unicode (wchar_t).
// Trên Linux/macOS hàm này giữ nguyên các byte của chuỗi.
inline std::filesystem::path toPath(const std::string& utf8Path) {
    return std::filesystem::u8path(utf8Path);
}

#endif
