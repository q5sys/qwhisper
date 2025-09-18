#include "audioprocessor.h"
#include "audiofilter.h"
#include <QDebug>
#include <cmath>
#include <algorithm>

AudioProcessor::AudioProcessor(QObject *parent)
    : QObject(parent)
    , m_audioFilter(new AudioFilter(this))
    , m_lowCutFreq(300.0)    // Default: 300 Hz high-pass (remove low-frequency noise)
    , m_highCutFreq(3400.0)  // Default: 3400 Hz low-pass (speech frequency range)
    , m_filterEnabled(true)
    , m_gainLinear(1.0)      // Default: no gain boost (0 dB)
    , m_autoGainEnabled(false)
    , m_autoGainTarget(0.1)  // Target RMS level (10% of full scale)
    , m_currentGain(1.0)
    , m_gainSmoothingFactor(0.95) // Smooth gain changes
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

void AudioProcessor::setGainBoost(double gainDb)
{
    // Convert dB to linear gain: gain_linear = 10^(gainDb/20)
    m_gainLinear = std::pow(10.0, gainDb / 20.0);
    qDebug() << "Gain boost set to" << gainDb << "dB (linear:" << m_gainLinear << ")";
}

void AudioProcessor::setAutoGainEnabled(bool enabled)
{
    m_autoGainEnabled = enabled;
    if (enabled) {
        qDebug() << "Auto gain control enabled with target level:" << m_autoGainTarget;
    } else {
        qDebug() << "Auto gain control disabled";
        m_currentGain = 1.0; // Reset to unity gain
    }
}

void AudioProcessor::setAutoGainTarget(double targetLevel)
{
    m_autoGainTarget = std::max(0.01, std::min(0.9, targetLevel)); // Clamp between 1% and 90%
    qDebug() << "Auto gain target level set to:" << m_autoGainTarget;
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
    
    // Apply gain boost (manual or automatic)
    processedData = applyGainBoost(processedData);
    
    // Emit the processed audio data
    emit processedAudio(processedData);
}

QByteArray AudioProcessor::applyGainBoost(const QByteArray &data)
{
    if (data.isEmpty()) {
        return data;
    }
    
    const qint16 *inputSamples = reinterpret_cast<const qint16*>(data.constData());
    const int sampleCount = data.size() / sizeof(qint16);
    
    if (sampleCount == 0) {
        return data;
    }
    
    QByteArray output(data.size(), 0);
    qint16 *outputSamples = reinterpret_cast<qint16*>(output.data());
    
    double gainToApply = m_gainLinear;
    
    // Apply automatic gain control if enabled
    if (m_autoGainEnabled) {
        double rms = calculateRMS(data);
        if (rms > 0.001) { // Avoid division by very small numbers
            double targetGain = m_autoGainTarget / rms;
            // Limit maximum AGC gain to prevent excessive amplification
            targetGain = std::min(targetGain, 50.0); // Max 34 dB boost
            
            // Smooth the gain changes to avoid artifacts
            m_currentGain = m_gainSmoothingFactor * m_currentGain + 
                           (1.0 - m_gainSmoothingFactor) * targetGain;
            
            gainToApply = m_currentGain;
        }
    }
    
    // Apply gain with clipping protection
    for (int i = 0; i < sampleCount; ++i) {
        double sample = static_cast<double>(inputSamples[i]) * gainToApply;
        
        // Soft clipping to prevent harsh distortion
        if (sample > 32767.0) {
            sample = 32767.0;
        } else if (sample < -32768.0) {
            sample = -32768.0;
        }
        
        outputSamples[i] = static_cast<qint16>(sample);
    }
    
    return output;
}

double AudioProcessor::calculateRMS(const QByteArray &data)
{
    const qint16 *samples = reinterpret_cast<const qint16*>(data.constData());
    const int sampleCount = data.size() / sizeof(qint16);
    
    if (sampleCount == 0) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (int i = 0; i < sampleCount; ++i) {
        double sample = static_cast<double>(samples[i]) / 32768.0; // Normalize to [-1, 1]
        sum += sample * sample;
    }
    
    return std::sqrt(sum / sampleCount);
}
