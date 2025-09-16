#include "audioprocessor.h"
#include "audiofilter.h"
#include <QDebug>

AudioProcessor::AudioProcessor(QObject *parent)
    : QObject(parent)
    , m_audioFilter(new AudioFilter(this))
    , m_lowCutFreq(300.0)    // Default: 300 Hz high-pass (remove low-frequency noise)
    , m_highCutFreq(3400.0)  // Default: 3400 Hz low-pass (speech frequency range)
    , m_filterEnabled(true)
{
}

AudioProcessor::~AudioProcessor()
{
}

void AudioProcessor::setFilterEnabled(bool enabled)
{
    m_filterEnabled = enabled;
    if (m_audioFilter) {
        m_audioFilter->setFilterEnabled(enabled);
    }
}

void AudioProcessor::setFilterFrequencies(double lowCut, double highCut)
{
    if (lowCut > 0 && highCut > lowCut) {
        m_lowCutFreq = lowCut;
        m_highCutFreq = highCut;
        qDebug() << "Audio filter frequencies set to:" << lowCut << "Hz -" << highCut << "Hz";
    }
}

void AudioProcessor::setSampleRate(int sampleRate)
{
    if (m_audioFilter) {
        m_audioFilter->setSampleRate(sampleRate);
    }
}

void AudioProcessor::processAudioData(const QByteArray &data)
{
    if (data.isEmpty()) {
        return;
    }
    
    QByteArray processedData = data;
    
    // Apply bandpass filtering if enabled
    if (m_filterEnabled && m_audioFilter) {
        processedData = m_audioFilter->applyBandpassFilter(data, m_lowCutFreq, m_highCutFreq);
    }
    
    // Emit the processed audio data
    emit processedAudio(processedData);
}
