#include "audiocapture.h"
#include "../ui/configwidget.h"
#include <QAudioSource>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QAudioFormat>
#include <QIODevice>
#include <QTimer>
#include <QDebug>
#include <QProcess>
#include <QRegularExpression>

QMap<QString, QString> AudioCapture::listPulseAudioSinks()
{
    QMap<QString, QString> sinks;
    QProcess process;
    process.start("pactl", QStringList() << "list" << "sinks");
    process.waitForFinished();

    QString output = process.readAllStandardOutput();
    QStringList sinkBlocks = output.split("Sink #");

    for (const QString &block : sinkBlocks) {
        if (block.trimmed().isEmpty()) continue;

        QString name;
        QString description;

        QStringList lines = block.split('\n');
        for (const QString &line : lines) {
            if (line.trimmed().startsWith("Name:")) {
                name = line.section(':', 1).trimmed();
            } else if (line.trimmed().startsWith("Description:")) {
                description = line.section(':', 1).trimmed();
            }
        }

        if (!name.isEmpty() && !description.isEmpty()) {
            sinks[description] = name;
        }
    }

    return sinks;
}

AudioCapture::AudioCapture(QObject *parent)
    : QObject(parent)
    , m_audioDevice(nullptr)
    , m_isCapturing(false)
    , m_isPaused(false)
    , m_sampleRate(16000)
    , m_channels(1)
    , m_sampleSize(16)
{
}

AudioCapture::~AudioCapture()
{
    stopCapture();
}

void AudioCapture::startCapture()
{
    if (m_isCapturing) return;
    
    setupAudioInput();
    
    if (m_audioDevice) {
        m_isCapturing = true;
        emit statusChanged("Audio capture started");
        
        if (m_pacatProcess) {
            connect(m_pacatProcess, &QProcess::readyReadStandardOutput, this, &AudioCapture::processAudioData);
        } else {
            if (!m_captureTimer) {
                m_captureTimer = new QTimer(this);
                connect(m_captureTimer, &QTimer::timeout, this, &AudioCapture::processAudioData);
            }
            m_captureTimer->start(200);
        }
    }
}

void AudioCapture::stopCapture()
{
    if (!m_isCapturing) return;
    
    if (m_captureTimer) {
        m_captureTimer->stop();
    }
    
    if (m_audioInput) {
        m_audioInput->stop();
    }

    if (m_pacatProcess) {
        m_pacatProcess->kill();
        m_pacatProcess->waitForFinished();
        delete m_pacatProcess;
        m_pacatProcess = nullptr;
    }
    
    m_audioDevice = nullptr;
    m_isCapturing = false;
    emit statusChanged("Audio capture stopped");
}

void AudioCapture::pauseCapture()
{
    if (!m_isCapturing) return;
    
    m_isPaused = !m_isPaused;
    
    if (m_isPaused) {
        if (m_audioInput) m_audioInput->suspend();
        emit statusChanged("Audio capture paused");
    } else {
        if (m_audioInput) m_audioInput->resume();
        emit statusChanged("Audio capture resumed");
    }
}

void AudioCapture::updateConfiguration(const AudioConfiguration &config)
{
    m_deviceId = config.device;
    m_audioSource = config.audioSource;
    // Apply other configuration settings
}

void AudioCapture::processAudioData()
{
    if (!m_audioDevice || !m_isCapturing || m_isPaused) return;
    
    QByteArray data = m_audioDevice->readAll();
    if (!data.isEmpty()) {
        emit audioDataReady(data);
        
        float level = calculateLevel(data);
        emit audioLevelChanged(level);
    }
}

void AudioCapture::setupAudioInput()
{
    QAudioFormat format;
    format.setSampleRate(m_sampleRate);
    format.setChannelCount(m_channels);
    format.setSampleFormat(QAudioFormat::Int16);
    
    // Get audio device
    QAudioDevice device;
    
    // Check if we're trying to capture system audio
    bool isSystemAudio = m_audioSource.toLower().contains("speaker") || 
                        m_audioSource.toLower().contains("system");
    
    if (isSystemAudio) {
        if (m_pacatProcess) {
            m_pacatProcess->kill();
            m_pacatProcess->waitForFinished();
            delete m_pacatProcess;
            m_pacatProcess = nullptr;
        }

        m_pacatProcess = new QProcess(this);
        QStringList args;
        args << "--record"
             << "-d" << (m_deviceId + ".monitor")
             << "--format=s16le"
             << "--rate=16000"
             << "--channels=1";
        
        m_pacatProcess->start("pacat", args);

        if (!m_pacatProcess->waitForStarted()) {
            qDebug() << "Failed to start pacat process:" << m_pacatProcess->errorString();
            emit statusChanged("Failed to start audio capture with pacat");
            return;
        }

        m_audioDevice = m_pacatProcess;
        qDebug() << "Successfully started audio capture with pacat for device:" << m_deviceId;
    } else {
        // For microphone input, use regular input devices
        if (!m_deviceId.isEmpty()) {
            const auto devices = QMediaDevices::audioInputs();
            for (const auto &d : devices) {
                if (d.id() == m_deviceId.toUtf8()) {
                    device = d;
                    break;
                }
            }
        }
        
        if (device.isNull()) {
            device = QMediaDevices::defaultAudioInput();
        }
    
        if (device.isNull()) {
            qDebug() << "No audio device available";
            emit statusChanged("No audio device available");
            return;
        }
    
        if (!device.isFormatSupported(format)) {
            format = device.preferredFormat();
            qDebug() << "Using preferred format - Sample rate:" << format.sampleRate() 
                     << "Channels:" << format.channelCount();
        }
    
        // Clean up any existing audio source
        if (m_audioInput) {
            m_audioInput->stop();
            m_audioInput.reset();
        }
    
        m_audioInput = std::make_unique<QAudioSource>(device, format);
        m_audioInput->setBufferSize(8192); // Set a reasonable buffer size
        m_audioDevice = m_audioInput->start(); // This returns the QIODevice to read from
    
        if (!m_audioDevice) {
            qDebug() << "Failed to start audio source with device:" << device.description();
            emit statusChanged("Failed to start audio capture");
        } else {
            qDebug() << "Successfully started audio capture with:" << device.description();
        }
    }
}

float AudioCapture::calculateLevel(const QByteArray &data)
{
    const qint16 *samples = reinterpret_cast<const qint16*>(data.constData());
    const int sampleCount = data.size() / sizeof(qint16);
    
    if (sampleCount == 0) return 0.0f;
    
    qint64 sum = 0;
    for (int i = 0; i < sampleCount; ++i) {
        sum += qAbs(samples[i]);
    }
    
    float average = static_cast<float>(sum) / sampleCount;
    float normalized = average / 32768.0f; // Normalize to 0-1 range
    
    return qBound(0.0f, normalized, 1.0f);
}
