#include "canonicalHuffman.h"

#include <algorithm>
#include <utility>

bool getHuffmanCodeLengths(const std::shared_ptr<HuffmanNode>& root,
                           std::uint8_t lengths[256])
{
    std::fill(lengths, lengths + 256, std::uint8_t{0});
    if (!root)
        return true;

    std::vector<std::pair<std::shared_ptr<HuffmanNode>, unsigned int>> pending;
    pending.push_back({root, 0});
    while (!pending.empty())
    {
        const auto current = pending.back();
        pending.pop_back();
        const std::shared_ptr<HuffmanNode>& node = current.first;
        const unsigned int depth = current.second;
        if (node->isLeaf)
        {
            const unsigned int codeLength = depth == 0 ? 1 : depth;
            if (codeLength > 32)
                return false;
            lengths[node->symbol] = static_cast<std::uint8_t>(codeLength);
            continue;
        }
        if (depth >= 32 || !node->left || !node->right)
            return false;
        pending.push_back({node->right, depth + 1});
        pending.push_back({node->left, depth + 1});
    }
    return true;
}

bool buildCanonicalCodes(const std::uint8_t lengths[256],
                         std::vector<CanonicalCode>& codes)
{
    codes.clear();
    std::uint64_t kraftUnits = 0;
    for (unsigned int symbol = 0; symbol < 256; ++symbol)
    {
        const std::uint8_t length = lengths[symbol];
        if (length == 0)
            continue;
        if (length > 32)
            return false;
        kraftUnits += std::uint64_t{1} << (32 - length);
        if (kraftUnits > (std::uint64_t{1} << 32))
            return false;
        codes.push_back({static_cast<std::uint8_t>(symbol), length, 0});
    }

    std::sort(codes.begin(), codes.end(),
              [](const CanonicalCode& left, const CanonicalCode& right)
              {
                  if (left.length != right.length)
                      return left.length < right.length;
                  return left.symbol < right.symbol;
              });

    std::uint64_t code = 0;
    std::uint8_t previousLength = 0;
    for (std::size_t i = 0; i < codes.size(); ++i)
    {
        const std::uint8_t length = codes[i].length;
        if (i != 0)
            code = (code + 1) << (length - previousLength);
        if (code >= (std::uint64_t{1} << length))
            return false;
        codes[i].bits = static_cast<std::uint32_t>(code);
        previousLength = length;
    }
    return true;
}
