#ifndef WHISPERPROCESSOR_H
#define WHISPERPROCESSOR_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <memory>
#include <vector>

struct AudioConfiguration;
struct whisper_context;
struct whisper_context_params;

class WhisperProcessor : public QObject
{
    Q_OBJECT

public:
    explicit WhisperProcessor(QObject *parent = nullptr);
    ~WhisperProcessor();

public slots:
    void processAudio(const QByteArray &audioData);
    void loadModel(const QString &modelName);
    void updateConfiguration(const AudioConfiguration &config);
    void setComputeDevice(int deviceType, int deviceId);
    void finishRecording();

signals:
    void transcriptionReady(const QString &text, qint64 timestamp);
    void statusChanged(const QString &status);
    void modelNotFound(const QString &modelName);

private:
    void initializeWhisperContext();
    void releaseWhisperContext();
    QString getModelPath(const QString &modelName);
    void processAccumulatedAudio();
    
    QString m_currentModel;
    bool m_modelLoaded;
    int m_computeDeviceType;  // 0 = CPU, 1 = CUDA
    int m_computeDeviceId;    // -1 for CPU, 0+ for GPU index
    
    // Whisper.cpp context
    whisper_context* m_whisperContext;
    whisper_context_params* m_contextParams;
    
    // Audio buffering and VAD
    std::vector<float> m_audioBuffer;
    float m_pickupThreshold;
    int m_minSpeechDuration;
    int m_maxSpeechDuration;
    int m_silenceDuration;
    bool m_isRecording;
    qint64 m_lastSoundTime;
    qint64 m_speechStartTime;
};

#endif // WHISPERPROCESSOR_H
