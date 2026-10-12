# Định dạng file .bin

Mọi số nhiều byte được lưu theo little-endian.

## Header

| Phần | Kích thước | Ý nghĩa |
|---|---|---|
| magic | 4 byte | "HUF1" để nhận diện file |
| uniqueCount | 2 byte (uint16) | Số ký tự khác nhau, từ 0 đến 256 |
| originalLength | 8 byte (uint64) | Số byte của dữ liệu gốc |
| Bảng tần suất | uniqueCount × 5 byte | Mỗi mục: symbol (1 byte) + freq (uint32, 4 byte) |

## Dữ liệu bit

Chuỗi mã Huffman nối liền nhau. Bit cuối cùng được đệm 0 cho đủ byte.
Người giải nén dựa vào originalLength để biết khi nào dừng.

## Trường hợp đặc biệt

- File rỗng: uniqueCount = 0, originalLength = 0, không có dữ liệu bit.
- Chỉ có 1 ký tự khác nhau: mã của ký tự đó là `0` (1 bit mỗi ký tự).
- Khi tần suất bằng nhau, ưu tiên symbol có giá trị nhỏ hơn. Quy ước này phải thống nhất giữa người nén và người giải nén.

## Ví dụ: nén chuỗi "aab"

- Tần suất: a = 2, b = 1
- Header: magic "HUF1", uniqueCount = 2, originalLength = 3
- Bảng tần suất: (0x61, 2), (0x62, 1)
- Mã Huffman: a = 0, b = 1
- Dữ liệu bit: 0 0 1, đệm thành byte 0x20