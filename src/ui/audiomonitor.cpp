#include "audiomonitor.h"
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <QTimer>
#include <QLinearGradient>
#include <cmath>

AudioMonitor::AudioMonitor(QWidget *parent)
    : QWidget(parent)
    , m_currentLevel(0.0f)
    , m_peakLevel(0.0f)
{
    setMinimumHeight(40);
    setMaximumHeight(80);
    
    // Initialize level history
    m_levelHistory.reserve(HISTORY_SIZE);
    for (int i = 0; i < HISTORY_SIZE; ++i) {
        m_levelHistory.append(0.0f);
    }
    
    // Setup decay timer for peak level
    m_decayTimer = new QTimer(this);
    connect(m_decayTimer, &QTimer::timeout, [this]() {
        m_peakLevel *= PEAK_DECAY_RATE;
        if (m_peakLevel < 0.01f) {
            m_peakLevel = 0.0f;
        }
        update();
    });
    m_decayTimer->start(50); // Update every 50ms
}

AudioMonitor::~AudioMonitor()
{
}

void AudioMonitor::updateLevel(float level)
{
    m_currentLevel = qBound(0.0f, level, 1.0f);
    
    // Update peak level
    if (m_currentLevel > m_peakLevel) {
        m_peakLevel = m_currentLevel;
    }
    
    // Update history
    m_levelHistory.removeFirst();
    m_levelHistory.append(m_currentLevel);
    
    update();
}

void AudioMonitor::updateWaveform(const QVector<float> &samples)
{
    m_waveformData = samples;
    update();
}

void AudioMonitor::clear()
{
    m_currentLevel = 0.0f;
    m_peakLevel = 0.0f;
    m_levelHistory.fill(0.0f);
    m_waveformData.clear();
    update();
}

void AudioMonitor::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    const int width = this->width();
    const int height = this->height();
    const int margin = 2;
    
    // Draw background
    painter.fillRect(rect(), QColor(40, 40, 40));
    
    // Draw border
    painter.setPen(QPen(QColor(60, 60, 60), 1));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
    
    // Calculate drawing area
    const int drawWidth = width - 2 * margin;
    const int drawHeight = height - 2 * margin;
    const int barHeight = drawHeight / 2;
    
    // Draw level meter (top half)
    if (drawHeight > 20) {
        const int meterY = margin;
        const int meterHeight = barHeight - 2;
        
        // Background of meter
        painter.fillRect(margin, meterY, drawWidth, meterHeight, QColor(20, 20, 20));
        
        // Create gradient for level meter
        QLinearGradient gradient(margin, 0, width - margin, 0);
        gradient.setColorAt(0.0, QColor(0, 200, 0));
        gradient.setColorAt(0.6, QColor(200, 200, 0));
        gradient.setColorAt(0.8, QColor(200, 100, 0));
        gradient.setColorAt(1.0, QColor(200, 0, 0));
        
        // Draw current level
        const int levelWidth = static_cast<int>(m_currentLevel * drawWidth);
        painter.fillRect(margin, meterY, levelWidth, meterHeight, gradient);
        
        // Draw peak indicator
        if (m_peakLevel > 0.0f) {
            const int peakX = margin + static_cast<int>(m_peakLevel * drawWidth);
            painter.setPen(QPen(Qt::white, 2));
            painter.drawLine(peakX, meterY, peakX, meterY + meterHeight);
        }
        
        // Draw scale marks
        painter.setPen(QPen(QColor(100, 100, 100), 1));
        for (int i = 1; i < 10; ++i) {
            const int x = margin + (drawWidth * i) / 10;
            painter.drawLine(x, meterY, x, meterY + 3);
            painter.drawLine(x, meterY + meterHeight - 3, x, meterY + meterHeight);
        }
    }
    
    // Draw waveform or history (bottom half)
    const int waveY = margin + barHeight + 2;
    const int waveHeight = barHeight - 4;
    const int waveCenterY = waveY + waveHeight / 2;
    
    if (!m_waveformData.isEmpty()) {
        // Draw waveform
        painter.setPen(QPen(QColor(100, 150, 255), 1));
        
        const int sampleCount = m_waveformData.size();
        const float xStep = static_cast<float>(drawWidth) / sampleCount;
        
        QPainterPath path;
        for (int i = 0; i < sampleCount; ++i) {
            const float x = margin + i * xStep;
            const float y = waveCenterY - (m_waveformData[i] * waveHeight / 2);
            
            if (i == 0) {
                path.moveTo(x, y);
            } else {
                path.lineTo(x, y);
            }
        }
        painter.drawPath(path);
    } else {
        // Draw level history
        painter.setPen(QPen(QColor(100, 255, 100), 1));
        
        const float xStep = static_cast<float>(drawWidth) / HISTORY_SIZE;
        
        QPainterPath path;
        for (int i = 0; i < m_levelHistory.size(); ++i) {
            const float x = margin + i * xStep;
            const float y = waveY + waveHeight - (m_levelHistory[i] * waveHeight);
            
            if (i == 0) {
                path.moveTo(x, y);
            } else {
                path.lineTo(x, y);
            }
        }
        painter.drawPath(path);
    }
    
    // Draw center line for waveform
    painter.setPen(QPen(QColor(60, 60, 60), 1, Qt::DashLine));
    painter.drawLine(margin, waveCenterY, width - margin, waveCenterY);
}
