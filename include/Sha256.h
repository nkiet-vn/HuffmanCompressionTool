#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

class Sha256 {
public:
    Sha256();

    void update(const std::uint8_t* data, std::size_t length);

    std::string finalizeHex();

private:
    void processBlock(const std::uint8_t* block);

    std::uint32_t state_[8];
    std::uint8_t buffer_[64];
    std::size_t bufferLength_;
    std::uint64_t totalBits_;
};
