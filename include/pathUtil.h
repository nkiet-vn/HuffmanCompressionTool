#ifndef PATH_UTIL_H
#define PATH_UTIL_H

#include <filesystem>
#include <string>
#include <system_error>

// Quy ước: mọi đường dẫn trong chương trình là chuỗi UTF-8.
// Trên Windows, std::ifstream/ofstream nhận `const char*` sẽ hiểu theo bảng mã ANSI (không phải UTF-8),
// nên đường dẫn có chữ có dấu (vd "Huffman-trê", "mẫu.txt") không mở được. Chuyển sang
// std::filesystem::path trước khi mở file để Windows dùng API Unicode (wchar_t).
// Trên Linux/macOS hàm này giữ nguyên các byte của chuỗi.
inline std::filesystem::path toPath(const std::string& utf8Path) {
    return std::filesystem::u8path(utf8Path);
}

inline bool areSameExistingFile(const std::filesystem::path& first,
                                const std::filesystem::path& second,
                                std::error_code& errorCode) {
    errorCode.clear();
    const bool firstExists = std::filesystem::exists(first, errorCode);
    if (errorCode || !firstExists) return false;

    const bool secondExists = std::filesystem::exists(second, errorCode);
    if (errorCode || !secondExists) return false;

    return std::filesystem::equivalent(first, second, errorCode);
}

#endif
