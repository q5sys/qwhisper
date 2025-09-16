#include "audiofilter.h"
#include <QDebug>
#include <algorithm>

AudioFilter::AudioFilter(QObject *parent)
    : QObject(parent)
    , m_sampleRate(16000)
    , m_filterEnabled(true)
{
}

AudioFilter::~AudioFilter()
{
}

void AudioFilter::setSampleRate(int sampleRate)
{
    if (sampleRate > 0 && sampleRate != m_sampleRate) {
        m_sampleRate = sampleRate;
        // Reset filters when sample rate changes
        m_highpassFilter.reset();
        m_lowpassFilter.reset();
    }
}

void AudioFilter::setFilterEnabled(bool enabled)
{
    m_filterEnabled = enabled;
}

bool AudioFilter::isFilterEnabled() const
{
    return m_filterEnabled;
}

double AudioFilter::ButterworthFilter::process(double input)
{
    // Direct Form II implementation of 2nd order Butterworth filter
    double output = a0 * input + a1 * x1 + a2 * x2 - b1 * y1 - b2 * y2;
    
    // Update delay line
    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;
    
    return output;
}

void AudioFilter::ButterworthFilter::reset()
{
    x1 = x2 = y1 = y2 = 0.0;
}

void AudioFilter::calculateFilterCoefficients(double frequency, bool isHighpass, ButterworthFilter &filter)
{
    // Normalize frequency (0 to 1, where 1 is Nyquist frequency)
    double normalizedFreq = frequency / (m_sampleRate * 0.5);
    
    // Clamp frequency to valid range
    normalizedFreq = std::max(0.001, std::min(0.999, normalizedFreq));
    
    // Calculate filter coefficients for 2nd order Butterworth filter
    double omega = M_PI * normalizedFreq;
    double sin_omega = std::sin(omega);
    double cos_omega = std::cos(omega);
    double alpha = sin_omega / std::sqrt(2.0); // Q = 1/sqrt(2) for Butterworth
    
    if (isHighpass) {
        // High-pass filter coefficients
        double norm = 1.0 + alpha;
        filter.a0 = (1.0 + cos_omega) / (2.0 * norm);
        filter.a1 = -(1.0 + cos_omega) / norm;
        filter.a2 = filter.a0;
        filter.b1 = -2.0 * cos_omega / norm;
        filter.b2 = (1.0 - alpha) / norm;
    } else {
        // Low-pass filter coefficients
        double norm = 1.0 + alpha;
        filter.a0 = (1.0 - cos_omega) / (2.0 * norm);
        filter.a1 = (1.0 - cos_omega) / norm;
        filter.a2 = filter.a0;
        filter.b1 = -2.0 * cos_omega / norm;
        filter.b2 = (1.0 - alpha) / norm;
    }
}

std::vector<double> AudioFilter::processAudioSamples(const std::vector<double> &samples, double lowCut, double highCut)
{
    std::vector<double> filtered = samples;
    
    // Apply high-pass filter (removes frequencies below lowCut)
    if (lowCut > 0) {
        calculateFilterCoefficients(lowCut, true, m_highpassFilter);
        for (size_t i = 0; i < filtered.size(); ++i) {
            filtered[i] = m_highpassFilter.process(filtered[i]);
        }
    }
    
    // Apply low-pass filter (removes frequencies above highCut)
    if (highCut > 0 && highCut < m_sampleRate * 0.5) {
        calculateFilterCoefficients(highCut, false, m_lowpassFilter);
        for (size_t i = 0; i < filtered.size(); ++i) {
            filtered[i] = m_lowpassFilter.process(filtered[i]);
        }
    }
    
    return filtered;
}

QByteArray AudioFilter::applyBandpassFilter(const QByteArray &input, double lowCut, double highCut)
{
    if (!m_filterEnabled || input.isEmpty()) {
        return input;
    }
    
    // Validate frequency range
    if (lowCut <= 0 || highCut <= 0 || lowCut >= highCut) {
        qDebug() << "Invalid filter frequencies: lowCut=" << lowCut << "highCut=" << highCut;
        return input;
    }
    
    // Convert byte array to 16-bit samples
    const qint16 *inputSamples = reinterpret_cast<const qint16*>(input.constData());
    const int sampleCount = input.size() / sizeof(qint16);
    
    if (sampleCount == 0) {
        return input;
    }
    
    // Convert to double for processing
    std::vector<double> samples(sampleCount);
    for (int i = 0; i < sampleCount; ++i) {
        samples[i] = static_cast<double>(inputSamples[i]) / 32768.0; // Normalize to [-1, 1]
    }
    
    // Apply bandpass filtering
    std::vector<double> filtered = processAudioSamples(samples, lowCut, highCut);
    
    // Convert back to 16-bit samples
    QByteArray output(input.size(), 0);
    qint16 *outputSamples = reinterpret_cast<qint16*>(output.data());
    
    for (int i = 0; i < sampleCount; ++i) {
        // Clamp and convert back to 16-bit
        double sample = std::max(-1.0, std::min(1.0, filtered[i]));
        outputSamples[i] = static_cast<qint16>(sample * 32767.0);
    }
    
    return output;
}
