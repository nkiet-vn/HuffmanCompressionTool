#pragma once

#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

// Một mục trong menu: mã phím, nhãn hiển thị, mô tả ngắn.
struct MenuItem
{
    std::string key;
    std::string label;
    std::string description;
};

struct MenuHeader
{
    std::string title;
    std::string subtitle;
    std::string version;
};

// Nhóm hiển thị / thu thập dữ liệu cho giao diện dòng lệnh.
// Tách khỏi MenuController để logic nghiệp vụ không phụ thuộc vào cin/cout.
class ConsoleUI
{
public:
    ConsoleUI(std::istream &input, std::ostream &output);

    // --- Hiển thị ---
    void showHeader(const MenuHeader &header) const;
    void showMenu(const std::vector<MenuItem> &items) const;
    void showSection(const std::string &title) const;
    void showDivider() const;
    void showSuccess(const std::string &message) const;
    void showError(const std::string &message) const;
    void showInfo(const std::string &message) const;
    void showWarning(const std::string &message) const;
    void showKeyValue(const std::string &key, const std::string &value) const;

    // --- Thu thập dữ liệu ---
    // Đọc một dòng, tự cắt khoảng trắng hai đầu và ký tự xuống dòng.
    std::string readLine(const std::string &prompt) const;
    // Đọc dòng cho tới khi có nội dung khác rỗng.
    std::string readNonEmptyLine(const std::string &prompt) const;
    // Đọc lựa chọn của người dùng; trả về std::nullopt khi hết dữ liệu vào (EOF).
    std::optional<std::string> readChoice(const std::string &prompt = "Chon: ") const;
    // Hỏi có/không; mặc định khi chỉ bấm Enter.
    bool confirm(const std::string &question, bool defaultValue) const;

    // Tạm dừng để người dùng đọc kết quả trước khi menu vẽ lại.
    void waitForEnter(const std::string &prompt = "\nNhan Enter de quay lai menu...") const;

    // --- Tiện ích ---
    // Kẻ bảng hai cột có căn lề phải cho cột số liệu.
    void showTable(const std::vector<std::string> &headers,
                   const std::vector<std::vector<std::string>> &rows) const;

    static std::string formatBytes(std::uintmax_t bytes);

    std::istream &in() const { return input_; }
    std::ostream &out() const { return output_; }

private:
    std::istream &input_;
    std::ostream &output_;
};
