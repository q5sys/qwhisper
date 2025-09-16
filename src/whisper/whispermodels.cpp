#include "whispermodels.h"
#include "../config/configmanager.h"
#include <QDir>
#include <QStandardPaths>
#include <QFile>

WhisperModels::WhisperModels(QObject *parent)
    : QObject(parent)
{
}

WhisperModels::~WhisperModels()
{
}

QStringList WhisperModels::availableModels()
{
    return QStringList{
        "tiny.en", "tiny",
        "base.en", "base",
        "small.en", "small",
        "medium.en", "medium",
        "large-v1", "large-v2", "large-v3",
        "turbo"
    };
}

QString WhisperModels::modelPath(const QString &modelName)
{
    // Use ConfigManager to get the model path
    return ConfigManager::instance().getModelPath(modelName);
}

bool WhisperModels::isModelDownloaded(const QString &modelName)
{
    return QFile::exists(modelPath(modelName));
}

QString WhisperModels::modelDescription(const QString &modelName)
{
    if (modelName.contains("tiny")) {
        return "Tiny: Fastest, least accurate (~39 MB)";
    } else if (modelName.contains("base")) {
        return "Base: Fast, good accuracy (~74 MB)";
    } else if (modelName.contains("small")) {
        return "Small: Balanced speed/accuracy (~244 MB)";
    } else if (modelName.contains("medium")) {
        return "Medium: Slower, better accuracy (~769 MB)";
    } else if (modelName.contains("large")) {
        return "Large: Slowest, best accuracy (~1550 MB)";
    } else if (modelName.contains("turbo")) {
        return "Turbo: Fast, high quality (~809 MB)";
    }
    return "Unknown model";
}

size_t WhisperModels::getModelMemoryRequirement(const QString &modelName)
{
    // Return memory requirements in bytes
    // These are approximate values based on model sizes plus runtime overhead
    // We add ~2x overhead for runtime memory usage
    if (modelName.contains("tiny")) {
        return static_cast<size_t>(100) * 1024 * 1024;  // ~100 MB
    } else if (modelName.contains("base")) {
        return static_cast<size_t>(200) * 1024 * 1024;  // ~200 MB
    } else if (modelName.contains("small")) {
        return static_cast<size_t>(600) * 1024 * 1024;  // ~600 MB
    } else if (modelName.contains("medium")) {
        return static_cast<size_t>(1800) * 1024 * 1024; // ~1.8 GB
    } else if (modelName.contains("large")) {
        return static_cast<size_t>(3500) * 1024 * 1024; // ~3.5 GB
    } else if (modelName.contains("turbo")) {
        return static_cast<size_t>(1900) * 1024 * 1024; // ~1.9 GB
    }
    return static_cast<size_t>(200) * 1024 * 1024; // Default ~200 MB
}
