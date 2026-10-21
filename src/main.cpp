#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
using namespace std;

bool countFrequency(const string& fileName, uint64_t freq[256]) {
    for (int i = 0; i < 256; i++)
        freq[i] = 0;

    ifstream file(fileName, ios::binary);
    if (!file.is_open()) {
        cout << "Khong mo duoc file nguon!\n";
        return false;
    }

    char ch;
    while (file.get(ch)) {
        unsigned char byte = static_cast<unsigned char>(ch);
        freq[byte]++;
    }

    return !file.bad();
}

bool writeUint64LE(ofstream& file, uint64_t value) {
    for (int i = 0; i < 8; i++) {
        char byte = static_cast<char>(value & 0xFF);
        file.put(byte);
        value >>= 8;
    }
    return file.good();
}

bool readUint64LE(ifstream& file, uint64_t& value) {
    value = 0;

    for (int i = 0; i < 8; i++) {
        char ch;
        if (!file.get(ch))
            return false;

        uint64_t byte = static_cast<unsigned char>(ch);
        value |= byte << (i * 8);
    }

    return true;
}

bool writeHeader(ofstream& file, const uint64_t freq[256]) {
    const char magic[4] = {'H', 'F', '0', '1'};
    file.write(magic, 4);

    for (int i = 0; i < 256; i++) {
        if (!writeUint64LE(file, freq[i]))
            return false;
    }

    return file.good();
}

bool readHeader(ifstream& file, uint64_t freq[256]) {
    char magic[4];

    if (!file.read(magic, 4))
        return false;

    if (magic[0] != 'H' || magic[1] != 'F' ||
        magic[2] != '0' || magic[3] != '1') {
        cout << "Header khong hop le!\n";
        return false;
    }

    for (int i = 0; i < 256; i++) {
        if (!readUint64LE(file, freq[i])) {
            cout << "Header bi thieu du lieu!\n";
            return false;
        }
    }

    return true;
}

bool writeBinFile(const string& source, const string& destination,
                  const uint64_t freq[256]) {
    if (source == destination) {
        cout << "File nguon va file dich khong duoc trung nhau!\n";
        return false;
    }

    ifstream in(source, ios::binary);
    if (!in.is_open()) {
        cout << "Khong mo duoc file nguon!\n";
        return false;
    }

    ofstream out(destination, ios::binary);
    if (!out.is_open()) {
        cout << "Khong tao duoc file .bin!\n";
        return false;
    }

    if (!writeHeader(out, freq)) {
        cout << "Loi khi ghi header!\n";
        return false;
    }

    char buffer[65536];

    while (true) {
        in.read(buffer, sizeof(buffer));
        streamsize bytesRead = in.gcount();

        if (bytesRead > 0) {
            out.write(buffer, bytesRead);
            if (!out) {
                cout << "Loi khi ghi du lieu!\n";
                return false;
            }
        }

        if (in.eof())
            break;

        if (!in) {
            cout << "Loi khi doc file nguon!\n";
            return false;
        }
    }

    out.close();
    return !out.fail();
}

bool compareFrequency(const uint64_t a[256], const uint64_t b[256]) {
    for (int i = 0; i < 256; i++) {
        if (a[i] != b[i])
            return false;
    }
    return true;
}

bool comparePayload(const string& source, const string& binFile) {
    ifstream in(source, ios::binary);
    ifstream bin(binFile, ios::binary);

    if (!in.is_open() || !bin.is_open())
        return false;

    uint64_t freq[256];

    if (!readHeader(bin, freq))
        return false;

    char a[65536], b[65536];

    while (true) {
        in.read(a, sizeof(a));
        streamsize sizeA = in.gcount();

        bin.read(b, sizeof(b));
        streamsize sizeB = bin.gcount();

        if (sizeA != sizeB)
            return false;

        for (streamsize i = 0; i < sizeA; i++) {
            if (a[i] != b[i])
                return false;
        }

        if (in.eof() && bin.eof())
            return true;

        if (!in || !bin)
            return false;
    }
}

int main() {
    string inputFile, outputFile;

    cout << "Nhap ten file nguon: ";
    getline(cin, inputFile);

    cout << "Nhap ten file .bin dau ra: ";
    getline(cin, outputFile);

    uint64_t freq[256], restoredFreq[256];

    if (!countFrequency(inputFile, freq))
        return 1;

    cout << "\nBang tan suat byte:\n";
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0)
            cout << "Byte " << i << ": " << freq[i] << '\n';
    }

    if (!writeBinFile(inputFile, outputFile, freq)) {
        cout << "Tao file .bin that bai!\n";
        return 1;
    }

    cout << "\nGhi header vao file .bin thanh cong!\n";

    ifstream bin(outputFile, ios::binary);
    if (!bin.is_open()) {
        cout << "Khong mo duoc file .bin!\n";
        return 1;
    }

    if (!readHeader(bin, restoredFreq)) {
        cout << "Doc header that bai!\n";
        return 1;
    }

    cout << "Doc header thanh cong!\n";

    if (compareFrequency(freq, restoredFreq))
        cout << "Kiem tra: Bang tan suat khoi phuc chinh xac.\n";
    else {
        cout << "Kiem tra: Bang tan suat khong khop.\n";
        return 1;
    }

    bin.close();

    if (comparePayload(inputFile, outputFile))
        cout << "Kiem tra: Du lieu goc duoc luu day du.\n";
    else {
        cout << "Kiem tra: Du lieu khong khop!\n";
        return 1;
    }

    return 0;
}
