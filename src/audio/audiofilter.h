#ifndef AUDIOFILTER_H
#define AUDIOFILTER_H

#include <QObject>
#include <QByteArray>
#include <vector>
#include <cmath>

class AudioFilter : public QObject
{
    Q_OBJECT

public:
    explicit AudioFilter(QObject *parent = nullptr);
    ~AudioFilter();
    
    QByteArray applyBandpassFilter(const QByteArray &input, double lowCut, double highCut);
    void setSampleRate(int sampleRate);
    void setFilterEnabled(bool enabled);
    bool isFilterEnabled() const;

private:
    struct ButterworthFilter {
        double a0, a1, a2, b1, b2;
        double x1, x2, y1, y2;
        
        ButterworthFilter() : a0(1), a1(0), a2(0), b1(0), b2(0), x1(0), x2(0), y1(0), y2(0) {}
        
        double process(double input);
        void reset();
    };
    
    void calculateFilterCoefficients(double frequency, bool isHighpass, ButterworthFilter &filter);
    std::vector<double> processAudioSamples(const std::vector<double> &samples, double lowCut, double highCut);
    
    int m_sampleRate;
    bool m_filterEnabled;
    ButterworthFilter m_highpassFilter;
    ButterworthFilter m_lowpassFilter;
};

#endif // AUDIOFILTER_H
