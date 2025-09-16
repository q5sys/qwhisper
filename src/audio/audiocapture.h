#ifndef AUDIOCAPTURE_H
#define AUDIOCAPTURE_H

#include <QObject>
#include <QByteArray>
#include <memory>
#include <QMap>

QT_BEGIN_NAMESPACE
class QAudioSource;
class QAudioDevice;
class QIODevice;
class QTimer;
class QProcess;
QT_END_NAMESPACE

struct AudioConfiguration;

class AudioCapture : public QObject
{
    Q_OBJECT

public:
    explicit AudioCapture(QObject *parent = nullptr);
    ~AudioCapture();

    static QMap<QString, QString> listPulseAudioSinks();

public slots:
    void startCapture();
    void stopCapture();
    void pauseCapture();
    void updateConfiguration(const AudioConfiguration &config);

signals:
    void audioDataReady(const QByteArray &data);
    void audioLevelChanged(float level);
    void statusChanged(const QString &status);

private slots:
    void processAudioData();

private:
    void setupAudioInput();
    float calculateLevel(const QByteArray &data);
    
    std::unique_ptr<QAudioSource> m_audioInput;
    QProcess *m_pacatProcess = nullptr;
    QIODevice *m_audioDevice;
    QTimer *m_captureTimer = nullptr;
    bool m_isCapturing;
    bool m_isPaused;
    
    // Configuration
    QString m_deviceId;
    QString m_audioSource;  // "microphone" or "speaker"
    int m_sampleRate;
    int m_channels;
    int m_sampleSize;
};

#endif // AUDIOCAPTURE_H
