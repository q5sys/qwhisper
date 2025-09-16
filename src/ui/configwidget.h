#ifndef CONFIGWIDGET_H
#define CONFIGWIDGET_H

#include <QWidget>
#include <QVariantMap>
#include "../whisper/devicemanager.h"

QT_BEGIN_NAMESPACE
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QGroupBox;
class QPushButton;
class QLabel;
class QSlider;
QT_END_NAMESPACE

struct AudioConfiguration {
    QString model;
    QString device;
    QString audioSource;
    int pickupThreshold;
    double minSpeechDuration;
    double maxSpeechDuration;
    bool useBandpass;
    double lowCutFreq;
    double highCutFreq;
    bool includeTimestamps;  // Include timestamps in UI and all outputs
    
    // Compute device options
    int computeDeviceType;  // 0 = CPU, 1 = CUDA
    int computeDeviceId;    // -1 for CPU, 0+ for GPU index
    
    // Output options
    bool outputToWindow;
    bool outputToFile;
    bool outputToClipboard;
    QString outputFilePath;
};

class ConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigWidget(QWidget *parent = nullptr);
    ~ConfigWidget();
    
    AudioConfiguration getConfiguration() const;
    void setConfiguration(const AudioConfiguration &config);
    
    void saveSettings();
    void loadSettings();
    
    // Enable/disable controls during recording
    void setRecordingState(bool isRecording);

signals:
    void configurationChanged(const AudioConfiguration &config);
    void modelChanged(const QString &model);
    void deviceChanged(const QString &device);
    void computeDeviceChanged(int deviceType, int deviceId);

private slots:
    void onModelChanged(int index);
    void onDeviceChanged(int index);
    void onAudioSourceChanged(int index);
    void onComputeDeviceChanged(int index);
    void onPickupThresholdChanged(int value);
    void onMinSpeechDurationChanged(double value);
    void onMaxSpeechDurationChanged(double value);
    void onBandpassToggled(bool checked);
    void onLowCutChanged(double value);
    void onHighCutChanged(double value);
    void onTimestampsToggled(bool checked);
    void onOutputOptionsChanged();
    void onBrowseOutputFile();
    void refreshAudioDevices();
    void refreshComputeDevices();
    void updateDeviceColors();
    void emitConfigurationChanged();

private:
    void setupUi();
    void connectSignals();
    void populateModels();
    void populateAudioDevices();
    
    // Model selection
    QGroupBox *m_modelGroup;
    QComboBox *m_modelCombo;
    QLabel *m_modelDescLabel;
    QComboBox *m_computeDeviceCombo;
    QLabel *m_computeDeviceLabel;
    
    // Audio input
    QGroupBox *m_audioGroup;
    QComboBox *m_audioSourceCombo;
    QComboBox *m_deviceCombo;
    QPushButton *m_refreshDevicesButton;
    
    // Voice activity detection
    QGroupBox *m_vadGroup;
    QSlider *m_pickupSlider;
    QLabel *m_pickupLabel;
    QDoubleSpinBox *m_minSpeechSpin;
    QDoubleSpinBox *m_maxSpeechSpin;
    
    // Audio filtering
    QGroupBox *m_filterGroup;
    QCheckBox *m_bandpassCheck;
    QDoubleSpinBox *m_lowCutSpin;
    QDoubleSpinBox *m_highCutSpin;
    
    // Output options
    QGroupBox *m_outputGroup;
    QCheckBox *m_outputWindowCheck;
    QCheckBox *m_outputTypeToWindowCheck;  // New checkbox for typing to active window
    QCheckBox *m_outputFileCheck;
    QCheckBox *m_outputClipboardCheck;
    QCheckBox *m_timestampsCheck;
    QPushButton *m_browseFileButton;
    QLabel *m_outputFileLabel;
    
    // Current configuration
    AudioConfiguration m_config;
    
    // Store available devices for color updating
    QList<DeviceManager::DeviceInfo> m_availableDevices;
};

#endif // CONFIGWIDGET_H
