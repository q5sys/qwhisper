#include "whisperprocessor.h"
#include "../ui/configwidget.h"
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <vector>
#include <cmath>

// Include whisper.cpp header
extern "C" {
#include "include/whisper.h"
}

WhisperProcessor::WhisperProcessor(QObject *parent)
    : QObject(parent)
    , m_modelLoaded(false)
    , m_computeDeviceType(0)  // Default to CPU
    , m_computeDeviceId(-1)
    , m_whisperContext(nullptr)
    , m_contextParams(nullptr)
    , m_pickupThreshold(0.01f)  // Default VAD threshold
    , m_minSpeechDuration(5000)  // Default 5 seconds min
    , m_maxSpeechDuration(5000)  // Default 5 seconds max
    , m_silenceDuration(1000)    // 1 second of silence to stop recording
    , m_isRecording(false)
    , m_lastSoundTime(0)
    , m_speechStartTime(0)
{
}

WhisperProcessor::~WhisperProcessor()
{
    releaseWhisperContext();
}

void WhisperProcessor::processAudio(const QByteArray &audioData)
{
    if (!m_modelLoaded || !m_whisperContext) {
        qDebug() << "Model not loaded or context not initialized, skipping audio processing";
        return;
    }
    
    // Convert audio data to float samples (assuming 16-bit PCM)
    const int16_t* samples16 = reinterpret_cast<const int16_t*>(audioData.data());
    int sampleCount = audioData.size() / sizeof(int16_t);
    
    std::vector<float> samples(sampleCount);
    float maxAmplitude = 0.0f;
    float avgAmplitude = 0.0f;
    for (int i = 0; i < sampleCount; ++i) {
        samples[i] = samples16[i] / 32768.0f;
        float absVal = std::abs(samples[i]);
        maxAmplitude = std::max(maxAmplitude, absVal);
        avgAmplitude += absVal;
    }
    avgAmplitude /= sampleCount;
    
    // Add samples to buffer
    m_audioBuffer.insert(m_audioBuffer.end(), samples.begin(), samples.end());
    
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    // Voice Activity Detection - use average amplitude for better detection
    bool hasSound = avgAmplitude > m_pickupThreshold;
    
    // Debug output every second
    static qint64 lastDebugTime = 0;
    if (currentTime - lastDebugTime > 1000) {
        qDebug() << "Audio stats - Avg amplitude:" << avgAmplitude 
                 << "Max amplitude:" << maxAmplitude 
                 << "Threshold:" << m_pickupThreshold
                 << "Has sound:" << hasSound
                 << "Recording:" << m_isRecording
                 << "Buffer size:" << m_audioBuffer.size();
        lastDebugTime = currentTime;
    }
    
    if (hasSound) {
        m_lastSoundTime = currentTime;
        
        if (!m_isRecording) {
            // Start recording
            m_isRecording = true;
            m_speechStartTime = currentTime;
            qDebug() << "Speech detected, starting recording at threshold:" << m_pickupThreshold;
        }
    }
    
    if (m_isRecording) {
        qint64 speechDuration = currentTime - m_speechStartTime;
        qint64 silenceDuration = currentTime - m_lastSoundTime;
        
        // Check if we should stop recording
        bool shouldStop = false;
        QString stopReason;
        
        // Process if we've reached max duration
        if (speechDuration >= m_maxSpeechDuration) {
            shouldStop = true;
            stopReason = QString("max duration reached (%1ms)").arg(m_maxSpeechDuration);
        }
        // Or if we have silence after minimum duration
        else if (silenceDuration >= m_silenceDuration && speechDuration >= m_minSpeechDuration) {
            shouldStop = true;
            stopReason = QString("silence detected after min duration (%1ms silence, %2ms speech)")
                        .arg(silenceDuration).arg(speechDuration);
        }
        
        if (shouldStop) {
            qDebug() << "Stopping recording:" << stopReason 
                     << "- Buffer size:" << m_audioBuffer.size() << "samples"
                     << "(" << (m_audioBuffer.size() / 16000.0) << "seconds)";
            
            // Process the accumulated audio
            processAccumulatedAudio();
            
            // Reset for next speech segment
            m_audioBuffer.clear();
            m_isRecording = false;
            m_speechStartTime = 0;
        }
    }
    
    // Prevent buffer from growing too large when not recording
    if (!m_isRecording && m_audioBuffer.size() > 16000 * 2) { // Keep last 2 seconds
        m_audioBuffer.erase(m_audioBuffer.begin(), m_audioBuffer.begin() + (m_audioBuffer.size() - 16000 * 2));
    }
}

void WhisperProcessor::finishRecording()
{
    qDebug() << "finishRecording() called - Processing any remaining audio";
    
    // If we have audio in the buffer and we're currently recording, process it immediately
    if (m_isRecording && !m_audioBuffer.empty()) {
        qDebug() << "Processing remaining audio buffer on stop - Buffer size:" << m_audioBuffer.size() 
                 << "samples (" << (m_audioBuffer.size() / 16000.0) << "seconds)";
        
        // Process the accumulated audio regardless of duration/silence requirements
        processAccumulatedAudio();
        
        // Clear the buffer and reset recording state
        m_audioBuffer.clear();
        m_isRecording = false;
        m_speechStartTime = 0;
    } else if (!m_audioBuffer.empty()) {
        qDebug() << "Processing remaining audio buffer (not actively recording) - Buffer size:" << m_audioBuffer.size() 
                 << "samples (" << (m_audioBuffer.size() / 16000.0) << "seconds)";
        
        // Process any remaining audio even if not actively recording
        processAccumulatedAudio();
        m_audioBuffer.clear();
    } else {
        qDebug() << "No audio to process on stop";
    }
}

void WhisperProcessor::processAccumulatedAudio()
{
    if (m_audioBuffer.empty() || !m_whisperContext) {
        qDebug() << "Cannot process: buffer empty or no context";
        return;
    }
    
    qDebug() << "Processing accumulated audio - Buffer size:" << m_audioBuffer.size() 
             << "samples (" << (m_audioBuffer.size() / 16000.0) << "seconds)";
    
    // Process with whisper
    whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    wparams.print_progress = false;
    wparams.print_special = false;
    wparams.print_realtime = false;
    wparams.print_timestamps = false;
    wparams.single_segment = false;
    wparams.no_context = true;
    wparams.language = "en";
    wparams.n_threads = 4;
    wparams.suppress_blank = true;
    
    qDebug() << "Starting whisper processing...";
    int result = whisper_full(m_whisperContext, wparams, m_audioBuffer.data(), m_audioBuffer.size());
    
    if (result == 0) {
        // Get the transcription
        int n_segments = whisper_full_n_segments(m_whisperContext);
        qDebug() << "Whisper processing complete - Found" << n_segments << "segments";
        
        QString transcription;
        
        for (int i = 0; i < n_segments; ++i) {
            const char* text = whisper_full_get_segment_text(m_whisperContext, i);
            if (text) {
                QString segment = QString::fromUtf8(text).trimmed();
                qDebug() << "Segment" << i << ":" << segment;
                
                if (!segment.isEmpty() && segment != "[BLANK_AUDIO]") {
                    if (!transcription.isEmpty()) {
                        transcription += " ";
                    }
                    transcription += segment;
                }
            }
        }
        
        if (!transcription.isEmpty()) {
            qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
            emit transcriptionReady(transcription, timestamp);
            qDebug() << "Final transcription:" << transcription;
        } else {
            qDebug() << "No valid transcription found in segments";
        }
    } else {
        qDebug() << "Whisper processing failed with error code:" << result;
    }
}

void WhisperProcessor::loadModel(const QString &modelName)
{
    m_currentModel = modelName;
    emit statusChanged(QString("Loading model: %1").arg(modelName));
    
    // Release any existing context
    releaseWhisperContext();
    
    // Get model path
    QString modelPath = getModelPath(modelName);
    if (modelPath.isEmpty() || !QFile::exists(modelPath)) {
        emit statusChanged(QString("Model file not found: %1").arg(modelName));
        emit modelNotFound(modelName);  // Emit signal to trigger download prompt
        m_modelLoaded = false;
        return;
    }
    
    // Initialize whisper context with the model
    initializeWhisperContext();
    
    // Load the model with the configured parameters
    m_whisperContext = whisper_init_from_file_with_params(
        modelPath.toLocal8Bit().constData(), 
        *m_contextParams
    );
    
    if (m_whisperContext) {
        m_modelLoaded = true;
        emit statusChanged(QString("Model loaded: %1 (Device: %2)")
            .arg(modelName)
            .arg(m_computeDeviceType == 0 ? "CPU" : QString("GPU %1").arg(m_computeDeviceId)));
    } else {
        m_modelLoaded = false;
        emit statusChanged(QString("Failed to load model: %1").arg(modelName));
    }
}

void WhisperProcessor::updateConfiguration(const AudioConfiguration &config)
{
    // Check if compute device changed
    if (config.computeDeviceType != m_computeDeviceType || 
        config.computeDeviceId != m_computeDeviceId) {
        setComputeDevice(config.computeDeviceType, config.computeDeviceId);
    }
    
    // Apply configuration settings
    if (config.model != m_currentModel) {
        loadModel(config.model);
    }
    
    // Update VAD settings
    // Convert UI threshold (50-500) to amplitude threshold (0.001-0.05)
    m_pickupThreshold = config.pickupThreshold / 10000.0f;  // 120 -> 0.012
    m_minSpeechDuration = config.minSpeechDuration * 1000; // Convert to ms
    m_maxSpeechDuration = config.maxSpeechDuration * 1000; // Convert to ms
    
    qDebug() << "Updated VAD settings - UI Threshold:" << config.pickupThreshold
             << "-> Amplitude threshold:" << m_pickupThreshold
             << "Min duration:" << m_minSpeechDuration << "ms"
             << "Max duration:" << m_maxSpeechDuration << "ms";
}

void WhisperProcessor::setComputeDevice(int deviceType, int deviceId)
{
    if (m_computeDeviceType != deviceType || m_computeDeviceId != deviceId) {
        m_computeDeviceType = deviceType;
        m_computeDeviceId = deviceId;
        
        // If a model is loaded, reload it with the new device
        if (m_modelLoaded && !m_currentModel.isEmpty()) {
            loadModel(m_currentModel);
        }
    }
}

void WhisperProcessor::initializeWhisperContext()
{
    // Clean up any existing context params
    if (m_contextParams) {
        whisper_free_context_params(m_contextParams);
        m_contextParams = nullptr;
    }
    
    // Create new context params
    m_contextParams = whisper_context_default_params_by_ref();
    if (m_contextParams) {
        // Set device parameters
        if (m_computeDeviceType == 1) {  // CUDA
            m_contextParams->use_gpu = true;
            m_contextParams->gpu_device = m_computeDeviceId;
            qDebug() << "Initializing Whisper with CUDA device:" << m_computeDeviceId;
        } else {
            m_contextParams->use_gpu = false;
            qDebug() << "Initializing Whisper with CPU";
        }
        
        // Enable flash attention for better performance
        m_contextParams->flash_attn = true;
    }
}

void WhisperProcessor::releaseWhisperContext()
{
    if (m_whisperContext) {
        whisper_free(m_whisperContext);
        m_whisperContext = nullptr;
    }
    
    if (m_contextParams) {
        whisper_free_context_params(m_contextParams);
        m_contextParams = nullptr;
    }
    
    m_modelLoaded = false;
}

QString WhisperProcessor::getModelPath(const QString &modelName)
{
    // Check common model locations
    QStringList searchPaths;
    
    // User's home directory
    QString homeModels = QDir::homePath() + "/.cache/whisper";
    searchPaths << homeModels;
    
    // Application data directory
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/models";
    searchPaths << appData;
    
    // Current directory
    searchPaths << QDir::currentPath() + "/models";
    
    // Build directory (for development)
    searchPaths << QDir::currentPath() + "/build/models";
    
    // Check each path for the model file
    QString modelFileName = QString("ggml-%1.bin").arg(modelName);
    for (const QString &path : searchPaths) {
        QString fullPath = path + "/" + modelFileName;
        if (QFile::exists(fullPath)) {
            qDebug() << "Found model at:" << fullPath;
            return fullPath;
        }
    }
    
    qDebug() << "Model not found:" << modelFileName;
    qDebug() << "Searched paths:" << searchPaths;
    
    return QString();
}
