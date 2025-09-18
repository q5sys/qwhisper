#include "configwidget.h"
#include "../audio/audiocapture.h"
#include "../whisper/devicemanager.h"
#include "../whisper/whispermodels.h"
#include "../config/configmanager.h"
#include "../output/windowtyper.h"
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QSettings>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QMessageBox>

ConfigWidget::ConfigWidget(QWidget *parent)
    : QWidget(parent)
{
    // Set default configuration
    m_config.model = "base";
    m_config.audioSource = "microphone";
    m_config.pickupThreshold = 120;
    m_config.minSpeechDuration = 0.0;
    m_config.maxSpeechDuration = 10.0;
    m_config.useBandpass = true;
    m_config.lowCutFreq = 80.0;
    m_config.highCutFreq = 6000.0;
    m_config.includeTimestamps = false;
    m_config.computeDeviceType = 0;  // Default to CPU
    m_config.computeDeviceId = -1;
    m_config.gainBoostDb = 0.0;      // Default: no gain boost
    m_config.autoGainEnabled = false; // Default: manual gain control
    m_config.autoGainTarget = 0.1;   // Default: 10% target level
    m_config.outputToWindow = true;
    m_config.outputToFile = false;
    m_config.outputToClipboard = false;
    
    setupUi();
    connectSignals();
    populateModels();
    populateAudioDevices();
    refreshComputeDevices();
}

ConfigWidget::~ConfigWidget()
{
}

void ConfigWidget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    
    // Model Selection Group
    m_modelGroup = new QGroupBox(tr("Whisper Model"), this);
    QGridLayout *modelLayout = new QGridLayout(m_modelGroup);
    
    QLabel *modelLabel = new QLabel(tr("Model:"), this);
    m_modelCombo = new QComboBox(this);
    m_modelDescLabel = new QLabel(tr("Select the Whisper model to use"), this);
    m_modelDescLabel->setWordWrap(true);
    m_modelDescLabel->setStyleSheet("QLabel { color: gray; }");
    
    QLabel *computeLabel = new QLabel(tr("Compute:"), this);
    m_computeDeviceCombo = new QComboBox(this);
    m_computeDeviceLabel = new QLabel(tr("Select compute device"), this);
    m_computeDeviceLabel->setWordWrap(true);
    m_computeDeviceLabel->setStyleSheet("QLabel { color: gray; }");
    
    modelLayout->addWidget(modelLabel, 0, 0);
    modelLayout->addWidget(m_modelCombo, 0, 1);
    modelLayout->addWidget(m_modelDescLabel, 1, 0, 1, 2);
    modelLayout->addWidget(computeLabel, 2, 0);
    modelLayout->addWidget(m_computeDeviceCombo, 2, 1);
    modelLayout->addWidget(m_computeDeviceLabel, 3, 0, 1, 2);
    
    // Audio Input Group
    m_audioGroup = new QGroupBox(tr("Audio Input"), this);
    QGridLayout *audioLayout = new QGridLayout(m_audioGroup);
    
    QLabel *sourceLabel = new QLabel(tr("Source:"), this);
    m_audioSourceCombo = new QComboBox(this);
    m_audioSourceCombo->addItems({"Microphone", "Speaker/System"});
    
    QLabel *deviceLabel = new QLabel(tr("Device:"), this);
    m_deviceCombo = new QComboBox(this);
    m_refreshDevicesButton = new QPushButton(tr("Refresh"), this);
    m_refreshDevicesButton->setMaximumWidth(80);
    
    audioLayout->addWidget(sourceLabel, 0, 0);
    audioLayout->addWidget(m_audioSourceCombo, 0, 1, 1, 2);
    audioLayout->addWidget(deviceLabel, 1, 0);
    audioLayout->addWidget(m_deviceCombo, 1, 1);
    audioLayout->addWidget(m_refreshDevicesButton, 1, 2);
    
    // Voice Activity Detection Group
    m_vadGroup = new QGroupBox(tr("Voice Activity Detection"), this);
    QGridLayout *vadLayout = new QGridLayout(m_vadGroup);
    
    QLabel *pickupLabel = new QLabel(tr("Pickup Threshold:"), this);
    m_pickupSlider = new QSlider(Qt::Horizontal, this);
    m_pickupSlider->setRange(50, 500);
    m_pickupSlider->setValue(120);
    m_pickupLabel = new QLabel(tr("120"), this);
    m_pickupLabel->setMinimumWidth(40);
    
    QLabel *minSpeechLabel = new QLabel(tr("Min Speech (sec):"), this);
    m_minSpeechSpin = new QDoubleSpinBox(this);
    m_minSpeechSpin->setRange(0.0, 5.0);
    m_minSpeechSpin->setSingleStep(0.1);
    m_minSpeechSpin->setValue(0.0);
    
    QLabel *maxSpeechLabel = new QLabel(tr("Max Speech (sec):"), this);
    m_maxSpeechSpin = new QDoubleSpinBox(this);
    m_maxSpeechSpin->setRange(1.0, 30.0);
    m_maxSpeechSpin->setSingleStep(1.0);
    m_maxSpeechSpin->setValue(10.0);
    
    vadLayout->addWidget(pickupLabel, 0, 0);
    vadLayout->addWidget(m_pickupSlider, 0, 1);
    vadLayout->addWidget(m_pickupLabel, 0, 2);
    vadLayout->addWidget(minSpeechLabel, 1, 0);
    vadLayout->addWidget(m_minSpeechSpin, 1, 1, 1, 2);
    vadLayout->addWidget(maxSpeechLabel, 2, 0);
    vadLayout->addWidget(m_maxSpeechSpin, 2, 1, 1, 2);
    
    // Audio Filtering Group
    m_filterGroup = new QGroupBox(tr("Audio Filtering"), this);
    QGridLayout *filterLayout = new QGridLayout(m_filterGroup);
    
    m_bandpassCheck = new QCheckBox(tr("Enable Bandpass Filter"), this);
    m_bandpassCheck->setChecked(true);
    
    QLabel *lowCutLabel = new QLabel(tr("Low Cut (Hz):"), this);
    m_lowCutSpin = new QDoubleSpinBox(this);
    m_lowCutSpin->setRange(20.0, 1000.0);
    m_lowCutSpin->setSingleStep(10.0);
    m_lowCutSpin->setValue(80.0);
    
    QLabel *highCutLabel = new QLabel(tr("High Cut (Hz):"), this);
    m_highCutSpin = new QDoubleSpinBox(this);
    m_highCutSpin->setRange(1000.0, 20000.0);
    m_highCutSpin->setSingleStep(100.0);
    m_highCutSpin->setValue(6000.0);
    
    filterLayout->addWidget(m_bandpassCheck, 0, 0, 1, 2);
    filterLayout->addWidget(lowCutLabel, 1, 0);
    filterLayout->addWidget(m_lowCutSpin, 1, 1);
    filterLayout->addWidget(highCutLabel, 2, 0);
    filterLayout->addWidget(m_highCutSpin, 2, 1);
    
    // Audio Gain Control Group
    m_gainGroup = new QGroupBox(tr("Audio Gain Control"), this);
    QGridLayout *gainLayout = new QGridLayout(m_gainGroup);
    
    QLabel *gainBoostLabel = new QLabel(tr("Gain Boost (dB):"), this);
    m_gainBoostSpin = new QDoubleSpinBox(this);
    m_gainBoostSpin->setRange(-20.0, 40.0);
    m_gainBoostSpin->setSingleStep(1.0);
    m_gainBoostSpin->setValue(0.0);
    m_gainBoostSpin->setToolTip(tr("Manual gain boost in decibels. Positive values increase volume, negative values decrease it."));
    
    m_gainBoostLabel = new QLabel(tr("0.0 dB"), this);
    m_gainBoostLabel->setMinimumWidth(60);
    m_gainBoostLabel->setStyleSheet("QLabel { color: gray; }");
    
    m_autoGainCheck = new QCheckBox(tr("Enable Automatic Gain Control"), this);
    m_autoGainCheck->setChecked(false);
    m_autoGainCheck->setToolTip(tr("Automatically adjust gain to maintain consistent audio levels"));
    
    QLabel *autoGainTargetLabel = new QLabel(tr("AGC Target Level:"), this);
    m_autoGainTargetSpin = new QDoubleSpinBox(this);
    m_autoGainTargetSpin->setRange(0.01, 0.9);
    m_autoGainTargetSpin->setSingleStep(0.01);
    m_autoGainTargetSpin->setValue(0.1);
    m_autoGainTargetSpin->setDecimals(2);
    m_autoGainTargetSpin->setEnabled(false);
    m_autoGainTargetSpin->setToolTip(tr("Target audio level for automatic gain control (0.01 to 0.9)"));
    
    m_autoGainTargetLabel = new QLabel(tr("10%"), this);
    m_autoGainTargetLabel->setMinimumWidth(40);
    m_autoGainTargetLabel->setStyleSheet("QLabel { color: gray; }");
    m_autoGainTargetLabel->setEnabled(false);
    
    gainLayout->addWidget(gainBoostLabel, 0, 0);
    gainLayout->addWidget(m_gainBoostSpin, 0, 1);
    gainLayout->addWidget(m_gainBoostLabel, 0, 2);
    gainLayout->addWidget(m_autoGainCheck, 1, 0, 1, 3);
    gainLayout->addWidget(autoGainTargetLabel, 2, 0);
    gainLayout->addWidget(m_autoGainTargetSpin, 2, 1);
    gainLayout->addWidget(m_autoGainTargetLabel, 2, 2);
    
    // Output Options Group
    m_outputGroup = new QGroupBox(tr("Output Options"), this);
    QVBoxLayout *outputLayout = new QVBoxLayout(m_outputGroup);
    
    m_outputWindowCheck = new QCheckBox(tr("Output to Transcript Window"), this);
    m_outputWindowCheck->setChecked(true);
    
    // Add new checkbox for typing to active window
    m_outputTypeToWindowCheck = new QCheckBox(tr("Type to Active Window (hover for info)"), this);
    m_outputTypeToWindowCheck->setChecked(false);
    
    // Check if window typing is available and add status label
    if (WindowTyper::isAvailable()) {
        m_outputTypeToWindowCheck->setToolTip(tr("Types text to the currently active window\n"
                                                  "Requires xdotool on X11 or XWayland\n\n"
                                                  "Status: %1").arg(WindowTyper::availabilityMessage()));
    } else {
        m_outputTypeToWindowCheck->setEnabled(false);
        m_outputTypeToWindowCheck->setToolTip(tr("Window typing is not available\n"
                                                  "Requires xdotool on X11 or XWayland\n\n"
                                                  "Status: %1").arg(WindowTyper::availabilityMessage()));
    }
    
    m_outputFileCheck = new QCheckBox(tr("Output to File"), this);
    
    QHBoxLayout *fileLayout = new QHBoxLayout();
    m_outputFileLabel = new QLabel(tr("No file selected"), this);
    m_outputFileLabel->setStyleSheet("QLabel { color: gray; }");
    m_browseFileButton = new QPushButton(tr("Browse..."), this);
    m_browseFileButton->setEnabled(false);
    fileLayout->addWidget(m_outputFileLabel);
    fileLayout->addWidget(m_browseFileButton);
    
    m_outputClipboardCheck = new QCheckBox(tr("Copy to Clipboard"), this);
    m_timestampsCheck = new QCheckBox(tr("Include Timestamps"), this);
    m_timestampsCheck->setToolTip(tr("Include timestamps in transcript window and all outputs"));
    
    outputLayout->addWidget(m_outputWindowCheck);
    outputLayout->addWidget(m_outputTypeToWindowCheck);
    outputLayout->addWidget(m_outputFileCheck);
    outputLayout->addLayout(fileLayout);
    outputLayout->addWidget(m_outputClipboardCheck);
    outputLayout->addWidget(m_timestampsCheck);
    
    // Add all groups to main layout
    mainLayout->addWidget(m_modelGroup);
    mainLayout->addWidget(m_audioGroup);
    mainLayout->addWidget(m_vadGroup);
    mainLayout->addWidget(m_filterGroup);
    mainLayout->addWidget(m_gainGroup);
    mainLayout->addWidget(m_outputGroup);
    mainLayout->addStretch();
}

void ConfigWidget::connectSignals()
{
    connect(m_modelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConfigWidget::onModelChanged);
    connect(m_computeDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConfigWidget::onComputeDeviceChanged);
    connect(m_deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConfigWidget::onDeviceChanged);
    connect(m_audioSourceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConfigWidget::onAudioSourceChanged);
    connect(m_pickupSlider, &QSlider::valueChanged,
            this, &ConfigWidget::onPickupThresholdChanged);
    connect(m_minSpeechSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ConfigWidget::onMinSpeechDurationChanged);
    connect(m_maxSpeechSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ConfigWidget::onMaxSpeechDurationChanged);
    connect(m_bandpassCheck, &QCheckBox::toggled,
            this, &ConfigWidget::onBandpassToggled);
    connect(m_lowCutSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ConfigWidget::onLowCutChanged);
    connect(m_highCutSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ConfigWidget::onHighCutChanged);
    connect(m_timestampsCheck, &QCheckBox::toggled,
            this, &ConfigWidget::onTimestampsToggled);
    connect(m_outputWindowCheck, &QCheckBox::toggled,
            this, &ConfigWidget::onOutputOptionsChanged);
    connect(m_outputTypeToWindowCheck, &QCheckBox::toggled,
            this, &ConfigWidget::onOutputOptionsChanged);
    connect(m_outputFileCheck, &QCheckBox::toggled,
            this, &ConfigWidget::onOutputOptionsChanged);
    connect(m_outputClipboardCheck, &QCheckBox::toggled,
            this, &ConfigWidget::onOutputOptionsChanged);
    connect(m_browseFileButton, &QPushButton::clicked,
            this, &ConfigWidget::onBrowseOutputFile);
    connect(m_refreshDevicesButton, &QPushButton::clicked,
            this, &ConfigWidget::refreshAudioDevices);
    
    // Gain control connections
    connect(m_gainBoostSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ConfigWidget::onGainBoostChanged);
    connect(m_autoGainCheck, &QCheckBox::toggled,
            this, &ConfigWidget::onAutoGainToggled);
    connect(m_autoGainTargetSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ConfigWidget::onAutoGainTargetChanged);
    
    // Enable/disable filter controls based on checkbox
    connect(m_bandpassCheck, &QCheckBox::toggled, [this](bool checked) {
        m_lowCutSpin->setEnabled(checked);
        m_highCutSpin->setEnabled(checked);
    });
    
    // Enable/disable AGC target controls based on checkbox
    connect(m_autoGainCheck, &QCheckBox::toggled, [this](bool checked) {
        m_autoGainTargetSpin->setEnabled(checked);
        m_autoGainTargetLabel->setEnabled(checked);
    });
    
    // Enable/disable file browse button based on checkbox
    connect(m_outputFileCheck, &QCheckBox::toggled, [this](bool checked) {
        m_browseFileButton->setEnabled(checked);
    });
}

void ConfigWidget::populateModels()
{
    m_modelCombo->clear();
    m_modelCombo->addItems({
        "tiny.en", "tiny",
        "base.en", "base",
        "small.en", "small",
        "medium.en", "medium",
        "large-v1", "large-v2", "large-v3",
        "turbo"
    });
    
    // Set default to base
    m_modelCombo->setCurrentText("base");
}

void ConfigWidget::populateAudioDevices()
{
    m_deviceCombo->clear();
    
    // Check which audio source is selected
    QString source = m_audioSourceCombo->currentText().toLower();
    
    if (source.contains("speaker") || source.contains("system")) {
        // Use the new function to list PulseAudio sinks
        QMap<QString, QString> sinks = AudioCapture::listPulseAudioSinks();
        if (sinks.isEmpty()) {
            m_deviceCombo->addItem(tr("No PulseAudio sinks found"), "");
            m_deviceCombo->setToolTip(tr("Could not find any PulseAudio sinks. Is PulseAudio running?"));
        } else {
            for (auto it = sinks.constBegin(); it != sinks.constEnd(); ++it) {
                m_deviceCombo->addItem(it.key(), it.value());
            }
        }
    } else {
        // For microphone capture, we need regular input devices (excluding monitors)
        const auto devices = QMediaDevices::audioInputs();
        bool foundMic = false;
        
        for (const QAudioDevice &device : devices) {
            QString deviceName = device.description().toLower();
            QString deviceId = QString::fromUtf8(device.id()).toLower();
            
            // Exclude monitor devices from microphone list
            if (!deviceName.contains("monitor") && !deviceId.contains("monitor") &&
                !deviceName.contains("loopback") && !deviceId.contains("loopback")) {
                m_deviceCombo->addItem(device.description(), device.id());
                foundMic = true;
            }
        }
        
        if (!foundMic) {
            m_deviceCombo->addItem(tr("No audio input devices found"), "");
        }
    }
    
    if (m_deviceCombo->count() > 0) {
        m_deviceCombo->setCurrentIndex(0);
    }
}

AudioConfiguration ConfigWidget::getConfiguration() const
{
    return m_config;
}

void ConfigWidget::setConfiguration(const AudioConfiguration &config)
{
    m_config = config;
    
    // Update UI elements
    m_modelCombo->setCurrentText(config.model);
    m_audioSourceCombo->setCurrentText(config.audioSource);
    m_pickupSlider->setValue(config.pickupThreshold);
    m_minSpeechSpin->setValue(config.minSpeechDuration);
    m_maxSpeechSpin->setValue(config.maxSpeechDuration);
    m_bandpassCheck->setChecked(config.useBandpass);
    m_lowCutSpin->setValue(config.lowCutFreq);
    m_highCutSpin->setValue(config.highCutFreq);
    m_gainBoostSpin->setValue(config.gainBoostDb);
    m_gainBoostLabel->setText(QString("%1 dB").arg(config.gainBoostDb, 0, 'f', 1));
    m_autoGainCheck->setChecked(config.autoGainEnabled);
    m_autoGainTargetSpin->setValue(config.autoGainTarget);
    m_autoGainTargetLabel->setText(QString("%1%").arg(static_cast<int>(config.autoGainTarget * 100)));
    m_timestampsCheck->setChecked(config.includeTimestamps);
    m_outputWindowCheck->setChecked(true);  // Always show in transcript window
    m_outputTypeToWindowCheck->setChecked(config.outputToWindow);  // This is for typing to active window
    m_outputFileCheck->setChecked(config.outputToFile);
    m_outputClipboardCheck->setChecked(config.outputToClipboard);
    
    if (!config.outputFilePath.isEmpty()) {
        m_outputFileLabel->setText(config.outputFilePath);
    }
}

void ConfigWidget::saveSettings()
{
    // Save configuration to ConfigManager
    QJsonObject audioConfig;
    audioConfig["model"] = m_config.model;
    audioConfig["audioSource"] = m_config.audioSource;
    audioConfig["device"] = m_config.device;
    audioConfig["computeDeviceType"] = m_config.computeDeviceType;
    audioConfig["computeDeviceId"] = m_config.computeDeviceId;
    audioConfig["pickupThreshold"] = m_config.pickupThreshold;
    audioConfig["minSpeechDuration"] = m_config.minSpeechDuration;
    audioConfig["maxSpeechDuration"] = m_config.maxSpeechDuration;
    audioConfig["useBandpass"] = m_config.useBandpass;
    audioConfig["lowCutFreq"] = m_config.lowCutFreq;
    audioConfig["highCutFreq"] = m_config.highCutFreq;
    audioConfig["gainBoostDb"] = m_config.gainBoostDb;
    audioConfig["autoGainEnabled"] = m_config.autoGainEnabled;
    audioConfig["autoGainTarget"] = m_config.autoGainTarget;
    audioConfig["includeTimestamps"] = m_config.includeTimestamps;
    audioConfig["outputToWindow"] = m_config.outputToWindow;
    audioConfig["outputToFile"] = m_config.outputToFile;
    audioConfig["outputToClipboard"] = m_config.outputToClipboard;
    audioConfig["outputFilePath"] = m_config.outputFilePath;
    
    ConfigManager::instance().saveAudioConfiguration(audioConfig);
}

void ConfigWidget::loadSettings()
{
    // Load configuration from ConfigManager
    QJsonObject audioConfig = ConfigManager::instance().loadAudioConfiguration();
    
    if (!audioConfig.isEmpty()) {
        m_config.model = audioConfig.value("model").toString("base");
        m_config.audioSource = audioConfig.value("audioSource").toString("microphone");
        m_config.device = audioConfig.value("device").toString("");
        m_config.computeDeviceType = audioConfig.value("computeDeviceType").toInt(0);
        m_config.computeDeviceId = audioConfig.value("computeDeviceId").toInt(-1);
        m_config.pickupThreshold = audioConfig.value("pickupThreshold").toInt(120);
        m_config.minSpeechDuration = audioConfig.value("minSpeechDuration").toDouble(0.0);
        m_config.maxSpeechDuration = audioConfig.value("maxSpeechDuration").toDouble(10.0);
        m_config.useBandpass = audioConfig.value("useBandpass").toBool(true);
        m_config.lowCutFreq = audioConfig.value("lowCutFreq").toDouble(80.0);
        m_config.highCutFreq = audioConfig.value("highCutFreq").toDouble(6000.0);
        m_config.gainBoostDb = audioConfig.value("gainBoostDb").toDouble(0.0);
        m_config.autoGainEnabled = audioConfig.value("autoGainEnabled").toBool(false);
        m_config.autoGainTarget = audioConfig.value("autoGainTarget").toDouble(0.1);
        // Handle legacy settings
        if (audioConfig.contains("useTimestamps")) {
            m_config.includeTimestamps = audioConfig.value("useTimestamps").toBool(false);
        } else if (audioConfig.contains("showTimestampsInUI") || audioConfig.contains("includeTimestampsInOutput")) {
            // If either of the old split settings exist, use them
            bool showInUI = audioConfig.value("showTimestampsInUI").toBool(false);
            bool includeInOutput = audioConfig.value("includeTimestampsInOutput").toBool(false);
            m_config.includeTimestamps = showInUI || includeInOutput;  // Enable if either was enabled
        } else {
            m_config.includeTimestamps = audioConfig.value("includeTimestamps").toBool(false);
        }
        m_config.outputToWindow = audioConfig.value("outputToWindow").toBool(true);
        m_config.outputToFile = audioConfig.value("outputToFile").toBool(false);
        m_config.outputToClipboard = audioConfig.value("outputToClipboard").toBool(false);
        m_config.outputFilePath = audioConfig.value("outputFilePath").toString("");
    }
    
    setConfiguration(m_config);
}

void ConfigWidget::onModelChanged(int index)
{
    m_config.model = m_modelCombo->currentText();
    
    // Update description based on model
    QString desc;
    if (m_config.model.contains("tiny")) {
        desc = tr("Tiny: Fastest, least accurate (~39 MB)");
    } else if (m_config.model.contains("base")) {
        desc = tr("Base: Fast, good accuracy (~74 MB)");
    } else if (m_config.model.contains("small")) {
        desc = tr("Small: Balanced speed/accuracy (~244 MB)");
    } else if (m_config.model.contains("medium")) {
        desc = tr("Medium: Slower, better accuracy (~769 MB)");
    } else if (m_config.model.contains("large")) {
        desc = tr("Large: Slowest, best accuracy (~1550 MB)");
    } else if (m_config.model.contains("turbo")) {
        desc = tr("Turbo: Fast, high quality (~809 MB)");
    }
    
    m_modelDescLabel->setText(desc);
    
    // Update device dropdown colors based on memory requirements
    updateDeviceColors();
    
    emit modelChanged(m_config.model);
    emitConfigurationChanged();
}

void ConfigWidget::onDeviceChanged(int index)
{
    if (index >= 0) {
        m_config.device = m_deviceCombo->itemData(index).toString();
        emit deviceChanged(m_config.device);
        emitConfigurationChanged();
    }
}

void ConfigWidget::onAudioSourceChanged(int index)
{
    m_config.audioSource = m_audioSourceCombo->currentText().toLower();
    // Refresh device list based on new audio source
    populateAudioDevices();
    emitConfigurationChanged();
}

void ConfigWidget::onPickupThresholdChanged(int value)
{
    m_config.pickupThreshold = value;
    m_pickupLabel->setText(QString::number(value));
    emitConfigurationChanged();
}

void ConfigWidget::onMinSpeechDurationChanged(double value)
{
    m_config.minSpeechDuration = value;
    emitConfigurationChanged();
}

void ConfigWidget::onMaxSpeechDurationChanged(double value)
{
    m_config.maxSpeechDuration = value;
    emitConfigurationChanged();
}

void ConfigWidget::onBandpassToggled(bool checked)
{
    m_config.useBandpass = checked;
    emitConfigurationChanged();
}

void ConfigWidget::onLowCutChanged(double value)
{
    m_config.lowCutFreq = value;
    emitConfigurationChanged();
}

void ConfigWidget::onHighCutChanged(double value)
{
    m_config.highCutFreq = value;
    emitConfigurationChanged();
}

void ConfigWidget::onGainBoostChanged(double value)
{
    m_config.gainBoostDb = value;
    m_gainBoostLabel->setText(QString("%1 dB").arg(value, 0, 'f', 1));
    emitConfigurationChanged();
}

void ConfigWidget::onAutoGainToggled(bool checked)
{
    m_config.autoGainEnabled = checked;
    emitConfigurationChanged();
}

void ConfigWidget::onAutoGainTargetChanged(double value)
{
    m_config.autoGainTarget = value;
    m_autoGainTargetLabel->setText(QString("%1%").arg(static_cast<int>(value * 100)));
    emitConfigurationChanged();
}

void ConfigWidget::onTimestampsToggled(bool checked)
{
    m_config.includeTimestamps = checked;
    emitConfigurationChanged();
}

void ConfigWidget::onOutputOptionsChanged()
{
    m_config.outputToWindow = m_outputTypeToWindowCheck->isChecked();  // This now refers to typing to active window
    m_config.outputToFile = m_outputFileCheck->isChecked();
    m_config.outputToClipboard = m_outputClipboardCheck->isChecked();
    emitConfigurationChanged();
}

void ConfigWidget::onBrowseOutputFile()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Select Output File"),
        QString(),
        tr("Text Files (*.txt);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        m_config.outputFilePath = fileName;
        m_outputFileLabel->setText(fileName);
        emitConfigurationChanged();
    }
}

void ConfigWidget::refreshAudioDevices()
{
    populateAudioDevices();
}

void ConfigWidget::refreshComputeDevices()
{
    m_computeDeviceCombo->clear();
    
    // Get available devices from DeviceManager
    DeviceManager& deviceManager = DeviceManager::instance();
    QList<DeviceManager::DeviceInfo> devices = deviceManager.getAvailableDevices();
    
    // Store devices for later use in color updating
    m_availableDevices = devices;
    
    int defaultIndex = 0;
    int currentIndex = 0;
    
    for (const auto& device : devices) {
        QString displayName = DeviceManager::formatDeviceNameWithMemory(device);
        
        // Store device type and ID in the item data
        QVariantMap deviceData;
        deviceData["type"] = static_cast<int>(device.type);
        deviceData["id"] = device.deviceId;
        
        m_computeDeviceCombo->addItem(displayName, deviceData);
        
        // Check if this is the currently selected device
        if (device.type == m_config.computeDeviceType && 
            device.deviceId == m_config.computeDeviceId) {
            defaultIndex = currentIndex;
        }
        currentIndex++;
    }
    
    if (m_computeDeviceCombo->count() > 0) {
        m_computeDeviceCombo->setCurrentIndex(defaultIndex);
        
        // Update the description label
        if (defaultIndex < devices.size()) {
            const auto& device = devices[defaultIndex];
            if (device.type == DeviceManager::CPU) {
                m_computeDeviceLabel->setText(tr("Using CPU for inference"));
            } else {
                m_computeDeviceLabel->setText(tr("Using GPU for accelerated inference"));
            }
        }
    } else {
        m_computeDeviceLabel->setText(tr("No compute devices available"));
    }
    
    // Apply color coding based on current model
    updateDeviceColors();
}

void ConfigWidget::updateDeviceColors()
{
    // Get memory requirement for current model
    size_t requiredMemory = WhisperModels::getModelMemoryRequirement(m_config.model);
    
    // Update colors for each device in the dropdown
    for (int i = 0; i < m_computeDeviceCombo->count() && i < m_availableDevices.size(); ++i) {
        const auto& device = m_availableDevices[i];
        
        // Determine color based on memory availability
        QColor textColor;
        
        // Check if device has enough free memory (applies to both CPU and GPU)
        if (device.memoryFree >= requiredMemory) {
            textColor = QColor(0, 128, 0);  // Green - sufficient memory
        } else {
            textColor = QColor(200, 0, 0);  // Red - insufficient memory
        }
        
        // Apply color to the item
        m_computeDeviceCombo->setItemData(i, textColor, Qt::ForegroundRole);
    }
}

void ConfigWidget::onComputeDeviceChanged(int index)
{
    if (index >= 0 && index < m_computeDeviceCombo->count()) {
        QVariantMap deviceData = m_computeDeviceCombo->itemData(index).toMap();
        m_config.computeDeviceType = deviceData["type"].toInt();
        m_config.computeDeviceId = deviceData["id"].toInt();
        
        // Update description
        if (m_config.computeDeviceType == 0) { // CPU
            m_computeDeviceLabel->setText(tr("Using CPU for inference"));
        } else { // GPU
            m_computeDeviceLabel->setText(tr("Using GPU for accelerated inference"));
        }
        
        emit computeDeviceChanged(m_config.computeDeviceType, m_config.computeDeviceId);
        emitConfigurationChanged();
    }
}

void ConfigWidget::emitConfigurationChanged()
{
    emit configurationChanged(m_config);
}

void ConfigWidget::setRecordingState(bool isRecording)
{
    // Disable timestamp checkbox and other critical settings during recording
    m_timestampsCheck->setEnabled(!isRecording);
    
    // Optionally disable other settings that shouldn't change during recording
    m_modelCombo->setEnabled(!isRecording);
    m_computeDeviceCombo->setEnabled(!isRecording);
    m_audioSourceCombo->setEnabled(!isRecording);
    m_deviceCombo->setEnabled(!isRecording);
    m_refreshDevicesButton->setEnabled(!isRecording);
    
    // You might want to keep some settings enabled like volume threshold
    // m_pickupSlider->setEnabled(true);  // Keep this enabled if you want
    
    // Add a tooltip to explain why it's disabled
    if (isRecording) {
        m_timestampsCheck->setToolTip(tr("Cannot change timestamp settings during recording"));
        m_modelCombo->setToolTip(tr("Cannot change model during recording"));
        m_audioSourceCombo->setToolTip(tr("Cannot change audio source during recording"));
        m_deviceCombo->setToolTip(tr("Cannot change device during recording"));
    } else {
        m_timestampsCheck->setToolTip(tr("Include timestamps in transcript window and all outputs"));
        m_modelCombo->setToolTip("");
        m_audioSourceCombo->setToolTip("");
        m_deviceCombo->setToolTip("");
    }
}
