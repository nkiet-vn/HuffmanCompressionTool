#include "bitReader.h"
#include "pathUtil.h"

BitReader::BitReader(const std::string& filename) : buffer(0), bitIndex(-1) {
    file.open(toPath(filename), std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Lỗi BitReader: Không thể mở file " + filename);
    }
}

BitReader::BitReader(const std::string& filename, std::streamoff startOffset)
    : buffer(0), bitIndex(-1) {
    file.open(toPath(filename), std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Lỗi BitReader: Không thể mở file " + filename);
    }
    file.seekg(startOffset, std::ios::beg);
    if (!file) {
        throw std::runtime_error("Lỗi BitReader: Không thể di chuyển tới offset " +
                                 std::to_string(startOffset) + " trong file " + filename);
    }
}

BitReader::~BitReader() {
    if (file.is_open()) {
        file.close();
    }
}

void BitReader::loadNextByte() {
    char c;
    if (file.get(c)) {
        buffer = static_cast<uint8_t>(c);
        bitIndex = 7; // Bắt đầu đọc từ bit cao nhất (MSB)
    } else {
        bitIndex = -1; // Đã hết file
    }
}

int BitReader::readBit() {
    if (bitIndex < 0) {
        loadNextByte();
    }

    if (bitIndex < 0) {
        return -1; // Hết dữ liệu (EOF)
    }

    // Đọc bit bằng phép dịch bit >> và phép AND & theo đúng yêu cầu Jira
    int bit = (buffer >> bitIndex) & 1;
    bitIndex--;

    return bit;
}

bool BitReader::isEnd() {
    if (bitIndex >= 0) {
        return false;
    }
    return file.peek() == EOF;
}
