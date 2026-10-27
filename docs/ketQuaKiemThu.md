# Kết quả kiểm thử

Sinh ra bởi test `verify_samples` (chạy tự động trong CTest). Cột **Hash** so sánh SHA-256 của
file gốc và file phục hồi sau khi nén + giải nén.

| File | Gốc (byte) | Nén (byte) | Tỷ lệ nén | Hash | Kết quả |
|---|---:|---:|---:|---|---|
| ngắn | 17 | 139 | -717.65% | KHỚP | PASS |
| dài | 324000 | 182168 | 43.78% | KHỚP | PASS |
| tiếng Việt UTF-8 | 68600 | 41406 | 39.64% | KHỚP | PASS |

## Ghi chú

- File rất ngắn (như `ngắn`, 17 byte) nén ra **lớn hơn** file gốc: header tốn
  `14 + 9 × số ký tự` byte nên với file dưới vài chục byte, phần header lớn hơn phần dữ liệu
  tiết kiệm được. Chương trình in cảnh báo rõ ràng sau khi nén. Hướng cải tiến: Canonical Huffman
  (chỉ lưu độ dài mã) hoặc chế độ "lưu nguyên" khi nén không có lợi.
- Với dữ liệu ngẫu nhiên (không nén được), file nén lớn hơn gốc khoảng `0.23%` — chỉ do header
  và bit đệm, dữ liệu được khôi phục nguyên vẹn.
- Tỷ lệ nén = `(1 − kích thước nén / kích thước gốc) × 100%`; giá trị âm nghĩa là file nén lớn hơn.

## Kiểm tra bổ sung

Các test `roundtrip`, `compress`, `minheap`, `huffmanCore` cũng chạy trong CTest và kiểm tra:

- Khôi phục chính xác từng byte với file rỗng, 1 ký tự, hai ký tự, tiếng Việt có dấu,
  và đủ 256 giá trị byte.
- Từ chối header hỏng: sai magic, header thiếu, `uniqueCount > 256`, tổng tần suất sai.
- Từ chối file bị cắt (thiếu dữ liệu bit) và file có dữ liệu thừa ở cuối.
- Min-Heap: thứ tự pop theo `(freq, minSymbol)`, tie-break bằng `minSymbol`, thao tác trên heap rỗng.
- Bảng mã prefix-free, bất đẳng thức Kraft, cây 1 ký tự.
