# HuffmanCompressionTool

Ứng dụng nén và giải nén tệp tin bằng thuật toán Huffman, viết bằng C++17.

## Cấu trúc thư mục

```
HuffmanCompressionTool/
├── CMakeLists.txt
├── README.md
├── docs/
│   └── fileFormat.md     # Định dạng file .bin (nguồn chuẩn)
├── include/
│   ├── bitWriter.h       # Ghi bit ra stream
│   └── bitReader.h       # Đọc bit từ stream
├── src/
│   └── main.cpp          # Điểm vào chương trình
└── tests/                # Unit test (sẽ bổ sung)
```

## Cách build

Yêu cầu: CMake 3.16 trở lên và một trình biên dịch C++17 (g++, clang++ hoặc MSVC).

```bash
cmake -S . -B build
cmake --build build
```

File chạy được tạo ra trong thư mục `build/`.

## Định dạng file

Chi tiết định dạng `.bin`, quy tắc xây dựng cây Huffman và quy tắc kiểm tra khi giải nén nằm trong [docs/fileFormat.md](docs/fileFormat.md). Đây là nguồn chuẩn, mọi thay đổi định dạng phải cập nhật tài liệu này trước.

## Quy trình Git

- `main`: nhánh ổn định, chỉ nhận code từ `develop`.
- `develop`: nhánh tích hợp, các nhánh feature merge vào đây.
- `feature/<tênChức năng>`: nhánh làm việc cho từng công việc, ví dụ `feature/designFileFormat`.

Quy trình làm việc:
1. Tạo nhánh feature từ `develop`.
2. Commit và push lên nhánh feature.
3. Mở Pull Request vào `develop`, nhờ ít nhất một thành viên review.
4. Sau khi được approve, merge vào `develop`.

Không commit trực tiếp lên `main` hoặc `develop`.

## Quy ước commit

Tiêu đề commit bắt đầu bằng mã ticket Jira, theo mẫu `DSA-<số>: <mô tả ngắn>`, ví dụ `DSA-1: add README`.

## Quy ước đặt tên

- Biến, hàm, tên file: **camelCase**, ví dụ `writeBit`, `freqTable`, `bitWriter.h`.
- Lớp và struct: **PascalCase**, ví dụ `BitWriter`, `BitReader`.
- Hằng số: viết hoa có gạch dưới, ví dụ `MAGIC_NUMBER`.

## Phạm vi hiện tại

Phiên bản này hỗ trợ nén và giải nén toàn bộ file trong một lần xử lý. Chưa có kiểm tra toàn vẹn (checksum) và chưa xử lý theo khối. Các nội dung này được ghi nhận là việc sau này, xem mục "Ngoài phạm vi hiện tại" trong `docs/fileFormat.md`.