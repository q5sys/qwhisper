#ifndef AUDIOPROCESSOR_H
#define AUDIOPROCESSOR_H

#include <QObject>
#include <QByteArray>

class AudioFilter;

class AudioProcessor : public QObject
{
    Q_OBJECT

public:
    explicit AudioProcessor(QObject *parent = nullptr);
    ~AudioProcessor();
    
    void setFilterEnabled(bool enabled);
    void setFilterFrequencies(double lowCut, double highCut);
    void setSampleRate(int sampleRate);
    void setGainBoost(double gainDb);
    void setAutoGainEnabled(bool enabled);
    void setAutoGainTarget(double targetLevel);

public slots:
    void processAudioData(const QByteArray &data);

signals:
    void processedAudio(const QByteArray &data);

private:
    QByteArray applyGainBoost(const QByteArray &data);
    double calculateRMS(const QByteArray &data);
    
    AudioFilter *m_audioFilter;
    double m_lowCutFreq;
    double m_highCutFreq;
    bool m_filterEnabled;
    
    // Gain control
    double m_gainLinear;        // Linear gain multiplier
    bool m_autoGainEnabled;     // Automatic gain control
    double m_autoGainTarget;    // Target RMS level for AGC
    double m_currentGain;       // Current AGC gain
    double m_gainSmoothingFactor; // Smoothing factor for AGC
};

#endif // AUDIOPROCESSOR_H
