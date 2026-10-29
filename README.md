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
│   ├── fileFormat.h            # Hằng số HUF1/HUF2/HUF3 + LEB128 + little-endian
│   ├── canonicalHuffman.h      # Tính độ dài cây và gán mã canonical cho HUF3
│   ├── huffmanTree.h           # Cây Huffman, HuffmanNode (kiểu nút duy nhất) và bảng mã
│   ├── minHeap.h               # Min-Heap tự viết, chứa HuffmanNode (khóa: freq, minSymbol)
│   ├── compressor.h            # Class nén
│   ├── decompressor.h          # Class giải nén
│   ├── compressionStats.h      # Thống kê tỷ lệ nén
│   ├── compressionVerifier.h   # Xác minh round-trip
│   ├── fileHasher.h            # Tính hash SHA-256 của file
│   ├── sha256.h                # Thuật toán SHA-256
│   ├── messages.h              # Chuỗi hiển thị tiếng Việt UTF-8 / ASCII
│   ├── iCodec.h                # Interface đa hình nén/giải nén
│   ├── consoleUI.h             # Tầng hiển thị CLI (banner, menu, bảng, prompt)
│   ├── menuController.h        # Tầng điều phối menu (không đụng cin/cout)
│   └── pathUtil.h              # chuyển đường dẫn UTF-8 sang kiểu hệ điều hành (toPath())
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
├── file_mau_txt/               # Tệp mẫu dùng cho kiểm thử thủ công / verify_samples
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
    ├── test_huf2.cpp           # HUF3 round-trip/fuzz, tương thích HUF2/HUF1
    ├── test_canonicalHuffman.cpp # Kraft và giới hạn độ dài mã canonical
    ├── test_output_guard.cpp   # Xóa file đích dở dang khi CRC sai / file bị cắt
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
- CLI (`-c`, `-d`, `-v`, `-i`) và menu tương tác dùng **chung** `MenuController`, nên thông báo/kết quả
  giống hệt nhau ở cả hai đường vào.

## Cách build

Yêu cầu: CMake 3.16 trở lên và một trình biên dịch C++17 (g++, clang++ hoặc MSVC).
Build type mặc định là `Release`; cờ cảnh báo `-Wall -Wextra -Wpedantic` (`/W4` với MSVC) được bật sẵn.

```bash
cmake -S . -B build
cmake --build build
```

File chạy được tạo trong `build/` (Windows: `build/Release/`).
Mặc định CMake cũng build test; dùng `-DHUFFMAN_BUILD_TESTS=OFF` để chỉ build ứng dụng.
Sau khi build, `cmake --install build --prefix <thư_mục_cài_đặt>` cài executable vào `bin/`.

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
  Công cụ nén và giải nén tệp bằng thuật toán Huffman
==============================================================
   [1] Nén tệp                                 nén tệp thành định dạng HUF3
   [2] Giải nén tệp                            khôi phục tệp từ .bin
   [3] Nén và xác minh SHA-256                 nén + giải nén lại + so sánh hash
   [4] Xem thông tin tệp                       kích thước, định dạng, tỷ lệ, SHA-256
   [5] Hướng dẫn / trợ giúp                    cách dùng dòng lệnh
   [6] Thông tin phần mềm                      phiên bản, cấu trúc dữ liệu chính
   [0] Thoát
--------------------------------------------------------------
Chọn chức năng [0-6]:
```

Đặc điểm của menu:

- Chọn `0` để thoát; lựa chọn sai được báo lỗi và vẽ lại menu thay vì thoát chương trình.
- Nhập đường dẫn file nguồn không tồn tại → báo lỗi và quay lại menu.
- File đích đã tồn tại → hỏi xác nhận ghi đè (`[y/N]`).
- Kết quả in dạng bảng có căn lề: kích thước (B/KB/MB), tỷ lệ nén, SHA-256.
- Mục `[4]` nhận diện HUF3-HUFFMAN/STORED, HUF2, HUF1 hay tệp thường, đồng thời hiển thị
  kích thước gốc, số ký hiệu (nếu có), tỷ lệ nén và SHA-256; mục `[5]`/`[6]` in hướng dẫn và thông tin phần mềm.
- Giao diện mặc định hiển thị tiếng Việt UTF-8; dùng `--ascii` hoặc `HUFFMAN_ASCII=1` để hiển thị
  bản không dấu trên console không hỗ trợ UTF-8.

### Dòng lệnh

```bash
# Nén
huffman -c <file_nguon> <file_nen.bin>

# Giải nén
huffman -d <file_nen.bin> <file_dich>

# Nén + xác minh khôi phục 100% bằng SHA-256
huffman -v <file_nguon> <file_nen.bin>

# Xem thông tin và SHA-256 của tệp
huffman -i <file>

# Trợ giúp
huffman --help

# Dùng giao diện không dấu
huffman --ascii -i <file>
```

Sau khi nén, chương trình in kích thước gốc, kích thước nén và tỷ lệ nén (%).
Chế độ `-v` giải nén ra file tạm và so sánh SHA-256 với file gốc.
Tham số sai (chế độ không hợp lệ, thiếu đối số) luôn báo lỗi và trả exit code 1.
`-i` dùng chung logic hiển thị với mục `[4]`; tệp không tồn tại/lỗi đọc trả exit code 1.
Tác vụ với tệp lớn hơn 10 MiB hiển thị tiến trình phần trăm khi stdout là terminal; khi chuyển
hướng stdout, tiến trình bị tắt để không làm nhiễu pipe và CI.

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
| magic `"HUF3"` | 4 byte |
| mode (0 = HUFFMAN, 1 = STORED) | 1 byte |
| originalLength | 8 byte (uint64 LE) |
| crc32 của dữ liệu gốc | 4 byte |
| *(chỉ HUFFMAN)* uniqueCount | 2 byte |
| *(chỉ HUFFMAN)* bảng độ dài mã canonical (symbol + codeLength) | 2 byte/symbol |
| Dữ liệu | HUFFMAN: bit MSB-first + đệm 0; STORED: byte gốc |

Header cố định 17 byte. HUF3 vẫn ghi được cả mode HUFFMAN và STORED. HUF2 (bảng tần suất LEB128)
và HUF1 (bảng tần suất 8 byte, không checksum) vẫn đọc được để tương thích ngược.

## Kiểm thử

| Test | Nội dung |
|---|---|
| `huffmanTree` | Dựng cây Huffman cơ bản |
| `minheap` | Min-Heap tự viết: thứ tự pop, tie-break `minSymbol`, lỗi biên |
| `bitReader` | Đọc bit MSB-first |
| `bitWriter` | Ghi bit MSB-first |
| `huffmanCore` | Thứ tự heap, tie-break, prefix-free, cây 1 ký tự |
| `roundtrip` | Nén rồi giải nén, so sánh từng byte; từ chối header hỏng HUF1/HUF2 |
| `compress` | Khớp HUF3 (ví dụ `"aab"`), bảng canonical, file rỗng, file nhỏ không phình |
| `huf2` | HUF3 round-trip, giới hạn `nén <= gốc + 17`, CRC, bảng hỏng, tương thích HUF2/HUF1, fuzz 500 lần |
| `canonicalHuffman` | Gán mã theo canonical, Kraft, mã dài tối đa 32 bit |
| `menu` | ConsoleUI (trim, EOF, bảng UTF-8/ASCII, căn cột code point) + thông tin HUF3/HUF2/HUF1/plain, codec không ghi stderr |
| `cli_info`, `cli_ascii_info`, `cli_info_missing`, `cli_info_bad_args` | CLI `-i`, chế độ `--ascii`, tệp không tồn tại và tham số thiếu |
| `testT10` | SHA-256, thống kê, xác minh |
| `verify_samples` | Kiểm thử tổng hợp với file mẫu |
| `output_guard` | CRC sai và dữ liệu Huffman bị cắt không để lại file đích hoặc file tạm |

Các test dùng `assert` đều có `#undef NDEBUG` nên vẫn kiểm tra thật khi build Release.
Test chạy trong thư mục build nên không để lại rác ở thư mục gốc.

Xem kết quả kiểm thử chi tiết trong [docs/ketQuaKiemThu.md](docs/ketQuaKiemThu.md).

Các interface `ICompressor` và `IDecompressor` được dùng đa hình bởi `CompressionVerifier`.
`testT10` truyền `IdentityCodec` giả qua các interface, xác nhận verifier gọi đúng thao tác nén
và giải nén; do đó các interface được giữ lại.

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

Đo bằng bản Release trên Intel Core i5-6440HQ @ 2.60 GHz, Windows 10 Home 22H2
(build 19045), MinGW-w64 GCC 16.2.0 (MSYS2 UCRT64). Mỗi phép đo là thời gian chạy CLI,
thực hiện 3 lần trên cùng tệp và lấy trung vị; SHA-256 xác nhận dữ liệu giải nén khớp tệp gốc.

| Dữ liệu | Kích thước đầu vào | Nén (trung vị) | Giải nén (trung vị) |
|---|---:|---:|---:|
| Văn bản tiếng Việt lặp đoạn | 99 999 915 byte (~100 MB) | 1.961 s | 9.618 s |
| Ngẫu nhiên | 100 000 000 byte (100 MB) | 2.094 s | 0.400 s |

Đo riêng hiệu năng giải nén HUFFMAN trên cùng tệp văn bản tiếng Việt 25 999 944 byte (~26 MB),
3 lần mỗi phiên bản, trung vị: trước refactor **2.931 s**, sau refactor **1.390 s**
(nhanh hơn khoảng 52.6%). Hai lần đo dùng cùng cấu hình Release và xác nhận SHA-256 sau giải nén.

Giải nén đọc phần header vào RAM rồi dùng `BitReader` đọc phần dữ liệu bit. Cả nén và giải nén xử lý
theo khối 64 KB (`IO_CHUNK_SIZE`) và tính CRC32 trên luồng dữ liệu, nên bộ nhớ không phụ thuộc kích thước file.

## Hạn chế và hướng cải tiến

- File nén luôn tốn thêm header cố định 17 byte, nên file cực ngắn (1–17 byte) vẫn lớn hơn gốc đúng
  chừng đó — nhưng không bao giờ phình quá `gốc + 17`, khác hẳn HUF1 trước đây (file 17 byte ra 139 byte).
  Hướng cải tiến: Canonical Huffman để chỉ lưu độ dài mã.
- Chưa hỗ trợ nén theo khối (streaming): nén đọc file nguồn 2 lượt (lượt 1 đếm tần suất + CRC,
  lượt 2 mã hóa).
