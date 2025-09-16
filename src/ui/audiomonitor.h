#ifndef AUDIOMONITOR_H
#define AUDIOMONITOR_H

#include <QWidget>
#include <QVector>

QT_BEGIN_NAMESPACE
class QPaintEvent;
class QTimer;
QT_END_NAMESPACE

class AudioMonitor : public QWidget
{
    Q_OBJECT

public:
    explicit AudioMonitor(QWidget *parent = nullptr);
    ~AudioMonitor();

public slots:
    void updateLevel(float level);
    void updateWaveform(const QVector<float> &samples);
    void clear();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    float m_currentLevel;
    float m_peakLevel;
    QVector<float> m_levelHistory;
    QVector<float> m_waveformData;
    QTimer *m_decayTimer;
    
    static constexpr int HISTORY_SIZE = 100;
    static constexpr float PEAK_DECAY_RATE = 0.95f;
};

#endif // AUDIOMONITOR_H
