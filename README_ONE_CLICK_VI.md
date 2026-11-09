# Huffman Compression Tool — sửa code, nhấp đúp để build và chạy

**Tải nguyên thư mục dự án, không chỉ một file `.exe`.** Đây là dự án C++17 + Qt 6 / QML.
Người nhận được phép sửa file trong `src/`, `include/` và `gui/`, sau đó **nhấp đúp `BUILD_AND_RUN.cmd`**.
Script sẽ tự cấu hình CMake, **biên dịch phần đã thay đổi** và mở bản GUI mới vừa được build.
Không cần mở MSYS2 terminal hay gõ lệnh mỗi lần sửa code.

## Điều kiện trên từng máy Windows 10/11 x64

- Cần cài **MSYS2** một lần tại `C:\msys64` (https://www.msys2.org/).
- Nhấp đúp **`INSTALL_BUILD_TOOLS.cmd` một lần** để cài GCC C++17, Ninja, CMake và Qt 6 UCRT64.
  Nếu vừa cài MSYS2 và gói không tải được, cập nhật trong MSYS2 UCRT64 bằng `pacman -Syu` rồi mở lại terminal.
- Từ các lần tiếp theo: **chỉ nhấp đúp `BUILD_AND_RUN.cmd`** sau khi chỉnh sửa mã nguồn.
- Nếu MSYS2 được cài nơi khác, đặt biến môi trường Windows `MSYS2_ROOT` trỏ tới thư mục cài MSYS2.

## Ví dụ

Thư mục mã nguồn:

```text
D:\CODE\HuffmanCompressionTool\
├── BUILD_AND_RUN.cmd           <- Nhấp đúp mỗi lần sửa code
├── INSTALL_BUILD_TOOLS.cmd     <- Cài môi trường lần đầu (mỗi máy)
├── CMakeLists.txt
├── src/
├── include/
├── gui/
└── scripts/
    └── build-and-run-ucrt64.sh
```

1. Mở `gui/qml/Main.qml` hoặc sửa `src/compressor.cpp` trong IDE.
2. Lưu file.
3. Nhấp đúp `BUILD_AND_RUN.cmd`.
4. CMake/Ninja chỉ biên dịch lại phần có thay đổi. Sau khi build thành công, chương trình GUI mở ngay.

Bản chạy mới nằm tại `build-ucrt64\gui\huffman_gui.exe`.
**Không gửi file này riêng lẻ cho người không cài Qt**; file đó cần DLL Qt/MinGW để chạy.

## Tùy chọn

- Chạy cả các bài test trước khi mở GUI: đặt biến môi trường `HUFFMAN_RUN_TESTS=1`.
- Tạo bộ cài / portable để người *chỉ sử dụng* chương trình (không sửa code): nhấp đúp `TAO_FILE_EXE.cmd`
  sau khi cài thêm `mingw-w64-ucrt-x86_64-nsis`. Các file xuất ra trong `dist/`.
- **Setup.exe không biên dịch code**: nó là gói cài chạy đã build trước. Người sửa code cần source + compiler + Qt SDK.

## Lưu ý quan trọng

Không thể biên dịch C++/Qt trên một máy Windows hoàn toàn trống chỉ nhờ một file `.cmd`.
Bộ source này **không chứa toolchain/Qt SDK** (quá lớn). Cài môi trường một lần cho mỗi máy,
sau đó việc **sửa code → build → chạy chỉ cần một lần nhấp đúp**.
Không tự ý xóa các file nguồn; script chỉ xóa cache/build khi phát hiện đường dẫn nguồn hoặc generator đã đổi.
Đường dẫn không dấu là an toàn nhất với `qmlimportscanner` trong một số cấu hình MSYS2.

## Trạng thái kiểm thử

Core/CLI đã được build và chạy test trong môi trường Linux trước đây. Phần Qt GUI và
`.cmd` trên Windows cần xác minh trên MSYS2 UCRT64 của bạn; không tuyên bố đã test thực tế ở đây.
