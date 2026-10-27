#pragma once

#include <optional>
#include <string>
#include <vector>

#include "consoleUI.h"

class Compressor;
class Decompressor;

// Điều phối menu tương tác: vòng lặp chọn chức năng + các thao tác nghiệp vụ.
// Lớp này không đọc/ghi trực tiếp cin/cout mà đi qua ConsoleUI.
class MenuController {
public:
    MenuController(ConsoleUI& ui, Compressor& compressor, Decompressor& decompressor);

    // Kết quả chạy: 0 = thoát bình thường.
    int run();

    // Các thao tác công khai để test hoặc tái sử dụng từ chế độ dòng lệnh.
    bool compressFile(const std::string& inputPath, const std::string& outputPath);
    bool decompressFile(const std::string& inputPath, const std::string& outputPath);
    bool compressAndVerify(const std::string& inputPath, const std::string& compressedPath);
    void showFileInfo(const std::string& path) const;

private:
    // --- Xử lý từng mục menu ---
    void handleCompress();
    void handleDecompress();
    void handleVerify();
    void handleFileInfo();
    void handleHelp() const;
    void handleAbout() const;

    // --- Tiện ích nội bộ ---
    std::vector<MenuItem> buildMenuItems() const;
    std::optional<std::string> askExistingFile(const std::string& prompt) const;
    bool confirmOverwrite(const std::string& outputPath) const;

    ConsoleUI& ui_;
    Compressor& compressor_;
    Decompressor& decompressor_;
    bool running_ = true;
};
