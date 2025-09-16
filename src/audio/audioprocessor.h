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

public slots:
    void processAudioData(const QByteArray &data);

signals:
    void processedAudio(const QByteArray &data);

private:
    AudioFilter *m_audioFilter;
    double m_lowCutFreq;
    double m_highCutFreq;
    bool m_filterEnabled;
};

#endif // AUDIOPROCESSOR_H
