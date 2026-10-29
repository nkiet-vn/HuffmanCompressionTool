#include "bitReader.h"
#include "pathUtil.h"

BitReader::BitReader(const std::string& filename)
    : blockPosition(0), blockSize(0), reachedEnd(false), buffer(0), bitIndex(-1) {
    file.open(toPath(filename), std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Lỗi BitReader: Không thể mở file " + filename);
    }
}

BitReader::BitReader(const std::string& filename, std::streamoff startOffset)
    : blockPosition(0), blockSize(0), reachedEnd(false), buffer(0), bitIndex(-1) {
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
    if (blockPosition == blockSize) {
        if (reachedEnd) {
            bitIndex = -1;
            return;
        }

        file.read(reinterpret_cast<char*>(byteBlock.data()),
                  static_cast<std::streamsize>(byteBlock.size()));
        const std::streamsize bytesRead = file.gcount();
        blockPosition = 0;
        blockSize = bytesRead > 0 ? static_cast<std::size_t>(bytesRead) : 0;
        reachedEnd = bytesRead < static_cast<std::streamsize>(byteBlock.size());
    }

    if (blockPosition < blockSize) {
        buffer = byteBlock[blockPosition++];
        bitIndex = 7; // Bắt đầu đọc từ bit cao nhất (MSB)
    } else {
        bitIndex = -1; // Đã hết dữ liệu
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
    loadNextByte();
    return bitIndex < 0;
}
