#include <iostream>
#include <string>
#include "compressor.h"
#include "decompressor.h"

using namespace std;

int main(int argc, char* argv[]) {
    // Dùng dòng lệnh: huffman -c <nguon> <dich.bin>  |  huffman -d <nen.bin> <dich>
    if (argc >= 4) {
        string mode = argv[1];
        string inputPath = argv[2];
        string outputPath = argv[3];

        if (mode == "-c") {
            cout << "Dang nen: " << inputPath << " -> " << outputPath << endl;
            if (Compressor::compress(inputPath, outputPath)) {
                cout << "Nen thanh cong!\n";
                return 0;
            }
            cerr << "Nen that bai!\n";
            return 1;
        }
        if (mode == "-d") {
            cout << "Dang giai nen: " << inputPath << " -> " << outputPath << endl;
            if (Decompressor::decompress(inputPath, outputPath)) {
                cout << "Giai nen thanh cong!\n";
                return 0;
            }
            cerr << "Giai nen that bai!\n";
            return 1;
        }
    }

    // Chế độ tương tác
    string inputFile, outputFile;
    cout << "Nhap ten file nguon: ";
    getline(cin, inputFile);
    cout << "Nhap ten file .bin dau ra: ";
    getline(cin, outputFile);

    if (!Compressor::compress(inputFile, outputFile)) {
        cout << "Tao file .bin that bai!\n";
        return 1;
    }
    cout << "Nen thanh cong!\n";
    return 0;
}
