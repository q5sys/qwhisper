#include "mainwindow.h"
#include "ui/configwidget.h"
#include "ui/transcriptwidget.h"
#include "ui/audiomonitor.h"
#include "ui/settingsdialog.h"
#include "audio/audiocapture.h"
#include "audio/audioprocessor.h"
#include "whisper/whisperprocessor.h"
#include "whisper/modeldownloader.h"
#include "output/outputmanager.h"

#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QTimer>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_isRecording(false)
    , m_isPaused(false)
{
    setupUi();
    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    
    // Initialize core components
    m_audioCapture = std::make_unique<AudioCapture>();
    m_audioProcessor = std::make_unique<AudioProcessor>();
    m_whisperProcessor = std::make_unique<WhisperProcessor>();
    m_outputManager = std::make_unique<OutputManager>();
    m_modelDownloader = std::make_unique<ModelDownloader>();
    
    // Setup threads
    m_audioThread = new QThread(this);
    m_whisperThread = new QThread(this);
    
    m_audioCapture->moveToThread(m_audioThread);
    m_whisperProcessor->moveToThread(m_whisperThread);
    
    connectSignals();
    loadSettings();
    
    // Start threads
    m_audioThread->start();
    m_whisperThread->start();
    
    setWindowTitle("QWhisper - Real-time Speech Recognition");
    resize(1200, 800);
}

MainWindow::~MainWindow()
{
    saveSettings();
    
    // Stop threads
    if (m_audioThread->isRunning()) {
        m_audioThread->quit();
        m_audioThread->wait();
    }
    
    if (m_whisperThread->isRunning()) {
        m_whisperThread->quit();
        m_whisperThread->wait();
    }
}

void MainWindow::setupUi()
{
    // Create central widget and layout
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Create main horizontal splitter
    m_centralSplitter = new QSplitter(Qt::Horizontal, this);
    
    // Create config widget (left panel)
    m_configWidget = new ConfigWidget(this);
    m_configWidget->setMaximumWidth(350);
    
    // Create transcript widget (right panel)
    m_transcriptWidget = new TranscriptWidget(this);
    
    // Add widgets to splitter
    m_centralSplitter->addWidget(m_configWidget);
    m_centralSplitter->addWidget(m_transcriptWidget);
    m_centralSplitter->setStretchFactor(0, 0);
    m_centralSplitter->setStretchFactor(1, 1);
    
    // Create audio monitor (bottom)
    m_audioMonitor = new AudioMonitor(this);
    m_audioMonitor->setMaximumHeight(60);
    
    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->addWidget(m_centralSplitter);
    mainLayout->addWidget(m_audioMonitor);
    mainLayout->setContentsMargins(5, 5, 5, 5);
}

void MainWindow::createActions()
{
    // File actions
    m_startAction = new QAction(QIcon(":/icons/start.png"), tr("&Start"), this);
    m_startAction->setShortcut(QKeySequence("F5"));
    m_startAction->setStatusTip(tr("Start recording"));
    connect(m_startAction, &QAction::triggered, this, &MainWindow::onStartRecording);
    
    m_stopAction = new QAction(QIcon(":/icons/stop.png"), tr("S&top"), this);
    m_stopAction->setShortcut(QKeySequence("F6"));
    m_stopAction->setStatusTip(tr("Stop recording"));
    m_stopAction->setEnabled(false);
    connect(m_stopAction, &QAction::triggered, this, &MainWindow::onStopRecording);
    
    m_pauseAction = new QAction(QIcon(":/icons/pause.png"), tr("&Pause"), this);
    m_pauseAction->setShortcut(QKeySequence("F7"));
    m_pauseAction->setStatusTip(tr("Pause recording"));
    m_pauseAction->setEnabled(false);
    connect(m_pauseAction, &QAction::triggered, this, &MainWindow::onPauseRecording);
    
    m_saveTranscriptAction = new QAction(QIcon(":/icons/save.png"), tr("&Save Transcript"), this);
    m_saveTranscriptAction->setShortcut(QKeySequence::Save);
    m_saveTranscriptAction->setStatusTip(tr("Save transcript to file"));
    connect(m_saveTranscriptAction, &QAction::triggered, m_transcriptWidget, &TranscriptWidget::saveTranscript);
    
    m_clearTranscriptAction = new QAction(QIcon(":/icons/clear.png"), tr("&Clear Transcript"), this);
    m_clearTranscriptAction->setShortcut(QKeySequence("Ctrl+L"));
    m_clearTranscriptAction->setStatusTip(tr("Clear transcript"));
    connect(m_clearTranscriptAction, &QAction::triggered, m_transcriptWidget, &TranscriptWidget::clearTranscript);
    
    m_exitAction = new QAction(tr("E&xit"), this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setStatusTip(tr("Exit the application"));
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
    
    // Edit actions
    m_settingsAction = new QAction(QIcon(":/icons/settings.png"), tr("&Settings"), this);
    m_settingsAction->setShortcut(QKeySequence::Preferences);
    m_settingsAction->setStatusTip(tr("Configure application settings"));
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::onSettings);
    
    // Help actions
    m_aboutAction = new QAction(tr("&About"), this);
    m_aboutAction->setStatusTip(tr("Show the application's About box"));
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::createMenus()
{
    // File menu
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_startAction);
    m_fileMenu->addAction(m_stopAction);
    m_fileMenu->addAction(m_pauseAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_saveTranscriptAction);
    m_fileMenu->addAction(m_clearTranscriptAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exitAction);
    
    // Edit menu
    m_editMenu = menuBar()->addMenu(tr("&Edit"));
    m_editMenu->addAction(m_settingsAction);
    
    // View menu
    m_viewMenu = menuBar()->addMenu(tr("&View"));
    // Add view options here
    
    // Help menu
    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_aboutAction);
}

void MainWindow::createToolBars()
{
    m_mainToolBar = addToolBar(tr("Main"));
    m_mainToolBar->addAction(m_startAction);
    m_mainToolBar->addAction(m_stopAction);
    m_mainToolBar->addAction(m_pauseAction);
    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(m_saveTranscriptAction);
    m_mainToolBar->addAction(m_clearTranscriptAction);
    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(m_settingsAction);
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::connectSignals()
{
    // Connect config widget to audio capture
    connect(m_configWidget, &ConfigWidget::configurationChanged,
            m_audioCapture.get(), &AudioCapture::updateConfiguration);
    
    // Connect config widget timestamp setting to transcript widget
    connect(m_configWidget, &ConfigWidget::configurationChanged,
            [this](const AudioConfiguration &config) {
                m_transcriptWidget->setShowTimestamps(config.includeTimestamps);
            });
    
    // Connect config widget to whisper processor
    connect(m_configWidget, &ConfigWidget::modelChanged,
            m_whisperProcessor.get(), &WhisperProcessor::loadModel);
    
    // Connect audio capture to audio processor (with filtering)
    connect(m_audioCapture.get(), &AudioCapture::audioDataReady,
            m_audioProcessor.get(), &AudioProcessor::processAudioData);
    
    // Connect audio processor to whisper processor (filtered audio)
    connect(m_audioProcessor.get(), &AudioProcessor::processedAudio,
            m_whisperProcessor.get(), &WhisperProcessor::processAudio);
    
    // Connect audio capture to audio monitor (for level display)
    connect(m_audioCapture.get(), &AudioCapture::audioLevelChanged,
            m_audioMonitor, &AudioMonitor::updateLevel);
    
    // Connect whisper processor to transcript widget
    connect(m_whisperProcessor.get(), &WhisperProcessor::transcriptionReady,
            this, &MainWindow::onTranscriptionReceived);
    
    // Connect main window recording controls
    connect(this, &MainWindow::startRecording,
            m_audioCapture.get(), &AudioCapture::startCapture);
    connect(this, &MainWindow::stopRecording,
            m_audioCapture.get(), &AudioCapture::stopCapture);
    connect(this, &MainWindow::stopRecording,
            m_whisperProcessor.get(), &WhisperProcessor::finishRecording);
    connect(this, &MainWindow::pauseRecording,
            m_audioCapture.get(), &AudioCapture::pauseCapture);
    
    // Connect status updates
    connect(m_audioCapture.get(), &AudioCapture::statusChanged,
            this, &MainWindow::onStatusChanged);
    connect(m_whisperProcessor.get(), &WhisperProcessor::statusChanged,
            this, &MainWindow::onStatusChanged);
    
    // Connect model download signals
    connect(m_whisperProcessor.get(), &WhisperProcessor::modelNotFound,
            this, &MainWindow::onModelNotFound);
    connect(m_modelDownloader.get(), &ModelDownloader::downloadComplete,
            this, &MainWindow::onModelDownloadComplete);
    connect(m_modelDownloader.get(), &ModelDownloader::downloadFailed,
            this, &MainWindow::onModelDownloadFailed);
}

void MainWindow::onStartRecording()
{
    if (!m_isRecording) {
        m_isRecording = true;
        m_startAction->setEnabled(false);
        m_stopAction->setEnabled(true);
        m_pauseAction->setEnabled(true);
        
        // Disable configuration changes during recording
        m_configWidget->setRecordingState(true);
        
        // Get configuration from config widget
        auto config = m_configWidget->getConfiguration();
        m_audioCapture->updateConfiguration(config);
        
        // Configure audio processor with sample rate and filter settings
        m_audioProcessor->setSampleRate(16000); // Default sample rate for Whisper
        m_audioProcessor->setFilterEnabled(true);
        m_audioProcessor->setFilterFrequencies(300.0, 3400.0); // Speech frequency range
        
        m_whisperProcessor->updateConfiguration(config);
        m_outputManager->updateConfiguration(config);
        
        emit startRecording();
        statusBar()->showMessage(tr("Recording with audio filtering..."));
    }
}

void MainWindow::onStopRecording()
{
    if (m_isRecording) {
        m_isRecording = false;
        m_isPaused = false;
        m_startAction->setEnabled(true);
        m_stopAction->setEnabled(false);
        m_pauseAction->setEnabled(false);
        
        // Re-enable configuration changes after recording
        m_configWidget->setRecordingState(false);
        
        emit stopRecording();
        statusBar()->showMessage(tr("Stopped"));
    }
}

void MainWindow::onPauseRecording()
{
    if (m_isRecording) {
        m_isPaused = !m_isPaused;
        m_pauseAction->setText(m_isPaused ? tr("Resume") : tr("Pause"));
        emit pauseRecording();
        statusBar()->showMessage(m_isPaused ? tr("Paused") : tr("Recording..."));
    }
}

void MainWindow::onTranscriptionReceived(const QString &text, qint64 timestamp)
{
    // Add to transcript widget
    m_transcriptWidget->appendTranscription(text, timestamp);
    
    // Get current configuration to check if timestamps should be included in output
    auto config = m_configWidget->getConfiguration();
    
    // Send to output manager for additional outputs (file, clipboard, etc.)
    // Include timestamps in output if configured
    if (config.includeTimestamps) {
        QString timestampedText = QString("[%1] %2")
            .arg(QDateTime::fromMSecsSinceEpoch(timestamp).toString("hh:mm:ss"))
            .arg(text);
        m_outputManager->handleTranscription(timestampedText, timestamp);
    } else {
        m_outputManager->handleTranscription(text, timestamp);
    }
}

void MainWindow::onAudioLevelChanged(float level)
{
    m_audioMonitor->updateLevel(level);
}

void MainWindow::onStatusChanged(const QString &status)
{
    statusBar()->showMessage(status, 2000);
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, tr("About QWhisper"),
        tr("<h2>QWhisper 1.0</h2>"
           "<p>A Qt6-based real-time speech recognition application using Whisper.</p>"
           "<p>Features:</p>"
           "<ul>"
           "<li>Real-time audio transcription</li>"
           "<li>Multiple Whisper model support</li>"
           "<li>Audio filtering and processing</li>"
           "<li>Interactive transcript editing</li>"
           "<li>Multiple output formats</li>"
           "</ul>"));
}

void MainWindow::onSettings()
{
    SettingsDialog dialog(this);
    dialog.exec();
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.beginGroup("MainWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("splitterState", m_centralSplitter->saveState());
    settings.endGroup();
    
    // Save config widget settings
    m_configWidget->saveSettings();
}

void MainWindow::loadSettings()
{
    QSettings settings;
    settings.beginGroup("MainWindow");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    m_centralSplitter->restoreState(settings.value("splitterState").toByteArray());
    settings.endGroup();
    
    // Load config widget settings
    m_configWidget->loadSettings();
}

void MainWindow::onModelNotFound(const QString &modelName)
{
    // Ask user if they want to download the model
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        tr("Model Not Found"),
        tr("The model '%1' was not found on your system.\n\n"
           "Would you like to download it now?\n"
           "This may take several minutes depending on your internet connection.")
           .arg(modelName),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        // Start the download
        m_modelDownloader->downloadModel(modelName, this);
        statusBar()->showMessage(tr("Downloading %1 model...").arg(modelName));
    } else {
        statusBar()->showMessage(tr("Model download canceled"), 3000);
    }
}

void MainWindow::onModelDownloadComplete(const QString &modelName, const QString &filePath)
{
    Q_UNUSED(filePath)
    
    statusBar()->showMessage(tr("Model %1 downloaded successfully").arg(modelName), 5000);
    
    // Reload the model now that it's downloaded
    m_whisperProcessor->loadModel(modelName);
}

void MainWindow::onModelDownloadFailed(const QString &modelName, const QString &error)
{
    statusBar()->showMessage(tr("Failed to download model %1").arg(modelName), 5000);
    
    QMessageBox::critical(this, tr("Download Failed"),
                         tr("Failed to download the %1 model:\n%2")
                         .arg(modelName)
                         .arg(error));
}
