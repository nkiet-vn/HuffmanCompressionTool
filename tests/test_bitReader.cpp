#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>
#include "bitReader.h"

// Tạo file nhị phân mẫu để giả lập kết quả do BitWriter ghi ra
void createDummyFile(const std::string& filename) {
    std::ofstream out(filename, std::ios::binary);
    unsigned char byteData = 0xB2; // Binary: 10110010
    out.write(reinterpret_cast<char*>(&byteData), 1);
    out.close();
}

int main() {
    std::string testFile = "test_temp.bin";
    createDummyFile(testFile);

    // Dãy bit kì vọng tương ứng với 0xB2 (1, 0, 1, 1, 0, 0, 1, 0)
    std::vector<int> expectedBits = {1, 0, 1, 1, 0, 0, 1, 0};

    std::cout << "=== Kiem tra BitReader (DoD Test) ===" << std::endl;
    
    try {
        BitReader reader(testFile);
        
        for (size_t i = 0; i < expectedBits.size(); ++i) {
            assert(!reader.isEnd());
            int bit = reader.readBit();
            std::cout << "Bit " << i << ": Expected = " << expectedBits[i] << " | Got = " << bit << std::endl;
            assert(bit == expectedBits[i]);
        }

        std::cout << "\n--> PASS: BitReader doc chinh xac chuoi bit cua BitWriter!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Loi: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
