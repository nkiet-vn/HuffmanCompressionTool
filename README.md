# HuffmanCompressionTool

Ứng dụng nén và giải nén tệp tin bằng thuật toán Huffman, viết bằng C++17.

## Cấu trúc thư mục

```
HuffmanCompressionTool/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── .gitignore
├── .github/workflows/ci.yml    # CI build + test trên Linux/macOS/Windows
├── docs/
│   ├── fileFormat.md           # Định dạng file .bin (nguồn chuẩn)
│   └── ketQuaKiemThu.md        # Kết quả kiểm thử
├── include/
│   ├── bitWriter.h             # Ghi bit ra stream
│   ├── bitReader.h             # Đọc bit từ stream (có hỗ trợ offset)
│   ├── crc32.h                 # CRC-32 IEEE 802.3 (bảng tra 256 phần tử)
│   ├── fileFormat.h            # Hằng số HUF2 + LEB128 + little-endian dùng chung
│   ├── huffmanTree.h           # Cây Huffman, HuffmanNode (kiểu nút duy nhất) và bảng mã
│   ├── minHeap.h               # Min-Heap tự viết, chứa HuffmanNode (khóa: freq, minSymbol)
│   ├── compressor.h            # Class nén
│   ├── decompressor.h          # Class giải nén
│   ├── compressionStats.h      # Thống kê tỷ lệ nén
│   ├── compressionVerifier.h   # Xác minh round-trip
│   ├── fileHasher.h            # Tính hash SHA-256 của file
│   ├── sha256.h                # Thuật toán SHA-256
│   ├── iCodec.h                # Interface nén/giải nén
│   ├── consoleUI.h             # Tầng hiển thị CLI (banner, menu, bảng, prompt)
│   └── menuController.h        # Tầng điều phối menu (không đụng cin/cout)
├── src/
│   ├── main.cpp                # Điểm vào chương trình (chỉ parse tham số)
│   ├── consoleUI.cpp           # Triển khai tầng hiển thị
│   ├── menuController.cpp      # Triển khai vòng lặp menu + nghiệp vụ
│   ├── huffmanTree.cpp         # Xây dựng cây Huffman bằng MinHeap tự viết
│   ├── compressor.cpp          # Logic nén (ghi qua file tạm rồi đổi tên)
│   ├── decompressor.cpp        # Logic giải nén (dùng BitReader)
│   ├── bitReader.cpp           # Đọc bit từ file
│   ├── crc32.cpp               # Cài đặt CRC-32 (tự viết, không thư viện ngoài)
│   ├── fileFormat.cpp          # Tiện ích định dạng (LEB128, little-endian, nhận diện magic)
│   ├── compressionStats.cpp    # Tính toán thống kê
│   ├── compressionVerifier.cpp # Xác minh toàn vẹn
│   ├── fileHasher.cpp          # Hash file
│   └── sha256.cpp              # Triển khai SHA-256
└── tests/
    ├── test_huffmanTree.cpp
    ├── test_bitReader.cpp
    ├── test_bitWriter.cpp
    ├── test_minheap.cpp
    ├── test_menu.cpp
    ├── test_roundtrip.cpp
    ├── test_compress.cpp
    ├── test_huffmancore.cpp
    ├── testT10.cpp
    ├── test_huf2.cpp           # HUF2: round-trip, giới hạn kích thước, CRC, HUF1 cũ, fuzz
    └── verify_samples.cpp
```

### Kiến trúc tầng giao diện

```
main.cpp          -> chỉ phân tích tham số dòng lệnh, chọn chế độ
  |
  +-- MenuController  -> vòng lặp menu, điều phối nghiệp vụ
        |
        +-- ConsoleUI  -> toàn bộ cin/cout, định dạng bảng, prompt, thông báo
        +-- Compressor / Decompressor / FileHasher / CompressionStats
```

- `ConsoleUI` là **tầng trình bày thuần** (không biết gì về nén): nhận `istream&`/`ostream&`
  qua constructor nên test được bằng `std::istringstream`/`std::ostringstream`.
- `MenuController` là **tầng điều khiển**: chỉ gọi `ui_` và các service, không dùng `cin`/`cout` trực tiếp.
- CLI (`-c`, `-d`, `-v`) và menu tương tác dùng **chung** `MenuController`, nên thông báo/kết quả
  giống hệt nhau ở cả hai đường vào.

## Cách build

Yêu cầu: CMake 3.16 trở lên và một trình biên dịch C++17 (g++, clang++ hoặc MSVC).
Build type mặc định là `Release`; cờ cảnh báo `-Wall -Wextra -Wpedantic` (`/W4` với MSVC) được bật sẵn.

```bash
cmake -S . -B build
cmake --build build
```

File chạy được tạo trong `build/` (Windows: `build/Release/`).

Chạy toàn bộ test:

```bash
ctest --test-dir build -C Release --output-on-failure
```

## Cách sử dụng

### Menu tương tác (khuyến nghị cho người dùng cuối)

```bash
huffman            # chạy không tham số
huffman --menu     # hoặc -m
```

```
==============================================================
  Huffman Compression Tool  v1.0.0
  Cong cu nen va giai nen file bang thuat toan Huffman
==============================================================
   [1] Nen file                                nen file bat ky thanh dinh dang HUF2
   [2] Giai nen file                           khoi phuc file tu .bin
   [3] Nen va xac minh SHA-256                 nen + giai nen lai + so sanh hash
   [4] Xem thong tin file                      kich thuoc, dinh dang HUF2
   [5] Huong dan / tro giup                    cach dung dong lenh
   [6] Thong tin phan mem                      phien ban, cau truc du lieu chinh
   [0] Thoat
--------------------------------------------------------------
Chon chuc nang [0-6]:
```

Đặc điểm của menu:

- Chọn `0` để thoát; lựa chọn sai được báo lỗi và vẽ lại menu thay vì thoát chương trình.
- Nhập đường dẫn file nguồn không tồn tại → báo lỗi và quay lại menu.
- File đích đã tồn tại → hỏi xác nhận ghi đè (`[y/N]`).
- Kết quả in dạng bảng có căn lề: kích thước (B/KB/MB), tỷ lệ nén, SHA-256.
- Mục `[4]` nhận diện file HUF2, HUF1 cũ hay file thường; mục `[5]`/`[6]` in hướng dẫn và thông tin phần mềm.

### Dòng lệnh

```bash
# Nén
huffman -c <file_nguon> <file_nen.bin>

# Giải nén
huffman -d <file_nen.bin> <file_dich>

# Nén + xác minh khôi phục 100% bằng SHA-256
huffman -v <file_nguon> <file_nen.bin>

# Trợ giúp
huffman --help
```

Sau khi nén, chương trình in kích thước gốc, kích thước nén và tỷ lệ nén (%).
Chế độ `-v` giải nén ra file tạm và so sánh SHA-256 với file gốc.
Tham số sai (chế độ không hợp lệ, thiếu đối số) luôn báo lỗi và trả exit code 1.

## Tính năng

- **Menu console** với kiến trúc tách tầng (`ConsoleUI` + `MenuController`), test được bằng
  `istringstream`/`ostringstream` mà không cần chạy chương trình thật.
- **Min-Heap tự viết** (`include/minHeap.h`) là cấu trúc dữ liệu chính để dựng cây Huffman,
  so sánh theo khóa `(freq, minSymbol)` đúng đặc tả trong `docs/fileFormat.md`.
- **BitWriter/BitReader** đọc ghi bit MSB-first; bộ giải nén dùng `BitReader` (không còn lambda trùng lặp).
- **Tự chọn HUFFMAN / STORED**: file nén **không bao giờ lớn hơn gốc quá 17 byte** (header),
  nên file nhỏ hay dữ liệu ngẫu nhiên không còn bị phình.
- **CRC32 trong file** (`src/crc32.cpp`, bảng tra 256 phần tử): bộ giải nén tự kiểm tra dữ liệu gốc,
  sửa 1 bit là phát hiện được; đồng thời vẫn xác minh bằng SHA-256 (`-v`).
- **Bảng tần suất nén bằng LEB128** thay vì 9 byte/ký tự.
- **Tương thích ngược**: vẫn giải nén được file HUF1 cũ.
- **Thống kê tỷ lệ nén** hiển thị ngay trên CLI.
- **An toàn dữ liệu**: từ chối nén khi nguồn đích trùng nhau (kiểm tra bằng
  `std::filesystem::equivalent`, kể cả khi đường dẫn viết khác nhau); ghi ra file tạm rồi đổi tên;
  từ chối giải nén đè lên chính file nén, bảo vệ nguồn và file tạm đã tồn tại;
  xóa file đích dở dang khi giải nén lỗi.
- **Kiểm tra chặt khi giải nén**: đối chiếu số bit cần thiết với số bit có sẵn, từ chối dữ liệu
  thừa ở cuối file và dữ liệu bị cắt.

## Định dạng file

Chi tiết định dạng `.bin`, quy tắc xây dựng cây Huffman và quy tắc kiểm tra khi giải nén nằm trong
[docs/fileFormat.md](docs/fileFormat.md). Đây là nguồn chuẩn, mọi thay đổi định dạng phải cập nhật tài liệu này trước.

Tóm tắt:

| Phần | Kích thước |
|---|---|
| magic `"HUF2"` | 4 byte |
| mode (0 = HUFFMAN, 1 = STORED) | 1 byte |
| originalLength | 8 byte (uint64 LE) |
| crc32 của dữ liệu gốc | 4 byte |
| *(chỉ HUFFMAN)* uniqueCount | 2 byte |
| *(chỉ HUFFMAN)* bảng tần suất (symbol + freq LEB128) | ~2–3 byte/symbol |
| Dữ liệu | HUFFMAN: bit MSB-first + đệm 0; STORED: byte gốc |

Header cố định 17 byte. Từ HUF1 cũ: header 14 byte, bảng 9 byte/symbol, không checksum, không có
mode STORED — vẫn đọc được nhưng không ghi nữa.

## Kiểm thử

| Test | Nội dung |
|---|---|
| `huffmanTree` | Dựng cây Huffman cơ bản |
| `minheap` | Min-Heap tự viết: thứ tự pop, tie-break `minSymbol`, lỗi biên |
| `bitReader` | Đọc bit MSB-first |
| `bitWriter` | Ghi bit MSB-first |
| `huffmanCore` | Thứ tự heap, tie-break, prefix-free, cây 1 ký tự |
| `roundtrip` | Nén rồi giải nén, so sánh từng byte; từ chối header hỏng HUF1/HUF2 |
| `compress` | Khớp đặc tả HUF2 (ví dụ `"aab"`), bố cục HUFFMAN, file rỗng, file nhỏ không phình |
| `huf2` | Round-trip 7 loại file, giới hạn `nén <= gốc + 17`, lỗi CRC khi sửa bit, header hỏng, HUF1 cũ, bảo vệ file nguồn/đích, fuzz 500 lần |
| `menu` | ConsoleUI (trim, EOF, bảng, tag thông báo) + luồng menu nén/giải nén/xác minh/thông tin |
| `testT10` | SHA-256, thống kê, xác minh |
| `verify_samples` | Kiểm thử tổng hợp với file mẫu |

Các test dùng `assert` đều có `#undef NDEBUG` nên vẫn kiểm tra thật khi build Release.
Test chạy trong thư mục build nên không để lại rác ở thư mục gốc.

Xem kết quả kiểm thử chi tiết trong [docs/ketQuaKiemThu.md](docs/ketQuaKiemThu.md).

## Quy trình Git

- `main`: nhánh ổn định, chỉ nhận code từ `develop`.
- `develop`: nhánh tích hợp, các nhánh feature merge vào đây.
- `feature/<tênChứcNăng>`: nhánh làm việc cho từng công việc, ví dụ `feature/designFileFormat`.

Quy trình làm việc:
1. Tạo nhánh feature từ `develop`.
2. Commit và push lên nhánh feature.
3. Mở Pull Request vào `develop`, nhờ ít nhất một thành viên review.
4. Sau khi được approve, merge vào `develop`.

Không commit trực tiếp lên `main` hoặc `develop`. CI (`.github/workflows/ci.yml`) chạy build và test
trên cả ba hệ điều hành ở cả Debug và Release.

## Quy ước commit

Tiêu đề commit bắt đầu bằng mã ticket Jira, theo mẫu `DSA-<số>: <mô tả ngắn>`, ví dụ `DSA-1: add README`.

## Quy ước đặt tên

- Biến, hàm, tên file: **camelCase**, ví dụ `writeBit`, `freqTable`, `bitWriter.h`, `minHeap.h`, `sha256.h`.
- Lớp và struct: **PascalCase**, ví dụ `BitWriter`, `BitReader`, `MinHeap`.
- Hằng số: viết hoa có gạch dưới, ví dụ `MAGIC_NUMBER`.

## Hiệu năng

- File văn bản 100 MB: nén ~1.7 s, giải nén ~4.8 s
- File ngẫu nhiên 100 MB: nén ~6.1 s, giải nén ~10.7 s

Giải nén đọc phần header vào RAM rồi dùng `BitReader` đọc phần dữ liệu bit. Cả nén và giải nén xử lý
theo khối 64 KB (`IO_CHUNK_SIZE`) và tính CRC32 trên luồng dữ liệu, nên bộ nhớ không phụ thuộc kích thước file.

## Hạn chế và hướng cải tiến

- File nén luôn tốn thêm header cố định 17 byte, nên file cực ngắn (1–17 byte) vẫn lớn hơn gốc đúng
  chừng đó — nhưng không bao giờ phình quá `gốc + 17`, khác hẳn HUF1 trước đây (file 17 byte ra 139 byte).
  Hướng cải tiến: Canonical Huffman để chỉ lưu độ dài mã.
- Chưa hỗ trợ nén theo khối (streaming): nén đọc file nguồn 2 lượt (lượt 1 đếm tần suất + CRC,
  lượt 2 mã hóa).
