#include "FoundryController.h"
#include "huffmanTree.h"
#include "canonicalHuffman.h"
#include "compressor.h"
#include "decompressor.h"
#include "fileHasher.h"
#include "fileInspector.h"
#include "messages.h"
#include "pathUtil.h"
#include <QtConcurrent/QtConcurrentRun>
#include <QClipboard>
#include <QDesktopServices>
#include <QFileInfo>
#include <QGuiApplication>
#include <QPointer>
#include <QUrl>
#include <QDir>
#include <QThread>
#include <QWindow>
#include <QStyleHints>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include <QVariant>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {
QString fromUtf8(const std::string& s) { return QString::fromUtf8(s.c_str(), static_cast<qsizetype>(s.size())); }
std::string toUtf8(const QString& s) { const auto data = s.toUtf8(); return std::string(data.constData(), static_cast<size_t>(data.size())); }
QString printable(unsigned c) {
    if (c == 32) return "SPACE";
    if (c == 10) return "LF";
    if (c == 13) return "CR";
    if (c == 9) return "TAB";
    if (c >= 33 && c <= 126) return QString(QChar(static_cast<char16_t>(c)));
    return QString("0x%1").arg(c,2,16,QLatin1Char('0')).toUpper();
}
QString codeBits(std::uint32_t value, unsigned length) {
    QString result;
    for (unsigned n = length; n > 0; --n) result += ((value >> (n - 1)) & 1u) ? '1' : '0';
    return result;
}
struct Cancelled final : std::exception {};
struct Analysis {
    QVariantList histogram, trace, codeRows;
    QVariantMap info;
    QString error;
};
struct TraceRecorder : TreeBuildListener {
    QVariantList& trace;
    unsigned sequence = 0;
    QVariantMap left, right;
    explicit TraceRecorder(QVariantList& target) : trace(target) {}
    static QVariantMap node(const HuffmanNode& n) {
        return {{"freq", QVariant::fromValue<qulonglong>(n.freq)},
                {"symbol", n.isLeaf ? printable(n.symbol) : QString("NODE")},
                {"minSymbol", int(n.minSymbol)},
                {"key", QString("(%1, %2)").arg(n.freq).arg(n.minSymbol)}};
    }
    void onSeed(const HuffmanNode& n) override {
        trace.push_back(QVariantMap{{"kind", "seed"},{"index",int(sequence++)},{"left",node(n)},
                       {"description",QString("SET SORT  /  %1").arg(printable(n.symbol))}});
    }
    void onPop(const HuffmanNode& a, const HuffmanNode& b) override {
        left = node(a); right = node(b);
    }
    void onMerge(const HuffmanNode& n) override {
        trace.push_back(QVariantMap{{"kind", "merge"},{"index",int(sequence++)},
                       {"left",left},{"right",right},{"parent",node(n)},
                       {"description",QString("CAST  /  %1 + %2 = %3").arg(left.value("freq").toString(),right.value("freq").toString(),QString::number(n.freq))},
                       {"tie", left.value("freq")==right.value("freq")}});
    }
};
Analysis analyze(const std::string& filename, const std::shared_ptr<std::atomic_bool>& cancelled) {
    Analysis a;
    std::ifstream file(toPath(filename),std::ios::binary);
    if (!file) { a.error = "Không mở được tệp nguồn."; return a; }
    std::uint64_t frequencies[256]{};
    std::vector<unsigned char> sample;
    sample.reserve(8);
    char bytes[64*1024];
    while (file.read(bytes,sizeof(bytes)) || file.gcount()>0) {
        if (cancelled->load()) return a;
        for (std::streamsize i=0;i<file.gcount();++i) {
            auto byte=static_cast<unsigned char>(bytes[i]);
            ++frequencies[byte];
            if(sample.size()<8)sample.push_back(byte);
        }
    }
    if (file.bad()) { a.error = "Không đọc được dữ liệu tệp nguồn."; return a; }
    std::uint64_t total=0;
    TraceRecorder recorder(a.trace);
    auto root=buildHuffmanTree(frequencies,total,&recorder);
    std::uint8_t lengths[256]{};
    std::vector<CanonicalCode> codes;
    if (!getHuffmanCodeLengths(root,lengths) || !buildCanonicalCodes(lengths,codes)) {
        a.error="Không thể tạo mã Huffman có độ dài tối đa 32 bit."; return a;
    }
    std::array<QString,256> bits;
    for (const auto& c:codes) bits[c.symbol]=codeBits(c.bits,c.length);
    std::vector<unsigned> sorted;
    for(unsigned i=0;i<256;++i) if(frequencies[i]) sorted.push_back(i);
    std::sort(sorted.begin(),sorted.end(),[&](unsigned x,unsigned y) {return frequencies[x]==frequencies[y]?x<y:frequencies[x]>frequencies[y];});
    const auto maxFrequency=sorted.empty()?0:frequencies[sorted.front()];
    for(const unsigned symbol:sorted) {
        QVariantMap row{{"label",printable(symbol)},
                        {"frequency",QVariant::fromValue<qulonglong>(frequencies[symbol])},
                        {"length",int(lengths[symbol])},{"bits",bits[symbol]},
                        {"key", QString::number(symbol)},
                        {"fraction",maxFrequency>0?double(frequencies[symbol])/double(maxFrequency):0.0}};
        a.histogram.push_back(row);
        a.codeRows.push_back(row);
    }
    QString originalRibbon,compressedRibbon;
    for(const unsigned char byte:sample) {
        originalRibbon+=codeBits(byte,8)+" ";
        compressedRibbon+=bits[byte]+" ";
    }
    a.info["originalRibbon"]=originalRibbon.trimmed();
    a.info["compressedRibbon"]=compressedRibbon.trimmed();
    a.info["sampleByteCount"]=int(sample.size());
    a.info["originalLength"]=QVariant::fromValue<qulonglong>(total);
    a.info["symbolCount"]=int(sorted.size());
    a.info["sampleBits"] = total ? QString::number(total*8) : "0";
    if (sorted.empty()) a.info["special"] = "EMPTY CASE / No sorts needed";
    else if(sorted.size()==1) a.info["special"] = "ONE SORT / Single-node tree. Canonical code: 0";
    return a;
}
QVariantMap inspectedMap(const InspectedFile& info) {
    QVariantMap result{{"readable",info.readable},{"valid",info.validHeader},
                       {"version",info.version},{"mode",info.mode},
                       {"fileSize",QVariant::fromValue<qulonglong>(info.fileSize)},
                       {"originalLength",QVariant::fromValue<qulonglong>(info.originalLength)},
                       {"symbolCount",info.symbolCount},{"crc",QString("%1").arg(info.crc32,8,16,QLatin1Char('0')).toUpper()},
                       {"error",fromUtf8(info.error)}};
    result["format"] = info.version==0 ? "PLAIN FILE" : QString("HUF%1 / %2").arg(info.version).arg(info.mode==0?"HUFFMAN":info.mode==1?"STORED":"UNKNOWN");
    if(info.version>0 && info.validHeader && info.originalLength>0)
        result["ratio"] = 100.0*(1.0-double(info.fileSize)/double(info.originalLength));
    QVariantList segments;
    for(const auto& s:info.segments)
        segments.push_back(QVariantMap{{"label",fromUtf8(s.name)},{"offset",QVariant::fromValue<qulonglong>(s.offset)},
                                       {"bytes",QVariant::fromValue<qulonglong>(s.length)}, {"hex",fromUtf8(s.hex)}});
    result["segments"]=segments;
    return result;
}
QString englishError(MessageId id) {
    switch(id) {
    case MessageId::None: return "No error was provided by the codec.";
    case MessageId::CodecOpenSource: return "Cannot open the source file.";
    case MessageId::CodecSameSourceDestination: return "Source and destination refer to the same file.";
    case MessageId::CodecCheckDestination: return "Cannot inspect the destination path.";
    case MessageId::CodecReadSource: return "The source file could not be read completely.";
    case MessageId::CodecSeekSource: return "Cannot seek back to the beginning of the source.";
    case MessageId::CodecSameTempSource: return "The temporary output resolves to the source file.";
    case MessageId::CodecCheckTemp: return "Cannot check the temporary output path.";
    case MessageId::CodecTempExists: return "The .tmp file already exists and cannot be overwritten.";
    case MessageId::CodecCreateOutput: return "Cannot create the output file.";
    case MessageId::CodecRenameTemp: return "Cannot commit the compressed output by renaming its temporary file.";
    case MessageId::CodecHeaderShort: return "The archive header is truncated.";
    case MessageId::CodecFrequencyTableShort: return "The frequency table is truncated.";
    case MessageId::CodecInvalidFrequencyVarint: return "A frequency LEB128 value is invalid.";
    case MessageId::CodecFrequencyOrder: return "Frequency table symbols are repeated or out of order.";
    case MessageId::CodecZeroFrequency: return "A frequency table entry has a zero count.";
    case MessageId::CodecFrequencyOverflow: return "Frequency accumulation exceeds the declared original length.";
    case MessageId::CodecFrequencyTotalMismatch: return "The frequencies do not sum to the original length.";
    case MessageId::CodecUniqueCountTooLarge: return "The header declares more than 256 distinct symbols.";
    case MessageId::CodecInvalidBitSize: return "The packed bit length is inconsistent with the code table.";
    case MessageId::CodecInvalidCodeTable: return "The canonical code-length table is corrupted.";
    case MessageId::CodecBitDataShort: return "The compressed bit stream is truncated.";
    case MessageId::CodecTrailingData: return "Unexpected bytes follow the archived payload.";
    case MessageId::CodecInvalidBitData: return "The compressed bit stream contains invalid or missing data.";
    case MessageId::CodecInvalidTree: return "The Huffman tree cannot be reconstructed.";
    case MessageId::CodecInvalidMode: return "Invalid archive mode: expected HUFFMAN (0) or STORED (1).";
    case MessageId::CodecUnknownFormat: return "Unknown format: expected HUF1, HUF2 or HUF3.";
    case MessageId::CodecStoredDataShort: return "The STORED payload is shorter than declared.";
    case MessageId::CodecWriteOutput: return "Could not write the decompressed output.";
    case MessageId::CodecChecksumMismatch: return "CRC32 mismatch: restored data failed integrity verification.";
    case MessageId::CodecFileSizeUnknown: return "Cannot determine the archive size.";
    case MessageId::CodecReadArchive: return "Cannot read the compressed archive.";
    case MessageId::CodecInternalFailure: return "The compressor could not complete its output.";
    default: return fromUtf8(message(id));
    }
}
QString displayError(MessageId id,const std::string& details,const QString& lang) {
    const QString reason=lang=="en"?englishError(id):fromUtf8(message(id,lang=="ascii"));
    return reason+fromUtf8(details);
}
}
FoundryListModel::FoundryListModel(QObject* parent) : QAbstractListModel(parent) {}
int FoundryListModel::rowCount(const QModelIndex& parent) const {return parent.isValid()?0:int(m_rows.size());}
QVariant FoundryListModel::data(const QModelIndex& index,int role) const {
    if(!index.isValid() || index.row()<0 || index.row()>=m_rows.size()) return {};
    const auto map=m_rows.at(index.row()).toMap();
    switch(role) {
    case LabelRole:return map.value("label");case FrequencyRole:return map.value("frequency");
    case LengthRole:return map.value("length");case BitsRole:return map.value("bits");
    case KeyRole:return map.value("key");case DescriptionRole:return map.value("description");
    }
    return {};
}
QHash<int,QByteArray> FoundryListModel::roleNames() const {
    return {{LabelRole,"symbolLabel"},{FrequencyRole,"frequency"},{LengthRole,"codeLength"},
            {BitsRole,"binaryCode"},{KeyRole,"keyValue"},{DescriptionRole,"description"}};
}
void FoundryListModel::setRows(QVariantList rows) {
    beginResetModel();m_rows=std::move(rows);endResetModel();
}
FoundryController::FoundryController(QObject* parent)
    : QObject(parent),m_settings("TypeFoundry","HuffmanCompressionTool"),m_codes(this),m_heap(this) {
    m_theme=m_settings.value("theme","system").toString();
    m_language=m_settings.value("language","vi").toString();
    bool osReduceMotion=false;
#ifdef Q_OS_WIN
    ANIMATIONINFO animation{sizeof(ANIMATIONINFO),TRUE};
    if(SystemParametersInfoW(SPI_GETANIMATION,sizeof(animation),&animation,0))
        osReduceMotion=!animation.iMinAnimate;
#endif
    m_reduceMotion=m_settings.value("reduceMotion",osReduceMotion).toBool();
    m_recentFiles=m_settings.value("recentFiles").toStringList();
}
FoundryController::~FoundryController() {if(m_cancel) m_cancel->store(true);}
void FoundryController::setProgress(int value) {
    value=qBound(0,value,100);
    if(value>m_progress) {m_progress=value;emit progressChanged();emit desktopProgress(value,m_busy);}
}
void FoundryController::setState(const QString& status,const QString& stage,const QString& detail) {
    m_status=status;m_stage=stage;m_detail=detail;emit changed();
}
void FoundryController::setSourceUrl(const QUrl& url) {setSource(url.isLocalFile()?url.toLocalFile():url.toString());}
void FoundryController::setDestinationUrl(const QUrl& url) {setDestination(url.isLocalFile()?url.toLocalFile():url.toString());}
void FoundryController::rememberFile(const QString& path) {
    m_recentFiles.removeAll(path);m_recentFiles.prepend(path);
    while(m_recentFiles.size()>7)m_recentFiles.removeLast();
    m_settings.setValue("recentFiles",m_recentFiles);
}
void FoundryController::setSource(const QString& path) {
    if(m_busy)return;
    m_source=path;m_destination.clear();m_lastOutput.clear();m_originalHash.clear();m_restoredHash.clear();
    m_verified=false;m_fileInfo.clear();m_histogram.clear();m_trace.clear();m_codes.setRows({});m_heap.setRows({});m_traceIndex=-1;
    if(!path.isEmpty()) {
        rememberFile(path);
        const QString dir=QFileInfo(path).absolutePath();
        const QString base=QFileInfo(path).fileName();
        m_destination= (m_screen==1 ? dir+"/"+base+".restored" : dir+"/"+base+".bin");
        if(m_screen==3) inspect(); else beginAnalysis(path);
    } else setState("empty","AWAITING MATERIAL","Drop a file or choose one from disk.");
    emit changed();
}
void FoundryController::setDestination(const QString& path) {if(m_busy)return;m_destination=path;emit changed();}
void FoundryController::setScreen(int value) {
    if(value<0||value>5 || value==m_screen)return;
    m_screen=value;
    if(!m_source.isEmpty()&&!m_busy) {
        const QString dir=QFileInfo(m_source).absolutePath();
        const QString base=QFileInfo(m_source).fileName();
        m_destination=(m_screen==1?dir+"/"+base+".restored":dir+"/"+base+".bin");
        if(m_screen==3)inspect();
    }
    emit changed();
}
void FoundryController::setTheme(const QString& value) {if(m_theme==value)return;m_theme=value;m_settings.setValue("theme",value);emit changed();}
void FoundryController::setLanguage(const QString& value) {if(m_language==value)return;m_language=value;m_settings.setValue("language",value);emit changed();}
void FoundryController::setReduceMotion(bool value) {m_reduceMotion=value;m_settings.setValue("reduceMotion",value);emit changed();}
void FoundryController::setTraceIndex(int value) {
    m_traceIndex=qBound(-1,value,m_trace.size()-1);
    if(m_traceIndex>=0) {
        const auto step=m_trace.at(m_traceIndex).toMap();
        const auto a=step.value("left").toMap(),b=step.value("right").toMap(),p=step.value("parent").toMap();
        m_heap.setRows({QVariantMap{{"label","POP / A"},{"key",a.value("key")} },
                        QVariantMap{{"label","POP / B"},{"key",b.value("key")} },
                        QVariantMap{{"label","PUSH / PARENT"},{"key",p.value("key")} }});
    } else m_heap.setRows({});
    emit changed();
}
void FoundryController::advanceTrace() {setTraceIndex(m_traceIndex+1);}
void FoundryController::retreatTrace() {setTraceIndex(m_traceIndex-1);}
void FoundryController::moveWindow(QObject* window) {
    if(auto* native=qobject_cast<QWindow*>(window)) native->startSystemMove();
}
void FoundryController::resizeWindow(QObject* window,int edges) {
    if(auto* native=qobject_cast<QWindow*>(window)) native->startSystemResize(Qt::Edges(edges));
}
void FoundryController::setWindowState(int x,int y,int w,int h) {
    m_settings.setValue("window/x",x);m_settings.setValue("window/y",y);
    m_settings.setValue("window/w",w);m_settings.setValue("window/h",h);
}
QVariantMap FoundryController::windowState() const {
    return {{"x",m_settings.value("window/x",-1)},{"y",m_settings.value("window/y",-1)},
            {"w",m_settings.value("window/w",1280)},{"h",m_settings.value("window/h",800)}};
}
QString FoundryController::titleForScreen(int n) const {
    const QStringList t={"SET THE TYPE","RELEASE THE TYPE","PROOF OF RESTORATION","THE HEADER / DISSECTED","OPERATOR'S INDEX","THE WORKSHOP"};
    return n>=0&&n<t.size()?t.at(n):QString();
}
QString FoundryController::helpForScreen(int n) const {
    const QStringList t={"A byte's frequency decides the height of its metal sort. Two lightest sorts form one heavier typeblock.",
                         "Read an archived forme. Rebuild the codebook, recover the original bytes, then check the checksum.",
                         "Compress. Restore a second copy. Compare its SHA-256 with the original; print a proof ticket.",
                         "Examine actual header bytes, offsets and their meaning without parsing any console output.",
                         "Every command has a corresponding terminal invocation. The CLI is still fully supported.",
                         "A deterministic min-heap and canonical Huffman codes, cast with no network or telemetry."};
    return n>=0&&n<t.size()?t.at(n):QString();
}
QString FoundryController::command() const {
    auto quote=[](const QString& path) {QString s=path;s.replace('"',"\\\"");return '"'+s+'"';};
    const QString cli=m_language=="ascii"?"huffman --ascii":"huffman";
    switch(m_screen) {
    case 0:return cli+" -c "+quote(m_source)+" "+quote(m_destination);
    case 1:return cli+" -d "+quote(m_source)+" "+quote(m_destination);
    case 2:return cli+" -v "+quote(m_source)+" "+quote(m_destination);
    case 3:return cli+" -i "+quote(m_source);
    case 4:return cli+" --help";
    default:return cli+" --menu";
    }
}
void FoundryController::copyText(const QString& value) const {QGuiApplication::clipboard()->setText(value);}
void FoundryController::showInFolder() {if(!m_lastOutput.isEmpty())QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_lastOutput).absolutePath()));}
void FoundryController::beginAnalysis(const QString& path) {
    setState("analysis","SETTING THE TYPE","Reading byte frequencies …");
    auto cancelled=std::make_shared<std::atomic_bool>(false);
    m_cancel=cancelled;
    QPointer<FoundryController> self(this);
    QtConcurrent::run([self,path,cancelled] {
        Analysis result=analyze(toUtf8(path),cancelled);
        if(!self)return;
        QMetaObject::invokeMethod(self,[self,path,cancelled,result=std::move(result)]() mutable {
            if(!self || self->m_source!=path || self->m_cancel!=cancelled || self->m_busy)return;
            self->m_histogram=std::move(result.histogram);
            self->m_trace=std::move(result.trace);
            self->m_codes.setRows(std::move(result.codeRows));
            self->m_fileInfo=std::move(result.info);
            self->setTraceIndex(-1);
            self->setState(result.error.isEmpty()?"ready":"error","TYPE CASE / READY",result.error);
        },Qt::QueuedConnection);
    });
}
void FoundryController::inspect() {
    if(m_source.isEmpty() || m_busy)return;
    const QString path=m_source;
    setState("analysis","EXAMINING THE HEADER","Reading file signature and SHA-256 …");
    QPointer<FoundryController> self(this);
    QtConcurrent::run([self,path] {
        auto info=inspectFile(toUtf8(path));
        auto sha=computeFileSha256(toUtf8(path));
        auto map=inspectedMap(info);
        map["sha256"]=sha?fromUtf8(*sha):QString("UNAVAILABLE");
        if(!self)return;
        QMetaObject::invokeMethod(self,[self,path,map=std::move(map)]() mutable {
            if(!self||self->m_source!=path||self->m_busy)return;
            self->m_fileInfo=std::move(map);
            self->setState("ready","HEADER / READ",self->m_fileInfo.value("error").toString());
        },Qt::QueuedConnection);
    });
}
void FoundryController::start() {startInternal(false);}
void FoundryController::cancel() {
    if(m_busy && m_cancel){m_cancel->store(true);setState("processing","STOP REQUESTED","Finishing current I/O block safely …");}
}
void FoundryController::startInternal(bool overwriteApproved) {
    if(m_busy || m_source.isEmpty() || m_screen>3)return;
    if(m_screen==3){inspect();return;}
    if(m_destination.isEmpty()){setState("error","NO OUTPUT","Choose a destination filename.");return;}
    const auto src=toPath(toUtf8(m_source)),dest=toPath(toUtf8(m_destination));
    std::error_code ec;
    if(areSameExistingFile(src,dest,ec) || (!ec && std::filesystem::absolute(src,ec)==std::filesystem::absolute(dest,ec))) {
        setState("error","INVALID DESTINATION","Source and destination refer to the same file.");return;
    }
    if(ec){setState("error","PATH ERROR",fromUtf8(ec.message()));return;}
    if(m_screen==2 && (areSameExistingFile(src,toPath(toUtf8(m_destination+".restored")),ec) ||
       (!ec && std::filesystem::absolute(src,ec)==std::filesystem::absolute(toPath(toUtf8(m_destination+".restored")),ec)))) {
        setState("error","INVALID RESTORE TARGET","The restored output would overwrite the source file.");return;
    }
    if(!overwriteApproved && (std::filesystem::exists(dest,ec) || (m_screen==2 && std::filesystem::exists(toPath(toUtf8(m_destination+".restored")),ec)))) {
        m_confirmSource=m_source;m_confirmDestination=m_destination;
        emit confirmOverwrite();return;
    }
    if(overwriteApproved && (m_confirmSource!=m_source || m_confirmDestination!=m_destination))return;
    m_busy=true;m_progress=0;m_verified=false;m_originalHash.clear();m_restoredHash.clear();
    emit progressChanged();emit desktopProgress(0,true);
    setState("processing","PRESS IN MOTION","Processing in a worker thread …");
    const QString source=m_source,destination=m_destination,lang=m_language;
    const int jobScreen=m_screen;
    auto cancelled=std::make_shared<std::atomic_bool>(false);
    m_cancel=cancelled;
    QPointer<FoundryController> self(this);
    QtConcurrent::run([self,source,destination,lang,jobScreen,cancelled] {
        bool ok=false,wasCancelled=false;
        QString error,originalHash,restoredHash;
        QVariantMap stats;
        const std::string input=toUtf8(source),output=toUtf8(destination);
        try {
            const auto progress=[self,cancelled,jobScreen](std::uint64_t done,std::uint64_t total,int phase) {
                if(cancelled->load())throw Cancelled{};
                int percent=total?int((double(done)/double(total))*100):100;
                percent=qBound(0,percent,100);
                if(jobScreen==2) percent=(phase==0?0:phase==1?33:66)+percent/3;
                if(!self)return;
                QMetaObject::invokeMethod(self,[self,percent,phase] {
                    if(!self)return;
                    self->setProgress(percent);
                    self->m_stage=phase==0?"COMPRESSION":phase==1?"RESTORATION":"CHECKSUM";
                    emit self->changed();
                },Qt::QueuedConnection);
            };
            Compressor compressor; Decompressor decompressor;
            if(jobScreen==0 || jobScreen==2) {
                if(jobScreen==2) {
                    auto hash=computeFileSha256(input);
                    if(!hash)throw std::runtime_error("Cannot hash original file");
                    originalHash=fromUtf8(*hash);
                    if(cancelled->load())throw Cancelled{};
                }
                ok=compressor.compressWithProgress(input,output,[&](auto done,auto total){progress(done,total,jobScreen==2?0:0);});
                if(!ok)error=displayError(compressor.lastError(),compressor.lastErrorDetails(),lang);
                if(ok) {
                    const auto inputSize=std::filesystem::file_size(toPath(input));
                    const auto compressedSize=std::filesystem::file_size(toPath(output));
                    stats["originalSize"]=QVariant::fromValue<qulonglong>(inputSize);
                    stats["compressedSize"]=QVariant::fromValue<qulonglong>(compressedSize);
                    stats["ratio"]= inputSize?100.0*(1.0-double(compressedSize)/double(inputSize)):0.0;
                    const auto info=inspectFile(output);
                    stats["format"]=inspectedMap(info).value("format");
                }
                if(ok && jobScreen==2) {
                    const std::string restored=output+".restored";
                    if(cancelled->load())throw Cancelled{};
                    ok=decompressor.decompressWithProgress(output,restored,[&](auto done,auto total){progress(done,total,1);});
                    if(!ok)error=displayError(decompressor.lastError(),decompressor.lastErrorDetails(),lang);
                    if(ok) {
                        if(cancelled->load())throw Cancelled{};
                        auto hash=computeFileSha256(restored);
                        if(!hash)throw std::runtime_error("Cannot hash restored file");
                        restoredHash=fromUtf8(*hash);
                        ok=originalHash==restoredHash;
                        if(!ok)error="SHA-256 mismatch: restored data differs from the source.";
                        stats["restoredPath"]=fromUtf8(restored);
                        progress(1,1,2);
                    }
                }
            } else {
                ok=decompressor.decompressWithProgress(input,output,[&](auto done,auto total){progress(done,total,0);});
                if(!ok)error=displayError(decompressor.lastError(),decompressor.lastErrorDetails(),lang);
            }
            if(cancelled->load())throw Cancelled{};
        } catch(const Cancelled&) {wasCancelled=true; error="Operation cancelled. Incomplete output was removed.";}
          catch(const std::exception& e){error=QString::fromUtf8(e.what());}
        if(wasCancelled) {
            std::error_code ec;
            std::filesystem::remove(toPath(output+".tmp"),ec);
            // Decompressor's OutputGuard owns removal of incomplete output.
            if(jobScreen==2)std::filesystem::remove(toPath(output+".restored"),ec);
        }
        if(!self)return;
        QMetaObject::invokeMethod(self,[self,ok,wasCancelled,error,originalHash,restoredHash,stats=std::move(stats)]() mutable {
            if(self)self->finishJob(ok,wasCancelled,error,originalHash,restoredHash,stats);
        },Qt::QueuedConnection);
    });
}
void FoundryController::finishJob(bool ok,bool cancelled,const QString& error,const QString& originalHash,
                                  const QString& restoredHash,const QVariantMap& stats) {
    m_busy=false;m_originalHash=originalHash;m_restoredHash=restoredHash;
    m_verified=(m_screen==2 && ok && !originalHash.isEmpty() && originalHash==restoredHash);
    if(ok) {
        m_lastOutput=m_destination;
        for(auto it=stats.cbegin();it!=stats.cend();++it)m_fileInfo[it.key()]=it.value();
        setProgress(100);
        const bool stored=m_fileInfo.value("format").toString().contains("STORED");
        setState("success",m_screen==2?"CERTIFICATE / VERIFIED":"JOB / COMPLETE",
                 m_screen==2?"SHA-256 comparison passed. Every restored byte matches.":
                 stored?"STORED: the payload is kept uncompressed because the Huffman header would cost more. Size is at most the original plus 17 bytes.":
                        "Written to disk. The Huffman-coded output is ready.");
        emit completed();
    } else if(cancelled) setState("cancelled","JOB / CANCELLED",error);
    else setState("error","JOB / FAILED",error+ (m_screen==1?" Incomplete output was deleted.":""));
    emit desktopProgress(0,false);
}
