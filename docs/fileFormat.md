# Định dạng file .bin (HUF3, có hỗ trợ đọc HUF2/HUF1)

Tài liệu này là nguồn chuẩn cho người nén và người giải nén. Mọi thay đổi phải được cả nhóm thống nhất trước khi sửa.

- **HUF3** là định dạng ghi hiện tại (mục 3.4).
- **HUF2** và **HUF1** là định dạng cũ, chỉ còn được **đọc** để tương thích ngược.

## 1. Quy ước chung

- Mọi số nhiều byte lưu theo **little-endian**.
- Thứ tự bit trong mỗi byte là **MSB-first**: bit được ghi đầu tiên nằm ở vị trí cao nhất (bit 7) của byte.
- Bit đệm (padding) ở cuối dữ liệu luôn là `0`.
- Số nguyên dài (tần suất) dùng **LEB128** (unsigned varint): xem mục 2.
- Tất cả đường dẫn là chuỗi UTF-8, file luôn mở ở chế độ `std::ios::binary`.

## 2. LEB128 (unsigned varint)

Ghi số nguyên không dấu theo từng nhóm 7 bit, **nhóm thấp trước**. Mỗi byte có 1 bit cờ ở vị trí
thấp nhất (bit 0): `1` = còn byte tiếp, `0` = byte cuối. 7 bit còn lại (bit 7..1) là dữ liệu.

Ví dụ:

| Giá trị | Bytes |
|---|---|
| 0 | `00` |
| 1 | `01` |
| 127 | `7F` |
| 128 | `80 01` |
| 300 | `AC 02` |
| 5 000 000 | `C0 91 B1 02` |

Ví dụ `300 = 0b1_0010_1100`: nhóm thấp 7 bit là `010_1100`. Vì còn byte tiếp, gắn cờ `1` vào bit 7
thành `1010 1100` = `0xAC`. Số còn lại là `300 >> 7 = 2` = `0x02`, không còn byte nên cờ `0` → `0x02`.

Trong HUF2, LEB128 chỉ dùng cho tần suất (`freq`), tối đa **10 byte** (`uint64`). Decoder phải từ chối
varint dài quá 10 byte, byte thứ 10 lớn hơn `0x01`, hoặc varint không kết thúc trong phạm vi cho phép.

## 3. Cấu trúc file HUF2

| Phần | Kích thước | Kiểu | Ý nghĩa |
|---|---|---|---|
| magic | 4 byte | ASCII | `"HUF2"`, nhận diện file |
| mode | 1 byte | uint8 | `0` = HUFFMAN, `1` = STORED. Giá trị khác → lỗi |
| originalLength | 8 byte | uint64 | Số byte của dữ liệu gốc |
| crc32 | 4 byte | uint32 | CRC-32 (IEEE 802.3) của dữ liệu **gốc**, tính trên toàn bộ file gốc |
| uniqueCount | 2 byte | uint16 | (chỉ HUFFMAN) Số symbol khác nhau, 0 → 256 |
| Bảng tần suất | thay đổi | | (chỉ HUFFMAN) `uniqueCount` mục: `symbol` (1 byte) + `freq` (LEB128) |
| Dữ liệu | phần còn lại | | HUFFMAN: chuỗi bit nối liền (MSB-first, đệm 0). STORED: byte gốc nguyên vẹn |

Header cố định = `4 + 1 + 8 + 4 = 17` byte (`HEADER_SIZE`). Với HUFFMAN, sau header là
`uniqueCount` (2 byte) rồi bảng tần suất, sau đó mới tới dữ liệu bit.

### 3.1. Mode STORED (mode = 1)

Sau header cố định là **đúng `originalLength` byte** của dữ liệu gốc, không có `uniqueCount`,
không có bảng tần suất, không có bit đệm. Kích thước file STORED = `17 + originalLength`.

### 3.2. Mode HUFFMAN (mode = 0)

- **Quy tắc bảng tần suất** (giống HUF1):
  - Các mục được sắp xếp theo `symbol` tăng dần.
  - Mỗi `symbol` xuất hiện tối đa một lần.
  - Mỗi `freq` phải lớn hơn 0. Symbol không xuất hiện thì không ghi vào bảng.
  - Tổng các `freq` phải bằng `originalLength`.
- **File rỗng**: `originalLength = 0`, `uniqueCount = 0`, không có bảng tần suất, không có dữ liệu.
- **Chỉ có một symbol khác nhau**: bảng có 1 mục, cây chỉ có một lá, mã của symbol là `0`
  (1 bit cho mỗi ký tự).

### 3.3. Vì sao có thiết kế này (lý do thay đổi so với HUF1)

1. **File nhỏ / dữ liệu không nén được bị phình** — được xử lý bằng **mode STORED**: bộ nén mã hóa
   Huffman ra file tạm, nếu kích thước kết quả **>=** kích thước phương án STORED thì dùng STORED.
   Hệ quả: với mọi đầu vào, `kích thước file nén <= kích thước gốc + 17`. File 17 byte chỉ còn 34 byte
   thay vì 139 byte, file rỗng còn 17 byte thay vì 14 byte nhưng **không bao giờ phình**.
2. **Không có checksum** — thêm **CRC32 của dữ liệu gốc** vào header. CRC32 được chọn thay vì
   CRC64/SHA-256 vì: là chuẩn phổ biến, phát hiện tốt lỗi đơn bit và lỗi cụm (xác suất bỏ sót ~2⁻³²),
   chỉ tốn 4 byte header, tự cài đặt bằng bảng tra 256 phần tử rất gọn và nhanh (`O(n)` một phép XOR/byte).
   SHA-256 đã có sẵn ở tầng công cụ (`-v`) nhưng 32 byte header là quá đắt cho file nhỏ.
3. **Bảng tần suất tốn 9 byte/ký tự** — `freq` đổi sang **LEB128**. Thực tế tần suất nhỏ chiếm đa số
   (văn bản thường < 128, < 16 384), nên mỗi `freq` thường chỉ 1–3 byte thay vì 8. Header bảng giảm
   từ `9 × n` xuống còn khoảng `2 × n` byte. Đổi lại là decode bảng phức tạp hơn một chút, nhưng bảng
   vẫn được kiểm tra chặt (mục 7).
4. **Thiếu khả năng tự kiểm tra khi giải nén** — bộ giải nén tính lại CRC32 trên dữ liệu vừa giải mã
   (theo khối 64 KB, không đọc cả file vào RAM) và so với `crc32` trong header. Không khớp → báo lỗi
   và xóa file đích dở dang. Kết hợp với các kiểm tra cấu trúc (mục 7), hầu như mọi hỏng ngầm đều
   bị phát hiện.

### 3.4. Cấu trúc HUF3 (định dạng ghi hiện tại)

HUF3 giữ nguyên header cố định 17 byte, mode và payload STORED của HUF2. Khác biệt chỉ ở mode
HUFFMAN: thay bảng tần suất bằng bảng độ dài mã chuẩn tắc.

| Phần | Kích thước | Kiểu | Ý nghĩa |
|---|---|---|---|
| magic | 4 byte | ASCII | `"HUF3"` |
| mode | 1 byte | uint8 | `0` = HUFFMAN, `1` = STORED |
| originalLength | 8 byte | uint64 | Số byte dữ liệu gốc |
| crc32 | 4 byte | uint32 | CRC-32 IEEE của dữ liệu gốc |
| uniqueCount | 2 byte | uint16 | (chỉ HUFFMAN) Số symbol có mã |
| Bảng độ dài mã | `uniqueCount × 2` byte | | (chỉ HUFFMAN) cặp `symbol` (uint8), `codeLength` (uint8) |
| Dữ liệu | phần còn lại | | HUFFMAN: mã chuẩn tắc MSB-first; STORED: byte gốc |

Các cặp trong bảng được ghi theo `symbol` tăng dần. `codeLength` nằm trong `[1, 32]`; không có
symbol nào được lặp. Bộ nén dùng Min-Heap và cây Huffman với khóa `(freq, minSymbol)` như trước,
nhưng chỉ để tính độ dài mã; mã thực tế được gán canonical theo thứ tự `(codeLength, symbol)`.
Mã đầu tiên của mỗi độ dài là mã kế tiếp sau mã trước đó, dịch trái theo chênh lệch độ dài.

Bộ giải nén kiểm tra bất đẳng thức Kraft (tổng đơn vị mã không vượt `2^32`), giải đúng
`originalLength` symbol, chỉ chấp nhận tối đa 7 bit đệm 0 cuối cùng và bắt buộc CRC32 khớp.
Nếu độ dài cây vượt 32 bit hoặc HUFFMAN không nhỏ hơn STORED, bộ nén chọn STORED. Do đó giới hạn
`kích thước nén <= kích thước gốc + 17` vẫn được giữ nguyên.

Ví dụ HUF3 HUFFMAN hợp lệ cho dữ liệu rỗng (19 byte; bộ nén thực tế chọn STORED 17 byte):

```
48 55 46 33                          magic "HUF3"
00                                   mode = HUFFMAN
00 00 00 00 00 00 00 00              originalLength = 0
00 00 00 00                          crc32 = 0
00 00                                uniqueCount = 0
```

Với dữ liệu không rỗng, sau `uniqueCount` là đúng hai byte cho mỗi symbol (`symbol`, `codeLength`);
không có tần suất trong HUF3 nên decoder không kiểm tra tổng tần suất.

## 4. Xây dựng cây Huffman (quy tắc bắt buộc, không đổi so với HUF1)

Người nén và người giải nén phải dựng ra **cùng một cây**, nên cần quy tắc xác định hoàn toàn.

**Khóa so sánh của mỗi nút:** cặp `(weight, minSymbol)`, trong đó:
- `weight` là tổng tần suất của nút (với lá là `freq` của symbol).
- `minSymbol` là symbol nhỏ nhất trong cây con của nút (với lá là chính symbol đó).

So sánh khóa theo thứ tự từ điển: so `weight` trước, nếu bằng nhau thì so `minSymbol`. Vì các cây con không giao nhau, `minSymbol` của hai nút khác nhau luôn khác nhau, nên khóa là duy nhất và cây luôn xác định.

**Thuật toán:**
1. Tạo một nút lá cho mỗi symbol trong bảng tần suất.
2. Lấy ra hai nút có khóa nhỏ nhất, gọi là `A` (nhỏ hơn) và `B`.
3. Tạo nút cha có con trái là `A`, con phải là `B`, `weight = A.weight + B.weight`, `minSymbol = min(A.minSymbol, B.minSymbol)`.
4. Đưa nút cha vào lại danh sách. Lặp lại bước 2 đến khi còn đúng một nút: gốc của cây.

Min-Heap tự viết (`include/minHeap.h`) thực hiện đúng quy tắc này.

**Gán mã cho HUF1/HUF2:**
- Đi từ gốc xuống lá. Cạnh trái ghi bit `0`, cạnh phải ghi bit `1`.
- Mã của symbol là chuỗi bit trên đường đi từ gốc đến lá đó.

HUF3 dùng cùng cây để lấy độ dài, sau đó gán mã canonical; thứ tự symbol trên dây không đổi
(tăng dần), còn thứ tự gán mã là `(độ dài, giá trị symbol)`.

## 5. CRC-32 (IEEE 802.3)

- Đa thức: `0xEDB88320` (dạng phản xạ của `0x04C11DB7`) — cùng chuẩn với PNG / zlib.
- Giá trị khởi tạo: `0xFFFFFFFF`. Sau khi xử lý toàn bộ byte, lấy phần bù `^ 0xFFFFFFFF`.
- Cài đặt: bảng tra 256 phần tử (`crc32Table()` trong `include/crc32.h`), tính theo khối 64 KB.
- CRC tính trên **dữ liệu gốc** (trước khi nén, hoặc trước khi lưu STORED), không tính trên header.
- Giá trị tham chiếu: `CRC32("123456789") = 0xCBF43926`, `CRC32("") = 0x00000000`.

## 6. Ví dụ: nén chuỗi `"aab"` (17 byte)

**Bước chọn mode:** bản STORED = `17 + 3 = 20` byte. Bản HUFFMAN = `17 (header) + 2 (uniqueCount)
+ 2 (symbol 'a' + freq LEB128 1 byte) + 2 (symbol 'b' + freq LEB128 1 byte) + 1 (byte bit) = 24` byte.
`24 >= 20` → chọn **STORED** (mode = 1).

Bước tính:
- Tần suất: `a` (0x61) = 2, `b` (0x62) = 1.
- CRC32 của `"aab"` = `0x690E2297` (little-endian: `97 22 0E 69`).
- Mã Huffman (nếu dùng): hai nút `b` có khóa `(1, 0x62)`, `a` có khóa `(2, 0x61)`. Nút nhỏ hơn là `b`,
  nên `b` là con trái (bit 0), `a` là con phải (bit 1) → `b` = `0`, `a` = `1`.
  Chuỗi bit `a a b` → `1 1 0`, đệm 5 bit 0 → `1100 0000` = `0xC0`.

File đầy đủ (20 byte, mode STORED):

```
48 55 46 32                          magic "HUF2"
01                                   mode = STORED
03 00 00 00 00 00 00 00              originalLength = 3
97 22 0E 69                          crc32 = 0x690E2297
61 61 62                             dữ liệu gốc "aab"
```

Để tham chiếu, nếu dữ liệu đủ lớn để bản HUFFMAN thắng thì phần đầu file có dạng (ví dụ cùng `"aab"`):

```
48 55 46 32                          magic "HUF2"
00                                   mode = HUFFMAN
03 00 00 00 00 00 00 00              originalLength = 3
97 22 0E 69                          crc32
02 00                                uniqueCount = 2
61 02                                symbol 'a', freq 2 (LEB128)
62 01                                symbol 'b', freq 1 (LEB128)
C0                                   dữ liệu bit: 110 + đệm 00000
```

## 7. Quy tắc kiểm tra khi giải nén

Bộ giải nén phải từ chối (báo lỗi, xóa file đích dở dang) nếu gặp một trong các trường hợp sau.
Các kiểm tra của HUF1 được giữ nguyên và áp dụng tương đương cho HUF2.

**Header HUF1/HUF2:**
- File ngắn hơn `HEADER_SIZE` (17 byte).
- Magic không phải `"HUF1"`, `"HUF2"` hoặc `"HUF3"`.
- `mode` khác `0` và `1`.

**Mode STORED:**
- Kích thước dữ liệu còn lại **khác đúng** `originalLength` (thiếu → lỗi, thừa → lỗi).
- CRC32 của dữ liệu vừa đọc khác `crc32` trong header (lỗi khi ghi file đích giữa chừng cũng bị bắt).

**Mode HUFFMAN:**
- Thiếu `uniqueCount` hoặc bảng tần suất khai báo.
- `uniqueCount > 256`.
- Varint `freq` dài quá 10 byte, hoặc không kết thúc trong phạm vi file.
- Có symbol trùng lặp / không sắp xếp tăng dần.
- Có `freq = 0`.
- Tổng các `freq` khác `originalLength` (phép cộng phải chống tràn `uint64`).
- `uniqueCount = 0` nhưng `originalLength > 0` (đã bị bắt bởi điều kiện tổng tần suất).
- Dữ liệu bit bị thiếu: số bit cần thiết lớn hơn số bit có trong file.
- Dữ liệu thừa ở cuối file: số bit có nhiều hơn số bit cần thiết từ 8 trở lên.
- Hết dữ liệu bit trước khi giải đủ `originalLength` symbol (gặp lá ảo / hết byte).
- **CRC32 của dữ liệu giải mã khác `crc32` trong header.**

**Mode HUF3-HUFFMAN:**
- `uniqueCount > 256`, số cặp khai báo bị thiếu, symbol trùng/lệch thứ tự, hoặc độ dài ngoài `[1, 32]`.
- Các độ dài mã vi phạm bất đẳng thức Kraft (mã bị chồng lấn).
- Không giải được đủ `originalLength` symbol, có mã bit không tồn tại, có hơn 7 bit đệm hoặc bit đệm khác 0.
- CRC32 của dữ liệu giải mã không khớp.

**Kiểm tra kích thước phần dữ liệu bit ngay từ đầu:** sau khi dựng cây, bộ giải nén tính
`số bit cần thiết = Σ (độ dài mã của symbol × freq của symbol)` (với cây 1 ký tự: `originalLength` bit)
và so sánh với `8 × (kích thước file − kích thước header − kích thước bảng tần suất)`.
Chênh lệch hợp lệ chỉ được nằm trong khoảng `[0, 7]` (bit đệm ở byte cuối).
Với cây 1 ký tự hoặc 2 ký tự cần đủ byte cho mọi bit: `ceil(requiredBits / 8)` byte.

**Ghi file đích:** dữ liệu giải mã được ghi theo khối 64 KB và CRC tính đồng thời trên chính dữ liệu
ghi ra. Chỉ đổi tên file tạm thành file đích khi CRC khớp; mọi lỗi giữa chừng đều xóa file tạm.

## 8. Giới hạn

- Không xử lý theo khối ở tầng nén (dữ liệu được đọc 2 lượt), nhưng bộ nhớ dùng cố định 64 KB.
- Bảng HUF3 dùng đúng 2 byte/symbol, nên kích thước metadata HUFFMAN là `O(uniqueCount)`.

## 9. HUF1 (định dạng cũ — chỉ đọc để tương thích ngược)

Bộ nén **không ghi** HUF1 nữa; bộ giải nén vẫn đọc được để các file `.bin` cũ không bị mất.

| Phần | Kích thước | Kiểu | Ý nghĩa |
|---|---|---|---|
| magic | 4 byte | ASCII | `"HUF1"` |
| uniqueCount | 2 byte | uint16 | Số symbol khác nhau, 0 → 256 |
| originalLength | 8 byte | uint64 | Số byte của dữ liệu gốc |
| Bảng tần suất | `uniqueCount × 9` byte | | `symbol` (uint8) + `freq` (uint64 LE) |
| Dữ liệu bit | Phần còn lại | | Chuỗi mã Huffman nối liền, đệm 0 |

Quy tắc dựng cây, gán mã, MSB-first, các kiểm tra cấu trúc giống HUF2/HUFFMAN (mục 4, 7),
trừ: không có `mode`, không có CRC32, `freq` là 8 byte cố định.

Ví dụ `"aab"` dạng HUF1 (33 byte):

```
48 55 46 31                          magic "HUF1"
02 00                                uniqueCount = 2
03 00 00 00 00 00 00 00              originalLength = 3
61 02 00 00 00 00 00 00 00           symbol 'a', freq 2
62 01 00 00 00 00 00 00 00           symbol 'b', freq 1
C0                                   dữ liệu bit: 110 + đệm 00000
```

## 10. Kiểm tra toàn vẹn ở tầng công cụ

Ngoài CRC32 trong định dạng, chế độ `huffman -v <nguồn> <nén.bin>` nén file, giải nén ra file tạm và
so sánh **SHA-256** của file gốc với file phục hồi. Hai lớp bảo vệ bổ sung nhau: CRC32 phát hiện hỏng
ngay khi giải nén (kể cả khi dùng công cụ khác), SHA-256 xác nhận ở tầng ứng dụng.

## 11. Lịch sử phiên bản

| Phiên bản | Thay đổi |
|---|---|
| HUF1 | Bản đầu: header 14 byte, bảng tần suất 9 byte/symbol, không checksum, không có mode STORED. |
| HUF2 | Thêm `mode` (HUFFMAN/STORED), `crc32`, `freq` dùng LEB128, `HEADER_SIZE` = 17 byte. Bộ giải nén đọc được cả HUF1. |
| HUF3 | Thay bảng tần suất HUFFMAN bằng độ dài mã canonical (2 byte/symbol); giữ header, CRC32 và STORED của HUF2. Decoder vẫn đọc HUF1/HUF2. |
