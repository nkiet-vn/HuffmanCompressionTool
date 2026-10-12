# HuffmanCompressionTool

**Huffman Archiver** là ứng dụng nén và giải nén tệp sử dụng thuật toán mã hóa Huffman, được xây dựng bằng ngôn ngữ C++17. Ứng dụng thực hiện nén không mất dữ liệu, giúp giảm dung lượng lưu trữ trong những trường hợp phù hợp và khôi phục chính xác nội dung tệp ban đầu sau khi giải nén.

Chương trình hỗ trợ cả tệp văn bản và tệp nhị phân, sử dụng định dạng lưu trữ riêng và tích hợp cơ chế kiểm tra tính toàn vẹn dữ liệu.

## 1. Giới thiệu

Huffman Archiver được phát triển nhằm vận dụng kiến thức về cấu trúc dữ liệu, giải thuật, thao tác bit và xử lý tệp nhị phân để xây dựng một ứng dụng nén dữ liệu hoàn chỉnh.

### Các tính năng chính

- **Nén không mất dữ liệu:** Khôi phục nội dung tệp ban đầu chính xác đến từng byte.
- **Hỗ trợ nhiều loại tệp:** Xử lý cả dữ liệu văn bản và dữ liệu nhị phân.
- **Mã hóa Huffman:** Xây dựng cây Huffman dựa trên tần suất xuất hiện của các byte.
- **Định dạng tệp riêng:** Lưu trữ thông tin cần thiết để giải nén dữ liệu.
- **Kiểm tra tính toàn vẹn:** Sử dụng mã kiểm tra dữ liệu (checksum) để phát hiện dữ liệu bị thay đổi hoặc hỏng.
- **Tối ưu bộ nhớ:** Xử lý dữ liệu theo từng khối 64 KB, hạn chế việc phải nạp toàn bộ tệp vào bộ nhớ.
- **Xử lý lỗi:** Phát hiện các trường hợp tệp đầu vào không hợp lệ, phần đầu tệp bị hỏng, dữ liệu bị cắt cụt hoặc phiên bản định dạng không được hỗ trợ.
- **Giao diện dòng lệnh:** Hỗ trợ nén, giải nén và xem thông tin tệp đã nén.
- **Giao diện tương tác:** Cung cấp menu điều khiển trên cửa sổ dòng lệnh và hiển thị thông tin kết quả xử lý.

## 2. Nguyên lý hoạt động

Mã hóa Huffman là thuật toán nén dữ liệu không mất thông tin, sử dụng các mã nhị phân có độ dài thay đổi tùy theo tần suất xuất hiện của từng ký hiệu. Thông thường, ký hiệu xuất hiện nhiều được gán mã ngắn hơn, trong khi ký hiệu xuất hiện ít được gán mã dài hơn.

Quá trình nén gồm các bước chính:

1. **Thống kê tần suất:** Đọc tệp đầu vào và đếm số lần xuất hiện của từng byte.
2. **Xây dựng cây Huffman:** Sử dụng hàng đợi ưu tiên để kết hợp các nút theo tần suất xuất hiện.
3. **Sinh mã nhị phân:** Duyệt cây Huffman để xác định mã tương ứng với từng byte.
4. **Mã hóa dữ liệu:** Thay thế các byte bằng mã Huffman tương ứng, sau đó ghi chuỗi bit vào tệp nhị phân.
5. **Lưu thông tin cần thiết:** Ghi phần đầu tệp và các thông tin liên quan để có thể khôi phục dữ liệu khi giải nén.

Quá trình giải nén thực hiện theo chiều ngược lại: đọc thông tin lưu trữ, tái tạo cây Huffman, giải mã chuỗi bit và kiểm tra tính toàn vẹn của dữ liệu sau khi khôi phục.

Nguyên tắc đúng đắn cốt lõi của ứng dụng là:

\[
\operatorname{GiảiNén}(\operatorname{Nén}(X))=X
\]

Trong đó, \(X\) là dữ liệu ban đầu. Hai tệp được xem là giống nhau khi nội dung của chúng trùng khớp hoàn toàn ở cấp độ byte, không phụ thuộc vào loại tệp hay cách diễn giải dữ liệu.

## 3. Kiến trúc chương trình

Ứng dụng được tổ chức thành các mô-đun độc lập nhằm phân tách thuật toán cốt lõi, xử lý tệp, giao diện người dùng và hoạt động kiểm thử.

| Mô-đun | Chức năng |
|---|---|
| `huffman_tree` | Xây dựng cây Huffman và sinh mã cho từng ký hiệu. |
| `bit_io` | Đọc, ghi từng bit và quản lý bộ đệm dữ liệu ở cấp độ bit. |
| `file_format` | Định nghĩa cấu trúc phần đầu tệp và định dạng lưu trữ dữ liệu nén. |
| `codec` | Điều phối quá trình nén và giải nén tệp. |
| `utils` | Cung cấp các hàm tiện ích dùng chung, chẳng hạn như đo thời gian thực thi. |
| `ui` | Xây dựng giao diện dòng lệnh tương tác và hiển thị kết quả xử lý. |
| `main` | Quản lý điểm bắt đầu chương trình, tham số dòng lệnh và luồng thực thi. |
| `tests` | Kiểm thử tính đúng đắn, khả năng xử lý lỗi và các trường hợp biên. |
| `bench` | Đo lường hiệu năng nén và giải nén. |

Thiết kế theo mô-đun giúp chương trình dễ bảo trì, thuận tiện cho việc kiểm thử và có khả năng mở rộng trong tương lai.

## 4. Thông số kỹ thuật

| Thuộc tính | Mô tả |
|---|---|
| Ngôn ngữ lập trình | C++ |
| Tiêu chuẩn ngôn ngữ | C++17 |
| Thuật toán nén | Mã hóa Huffman |
| Phương pháp nén | Nén không mất dữ liệu |
| Dữ liệu đầu vào | Tệp văn bản và tệp nhị phân |
| Định dạng đầu ra | Tệp nén `.bin` theo định dạng riêng |
| Phương thức xử lý | Đọc và ghi dữ liệu theo từng khối |
| Kích thước khối | 64 KB |
| Kiểm tra dữ liệu | Sử dụng checksum |
| Thư viện bên ngoài | Không yêu cầu thư viện nén của bên thứ ba |
| Công cụ xây dựng | CMake hoặc Make, tùy cấu hình dự án |

**Lưu ý:** Thuật toán Huffman không đảm bảo mọi tệp đều có dung lượng nhỏ hơn sau khi nén. Đối với tệp nhỏ hoặc dữ liệu không phù hợp, phần thông tin phụ trợ của định dạng lưu trữ có thể khiến tệp sau nén lớn hơn tệp ban đầu.

## 5. Yêu cầu hệ thống

Để biên dịch và chạy chương trình, cần chuẩn bị:

- Trình biên dịch hỗ trợ C++17, chẳng hạn GCC.
- CMake và/hoặc Make, tùy theo cấu hình xây dựng của dự án.
- Terminal hoặc cửa sổ dòng lệnh để thực thi chương trình.

Ứng dụng không yêu cầu cài đặt các thư viện nén dữ liệu của bên thứ ba.

## 6. Cài đặt và biên dịch

### Bước 1. Tải mã nguồn

```bash
git clone <repository-url>
cd HuffmanCompressionTool
```

Thay `<repository-url>` bằng địa chỉ kho lưu trữ Git thực tế của dự án.

### Bước 2. Biên dịch bằng CMake

Nếu dự án có tệp `CMakeLists.txt`, sử dụng các lệnh:

```bash
cmake -S . -B build
cmake --build build
```

Sau khi biên dịch thành công, tệp thực thi sẽ được tạo trong thư mục xây dựng tương ứng với hệ điều hành và cấu hình biên dịch.

### Biên dịch bằng Make

Nếu dự án cung cấp `Makefile` tương thích, có thể sử dụng:

```bash
make
make test
```

Lệnh `make test` dùng để chạy bộ kiểm thử nếu mục tiêu này đã được cấu hình trong dự án.

**Lưu ý:** Chỉ sử dụng phương thức xây dựng tương ứng với các tệp cấu hình thực tế trong kho mã nguồn.

## 7. Hướng dẫn sử dụng

Huffman Archiver hỗ trợ thao tác thông qua giao diện dòng lệnh và menu tương tác trên cửa sổ dòng lệnh.

### 7.1. Nén tệp

Ví dụ nén tệp `input.txt` thành `output.bin`:

```bash
huffman c input.txt -o output.bin
```

### 7.2. Giải nén tệp

Ví dụ khôi phục dữ liệu từ `output.bin` thành `restored.txt`:

```bash
huffman d output.bin -o restored.txt
```

### 7.3. Xem thông tin tệp nén

```bash
huffman --info output.bin
```

Chức năng này được thiết kế để hiển thị thông tin lưu trữ của tệp nén mà không cần thực hiện toàn bộ quá trình giải nén.

### 7.4. Xử lý đường dẫn có dấu cách

Đối với đường dẫn chứa dấu cách, cần đặt đường dẫn trong dấu ngoặc kép:

```bash
huffman c "tai lieu hoc tap.txt" -o "tep da nen.bin"
```

Trong các ví dụ trên, `huffman` là tên minh họa của tệp thực thi. Cần thay bằng tên hoặc đường dẫn thực tế sau khi biên dịch. Các tham số dòng lệnh phải tương ứng với những chức năng đã được triển khai trong phiên bản hiện tại.

### 7.5. Sử dụng giao diện tương tác

Khởi chạy chương trình và thực hiện theo các lựa chọn được hiển thị trên màn hình để nén tệp, giải nén tệp hoặc xem thông tin liên quan.

Các chức năng cụ thể phụ thuộc vào phiên bản chương trình đang sử dụng.

## 8. Kiểm thử và xác minh

Bộ kiểm thử tập trung đánh giá tính đúng đắn của quá trình nén, giải nén và khả năng xử lý các tình huống bất thường.

Các trường hợp cần được kiểm thử bao gồm:

- Tệp rỗng và tệp chỉ có một byte.
- Tệp chứa đầy đủ 256 giá trị byte có thể có.
- Tệp chứa các ký hiệu lặp lại nhiều lần.
- Tệp văn bản và dữ liệu nhị phân bất kỳ.
- Tệp có kích thước nhỏ hơn một khối xử lý.
- Tệp lớn và cây Huffman có độ sâu lớn.
- Phần đầu tệp bị cắt cụt hoặc sai cấu trúc.
- Phiên bản định dạng không được hỗ trợ.
- Dữ liệu nén bị thay đổi và checksum không khớp.
- Trường hợp tệp sau nén có dung lượng lớn hơn ban đầu.

Tiêu chí xác minh quan trọng nhất là nội dung tệp sau giải nén phải trùng khớp hoàn toàn với tệp ban đầu.

Có thể chạy bộ kiểm thử bằng lệnh sau nếu dự án đã cấu hình mục tiêu tương ứng:

```bash
make test
```

Kết quả kiểm thử cần phản ánh đúng số lượng trường hợp đã thực thi và số trường hợp đạt hoặc không đạt. Không nên công bố số liệu kiểm thử khi chưa xác minh bằng mã nguồn hiện tại.

## 9. Quy trình Git

- `main`: nhánh ổn định, chỉ merge từ `develop`
- `develop`: nhánh tích hợp, các feature merge vào đây
- `feature/...`: nhánh làm việc cho từng công việc

Commit message luôn bắt đầu bằng mã ticket, ví dụ `DSA-1: add README`.

## 10. Quy ước đặt tên

- Biến, hàm: camelCase, ví dụ `writeBit`, `freqTable`
- Lớp: PascalCase, ví dụ `BitWriter`, `BitReader`
- File: camelCase, ví dụ `bitWriter.h`

## 11. Hạn chế

- Huffman là thuật toán nén không mất dữ liệu nhưng không đảm bảo giảm dung lượng đối với mọi loại tệp.
- Hiệu quả nén phụ thuộc vào phân bố tần suất byte và lượng thông tin phụ trợ cần lưu trữ.
- Chương trình chỉ có thể giải nén các tệp tuân theo những phiên bản định dạng được hỗ trợ.
- Checksum giúp phát hiện một số trường hợp dữ liệu bị hỏng hoặc thay đổi, nhưng không thay thế cơ chế xác thực mật mã.
- Tốc độ xử lý phụ thuộc vào kích thước, đặc điểm dữ liệu, phần cứng và tùy chọn tối ưu của trình biên dịch.

## 12. Giá trị học thuật

Huffman Archiver là ví dụ thực hành về việc áp dụng kiến thức khoa học máy tính vào xây dựng một ứng dụng hoàn chỉnh.

Thông qua dự án, người học có thể củng cố các kiến thức và kỹ năng sau:

- Giải thuật tham lam và hàng đợi ưu tiên.
- Cây nhị phân và các phương pháp duyệt cây.
- Thống kê tần suất và mã hóa ký hiệu.
- Phép toán bit và xử lý dữ liệu ở cấp độ bit.
- Đọc, ghi tệp nhị phân và thiết kế định dạng dữ liệu.
- Quản lý bộ nhớ và xử lý tệp theo khối.
- Phát hiện lỗi và kiểm tra tính toàn vẹn dữ liệu.
- Thiết kế phần mềm theo mô-đun và kiểm thử tự động.

Dự án phù hợp cho mục đích học tập, nghiên cứu thuật toán, thực hành lập trình C++ và tìm hiểu nguyên lý xây dựng công cụ nén dữ liệu.

## 13. Trạng thái dự án

Huffman Archiver được phát triển theo hướng mô-đun hóa bằng C++17. Các tính năng, phiên bản định dạng được hỗ trợ, kết quả kiểm thử và số liệu hiệu năng cần được xác định dựa trên phiên bản mã nguồn thực tế trong kho lưu trữ.


