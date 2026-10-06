# HuffmanCompressionTool
## 1. Giới thiệu

**Huffman Archiver** nén một tệp bất kỳ (văn bản hoặc nhị phân) thành tệp `.bin` nhỏ hơn
và phục hồi lại **chính xác từng byte** khi giải nén. Dự án được viết bằng C++17 thuần,
không phụ thuộc thư viện ngoài, tổ chức thành các module tách biệt để dễ đọc, kiểm thử và mở rộng.

Mục tiêu thiết kế:

- **Đúng đắn:** nội dung phục hồi giống hệt bản gốc, được xác minh bằng checksum.
- **Ổn định:** xử lý theo khối 64 KB nên tệp lớn không làm tràn bộ nhớ; báo lỗi rõ ràng với tệp hỏng.
- **Rõ ràng về mặt học thuật:** thể hiện đầy đủ cây Huffman, hàng đợi ưu tiên, bảng bit và các phép toán Bitwise.

## 1. Bảng tổng hợp phân công

| Thành viên | Vai trò | Mảng phụ trách | Nhánh chính | Tệp chạm vào | 
|---|---|---|---|---|
| **Member 1** | Tech Lead + Core | `huffman_tree`, `bit_io` | `feature/IF-01-tree-determinism`, `feature/IF-02-bitio-hardening` | `huffman_tree.cpp/.hpp`, `bit_io.hpp` | 
| **Member 2** | Format + Codec | `file_format`, `codec` | `feature/IF-03-header-v2`, `feature/IF-04-codec-progress` | `file_format.cpp/.hpp`, `codec.cpp/.hpp` | 
| **Member 3** | CLI + UI | `main`, `ui` | `feature/IF-05-cli-ext`, `feature/IF-06-ui-report` | `main.cpp`, `ui.cpp/.hpp` | 
| **Member 4** | QA + Test + CI | `tests/` + CI | `feature/IF-07-test-suite`, `feature/IF-08-ci` | `tests/*.cpp`, `.github/workflows/ci.yml` | 
| **Member 5** | Docs + Benchmark + Hiệu năng | `README`, `MAPCODE`, `bench` | `docs/IF-09-mapcode`, `feature/IF-10-bench-parallel` | `MAPCODE.md`, `README.md`, `bench/` | 

---

## 2. Chi tiết nhiệm vụ theo từng người

### MEMBER 1 — Tech Lead & Core Algorithm
**Nhánh:** `feature/IF-01-tree-determinism` → `feature/IF-02-bitio-hardening`

| # | Nhiệm vụ | Hàm / vị trí cụ thể | Kết quả bàn giao (DoD) |
|---|---|---|---|
| 1.1 | Gộp `(freq, order)` thành comparator tường minh | `huffman_tree.cpp:13–18` — thay lambda rời bằng `struct NodeCmp` | `build()` cho cùng `pool_` với mọi thứ tự nạp |
| 1.2 | Chứng minh cây xác định (determinism) | thêm `huffman_tree.hpp:20` overload `build(const FreqTable&)` | 2 lần build cùng freq ⇒ mã giống hệt, in ra để chứng minh |
| 1.3 | Rà soát trường hợp biên cây 1 lá | `huffman_tree.cpp:28–32` (mã `"0"`) | Test `single` (1000 ký tự `a`) vẫn vòng tròn OK |
| 1.4 | Bảo toàn DFS không đệ quy | `huffman_tree.cpp:46–58` | Tệp Fibonacci (>3 MB, cây sâu) không tràn stack |
| 1.5 | Thêm API `bool isLeaf(int)`, `int depth()` cho UI/Test dùng | `huffman_tree.hpp:22–27` | API mới có comment, không phá API cũ |
| 1.6 | Gia cố `BitWriter` chống bit sai | `bit_io.hpp:16–23` — `putBit` chỉ nhận `bit&1` | Thêm test gọi `putBit(2)` ⇒ chỉ ghi bit thấp |
| 1.7 | Kiểm tra `BitReader` khi cạn khối giữa luồng | `bit_io.hpp:50–62` | Bit cuối tệp trả `-1`, không đọc rác |
| 1.8 | Review + merge toàn bộ PR của Member 2–5 | — | Mỗi PR có comment review, ghi rõ lý do approve |

**Lệnh kiểm tra:** `make test` (phải 9/9 ĐẠT) và `./huffman c tests/../../README.md out.bin`.

---

### MEMBER 2 — File Format & Codec Core
**Nhánh:** `feature/IF-03-header-v2` → `feature/IF-04-codec-progress`

| # | Nhiệm vụ | Hàm / vị trí cụ thể | Kết quả bàn giao (DoD) |
|---|---|---|---|
| 2.1 | Thêm `formatVersion` vào `Header` | `file_format.hpp:11–15` (thêm field `u16 version = 1`) | Ghi ở offset 16, đọc lại và validate |
| 2.2 | Ghi/đọc version | `writeHeader` `file_format.cpp:28–38`, `readHeader` `file_format.cpp:40–61` | Tệp v1 cũ vẫn đọc được (tương thích ngược) |
| 2.3 | Từ chối version lạ | `readHeader` — ném `std::runtime_error` nếu `version > kFormatVersion` | Có ca test ở Member 4 |
| 2.4 | Cập nhật bảng bố cục header trong comment | `file_format.hpp:2–4` + `MAPCODE.md` mục 3.4 | Tài liệu khớp mã nhị phân thật |
| 2.5 | Tách 1 lượt đếm freq ra hàm riêng | `codec.cpp:24–37` → `namespace {}` hàm `countFreq(in, Header&)` | `compressFile` ngắn hơn, dễ test riêng |
| 2.6 | Thêm callback tiến trình (tùy chọn) | `codec.hpp:22–23` thêm overload nhận `std::function<void(u64,u64)>` | CLI/UI có thể hiện tiến trình; API cũ giữ nguyên |
| 2.7 | Rà soát điểm chạm hiệu năng | `codec.cpp:50–54` (vòng nén) và `codec.cpp:86–102` (vòng giải) | Tránh copy `std::vector<u8>` thừa trong `putCode` |
| 2.8 | Bảo toàn kiểm tra checksum | `codec.cpp:106` — lệch ⇒ ném lỗi | Test "sửa 1 byte" vẫn FAIL đúng như mong đợi |

**Lưu ý hợp đồng:** nếu đổi bố cục header, phải báo Member 4 cập nhật ca "sai magic" và Member 5 cập nhật `MAPCODE.md` mục 3.4.

---

### MEMBER 3 — CLI & Console UI
**Nhánh:** `feature/IF-05-cli-ext` → `feature/IF-06-ui-report`

| # | Nhiệm vụ | Hàm / vị trí cụ thể | Kết quả bàn giao (DoD) |
|---|---|---|---|
| 3.1 | Thêm chế độ `--info <file.bin>` | `main.cpp:26–34` + hàm mới `huff::readInfo()` | In header (size, checksum, số ký tự) mà **không** giải nén |
| 3.2 | Hỗ trợ `-o` và đường dẫn có dấu cách | `main.cpp:10–15` (`usage`) | `huffman c "a b.txt" -o out.bin` chạy đúng |
| 3.3 | Mã thoát rõ ràng (0/1/2) | `main.cpp:31–38` | Sai tham số ⇒ 2; lỗi runtime ⇒ 1; thành công ⇒ 0 |
| 3.4 | Bật UTF-8 + ANSI tách thành hàm | `main.cpp:18–23` → `huff::ui::enableUtf8()` trong `ui.cpp` | Không còn code Windows thô trong `main` |
| 3.5 | Thêm `ui::progress(pct)` | `ui.cpp` cạnh `bar()` dòng 25–30 | Dùng được callback của Member 2 (2.6) |
| 3.6 | Thêm `symbolLabel` cho ký tự > 127 | `ui.cpp:38–46` | Hiện `0xNN` thay vì ký tự rác |
| 3.7 | Thêm mục menu `[5]` xem thông tin tệp `.bin` | `ui.cpp:99–141` (`runMenu`) | Không phá luồng `[1]..[4]`, `[0]` hiện có |
| 3.8 | Kiểm tra `report` khi tệp nhỏ hơn bản nén | `ui.cpp:66–97` (nhánh `saved < 0` dòng 79) | Thông báo "header làm tệp lớn hơn" vẫn hiện |

**Ràng buộc:** `ui.cpp` được phép include `codec.hpp` + `utils.hpp`; **không** được để lõi include ngược lại `ui.hpp`.

---

### MEMBER 4 — QA, Test Suite & CI
**Nhánh:** `feature/IF-07-test-suite` → `feature/IF-08-ci`

| # | Nhiệm vụ | Hàm / vị trí cụ thể | Kết quả bàn giao (DoD) |
|---|---|---|---|
| 4.1 | Thêm helper `expectThrow(name, fn)` | `tests/test_roundtrip.cpp:14–16` (cạnh macro `CHECK`) | Giảm lặp `try/catch` ở dòng 54–61 |
| 4.2 | Ca test: đúng 1 byte mọi ký tự | mở rộng danh sách dòng 40–46 | 256 ca 1 byte đều vòng tròn OK |
| 4.3 | Ca test: tệp nhỏ hơn 64 KB (1 khối) | `roundTrip()` dòng 22–29 | Bao phủ nhánh `gcount < kBufferSize` |
| 4.4 | Ca test: header cắt cụt | `write` dòng 18–20 + `decompressFile` | Ném lỗi "bị cắt cụt / hỏng header" |
| 4.5 | Ca test: `count > 256` | sửa byte thứ 17 của `.bin` | Ném lỗi "Header không hợp lệ" |
| 4.6 | Ca test: version lạ (sau 2.1–2.3) | `.bin` có `version=99` | Bị từ chối |
| 4.7 | Dọn tệp tạm ngay cả khi ném lỗi | `roundTrip` dòng 22–29 dùng RAII/`try` | Không rác `t_*.bin` sau `make test` |
| 4.8 | Trả mã thoát 0/1 chuẩn cho CI | `main` test dòng 64–66 | `ctest` đọc được PASS/FAIL |
| 4.9 | Thiết lập CI (Ubuntu + g++ 17) | `.github/workflows/ci.yml` (tệp mới) | Mỗi PR chạy `cmake --build` + `ctest` |
| 4.10 | Ghi `make test` là lệnh xác minh chuẩn | `README.md` mục Build | Trùng với `MAPCODE.md` mục 6 |

**DoD tổng:** bộ test hiện tại 9 ca → mở rộng tối thiểu 15 ca, **tất cả ĐẠT**; không ca nào bị "skip im lặng".

---

### MEMBER 5 — Tài liệu, Benchmark & Hỗ trợ hiệu năng
**Nhánh:** `docs/IF-09-mapcode` → `feature/IF-10-bench-parallel`

| # | Nhiệm vụ | Hàm / vị trí cụ thể | Kết quả bàn giao (DoD) |
|---|---|---|---|
| 5.1 | Đồng bộ `MAPCODE.md` sau mỗi merge | mục 2 (mục lục symbol), mục 5 (bảng "sửa gì vào đâu") | Số dòng/symbol khớp code sau merge |
| 5.2 | Bổ sung bảng phân công vào tài liệu dự án | thêm link `README_TEAM.md` trong `README.md` | Người mới đọc 1 phút hiểu ai làm gì |
| 5.3 | Viết `bench/bench.cpp` (tệp mới) | dùng `nowSeconds()` (`utils.cpp:22–25`) | In MB/s nén/giải cho 3 tệp mẫu |
| 5.4 | Bộ dữ liệu mẫu để bench | `bench/data/` + script tạo tệp 1/10/100 MB | Tái lập được, không commit tệp lớn (dùng `.gitignore`) |
| 5.5 | Báo cáo số liệu tăng tốc sau 2.7 | so sánh trước/sau tối ưu vòng bit | Bảng % tăng tốc, kèm cách đo |
| 5.6 | Khảo sát nén song song theo khối (nghiên cứu) | `codec.cpp:19–65` phân tích khả năng `std::thread` | Tài liệu phân tích + quyết định làm/không làm |
| 5.7 | (Tùy chọn) Thử nén song song 4 luồng | nhánh riêng `feature/IF-11-parallel` **chỉ khi** 5.6 kết luận khả thi | Có số đo; nếu không thắng thì đóng PR, giữ code cũ |
| 5.8 | Rà soát tệp phát hành | `.gitignore`, `release/v1.0` | Bản phát hành không chứa `build/`, `*.bin`, `t_*` |

---

## 3. Lộ trình 4 Sprint (2 tuần/sprint)

| Sprint | Member 1 | Member 2 | Member 3 | Member 4 | Member 5 |
|---|---|---|---|---|---|
| **S1** (nền tảng) | IF-01 (1.1→1.4) | IF-03 (2.1→2.4) | IF-05 (3.1→3.4) | IF-07 (4.1→4.4) | IF-09 (5.1→5.2) |
| **S2** (hoàn thiện) | IF-02 (1.5→1.7) | IF-04 (2.5→2.8) | IF-06 (3.5→3.8) | IF-07 (4.5→4.8) | IF-10 (5.3→5.4) |
| **S3** (cứng hoá) | Review toàn bộ + fix | Fix lỗi từ test mới | Fix UX theo phản hồi | IF-08 (4.9→4.10) + regression | IF-10 (5.5→5.6) |
| **S4** (phát hành) | `release/v1.0` + merge `main` | Đóng gói ví dụ | Quay video demo | Chạy full test trên CI | IF-11 (5.7) + IF-10 (5.8) |

---

