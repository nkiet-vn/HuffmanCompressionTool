#include "controllers/FoundryController.h"
#include <QApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QQuickWindow>
#include <QSystemTrayIcon>
#include <QWindow>
#include <QAbstractNativeEventFilter>
#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#include <shobjidl.h>

class WindowsSnapFilter final : public QAbstractNativeEventFilter {
public:
    explicit WindowsSnapFilter(QWindow* window) : m_window(window) {}
    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override {
        if(eventType != "windows_generic_MSG" || !m_window)return false;
        auto* msg = static_cast<MSG*>(message);
        if(msg->message != WM_NCHITTEST || msg->hwnd != reinterpret_cast<HWND>(m_window->winId()))return false;
        POINT p{GET_X_LPARAM(msg->lParam),GET_Y_LPARAM(msg->lParam)};
        ScreenToClient(msg->hwnd,&p);
        // Native HTMAXBUTTON allows the Windows 11 Snap Layouts flyout.
        const qreal scale=m_window->devicePixelRatio();
        const int clientWidth=qRound(m_window->width()*scale);
        if(p.y >= 0 && p.y < qRound(59*scale) &&
           p.x >= clientWidth-qRound(86*scale) && p.x < clientWidth-qRound(43*scale)) {
            *result = HTMAXBUTTON;return true;
        }
        return false;
    }
private:
    QWindow* m_window;
};
#endif
int main(int argc,char* argv[]) {
    QApplication app(argc,argv);
    app.setApplicationName("The Type Foundry");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("TypeFoundry");
    app.setWindowIcon(QIcon(":/qt/qml/TypeFoundry/resources/type-sort.png"));
    FoundryController controller;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("foundry",&controller);
    engine.loadFromModule("TypeFoundry","Main");
    if(engine.rootObjects().isEmpty())return 2;
    auto* window=qobject_cast<QWindow*>(engine.rootObjects().first());
#ifdef Q_OS_WIN
    const HRESULT comInit=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    WindowsSnapFilter snapFilter(window);
    app.installNativeEventFilter(&snapFilter);
#endif
    QSystemTrayIcon tray(app.windowIcon());
    tray.setToolTip("The Type Foundry");
    if(QSystemTrayIcon::isSystemTrayAvailable())tray.show();
    QObject::connect(&controller,&FoundryController::completed,&tray,[&] {
        if(window && !window->isActive() && tray.isVisible())
            tray.showMessage("The Type Foundry","Your file is ready.",QSystemTrayIcon::Information,5000);
    });
#ifdef Q_OS_WIN
    QObject::connect(&controller,&FoundryController::desktopProgress,window,[window](int percent,bool active) {
        ITaskbarList3* taskbar=nullptr;
        if(SUCCEEDED(CoCreateInstance(CLSID_TaskbarList,nullptr,CLSCTX_INPROC_SERVER,
                                     IID_ITaskbarList3,reinterpret_cast<void**>(&taskbar)))) {
            HWND handle=reinterpret_cast<HWND>(window->winId());
            taskbar->HrInit();
            taskbar->SetProgressState(handle,active?TBPF_NORMAL:TBPF_NOPROGRESS);
            if(active)taskbar->SetProgressValue(handle,static_cast<ULONGLONG>(percent),100);
            taskbar->Release();
        }
    });
#endif
    const int result=app.exec();
#ifdef Q_OS_WIN
    app.removeNativeEventFilter(&snapFilter);
    if(SUCCEEDED(comInit))CoUninitialize();
#endif
    return result;
}
