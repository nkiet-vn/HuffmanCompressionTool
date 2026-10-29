# Kết quả kiểm thử

Sinh ra bởi test `verify_samples` (chạy tự động trong CTest) với định dạng **HUF2**.
Cột **Hash** so sánh SHA-256 của file gốc và file phục hồi sau khi nén + giải nén.
Cột **Mode** là giá trị `mode` trong header file nén.

| File | Goc (byte) | Nen (byte) | Ty le nen | Mode | Hash | Ket qua |
|---|---:|---:|---:|---|---|---|
| ngan | 17 | 34 | -100.00% | STORED | KHOP | PASS |
| dai | 324000 | 181992 | 43.83% | HUFFMAN | KHOP | PASS |
| tieng_viet_utf8 | 68600 | 41033 | 40.19% | HUFFMAN | KHOP | PASS |
| ngau_nhien | 5000000 | 5000017 | -0.0003% | STORED | KHOP | PASS |

## So sánh với HUF1

| Tình huống | HUF1 (trước) | HUF2 (sau) | Ghi chú |
|---|---:|---:|---|
| File 17 byte | 139 byte (**-717%**) | 34 byte (-100%) | Chọn STORED: `17 + 17` byte, không còn phình |
| File 1 byte | 24 byte (-2300%) | 18 byte (-1700%) | Bị chặn trên bởi `gốc + 17` |
| File rỗng | 14 byte | 17 byte | HUFFMAN rỗng cần 19 byte nên STORED (17) thắng |
| 5 MB ngẫu nhiên | 5 002 318 byte (+0.046%) | 5 000 017 byte (+0.0003%) | chỉ còn header cố định 17 byte |

Tỷ lệ nén = `(1 − kích thước nén / kích thước gốc) × 100%`; giá trị âm nghĩa là file nén lớn hơn gốc.
Giá trị âm rất nhỏ được hiển thị với chữ số có nghĩa thay vì bị làm tròn về `0.00%`.

## Ghi chú thiết kế

- **Không bao giờ phình quá `kích thước gốc + 17` byte**: bộ nén thử HUFFMAN ra file tạm, nếu
  `kích thước HUFFMAN >= 17 + kích thước gốc` thì ghi lại theo mode STORED.
- **Checksum**: mỗi file HUF2 mang CRC32 của dữ liệu gốc. Bộ giải nén tính lại và từ chối nếu lệch.
- **Bảng tần suất gọn hơn**: `freq` dùng LEB128 (thường 1–3 byte thay vì 8). Với file "dài",
  header bảng giảm đáng kể so với HUF1 (`9 × uniqueCount` byte).
- **Bộ nhớ**: mọi thao tác đọc/ghi dùng khối 64 KB, không đọc cả file vào RAM (kể cả khi CRC).

## Kiểm thử bổ sung

| Test | Nội dung |
|---|---|
| `huf2` | Round-trip từng byte (rỗng, 1 byte, 17 byte, 1 MB toàn 0, 5 MB ngẫu nhiên, 256 byte, tiếng Việt); giới hạn `nén <= gốc + 17`; sửa 1 bit → lỗi CRC; sai magic / cắt file / byte thừa / mode sai; không để file đích/tạm dở dang; file HUF1 cũ; fuzz 500 lần |
| `roundtrip` | Nén rồi giải nén, so sánh từng byte; từ chối header hỏng của cả HUF1 và HUF2; đường dẫn Unicode |
| `compress` | Khớp đặc tả HUF2 (ví dụ `"aab"` ở mode STORED), bố cục file HUFFMAN, file rỗng, file nhỏ không phình |
| `menu` | ConsoleUI + luồng menu nén/giải nén/xác minh/thông tin file |
| `minheap`, `huffmanCore` | Min-Heap tự viết (khóa `(freq, minSymbol)`), cây 1 ký tự, bảng mã prefix-free |
| `output_guard` | CRC sai ở HUFFMAN/STORED và dữ liệu HUFFMAN bị cắt không để lại file đích/file tạm |

### Kết quả fuzz

500 lần đột biến ngẫu nhiên (lật bit, cắt file, thêm byte, ghi đè đoạn) trên file nén hợp lệ của
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
