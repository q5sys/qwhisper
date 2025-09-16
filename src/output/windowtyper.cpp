#include "windowtyper.h"
#include <QDebug>
#include <QProcess>
#include <QThread>
#include <QCoreApplication>

#ifdef Q_OS_LINUX
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <unistd.h>
#endif

WindowTyper::WindowTyper(QObject *parent)
    : QObject(parent)
    , m_enabled(false)
    , m_typingDelay(10)  // 10ms between characters
    , m_lineDelay(100)   // 100ms between lines
    , m_autoReturn(true)
    , m_backend(Backend_None)
    , m_timer(new QTimer(this))
    , m_display(nullptr)
{
    connect(m_timer, &QTimer::timeout, this, &WindowTyper::processNextLine);
    detectBackend();
}

WindowTyper::~WindowTyper()
{
#ifdef Q_OS_LINUX
    if (m_display) {
        XCloseDisplay(static_cast<Display*>(m_display));
    }
#endif
}

void WindowTyper::detectBackend()
{
#ifdef Q_OS_LINUX
    // Check if we're running under Wayland
    const char* waylandDisplay = getenv("WAYLAND_DISPLAY");
    const char* sessionType = getenv("XDG_SESSION_TYPE");
    
    bool isWayland = (waylandDisplay != nullptr) || 
                     (sessionType && strcmp(sessionType, "wayland") == 0);
    
    if (isWayland) {
        // On Wayland, first check if XWayland is available and xdotool works
        // This is preferred because it's more reliable than native Wayland tools
        QProcess checkXdotool;
        checkXdotool.start("which", QStringList() << "xdotool");
        checkXdotool.waitForFinished(1000);
        
        if (checkXdotool.exitCode() == 0) {
            // Test if xdotool actually works (XWayland is running)
            QProcess testXdotool;
            testXdotool.start("xdotool", QStringList() << "getmouselocation");
            testXdotool.waitForFinished(1000);
            
            if (testXdotool.exitCode() == 0) {
                // xdotool works via XWayland - use it
                m_backend = Backend_X11;  // Treat it as X11 backend
                qDebug() << "WindowTyper: Using xdotool via XWayland on Wayland session";
                
                // Try to open X11 display for direct X11 access
                Display* display = XOpenDisplay(nullptr);
                if (display) {
                    m_display = display;
                    int eventBase, errorBase, majorVersion, minorVersion;
                    if (!XTestQueryExtension(display, &eventBase, &errorBase, &majorVersion, &minorVersion)) {
                        // XTest not available, will fall back to xdotool command
                        XCloseDisplay(display);
                        m_display = nullptr;
                    }
                }
            } else {
                // xdotool doesn't work, try native Wayland tools
                
                // Check for ydotool availability
                QProcess checkYdotool;
                checkYdotool.start("which", QStringList() << "ydotool");
                checkYdotool.waitForFinished(1000);
                
                if (checkYdotool.exitCode() == 0) {
                    m_backend = Backend_Wayland_YDotool;
                    qDebug() << "WindowTyper: Using ydotool backend for Wayland";
                } else {
                    // Check for wtype as alternative
                    QProcess checkWtype;
                    checkWtype.start("which", QStringList() << "wtype");
                    checkWtype.waitForFinished(1000);
                    
                    if (checkWtype.exitCode() == 0) {
                        m_backend = Backend_Wayland_WType;
                        qDebug() << "WindowTyper: Using wtype backend for Wayland";
                    } else {
                        qWarning() << "WindowTyper: Wayland detected but no working typing tool found";
                        qWarning() << "WindowTyper: Install xdotool (for XWayland), ydotool, or wtype";
                        m_backend = Backend_None;
                    }
                }
            }
        } else {
            // No xdotool, try native Wayland tools
            // Check for ydotool availability
            QProcess checkYdotool;
            checkYdotool.start("which", QStringList() << "ydotool");
            checkYdotool.waitForFinished(1000);
            
            if (checkYdotool.exitCode() == 0) {
                m_backend = Backend_Wayland_YDotool;
                qDebug() << "WindowTyper: Using ydotool backend for Wayland";
            } else {
                // Check for wtype as alternative
                QProcess checkWtype;
                checkWtype.start("which", QStringList() << "wtype");
                checkWtype.waitForFinished(1000);
                
                if (checkWtype.exitCode() == 0) {
                    m_backend = Backend_Wayland_WType;
                    qDebug() << "WindowTyper: Using wtype backend for Wayland";
                } else {
                    qWarning() << "WindowTyper: Wayland detected but no typing tool found";
                    m_backend = Backend_None;
                }
            }
        }
    } else {
        // Pure X11 session
        // Try to open X11 display
        Display* display = XOpenDisplay(nullptr);
        if (display) {
            m_display = display;
            
            // Check if XTest extension is available
            int eventBase, errorBase, majorVersion, minorVersion;
            if (XTestQueryExtension(display, &eventBase, &errorBase, &majorVersion, &minorVersion)) {
                m_backend = Backend_X11;
                qDebug() << "WindowTyper: Using X11/XTest backend";
            } else {
                qWarning() << "WindowTyper: X11 display opened but XTest extension not available";
                XCloseDisplay(display);
                m_display = nullptr;
                m_backend = Backend_None;
            }
        } else {
            qWarning() << "WindowTyper: Could not open X11 display";
            m_backend = Backend_None;
        }
    }
#else
    qWarning() << "WindowTyper: Not supported on this platform";
    m_backend = Backend_None;
#endif
}

void WindowTyper::typeText(const QString &text)
{
    if (!m_enabled || m_backend == Backend_None) {
        if (m_backend == Backend_None) {
            emit typingError("Window typing not available on this system");
        }
        return;
    }
    
    // Split text into lines
    m_pendingLines = text.split('\n', Qt::KeepEmptyParts);
    
    if (m_pendingLines.isEmpty()) {
        return;
    }
    
    emit typingStarted();
    
    // Process first line immediately
    processNextLine();
}

void WindowTyper::processNextLine()
{
    if (m_pendingLines.isEmpty()) {
        emit typingFinished();
        m_timer->stop();
        return;
    }
    
    QString line = m_pendingLines.takeFirst();
    
    // Type the line
    simulateKeyPress(line);
    
    // Add return if needed
    if (m_autoReturn) {
        simulateReturn();
    }
    
    // Schedule next line if any
    if (!m_pendingLines.isEmpty()) {
        m_timer->start(m_lineDelay);
    } else {
        emit typingFinished();
    }
}

void WindowTyper::simulateKeyPress(const QString &text)
{
    switch (m_backend) {
        case Backend_X11:
            typeTextX11(text);
            break;
        case Backend_Wayland_YDotool:
        case Backend_Wayland_WType:
            typeTextWayland(text);
            break;
        default:
            typeTextFallback(text);
            break;
    }
}

void WindowTyper::simulateReturn()
{
#ifdef Q_OS_LINUX
    if (m_backend == Backend_X11) {
        if (m_display) {
            Display* display = static_cast<Display*>(m_display);
            
            // Simulate Return key press using XTest
            KeyCode keycode = XKeysymToKeycode(display, XK_Return);
            XTestFakeKeyEvent(display, keycode, True, 0);  // Key press
            XTestFakeKeyEvent(display, keycode, False, 0); // Key release
            XFlush(display);
        } else {
            // Use xdotool command as fallback (XWayland case)
            QProcess::execute("xdotool", QStringList() << "key" << "Return");
        }
    } else if (m_backend == Backend_Wayland_YDotool) {
        QProcess::execute("ydotool", QStringList() << "key" << "Return");
        
    } else if (m_backend == Backend_Wayland_WType) {
        QProcess::execute("wtype", QStringList() << "-k" << "Return");
    }
#endif
}

bool WindowTyper::typeTextX11(const QString &text)
{
#ifdef Q_OS_LINUX
    // If we don't have a display pointer, use xdotool command (XWayland case)
    if (!m_display) {
        // Use xdotool command as fallback (works with XWayland)
        QProcess process;
        process.start("xdotool", QStringList() << "type" << "--" << text);
        if (!process.waitForFinished(5000)) {
            qWarning() << "xdotool command timed out";
            return false;
        }
        return process.exitCode() == 0;
    }
    
    Display* display = static_cast<Display*>(m_display);
    
    // Type each character using XTest
    for (const QChar &ch : text) {
        // Convert QString character to KeySym
        KeySym keysym = 0;
        
        if (ch.unicode() < 128) {
            // ASCII character
            keysym = ch.unicode();
        } else {
            // Unicode character - use Unicode keysym
            keysym = 0x01000000 | ch.unicode();
        }
        
        // Get the keycode for this keysym
        KeyCode keycode = XKeysymToKeycode(display, keysym);
        
        if (keycode != 0) {
            // Check if we need shift
            bool needShift = false;
            if (ch.isUpper() || QString("!@#$%^&*()_+{}|:\"<>?").contains(ch)) {
                needShift = true;
            }
            
            if (needShift) {
                KeyCode shiftCode = XKeysymToKeycode(display, XK_Shift_L);
                XTestFakeKeyEvent(display, shiftCode, True, 0);
            }
            
            // Send key press and release
            XTestFakeKeyEvent(display, keycode, True, 0);
            XTestFakeKeyEvent(display, keycode, False, 0);
            
            if (needShift) {
                KeyCode shiftCode = XKeysymToKeycode(display, XK_Shift_L);
                XTestFakeKeyEvent(display, shiftCode, False, 0);
            }
            
            XFlush(display);
            
            // Small delay between characters
            if (m_typingDelay > 0) {
                QThread::msleep(m_typingDelay);
            }
        } else {
            // If we can't find a keycode, try using xdotool as fallback
            QProcess::execute("xdotool", QStringList() << "type" << QString(ch));
        }
    }
    
    return true;
#else
    Q_UNUSED(text)
    return false;
#endif
}

bool WindowTyper::typeTextWayland(const QString &text)
{
#ifdef Q_OS_LINUX
    if (m_backend == Backend_Wayland_YDotool) {
        // Use ydotool to type text
        // Note: ydotool requires ydotoold daemon to be running
        QProcess process;
        process.start("ydotool", QStringList() << "type" << text);
        if (!process.waitForFinished(5000)) {
            qWarning() << "ydotool command timed out";
            return false;
        }
        return process.exitCode() == 0;
        
    } else if (m_backend == Backend_Wayland_WType) {
        // Use wtype to type text
        QProcess process;
        process.start("wtype", QStringList() << text);
        if (!process.waitForFinished(5000)) {
            qWarning() << "wtype command timed out";
            return false;
        }
        return process.exitCode() == 0;
    }
#else
    Q_UNUSED(text)
#endif
    return false;
}

bool WindowTyper::typeTextFallback(const QString &text)
{
#ifdef Q_OS_LINUX
    // Try xdotool as last resort (works on X11, might work on XWayland)
    QProcess process;
    process.start("xdotool", QStringList() << "type" << "--" << text);
    if (!process.waitForFinished(5000)) {
        qWarning() << "xdotool command timed out";
        return false;
    }
    return process.exitCode() == 0;
#else
    Q_UNUSED(text)
    qWarning() << "No typing backend available";
    return false;
#endif
}

void WindowTyper::setEnabled(bool enabled)
{
    m_enabled = enabled;
    
    if (!enabled && m_timer->isActive()) {
        m_timer->stop();
        m_pendingLines.clear();
    }
}

bool WindowTyper::isAvailable()
{
    WindowTyper temp;
    return temp.m_backend != Backend_None;
}

QString WindowTyper::availabilityMessage()
{
    WindowTyper temp;
    
    switch (temp.m_backend) {
        case Backend_X11:
            return "Window typing available via X11/XTest";
        case Backend_Wayland_YDotool:
            return "Window typing available via ydotool (Wayland)";
        case Backend_Wayland_WType:
            return "Window typing available via wtype (Wayland)";
        case Backend_None:
        default:
            break;
    }
    
#ifdef Q_OS_LINUX
    const char* sessionType = getenv("XDG_SESSION_TYPE");
    if (sessionType && strcmp(sessionType, "wayland") == 0) {
        return "Wayland detected but no typing tool found. Install ydotool or wtype.";
    } else {
        return "Window typing not available. X11/XTest extension may be missing.";
    }
#else
    return "Window typing not supported on this platform";
#endif
}
