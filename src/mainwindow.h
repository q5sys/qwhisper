#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <memory>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QToolBar;
class QStatusBar;
class QSplitter;
QT_END_NAMESPACE

class ConfigWidget;
class TranscriptWidget;
class AudioMonitor;
class AudioCapture;
class AudioProcessor;
class WhisperProcessor;
class OutputManager;
class ModelDownloader;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void startRecording();
    void stopRecording();
    void pauseRecording();

private slots:
    void onStartRecording();
    void onStopRecording();
    void onPauseRecording();
    void onTranscriptionReceived(const QString &text, qint64 timestamp);
    void onAudioLevelChanged(float level);
    void onStatusChanged(const QString &status);
    void onAbout();
    void onSettings();
    void saveSettings();
    void loadSettings();
    void onModelNotFound(const QString &modelName);
    void onModelDownloadComplete(const QString &modelName, const QString &filePath);
    void onModelDownloadFailed(const QString &modelName, const QString &error);

private:
    void setupUi();
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void connectSignals();
    
    // UI Components
    ConfigWidget *m_configWidget;
    TranscriptWidget *m_transcriptWidget;
    AudioMonitor *m_audioMonitor;
    QSplitter *m_centralSplitter;
    
    // Core Components
    std::unique_ptr<AudioCapture> m_audioCapture;
    std::unique_ptr<AudioProcessor> m_audioProcessor;
    std::unique_ptr<WhisperProcessor> m_whisperProcessor;
    std::unique_ptr<OutputManager> m_outputManager;
    std::unique_ptr<ModelDownloader> m_modelDownloader;
    
    // Threads
    QThread *m_audioThread;
    QThread *m_whisperThread;
    
    // Actions
    QAction *m_startAction;
    QAction *m_stopAction;
    QAction *m_pauseAction;
    QAction *m_exitAction;
    QAction *m_aboutAction;
    QAction *m_settingsAction;
    QAction *m_saveTranscriptAction;
    QAction *m_clearTranscriptAction;
    
    // Menus
    QMenu *m_fileMenu;
    QMenu *m_editMenu;
    QMenu *m_viewMenu;
    QMenu *m_helpMenu;
    
    // Toolbars
    QToolBar *m_mainToolBar;
    
    // State
    bool m_isRecording;
    bool m_isPaused;
};

#endif // MAINWINDOW_H
