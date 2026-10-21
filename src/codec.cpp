#include "codec.h"
#include "bitWriter.h"
#include <array>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <queue>
#include <utility>
#include <vector>

namespace {

struct Node {
    uint64_t weight;
    int minSymbol;
    int symbol;  // -1 nếu là nút trong
    int left;
    int right;
};

void writeLE(std::ostream& out, uint64_t v, int bytes) {
    for (int i = 0; i < bytes; ++i) {
        out.put(static_cast<char>((v >> (8 * i)) & 0xFF));
    }
}

// Dựng cây theo fileFormat.md mục 3 và gán mã (trái = 0, phải = 1)
void buildCodes(const std::array<uint64_t, 256>& freq,
                std::array<std::string, 256>& codes) {
    std::vector<Node> nodes;
    for (int s = 0; s < 256; ++s) {
        if (freq[s] > 0) nodes.push_back({freq[s], s, s, -1, -1});
    }
    if (nodes.empty()) return;
    if (nodes.size() == 1) {  // chỉ 1 symbol: mã là "0"
        codes[nodes[0].symbol] = "0";
        return;
    }

    // Khóa (weight, minSymbol): nhỏ hơn được lấy ra trước
    auto greater = [&nodes](int a, int b) {
        if (nodes[a].weight != nodes[b].weight) return nodes[a].weight > nodes[b].weight;
        return nodes[a].minSymbol > nodes[b].minSymbol;
    };
    std::priority_queue<int, std::vector<int>, decltype(greater)> pq(greater);
    for (size_t i = 0; i < nodes.size(); ++i) pq.push(static_cast<int>(i));

    while (pq.size() > 1) {
        int a = pq.top(); pq.pop();  // A: khóa nhỏ hơn -> con trái
        int b = pq.top(); pq.pop();  // B -> con phải
        Node parent{nodes[a].weight + nodes[b].weight,
                    std::min(nodes[a].minSymbol, nodes[b].minSymbol), -1, a, b};
        nodes.push_back(parent);
        pq.push(static_cast<int>(nodes.size()) - 1);
    }

    std::vector<std::pair<int, std::string>> stack;
    stack.push_back({pq.top(), ""});
    while (!stack.empty()) {
        std::pair<int, std::string> cur = stack.back();
        stack.pop_back();
        const Node& n = nodes[cur.first];
        if (n.symbol >= 0) {
            codes[n.symbol] = cur.second;
        } else {
            stack.push_back({n.left, cur.second + "0"});
            stack.push_back({n.right, cur.second + "1"});
        }
    }
}

}  // namespace

bool compressFile(const std::string& inPath, const std::string& outPath, std::string& error) {
    std::ifstream in(inPath, std::ios::binary);
    if (!in) { error = "Khong mo duoc file dau vao: " + inPath; return false; }
    // fileFormat.md muc 6: xu ly toan bo file mot lan
    std::vector<unsigned char> data((std::istreambuf_iterator<char>(in)),
                                    std::istreambuf_iterator<char>());

    std::array<uint64_t, 256> freq{};
    for (unsigned char c : data) ++freq[c];

    std::array<std::string, 256> codes;
    buildCodes(freq, codes);

    std::ofstream out(outPath, std::ios::binary);
    if (!out) { error = "Khong tao duoc file dau ra: " + outPath; return false; }

    // Header: magic, uniqueCount, originalLength, bang tan suat (symbol tang dan)
    uint16_t unique = 0;
    for (int s = 0; s < 256; ++s) if (freq[s] > 0) ++unique;

    out.write("HUF1", 4);
    writeLE(out, unique, 2);
    writeLE(out, data.size(), 8);
    for (int s = 0; s < 256; ++s) {
        if (freq[s] > 0) {
            out.put(static_cast<char>(s));
            writeLE(out, freq[s], 8);
        }
    }

    // Du lieu bit
    BitWriter writer(out);
    for (unsigned char c : data) {
        for (char bit : codes[c]) writer.writeBit(bit == '1');
    }
    writer.flush();

    if (!out) { error = "Loi khi ghi file dau ra"; return false; }
    return true;
}