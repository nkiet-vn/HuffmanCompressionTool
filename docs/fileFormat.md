# Định dạng file .bin (HUF1)

Tài liệu này là nguồn chuẩn cho người nén và người giải nén. Mọi thay đổi phải được cả nhóm thống nhất trước khi sửa.

## 1. Quy ước chung

- Mọi số nhiều byte lưu theo **little-endian**.
- Thứ tự bit trong mỗi byte là **MSB-first**: bit được ghi đầu tiên nằm ở vị trí cao nhất (bit 7) của byte.
- Bit đệm (padding) ở cuối dữ liệu luôn là `0`.

## 2. Cấu trúc file

| Phần | Kích thước | Kiểu | Ý nghĩa |
|---|---|---|---|
| magic | 4 byte | ASCII | `"HUF1"`, nhận diện file |
| uniqueCount | 2 byte | uint16 | Số symbol khác nhau, từ 0 đến 256 |
| originalLength | 8 byte | uint64 | Số byte của dữ liệu gốc |
| Bảng tần suất | uniqueCount × 9 byte | | Mỗi mục: `symbol` (uint8, 1 byte) + `freq` (uint64, 8 byte) |
| Dữ liệu bit | Phần còn lại | | Chuỗi mã Huffman nối liền, đệm 0 đến đủ byte |

**Quy tắc bảng tần suất:**
- Các mục được sắp xếp theo `symbol` tăng dần.
- Mỗi `symbol` xuất hiện tối đa một lần.
- Mỗi `freq` phải lớn hơn 0. Symbol không xuất hiện thì không ghi vào bảng.

## 3. Xây dựng cây Huffman (quy tắc bắt buộc)

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

**Gán mã:**
- Đi từ gốc xuống lá. Cạnh trái ghi bit `0`, cạnh phải ghi bit `1`.
- Mã của symbol là chuỗi bit trên đường đi từ gốc đến lá đó.

**Trường hợp đặc biệt:**
- **File rỗng:** `uniqueCount = 0`, `originalLength = 0`, không có bảng tần suất và không có dữ liệu bit.
- **Chỉ có một symbol khác nhau:** cây chỉ có một lá, mã của symbol đó là `0` (1 bit cho mỗi ký tự).

## 4. Ví dụ: nén chuỗi `"aab"`

Bước tính:
- Tần suất: `a` (0x61) = 2, `b` (0x62) = 1.
- Hai nút: `b` có khóa `(1, 0x62)`, `a` có khóa `(2, 0x61)`. Nút nhỏ hơn là `b`, nên `b` là con trái (bit 0), `a` là con phải (bit 1).
- Mã: `b` = `0`, `a` = `1`.
- Chuỗi bit: `a a b` → `1 1 0`, đệm thêm 5 bit 0 → `1100 0000` = `0xC0`.

File đầy đủ (33 byte):

```
48 55 46 31                          magic "HUF1"
02 00                                uniqueCount = 2
03 00 00 00 00 00 00 00              originalLength = 3
61 02 00 00 00 00 00 00 00           symbol 'a', freq 2
62 01 00 00 00 00 00 00 00           symbol 'b', freq 1
C0                                   dữ liệu bit: 110 + đệm 00000
```

## 5. Quy tắc kiểm tra khi giải nén

Khi đọc file, bộ giải nén phải từ chối (báo lỗi) nếu gặp một trong các trường hợp sau:
- Magic khác `"HUF1"`.
- File ngắn hơn phần header hoặc bảng tần suất khai báo.
- `uniqueCount > 256`.
- Có symbol trùng lặp, không sắp xếp tăng dần, hoặc có `freq = 0` trong bảng.
- Tổng các `freq` khác `originalLength`.
- `uniqueCount = 0` nhưng `originalLength > 0`.
- Hết dữ liệu bit trước khi giải đủ `originalLength` symbol.

Bit đệm ở cuối bị bỏ qua. Bộ giải nén dừng khi đã giải đủ `originalLength` symbol, không dựa vào số bit còn lại.

## 6. Ngoài phạm vi hiện tại

- Không có checksum. Nếu cần kiểm tra toàn vẹn, sẽ tăng phiên bản thành `HUF2` và thêm trường CRC32. Mọi thay đổi định dạng phải cập nhật tài liệu này.
- Không xử lý theo khối. Toàn bộ file được xử lý một lần.

## 7. Thay đổi so với bản nháp trước

- Thêm quy tắc chọn bit 0/1 và quy tắc hòa tần suất dựa trên `minSymbol`.
- Thêm quy định MSB-first và bit đệm.
- Tần suất đổi từ uint32 sang uint64 để khớp với `originalLength`.
- Thêm quy tắc kiểm tra hợp lệ khi giải nén.
- Ví dụ `"aab"` đổi kết quả: `b` = 0, `a` = 1 (trước đó là `a` = 0, `b` = 1).