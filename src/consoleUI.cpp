#include "consoleUI.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace
{

    constexpr std::size_t kLabelWidth = 38;
    constexpr std::size_t kMenuWidth = 62;

    std::string trim(const std::string &text)
    {
        const auto first = text.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
            return "";
        const auto last = text.find_last_not_of(" \t\r\n");
        return text.substr(first, last - first + 1);
    }

    // "Copy as path" của Windows dán đường dẫn kèm dấu ngoặc kép: bỏ cặp ngoặc bao quanh.
    std::string stripQuotes(const std::string &text)
    {
        if (text.size() >= 2)
        {
            const char first = text.front();
            if ((first == '"' || first == '\'') && text.back() == first)
                return trim(text.substr(1, text.size() - 2));
        }
        return text;
    }

    std::string toLower(std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(),
                       [](unsigned char c)
                       { return static_cast<char>(std::tolower(c)); });
        return text;
    }

    std::string repeat(char c, std::size_t count)
    {
        return std::string(count, c);
    }

    // Can le phai chuoi cho cac cot so lieu.
    std::string padLeft(const std::string &text, std::size_t width)
    {
        if (text.size() >= width)
            return text;
        return repeat(' ', width - text.size()) + text;
    }

} // namespace

ConsoleUI::ConsoleUI(std::istream &input, std::ostream &output)
    : input_(input), output_(output) {}

void ConsoleUI::showHeader(const MenuHeader &header) const
{
    output_ << repeat('=', kMenuWidth) << "\n";
    output_ << "  " << header.title;
    if (!header.version.empty())
        output_ << "  v" << header.version;
    output_ << "\n";
    if (!header.subtitle.empty())
        output_ << "  " << header.subtitle << "\n";
    output_ << repeat('=', kMenuWidth) << "\n";
}

void ConsoleUI::showMenu(const std::vector<MenuItem> &items) const
{
    output_ << "\n";
    for (const MenuItem &item : items)
    {
        output_ << "   " << std::left << std::setw(4) << ("[" + item.key + "]")
                << std::setw(static_cast<int>(kLabelWidth)) << item.label;
        if (!item.description.empty())
            output_ << "  " << item.description;
        output_ << "\n";
    }
    output_ << repeat('-', kMenuWidth) << "\n";
}

void ConsoleUI::showSection(const std::string &title) const
{
    // "--- " + title + " " chiếm title.size() + 5 ký tự, phần còn lại là gạch ngang
    // để đường kẻ đúng bằng kMenuWidth.
    const std::size_t used = title.size() + 5;
    const std::size_t dashes = used < kMenuWidth ? kMenuWidth - used : 0;
    output_ << "\n--- " << title << " " << repeat('-', dashes) << "\n";
}

void ConsoleUI::showDivider() const
{
    output_ << repeat('-', kMenuWidth) << "\n";
}

void ConsoleUI::showSuccess(const std::string &message) const
{
    output_ << "[OK] " << message << "\n";
}

void ConsoleUI::showError(const std::string &message) const
{
    output_ << "[LOI] " << message << "\n";
}

void ConsoleUI::showInfo(const std::string &message) const
{
    output_ << "[i] " << message << "\n";
}

void ConsoleUI::showWarning(const std::string &message) const
{
    output_ << "[!] " << message << "\n";
}

void ConsoleUI::showKeyValue(const std::string &key, const std::string &value) const
{
    output_ << "  " << std::left << std::setw(22) << key << ": " << value << "\n";
}

std::string ConsoleUI::readLine(const std::string &prompt) const
{
    output_ << prompt << std::flush;
    std::string line;
    if (!std::getline(input_, line))
        return "";
    return trim(line);
}

std::string ConsoleUI::readNonEmptyLine(const std::string &prompt) const
{
    for (;;)
    {
        std::string value = stripQuotes(readLine(prompt));
        if (!value.empty())
            return value;
        showWarning("Gia tri khong duoc de trong. Vui long nhap lai.");
    }
}

std::optional<std::string> ConsoleUI::readChoice(const std::string &prompt) const
{
    output_ << prompt << std::flush;
    std::string line;
    if (!std::getline(input_, line))
        return std::nullopt;
    return toLower(trim(line));
}

bool ConsoleUI::confirm(const std::string &question, bool defaultValue) const
{
    output_ << question << (defaultValue ? " [Y/n]: " : " [y/N]: ") << std::flush;
    std::string line;
    if (!std::getline(input_, line))
        return defaultValue;
    line = toLower(trim(line));
    if (line.empty())
        return defaultValue;
    return line == "y" || line == "yes" || line == "c" || line == "co";
}

void ConsoleUI::waitForEnter(const std::string &prompt) const
{
    output_ << prompt << std::flush;
    std::string line;
    std::getline(input_, line);
}

void ConsoleUI::showTable(const std::vector<std::string> &headers,
                          const std::vector<std::vector<std::string>> &rows) const
{
    if (headers.empty())
        return;

    // Độ rộng từng cột = max(độ dài header, độ dài ô dài nhất).
    std::vector<std::size_t> widths(headers.size(), 0);
    for (std::size_t col = 0; col < headers.size(); ++col)
        widths[col] = headers[col].size();
    for (const auto &row : rows)
    {
        for (std::size_t col = 0; col < row.size() && col < widths.size(); ++col)
            widths[col] = std::max(widths[col], row[col].size());
    }

    auto printRow = [&](const std::vector<std::string> &cells)
    {
        output_ << "|";
        for (std::size_t col = 0; col < widths.size(); ++col)
        {
            const std::string cell = col < cells.size() ? cells[col] : "";
            output_ << " " << padLeft(cell, widths[col]) << " |";
        }
        output_ << "\n";
    };

    printRow(headers);
    output_ << "|";
    for (std::size_t col = 0; col < widths.size(); ++col)
        output_ << repeat('-', widths[col] + 2) << "|";
    output_ << "\n";
    for (const auto &row : rows)
        printRow(row);
}

std::string ConsoleUI::formatBytes(std::uintmax_t bytes)
{
    std::ostringstream stream;
    if (bytes < 1024)
    {
        stream << bytes << " B";
    }
    else if (bytes < 1024ULL * 1024ULL)
    {
        stream << std::fixed << std::setprecision(1)
               << static_cast<double>(bytes) / 1024.0 << " KB";
    }
    else if (bytes < 1024ULL * 1024ULL * 1024ULL)
    {
        stream << std::fixed << std::setprecision(1)
               << static_cast<double>(bytes) / (1024.0 * 1024.0) << " MB";
    }
    else
    {
        stream << std::fixed << std::setprecision(2)
               << static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0) << " GB";
    }
    return stream.str();
}
