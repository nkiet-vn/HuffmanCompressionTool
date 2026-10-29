# Kết quả kiểm thử

Sinh ra bởi test `verify_samples` (chạy tự động trong CTest) với định dạng ghi hiện tại **HUF3**.
Cột **Hash** so sánh SHA-256 của file gốc và file phục hồi sau khi nén + giải nén.
Cột **Mode** là giá trị `mode` trong header file nén.

| File | Goc (byte) | Nen HUF3 (byte) | Ty le nen | Mode | Hash | Ket qua |
|---|---:|---:|---:|---|---|---|
| ngan | 17 | 34 | -100.00% | STORED | KHOP | PASS |
| dai | 324000 | 181956 | 43.84% | HUFFMAN | KHOP | PASS |
| tieng_viet_utf8 | 68600 | 40970 | 40.28% | HUFFMAN | KHOP | PASS |
| ngau_nhien | 5000000 | 5000017 | -0.0003% | STORED | KHOP | PASS |

## So sánh với HUF1

| Tình huống | HUF1 (cũ) | HUF2 | HUF3 (hiện tại) |
|---|---:|---:|---:|
| File 17 byte | 139 byte | 34 byte | 34 byte (STORED) |
| File 1 byte | 24 byte | 18 byte | 18 byte (STORED) |
| File rỗng | 14 byte | 17 byte | 17 byte (STORED) |
| 5 MB ngẫu nhiên | 5 002 318 byte | 5 000 017 byte | 5 000 017 byte (STORED) |

## So sánh kích thước header HUFFMAN

So sánh kích thước header ứng viên (17 byte cố định + 2 byte `uniqueCount` + bảng), tính từ
tần suất thật của ba file mẫu. File ngẫu nhiên cuối cùng chọn STORED ở cả hai định dạng nên header
thực tế của nó vẫn là 17 byte; số dưới đây là chi phí nếu mã hóa HUFFMAN.

| File mẫu | Header HUF2-HUFFMAN (byte) | Header HUF3-HUFFMAN (byte) | Giảm (byte) |
|---|---:|---:|---:|
| dai | 117 | 81 | 36 |
| tieng_viet_utf8 | 208 | 145 | 63 |
| ngau_nhien | 1043 | 531 | 512 |

Tỷ lệ nén = `(1 − kích thước nén / kích thước gốc) × 100%`; giá trị âm nghĩa là file nén lớn hơn gốc.
Giá trị âm rất nhỏ được hiển thị với chữ số có nghĩa thay vì bị làm tròn về `0.00%`.

## Ghi chú thiết kế

- **Không bao giờ phình quá `kích thước gốc + 17` byte**: bộ nén thử HUFFMAN ra file tạm, nếu
  `kích thước HUFFMAN >= 17 + kích thước gốc` thì ghi lại theo mode STORED.
- **Checksum**: HUF2/HUF3 mang CRC32 của dữ liệu gốc. Bộ giải nén tính lại và từ chối nếu lệch.
- **Bảng canonical**: HUF3 chỉ lưu `symbol` và `codeLength`, mỗi symbol 2 byte thay vì tần suất
  LEB128 ở HUF2 hoặc tần suất 8 byte ở HUF1.
- **Bộ nhớ**: mọi thao tác đọc/ghi dùng khối 64 KB, không đọc cả file vào RAM (kể cả khi CRC).
- **CLI**: `-i` và mục `[4]` dùng chung `MenuController::showFileInfo`; thông báo được tập trung ở
  `include/messages.h`, và tiến trình chỉ xuất hiện với tệp lớn hơn 10 MiB khi stdout là terminal.

## Kiểm thử bổ sung

| Test | Nội dung |
|---|---|
| `huf2` | HUF3 round-trip theo dữ liệu byte, giới hạn `nén <= gốc + 17`, CRC/header hỏng/Kraft, tương thích HUF2/HUF1 và fuzz 500 lần |
| `canonicalHuffman` | Gán mã theo canonical, chấp nhận Kraft hợp lệ, từ chối Kraft quá tải, kiểm tra độ dài 32/33 bit và cây vượt giới hạn |
| `roundtrip` | Nén rồi giải nén, so sánh từng byte; từ chối header hỏng của HUF1/HUF2; đường dẫn Unicode |
| `compress` | Khớp đặc tả HUF3 (ví dụ `"aab"` ở mode STORED), bố cục canonical HUFFMAN, file rỗng, file nhỏ không phình |
| `menu` | ConsoleUI UTF-8/ASCII, căn bảng theo code point; thông tin HUF3/HUF2 ở cả hai mode, HUF1, file thường và lỗi thiếu file; codec không ghi trực tiếp ra stderr |
| `cli_info` | Lệnh `-i` hiển thị thông tin file thường và trả về thành công |
| `cli_ascii_info` | `--ascii -i` dùng đúng luồng thông tin, hiển thị không dấu |
| `cli_info_missing` | `-i` trả exit code khác 0 khi file không tồn tại |
| `cli_info_bad_args` | Thiếu đường dẫn sau `-i` trả exit code khác 0 |
| `testT10` | Mock `IdentityCodec` đi qua `ICompressor`/`IDecompressor`, verifier gọi cả nén và giải nén |
| `minheap`, `huffmanCore` | Min-Heap tự viết (khóa `(freq, minSymbol)`), cây 1 ký tự, bảng mã prefix-free |
| `output_guard` | CRC sai ở HUFFMAN/STORED và dữ liệu HUFFMAN bị cắt không để lại file đích/file tạm |

CI cấu hình và build riêng Debug/Release trên Ubuntu, Windows và macOS; sau Release, CLI nén rồi giải
nén mẫu `file_mau_txt/mau_giainen.txt` và so sánh byte-for-byte (`cmp` hoặc `fc /b`). Bước kiểm tra
hoa thường tên include chạy trên Ubuntu trước khi build.

### Kết quả fuzz

500 lần đột biến ngẫu nhiên (lật bit, cắt file, thêm byte, ghi đè đoạn) trên file HUF3 hợp lệ của
4 loại dữ liệu (STORED, HUFFMAN ×2, rỗng): **0 lần chấp nhận dữ liệu hỏng**, không crash, không treo.

## Hiệu năng giải nén

Đo Release trên cùng tệp văn bản tiếng Việt 25 999 944 byte (~26 MB), 3 lần mỗi phiên bản,
lấy trung vị. Cấu hình: Intel Core i5-6440HQ @ 2.60 GHz, Windows 10 Home build 19045,
MinGW-w64 GCC 16.2.0 (MSYS2 UCRT64). SHA-256 xác nhận file giải nén trùng file gốc.

| Phiên bản | Giải nén (trung vị) |
|---|---:|
| Trước refactor | 2.931 s |
| Sau refactor | 1.390 s |

Sau refactor nhanh hơn khoảng 52.6% trên mẫu đo này.
