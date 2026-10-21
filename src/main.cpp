#include <iostream>
<<<<<<< HEAD

int main() {
    std::cout << "HuffmanCompressionTool\n";
=======
#include <fstream>
#include <string>
#include <cstdint>
#include "codec.h"
using namespace std;

bool countFrequency(const string& fileName, uint64_t freq[256]) {
    for (int i = 0; i < 256; i++) {
        freq[i] = 0;
    }

    ifstream file(fileName, ios::binary);

    if (!file.is_open()) {
        cout << "Khong mo duoc file: " << fileName << endl;
        return false;
    }

    char ch;

    while (file.get(ch)) {
        unsigned char byte = static_cast<unsigned char>(ch);
        freq[byte]++;
    }

    if (file.bad()) {
        cout << "Loi khi doc file!" << endl;
        return false;
    }

    return true;
}

bool copyFile(const string& source, const string& destination) {
    if (source == destination) {
        cout << "File nguon va file dich khong duoc trung nhau!" << endl;
        return false;
    }

    ifstream in(source, ios::binary);

    if (!in.is_open()) {
        cout << "Khong mo duoc file nguon!" << endl;
        return false;
    }

    ofstream out(destination, ios::binary);

    if (!out.is_open()) {
        cout << "Khong tao duoc file dich!" << endl;
        return false;
    }

    char buffer[65536];

    while (true) {
        in.read(buffer, sizeof(buffer));
        streamsize bytesRead = in.gcount();

        if (bytesRead > 0) {
            out.write(buffer, bytesRead);

            if (!out) {
                cout << "Loi khi ghi file!" << endl;
                return false;
            }
        }

        if (in.eof()) {
            break;
        }

        if (!in) {
            cout << "Loi khi doc file!" << endl;
            return false;
        }
    }

    out.close();

    if (out.fail()) {
        cout << "Loi khi dong file dich!" << endl;
        return false;
    }

    return true;
}

bool compareFiles(const string& file1, const string& file2) {
    ifstream a(file1, ios::binary);
    ifstream b(file2, ios::binary);

    if (!a.is_open() || !b.is_open()) {
        cout << "Khong mo duoc file de so sanh!" << endl;
        return false;
    }

    char bufferA[65536];
    char bufferB[65536];

    while (true) {
        a.read(bufferA, sizeof(bufferA));
        streamsize sizeA = a.gcount();

        b.read(bufferB, sizeof(bufferB));
        streamsize sizeB = b.gcount();

        if (a.bad() || b.bad()) {
            cout << "Loi khi doc file de so sanh!" << endl;
            return false;
        }

        if (sizeA != sizeB) {
            return false;
        }

        for (streamsize i = 0; i < sizeA; i++) {
            if (bufferA[i] != bufferB[i]) {
                return false;
            }
        }

        if (a.eof() && b.eof()) {
            return true;
        }
    }
}

int main() {
    string inputFile;
    string outputFile;

    cout << "Nhap ten file nguon: ";
    getline(cin, inputFile);

    cout << "Nhap ten file sao chep: ";
    getline(cin, outputFile);

    uint64_t freq[256];
    
    if (argc == 5 && std::string(argv[1]) == "c" && std::string(argv[3]) == "-o") {
    std::string err;
    if (!compressFile(argv[2], argv[4], err)) { std::cerr << "Loi: " << err << "\n"; return 1; }
    std::cout << "Nen thanh cong: " << argv[4] << "\n";
    return 0;
}
    if (!countFrequency(inputFile, freq)) {
        return 1;
    }

    cout << "\nBang tan suat byte:\n";

    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            cout << "Byte " << i << ": " << freq[i] << endl;
        }
    }

    if (!copyFile(inputFile, outputFile)) {
        cout << "Sao chep file that bai!" << endl;
        return 1;
    }

    cout << "\nSao chep file thanh cong!" << endl;

    if (compareFiles(inputFile, outputFile)) {
        cout << "Kiem tra: Hai file giong nhau." << endl;
    } else {
        cout << "Kiem tra: Hai file khong giong nhau hoac co loi." << endl;
        return 1;
    }

>>>>>>> ee848b6 (DSA-70: add compress txt to bin)
    return 0;
}