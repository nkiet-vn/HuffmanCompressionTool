#pragma once

enum class MessageId
{
    None,
    ToolSubtitle,
    MenuCompress,
    MenuDecompress,
    MenuVerify,
    MenuFileInfo,
    MenuHelp,
    MenuAbout,
    MenuExit,
    MenuCompressDescription,
    MenuDecompressDescription,
    MenuVerifyDescription,
    MenuFileInfoDescription,
    MenuHelpDescription,
    MenuAboutDescription,
    ChoicePrompt,
    WaitPrompt,
    ErrorTag,
    SuccessTag,
    InfoTag,
    WarningTag,
    EmptyValue,
    SessionEnded,
    NoChoice,
    Goodbye,
    InvalidChoice,
    InvalidChoicePrefix,
    Compressing,
    CompressionFailed,
    OriginalSize,
    CompressedSize,
    CompressionRatio,
    CompressionStatsSection,
    LargerThanOriginal,
    SavedSpace,
    SavedSpaceSuffix,
    ByteUnit,
    Decompressing,
    DecompressionFailed,
    OutputSize,
    VerifySection,
    OriginalHashFailed,
    VerifyDecompressionFailed,
    RestoredHashFailed,
    OriginalHash,
    RestoredHash,
    HashMismatch,
    VerifySuccess,
    FileInfoSection,
    FilePath,
    FileSize,
    FileFormat,
    NotApplicable,
    FileHuf2Huffman,
    FileHuf2Stored,
    FileHuf3Huffman,
    FileHuf3Stored,
    FileHuf1,
    FilePlain,
    OriginalLength,
    UniqueCount,
    CompressionRate,
    Sha256,
    FileReadFailed,
    FileNotFound,
    OverwriteQuestion,
    OverwriteSuffix,
    CompressSection,
    DecompressSection,
    VerifyMenuSection,
    SourcePrompt,
    DestinationPrompt,
    CompressionDestinationPrompt,
    CompressedFilePrompt,
    InfoPathPrompt,
    HelpSection,
    HelpCommand,
    HelpDescription,
    HelpCompress,
    HelpDecompress,
    HelpVerify,
    HelpInfo,
    HelpMenu,
    HelpHelp,
    HelpMenuInfo,
    HelpCmdCompress,
    HelpCmdDecompress,
    HelpCmdVerify,
    HelpCmdInfo,
    AboutSection,
    Cancelled,
    Name,
    Version,
    Language,
    Format,
    DataStructures,
    Integrity,
    AboutFormat,
    AboutDataStructures,
    AboutIntegrity,
    CliTitle,
    Usage,
    CliCompress,
    CliDecompress,
    CliVerify,
    CliInfo,
    CliMenu,
    CliHelp,
    CliAscii,
    CliNoArgs,
    CliExamples,
    CliExampleCompress,
    CliExampleDecompress,
    CliExampleInfo,
    CliExampleVerify,
    InvalidMode,
    InvalidModeHelp,
    InvalidArgumentCount,
    Progress,
    CodecOpenSource,
    CodecSameSourceDestination,
    CodecCheckDestination,
    CodecReadSource,
    CodecSeekSource,
    CodecSameTempSource,
    CodecCheckTemp,
    CodecTempExists,
    CodecCreateOutput,
    CodecRenameTemp,
    CodecHeaderShort,
    CodecFrequencyTableShort,
    CodecInvalidFrequencyVarint,
    CodecFrequencyOrder,
    CodecZeroFrequency,
    CodecFrequencyOverflow,
    CodecFrequencyTotalMismatch,
    CodecUniqueCountTooLarge,
    CodecInvalidBitSize,
    CodecInvalidCodeTable,
    CodecBitDataShort,
    CodecTrailingData,
    CodecInvalidBitData,
    CodecInvalidTree,
    CodecInvalidMode,
    CodecUnknownFormat,
    CodecStoredDataShort,
    CodecWriteOutput,
    CodecChecksumMismatch,
    CodecFileSizeUnknown,
    CodecReadArchive,
    CodecInternalFailure,
};

inline const char *message(MessageId id, bool ascii = false)
{
#define MESSAGE_CASE(idValue, vietnamese, plain) \
    case MessageId::idValue: return ascii ? plain : vietnamese
    switch (id)
    {
        MESSAGE_CASE(None, "", "");
        MESSAGE_CASE(ToolSubtitle, "Công cụ nén và giải nén tệp bằng thuật toán Huffman", "Cong cu nen va giai nen tep bang thuat toan Huffman");
        MESSAGE_CASE(MenuCompress, "Nén tệp", "Nen tep");
        MESSAGE_CASE(MenuDecompress, "Giải nén tệp", "Giai nen tep");
        MESSAGE_CASE(MenuVerify, "Nén và xác minh SHA-256", "Nen va xac minh SHA-256");
        MESSAGE_CASE(MenuFileInfo, "Xem thông tin tệp", "Xem thong tin tep");
        MESSAGE_CASE(MenuHelp, "Hướng dẫn / trợ giúp", "Huong dan / tro giup");
        MESSAGE_CASE(MenuAbout, "Thông tin phần mềm", "Thong tin phan mem");
        MESSAGE_CASE(MenuExit, "Thoát", "Thoat");
        MESSAGE_CASE(MenuCompressDescription, "nén tệp thành định dạng HUF3", "nen tep thanh dinh dang HUF3");
        MESSAGE_CASE(MenuDecompressDescription, "khôi phục tệp từ .bin", "khoi phuc tep tu .bin");
        MESSAGE_CASE(MenuVerifyDescription, "nén + giải nén lại + so sánh hash", "nen + giai nen lai + so sanh hash");
        MESSAGE_CASE(MenuFileInfoDescription, "kích thước, định dạng, tỷ lệ, SHA-256", "kich thuoc, dinh dang, ty le, SHA-256");
        MESSAGE_CASE(MenuHelpDescription, "cách dùng dòng lệnh", "cach dung dong lenh");
        MESSAGE_CASE(MenuAboutDescription, "phiên bản, cấu trúc dữ liệu chính", "phien ban, cau truc du lieu chinh");
        MESSAGE_CASE(ChoicePrompt, "Chọn chức năng [0-6]: ", "Chon chuc nang [0-6]: ");
        MESSAGE_CASE(WaitPrompt, "\nNhấn Enter để quay lại menu...", "\nNhan Enter de quay lai menu...");
        MESSAGE_CASE(ErrorTag, "LỖI", "LOI");
        MESSAGE_CASE(SuccessTag, "OK", "OK");
        MESSAGE_CASE(InfoTag, "i", "i");
        MESSAGE_CASE(WarningTag, "!", "!");
        MESSAGE_CASE(EmptyValue, "Giá trị không được để trống. Vui lòng nhập lại.", "Gia tri khong duoc de trong. Vui long nhap lai.");
        MESSAGE_CASE(SessionEnded, "Kết thúc phiên làm việc.", "Ket thuc phien lam viec.");
        MESSAGE_CASE(NoChoice, "Bạn chưa chọn chức năng nào.", "Ban chua chon chuc nang nao.");
        MESSAGE_CASE(Goodbye, "Tạm biệt!", "Tam biet!");
        MESSAGE_CASE(InvalidChoice, "không hợp lệ. Hãy chọn từ 0 đến 6.", "khong hop le. Hay chon tu 0 den 6.");
        MESSAGE_CASE(InvalidChoicePrefix, "Lựa chọn '", "Lua chon '");
        MESSAGE_CASE(Compressing, "Đang nén: ", "Dang nen: ");
        MESSAGE_CASE(CompressionFailed, "Nén thất bại. ", "Nen that bai. ");
        MESSAGE_CASE(OriginalSize, "Kích thước gốc", "Kich thuoc goc");
        MESSAGE_CASE(CompressedSize, "Kích thước tệp nén", "Kich thuoc tep nen");
        MESSAGE_CASE(CompressionRatio, "Tỷ lệ nén", "Ty le nen");
        MESSAGE_CASE(CompressionStatsSection, "THỐNG KÊ NÉN", "THONG KE NEN");
        MESSAGE_CASE(LargerThanOriginal, "Tệp nén LỚN HƠN tệp gốc: header chiếm nhiều hơn phần dữ liệu tiết kiệm.", "Tep nen LON HON tep goc: header chiem nhieu hon phan du lieu tiet kiem.");
        MESSAGE_CASE(SavedSpace, "Đã tiết kiệm ", "Da tiet kiem ");
        MESSAGE_CASE(SavedSpaceSuffix, " dung lượng.", " dung luong.");
        MESSAGE_CASE(ByteUnit, " byte", " byte");
        MESSAGE_CASE(Decompressing, "Đang giải nén: ", "Dang giai nen: ");
        MESSAGE_CASE(DecompressionFailed, "Giải nén thất bại. Tệp .bin có thể bị hỏng hoặc không đúng định dạng HUF1/HUF2/HUF3.", "Giai nen that bai. Tep .bin co the bi hong hoac khong dung dinh dang HUF1/HUF2/HUF3.");
        MESSAGE_CASE(OutputSize, "Kích thước tệp đầu ra", "Kich thuoc tep dau ra");
        MESSAGE_CASE(VerifySection, "XÁC MINH KHÔI PHỤC (SHA-256)", "XAC MINH KHOI PHUC (SHA-256)");
        MESSAGE_CASE(OriginalHashFailed, "Không tính được SHA-256 của tệp gốc.", "Khong tinh duoc SHA-256 cua tep goc.");
        MESSAGE_CASE(VerifyDecompressionFailed, "Giải nén xác minh thất bại.", "Giai nen xac minh that bai.");
        MESSAGE_CASE(RestoredHashFailed, "Không tính được SHA-256 của tệp phục hồi.", "Khong tinh duoc SHA-256 cua tep phuc hoi.");
        MESSAGE_CASE(OriginalHash, "SHA-256 gốc", "SHA-256 goc");
        MESSAGE_CASE(RestoredHash, "SHA-256 phục hồi", "SHA-256 phuc hoi");
        MESSAGE_CASE(HashMismatch, "Hash không khớp - tệp phục hồi khác tệp gốc.", "Hash khong khop - tep phuc hoi khac tep goc.");
        MESSAGE_CASE(VerifySuccess, "Xác minh thành công: tệp phục hồi giống tệp gốc 100%.", "Xac minh thanh cong: tep phuc hoi giong tep goc 100%.");
        MESSAGE_CASE(FileInfoSection, "THÔNG TIN TỆP", "THONG TIN TEP");
        MESSAGE_CASE(FilePath, "Đường dẫn", "Duong dan");
        MESSAGE_CASE(FileSize, "Kích thước", "Kich thuoc");
        MESSAGE_CASE(FileFormat, "Định dạng", "Dinh dang");
        MESSAGE_CASE(NotApplicable, "Không áp dụng", "Khong ap dung");
        MESSAGE_CASE(FileHuf2Huffman, "HUF2-HUFFMAN", "HUF2-HUFFMAN");
        MESSAGE_CASE(FileHuf2Stored, "HUF2-STORED", "HUF2-STORED");
        MESSAGE_CASE(FileHuf3Huffman, "HUF3-HUFFMAN", "HUF3-HUFFMAN");
        MESSAGE_CASE(FileHuf3Stored, "HUF3-STORED", "HUF3-STORED");
        MESSAGE_CASE(FileHuf1, "HUF1", "HUF1");
        MESSAGE_CASE(FilePlain, "Tệp thường", "Tep thuong");
        MESSAGE_CASE(OriginalLength, "Kích thước gốc", "Kich thuoc goc");
        MESSAGE_CASE(UniqueCount, "Số ký hiệu khác nhau", "So ky hieu khac nhau");
        MESSAGE_CASE(CompressionRate, "Tỷ lệ nén", "Ty le nen");
        MESSAGE_CASE(Sha256, "SHA-256", "SHA-256");
        MESSAGE_CASE(FileReadFailed, "Không đọc được tệp: ", "Khong doc duoc tep: ");
        MESSAGE_CASE(FileNotFound, "Không tìm thấy tệp: ", "Khong tim thay tep: ");
        MESSAGE_CASE(OverwriteQuestion, "Tệp đích '", "Tep dich '");
        MESSAGE_CASE(OverwriteSuffix, "' đã tồn tại, ghi đè?", "' da ton tai, ghi de?");
        MESSAGE_CASE(CompressSection, "NÉN TỆP", "NEN TEP");
        MESSAGE_CASE(DecompressSection, "GIẢI NÉN TỆP", "GIAI NEN TEP");
        MESSAGE_CASE(VerifyMenuSection, "NÉN VÀ XÁC MINH", "NEN VA XAC MINH");
        MESSAGE_CASE(SourcePrompt, "Tệp nguồn: ", "Tep nguon: ");
        MESSAGE_CASE(DestinationPrompt, "Tệp đích: ", "Tep dich: ");
        MESSAGE_CASE(CompressionDestinationPrompt, "Tệp nén đầu ra (vd: data.bin): ", "Tep nen dau ra (vd: data.bin): ");
        MESSAGE_CASE(CompressedFilePrompt, "Tệp nén: ", "Tep nen: ");
        MESSAGE_CASE(InfoPathPrompt, "Đường dẫn tệp: ", "Duong dan tep: ");
        MESSAGE_CASE(HelpSection, "HƯỚNG DẪN SỬ DỤNG", "HUONG DAN SU DUNG");
        MESSAGE_CASE(HelpCommand, "Lệnh", "Lenh");
        MESSAGE_CASE(HelpDescription, "Mô tả", "Mo ta");
        MESSAGE_CASE(HelpCompress, "Nén tệp", "Nen tep");
        MESSAGE_CASE(HelpDecompress, "Giải nén tệp", "Giai nen tep");
        MESSAGE_CASE(HelpVerify, "Nén và xác minh SHA-256", "Nen va xac minh SHA-256");
        MESSAGE_CASE(HelpInfo, "Xem thông tin tệp", "Xem thong tin tep");
        MESSAGE_CASE(HelpCmdCompress, "-c <nguồn> <đích.bin>", "-c <nguon> <dich.bin>");
        MESSAGE_CASE(HelpCmdDecompress, "-d <đích.bin> <ra>", "-d <dich.bin> <ra>");
        MESSAGE_CASE(HelpCmdVerify, "-v <nguồn> <đích.bin>", "-v <nguon> <dich.bin>");
        MESSAGE_CASE(HelpCmdInfo, "-i <tệp>", "-i <tep>");
        MESSAGE_CASE(HelpMenu, "Mở menu tương tác này", "Mo menu tuong tac nay");
        MESSAGE_CASE(HelpHelp, "Hiển thị trợ giúp", "Hien thi tro giup");
        MESSAGE_CASE(HelpMenuInfo, "Chạy 'huffman' không tham số cũng sẽ mở menu tương tác.", "Chay 'huffman' khong tham so cung se mo menu tuong tac.");
        MESSAGE_CASE(AboutSection, "THÔNG TIN PHẦN MỀM", "THONG TIN PHAN MEM");
        MESSAGE_CASE(Cancelled, "Đã hủy thao tác.", "Da huy thao tac.");
        MESSAGE_CASE(Name, "Tên", "Ten");
        MESSAGE_CASE(Version, "Phiên bản", "Phien ban");
        MESSAGE_CASE(Language, "Ngôn ngữ", "Ngon ngu");
        MESSAGE_CASE(Format, "Định dạng tệp", "Dinh dang tep");
        MESSAGE_CASE(DataStructures, "Cấu trúc dữ liệu", "Cau truc du lieu");
        MESSAGE_CASE(Integrity, "Toàn vẹn dữ liệu", "Toan ven du lieu");
        MESSAGE_CASE(AboutFormat, "HUF3, đọc được HUF2/HUF1 (xem docs/fileFormat.md)", "HUF3, doc duoc HUF2/HUF1 (xem docs/fileFormat.md)");
        MESSAGE_CASE(AboutDataStructures, "Min-Heap tự viết (khóa: freq, minSymbol)", "Min-Heap tu viet (khoa: freq, minSymbol)");
        MESSAGE_CASE(AboutIntegrity, "CRC32 của dữ liệu gốc (HUF2/HUF3) + SHA-256 khi dùng -v", "CRC32 cua du lieu goc (HUF2/HUF3) + SHA-256 khi dung -v");
        MESSAGE_CASE(CliTitle, "Huffman Compression Tool - Công cụ nén dữ liệu Huffman (C++17)", "Huffman Compression Tool - Cong cu nen du lieu Huffman (C++17)");
        MESSAGE_CASE(Usage, "Sử dụng:", "Su dung:");
        MESSAGE_CASE(CliCompress, "  huffman -c <tệp_nguồn> <tệp_nén.bin>   Nén tệp", "  huffman -c <tep_nguon> <tep_nen.bin>   Nen tep");
        MESSAGE_CASE(CliDecompress, "  huffman -d <tệp_nén.bin> <tệp_đích>    Giải nén tệp", "  huffman -d <tep_nen.bin> <tep_dich>    Giai nen tep");
        MESSAGE_CASE(CliVerify, "  huffman -v <tệp_nguồn> <tệp_nén.bin>   Nén và xác minh khôi phục SHA-256", "  huffman -v <tep_nguon> <tep_nen.bin>   Nen va xac minh khoi phuc SHA-256");
        MESSAGE_CASE(CliInfo, "  huffman -i <tệp>                        Xem thông tin tệp", "  huffman -i <tep>                        Xem thong tin tep");
        MESSAGE_CASE(CliMenu, "  huffman -m, --menu                      Mở menu tương tác", "  huffman -m, --menu                      Mo menu tuong tac");
        MESSAGE_CASE(CliHelp, "  huffman --help                          Hiển thị hướng dẫn", "  huffman --help                          Hien thi huong dan");
        MESSAGE_CASE(CliAscii, "  huffman --ascii                         Hiển thị tiếng Việt không dấu", "  huffman --ascii                         Hien thi tieng Viet khong dau");
        MESSAGE_CASE(CliNoArgs, "Chạy không tham số cũng sẽ mở menu tương tác.", "Chay khong tham so cung se mo menu tuong tac.");
        MESSAGE_CASE(CliExamples, "Ví dụ:", "Vi du:");
        MESSAGE_CASE(CliExampleCompress, "  huffman -c input.txt output.bin", "  huffman -c input.txt output.bin");
        MESSAGE_CASE(CliExampleDecompress, "  huffman -d output.bin restored.txt", "  huffman -d output.bin restored.txt");
        MESSAGE_CASE(CliExampleInfo, "  huffman -i output.bin", "  huffman -i output.bin");
        MESSAGE_CASE(CliExampleVerify, "  huffman -v input.txt output.bin", "  huffman -v input.txt output.bin");
        MESSAGE_CASE(InvalidMode, "Lỗi: Chế độ không hợp lệ '", "Loi: Che do khong hop le '");
        MESSAGE_CASE(InvalidModeHelp, "'. Dùng '--help' để xem hướng dẫn.", "'. Dung '--help' de xem huong dan.");
        MESSAGE_CASE(InvalidArgumentCount, "Lỗi: Sai số lượng tham số. Dùng '--help' để xem hướng dẫn.", "Loi: Sai so luong tham so. Dung '--help' de xem huong dan.");
        MESSAGE_CASE(Progress, "Tiến trình", "Tien trinh");
        MESSAGE_CASE(CodecOpenSource, "Không mở được tệp nguồn", "Khong mo duoc tep nguon");
        MESSAGE_CASE(CodecSameSourceDestination, "Tệp nguồn và tệp đích không được trùng nhau!", "Tep nguon va tep dich khong duoc trung nhau!");
        MESSAGE_CASE(CodecCheckDestination, "Không kiểm tra được tệp đích", "Khong kiem tra duoc tep dich");
        MESSAGE_CASE(CodecReadSource, "Đọc tệp nguồn thất bại", "Doc tep nguon that bai");
        MESSAGE_CASE(CodecSeekSource, "Không quay lại được đầu tệp nguồn", "Khong quay lai duoc dau tep nguon");
        MESSAGE_CASE(CodecSameTempSource, "Tệp tạm không được trùng với tệp nguồn!", "Tep tam khong duoc trung voi tep nguon!");
        MESSAGE_CASE(CodecCheckTemp, "Không kiểm tra được tệp tạm", "Khong kiem tra duoc tep tam");
        MESSAGE_CASE(CodecTempExists, "Tệp tạm đã tồn tại, không ghi đè", "Tep tam da ton tai, khong ghi de");
        MESSAGE_CASE(CodecCreateOutput, "Không tạo được tệp", "Khong tao duoc tep");
        MESSAGE_CASE(CodecRenameTemp, "Không đổi tên được tệp tạm", "Khong doi ten duoc tep tam");
        MESSAGE_CASE(CodecHeaderShort, "Header bị thiếu dữ liệu", "Header bi thieu du lieu");
        MESSAGE_CASE(CodecFrequencyTableShort, "Bảng tần suất bị thiếu dữ liệu", "Bang tan suat bi thieu du lieu");
        MESSAGE_CASE(CodecInvalidFrequencyVarint, "Varint tần suất không hợp lệ", "Varint tan suat khong hop le");
        MESSAGE_CASE(CodecFrequencyOrder, "Bảng tần suất không sắp xếp tăng dần hoặc bị trùng symbol", "Bang tan suat khong sap xep tang dan hoac bi trung symbol");
        MESSAGE_CASE(CodecZeroFrequency, "Tần suất bằng 0 trong bảng tần suất", "Tan suat bang 0 trong bang tan suat");
        MESSAGE_CASE(CodecFrequencyOverflow, "Tổng tần suất không khớp originalLength", "Tong tan suat khong khop originalLength");
        MESSAGE_CASE(CodecFrequencyTotalMismatch, "Tổng tần suất khác originalLength", "Tong tan suat khac originalLength");
        MESSAGE_CASE(CodecUniqueCountTooLarge, "uniqueCount vượt quá 256", "uniqueCount vuot qua 256");
        MESSAGE_CASE(CodecInvalidBitSize, "Kích thước dữ liệu bit không hợp lệ", "Kich thuoc du lieu bit khong hop le");
        MESSAGE_CASE(CodecInvalidCodeTable, "Bảng độ dài mã không hợp lệ", "Bang do dai ma khong hop le");
        MESSAGE_CASE(CodecBitDataShort, "Dữ liệu bit bị thiếu", "Du lieu bit bi thieu");
        MESSAGE_CASE(CodecTrailingData, "Tệp nén có dữ liệu thừa ở cuối", "Tep nen co du lieu thua o cuoi");
        MESSAGE_CASE(CodecInvalidBitData, "Dữ liệu bit không hợp lệ hoặc bị thiếu", "Du lieu bit khong hop le hoac bi thieu");
        MESSAGE_CASE(CodecInvalidTree, "Không dựng được cây Huffman", "Khong dung duoc cay Huffman");
        MESSAGE_CASE(CodecInvalidMode, "Mode không hợp lệ (chỉ nhận 0 = Huffman, 1 = STORED)", "Mode khong hop le (chi nhan 0 = Huffman, 1 = STORED)");
        MESSAGE_CASE(CodecUnknownFormat, "Tệp không đúng định dạng HUF1/HUF2/HUF3 (magic không hợp lệ)", "Tep khong dung dinh dang HUF1/HUF2/HUF3 (magic khong hop le)");
        MESSAGE_CASE(CodecStoredDataShort, "Dữ liệu STORED bị thiếu", "Du lieu STORED bi thieu");
        MESSAGE_CASE(CodecWriteOutput, "Không ghi được tệp giải nén", "Khong ghi duoc tep giai nen");
        MESSAGE_CASE(CodecChecksumMismatch, "CRC32 không khớp: dữ liệu gốc bị hỏng", "CRC32 khong khop: du lieu goc bi hong");
        MESSAGE_CASE(CodecFileSizeUnknown, "Không xác định được kích thước tệp nén", "Khong xac dinh duoc kich thuoc tep nen");
        MESSAGE_CASE(CodecReadArchive, "Không đọc được tệp nén", "Khong doc duoc tep nen");
        MESSAGE_CASE(CodecInternalFailure, "Thao tác codec thất bại", "Thao tac codec that bai");
    }
#undef MESSAGE_CASE
    return "";
}
