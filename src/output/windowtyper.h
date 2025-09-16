#ifndef WINDOWTYPER_H
#define WINDOWTYPER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QStringList>
#include <memory>

class WindowTyper : public QObject
{
    Q_OBJECT

public:
    explicit WindowTyper(QObject *parent = nullptr);
    ~WindowTyper();
    
    void typeText(const QString &text);
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }
    
    // Configuration options
    void setTypingDelay(int msec) { m_typingDelay = msec; }
    int typingDelay() const { return m_typingDelay; }
    
    void setLineDelay(int msec) { m_lineDelay = msec; }
    int lineDelay() const { return m_lineDelay; }
    
    void setAutoReturn(bool enabled) { m_autoReturn = enabled; }
    bool autoReturn() const { return m_autoReturn; }
    
    // Check if window typing is available on this system
    static bool isAvailable();
    static QString availabilityMessage();

signals:
    void typingStarted();
    void typingFinished();
    void typingError(const QString &error);

private slots:
    void processNextLine();

private:
    enum Backend {
        Backend_None,
        Backend_X11,
        Backend_Wayland_YDotool,
        Backend_Wayland_WType
    };
    
    void detectBackend();
    bool typeTextX11(const QString &text);
    bool typeTextWayland(const QString &text);
    bool typeTextFallback(const QString &text);
    void simulateKeyPress(const QString &text);
    void simulateReturn();
    
    bool m_enabled;
    int m_typingDelay;  // Delay between characters (ms)
    int m_lineDelay;    // Delay between lines (ms)
    bool m_autoReturn;  // Automatically press Return after each line
    
    Backend m_backend;
    QTimer *m_timer;
    QStringList m_pendingLines;
    
    // Platform-specific handles
    void *m_display;  // X11 Display pointer (void* to avoid X11 headers in .h)
};

#endif // WINDOWTYPER_H
