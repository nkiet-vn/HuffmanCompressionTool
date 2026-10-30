#pragma once
#include <QAbstractListModel>
#include <QObject>
#include <QSettings>
#include <QVariantList>
#include <QVariantMap>
#include <QStringList>
#include <atomic>
#include <memory>

// Data originates in the C++ core, never in QML.
class FoundryListModel final : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles { LabelRole = Qt::UserRole + 1, FrequencyRole, LengthRole, BitsRole, KeyRole, DescriptionRole };
    explicit FoundryListModel(QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int,QByteArray> roleNames() const override;
    void setRows(QVariantList rows);
private:
    QVariantList m_rows;
};

class FoundryController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString source READ source NOTIFY changed)
    Q_PROPERTY(QString destination READ destination NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString stage READ stage NOTIFY changed)
    Q_PROPERTY(QString detail READ detail NOTIFY changed)
    Q_PROPERTY(QString lastOutput READ lastOutput NOTIFY changed)
    Q_PROPERTY(QString originalHash READ originalHash NOTIFY changed)
    Q_PROPERTY(QString restoredHash READ restoredHash NOTIFY changed)
    Q_PROPERTY(QString command READ command NOTIFY changed)
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY changed)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY changed)
    Q_PROPERTY(bool reduceMotion READ reduceMotion WRITE setReduceMotion NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool verified READ verified NOTIFY changed)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(int screen READ screen WRITE setScreen NOTIFY changed)
    Q_PROPERTY(int traceIndex READ traceIndex WRITE setTraceIndex NOTIFY changed)
    Q_PROPERTY(QVariantList histogram READ histogram NOTIFY changed)
    Q_PROPERTY(QVariantList trace READ trace NOTIFY changed)
    Q_PROPERTY(QVariantMap fileInfo READ fileInfo NOTIFY changed)
    Q_PROPERTY(QStringList recentFiles READ recentFiles NOTIFY changed)
    Q_PROPERTY(QAbstractItemModel* codeModel READ codeModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel* heapModel READ heapModel CONSTANT)
public:
    explicit FoundryController(QObject* parent=nullptr);
    ~FoundryController() override;
    QString source() const { return m_source; }
    QString destination() const { return m_destination; }
    QString status() const { return m_status; }
    QString stage() const { return m_stage; }
    QString detail() const { return m_detail; }
    QString lastOutput() const { return m_lastOutput; }
    QString originalHash() const { return m_originalHash; }
    QString restoredHash() const { return m_restoredHash; }
    QString command() const;
    QString theme() const { return m_theme; }
    QString language() const { return m_language; }
    bool reduceMotion() const { return m_reduceMotion; }
    bool busy() const { return m_busy; }
    bool verified() const { return m_verified; }
    int progress() const { return m_progress; }
    int screen() const { return m_screen; }
    int traceIndex() const { return m_traceIndex; }
    QVariantList histogram() const { return m_histogram; }
    QVariantList trace() const { return m_trace; }
    QVariantMap fileInfo() const { return m_fileInfo; }
    QStringList recentFiles() const { return m_recentFiles; }
    QAbstractItemModel* codeModel() { return &m_codes; }
    QAbstractItemModel* heapModel() { return &m_heap; }
    Q_INVOKABLE void setSourceUrl(const QUrl& url);
    Q_INVOKABLE void setDestinationUrl(const QUrl& url);
    Q_INVOKABLE void setSource(const QString& path);
    Q_INVOKABLE void setDestination(const QString& path);
    Q_INVOKABLE void start();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void inspect();
    Q_INVOKABLE void showInFolder();
    Q_INVOKABLE void advanceTrace();
    Q_INVOKABLE void retreatTrace();
    Q_INVOKABLE QString titleForScreen(int screen) const;
    Q_INVOKABLE QString helpForScreen(int screen) const;
    Q_INVOKABLE void copyText(const QString& value) const;
    void setScreen(int value);
    void setTraceIndex(int value);
    void setTheme(const QString& value);
    void setLanguage(const QString& value);
    void setReduceMotion(bool value);
    Q_INVOKABLE void setWindowState(int x, int y, int w, int h);
    Q_INVOKABLE void moveWindow(QObject* window);
    Q_INVOKABLE void resizeWindow(QObject* window, int edges);
    QVariantMap windowState() const;

signals:
    void changed();
    void progressChanged();
    void confirmOverwrite();
    void completed();
    void desktopProgress(int percent, bool active);

private:
    void beginAnalysis(const QString& path);
    void finishJob(bool ok, bool cancelled, const QString& error, const QString& originalHash,
                   const QString& restoredHash, const QVariantMap& stats);
    void rememberFile(const QString& path);
    void setState(const QString& status, const QString& stage, const QString& detail = {});
    void setProgress(int value);
    void startInternal(bool overwriteApproved);
    QSettings m_settings;
    QString m_source, m_destination, m_status = "empty", m_stage, m_detail, m_lastOutput;
    QString m_originalHash, m_restoredHash, m_theme, m_language;
    bool m_reduceMotion = false, m_busy = false, m_verified = false;
    int m_progress = 0, m_screen = 0, m_traceIndex = -1;
    QVariantList m_histogram, m_trace;
    QVariantMap m_fileInfo;
    QStringList m_recentFiles;
    FoundryListModel m_codes, m_heap;
    std::shared_ptr<std::atomic_bool> m_cancel;
    QString m_confirmSource, m_confirmDestination;
public:
    Q_INVOKABLE void approveOverwrite() { startInternal(true); }
};
