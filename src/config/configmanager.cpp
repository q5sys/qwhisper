#include "configmanager.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStandardPaths>
#include <QDebug>
#include <QFileInfo>

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
{
    // Determine config file path following XDG Base Directory specification
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString appConfigDir = QDir(configDir).filePath("qwhisper");
    m_configFilePath = QDir(appConfigDir).filePath(CONFIG_FILE_NAME);
    
    // Ensure config directory exists
    ensureConfigDirectoryExists();
    
    // Load existing config or create default
    if (!loadConfig()) {
        createDefaultConfig();
        saveConfig();
    }
}

ConfigManager::~ConfigManager()
{
    // Save any pending changes
    saveConfig();
}

ConfigManager& ConfigManager::instance()
{
    static ConfigManager instance;
    return instance;
}

void ConfigManager::ensureConfigDirectoryExists()
{
    QFileInfo fileInfo(m_configFilePath);
    QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

bool ConfigManager::loadConfig()
{
    QFile configFile(m_configFilePath);
    
    if (!configFile.exists()) {
        qDebug() << "Config file does not exist:" << m_configFilePath;
        return false;
    }
    
    if (!configFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open config file for reading:" << m_configFilePath;
        return false;
    }
    
    QByteArray data = configFile.readAll();
    configFile.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON in config file:" << m_configFilePath;
        return false;
    }
    
    m_config = doc.object();
    
    // Validate that models directory exists and is writable
    QString modelsDir = getModelsDirectory();
    if (!modelsDir.isEmpty()) {
        QDir dir(modelsDir);
        if (!dir.exists()) {
            // Try to create the directory
            if (!dir.mkpath(".")) {
                qWarning() << "Models directory does not exist and cannot be created:" << modelsDir;
                // Reset to default
                createDefaultConfig();
                return false;
            }
        }
    }
    
    emit configurationLoaded();
    return true;
}

bool ConfigManager::saveConfig()
{
    ensureConfigDirectoryExists();
    
    QFile configFile(m_configFilePath);
    if (!configFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open config file for writing:" << m_configFilePath;
        return false;
    }
    
    QJsonDocument doc(m_config);
    configFile.write(doc.toJson(QJsonDocument::Indented));
    configFile.close();
    
    qDebug() << "Configuration saved to:" << m_configFilePath;
    emit configurationSaved();
    return true;
}

void ConfigManager::createDefaultConfig()
{
    m_config = QJsonObject();
    
    // Set default models directory to the standard app data location
    QString defaultModelsDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    defaultModelsDir = QDir(defaultModelsDir).filePath("models");
    
    m_config[KEY_MODELS_DIRECTORY] = defaultModelsDir;
    
    // Ensure the default directory exists
    QDir dir(defaultModelsDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    qDebug() << "Created default configuration with models directory:" << defaultModelsDir;
}

QString ConfigManager::getModelsDirectory() const
{
    if (m_config.contains(KEY_MODELS_DIRECTORY)) {
        return m_config[KEY_MODELS_DIRECTORY].toString();
    }
    
    // Fallback to default
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(defaultDir).filePath("models");
}

void ConfigManager::setModelsDirectory(const QString& path)
{
    if (path.isEmpty()) {
        qWarning() << "Cannot set empty models directory path";
        return;
    }
    
    // Validate the path
    if (!isValidModelsDirectory(path)) {
        qWarning() << "Invalid models directory path:" << path;
        return;
    }
    
    QString oldPath = getModelsDirectory();
    
    // Update config
    m_config[KEY_MODELS_DIRECTORY] = path;
    
    // Save immediately
    if (saveConfig()) {
        // Optionally migrate existing models
        if (oldPath != path) {
            QDir oldDir(oldPath);
            QDir newDir(path);
            
            if (oldDir.exists() && newDir.exists()) {
                // Check if there are models to migrate
                QStringList filters;
                filters << "*.bin";
                QFileInfoList models = oldDir.entryInfoList(filters, QDir::Files);
                
                if (!models.isEmpty()) {
                    qDebug() << "Found" << models.size() << "models to potentially migrate";
                    // Note: Actual migration would be handled by UI with user confirmation
                }
            }
            
            emit modelsDirectoryChanged(path);
        }
    }
}

bool ConfigManager::isValidModelsDirectory(const QString& path) const
{
    if (path.isEmpty()) {
        return false;
    }
    
    QDir dir(path);
    
    // Check if directory exists or can be created
    if (!dir.exists()) {
        // Try to create it
        if (!dir.mkpath(".")) {
            qWarning() << "Cannot create directory:" << path;
            return false;
        }
    }
    
    // Check if directory is writable
    QFileInfo dirInfo(path);
    if (!dirInfo.isWritable()) {
        qWarning() << "Directory is not writable:" << path;
        return false;
    }
    
    return true;
}

QString ConfigManager::getModelPath(const QString& modelName) const
{
    QString modelsDir = getModelsDirectory();
    
    // Handle different model name formats
    QString fileName;
    if (modelName.startsWith("ggml-")) {
        fileName = modelName;
    } else {
        fileName = QString("ggml-%1.bin").arg(modelName);
    }
    
    return QDir(modelsDir).filePath(fileName);
}

QString ConfigManager::getConfigFilePath() const
{
    return m_configFilePath;
}

bool ConfigManager::migrateExistingModels(const QString& oldPath, const QString& newPath)
{
    if (oldPath == newPath) {
        return true;
    }
    
    QDir oldDir(oldPath);
    QDir newDir(newPath);
    
    if (!oldDir.exists()) {
        qDebug() << "Old models directory does not exist, nothing to migrate";
        return true;
    }
    
    if (!newDir.exists()) {
        if (!newDir.mkpath(".")) {
            qWarning() << "Failed to create new models directory:" << newPath;
            return false;
        }
    }
    
    // Get list of model files
    QStringList filters;
    filters << "*.bin";
    QFileInfoList models = oldDir.entryInfoList(filters, QDir::Files);
    
    bool success = true;
    for (const QFileInfo& modelInfo : models) {
        QString oldFilePath = modelInfo.absoluteFilePath();
        QString newFilePath = newDir.filePath(modelInfo.fileName());
        
        // Check if file already exists in new location
        if (QFile::exists(newFilePath)) {
            qDebug() << "Model already exists in new location:" << modelInfo.fileName();
            continue;
        }
        
        // Try to move the file
        if (QFile::rename(oldFilePath, newFilePath)) {
            qDebug() << "Migrated model:" << modelInfo.fileName();
        } else {
            // If move fails, try to copy
            if (QFile::copy(oldFilePath, newFilePath)) {
                qDebug() << "Copied model:" << modelInfo.fileName();
                // Try to remove the old file
                QFile::remove(oldFilePath);
            } else {
                qWarning() << "Failed to migrate model:" << modelInfo.fileName();
                success = false;
            }
        }
    }
    
    return success;
}

void ConfigManager::setApplicationState(const QString& key, const QJsonValue& value)
{
    // Get or create application state object
    QJsonObject appState;
    if (m_config.contains(KEY_APP_STATE)) {
        appState = m_config[KEY_APP_STATE].toObject();
    }
    
    // Set the value
    appState[key] = value;
    
    // Update config
    m_config[KEY_APP_STATE] = appState;
    
    // Save immediately
    saveConfig();
}

QJsonValue ConfigManager::getApplicationState(const QString& key, const QJsonValue& defaultValue) const
{
    if (m_config.contains(KEY_APP_STATE)) {
        QJsonObject appState = m_config[KEY_APP_STATE].toObject();
        if (appState.contains(key)) {
            return appState[key];
        }
    }
    return defaultValue;
}

void ConfigManager::saveAudioConfiguration(const QJsonObject& config)
{
    m_config[KEY_AUDIO_CONFIG] = config;
    saveConfig();
}

QJsonObject ConfigManager::loadAudioConfiguration() const
{
    if (m_config.contains(KEY_AUDIO_CONFIG)) {
        return m_config[KEY_AUDIO_CONFIG].toObject();
    }
    return QJsonObject();
}
