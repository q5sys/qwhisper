#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonValue>
#include <memory>

class ConfigManager : public QObject
{
    Q_OBJECT

public:
    static ConfigManager& instance();
    
    // Prevent copying
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    // Configuration file management
    bool loadConfig();
    bool saveConfig();
    void createDefaultConfig();
    
    // Model directory settings
    QString getModelsDirectory() const;
    void setModelsDirectory(const QString& path);
    bool isValidModelsDirectory(const QString& path) const;
    
    // Get the full path for a specific model
    QString getModelPath(const QString& modelName) const;
    
    // Application state settings
    void setApplicationState(const QString& key, const QJsonValue& value);
    QJsonValue getApplicationState(const QString& key, const QJsonValue& defaultValue = QJsonValue()) const;
    
    // Audio configuration
    void saveAudioConfiguration(const QJsonObject& config);
    QJsonObject loadAudioConfiguration() const;
    
    // Configuration file path
    QString getConfigFilePath() const;
    
signals:
    void modelsDirectoryChanged(const QString& newPath);
    void configurationSaved();
    void configurationLoaded();
    
private:
    explicit ConfigManager(QObject *parent = nullptr);
    ~ConfigManager();
    
    void ensureConfigDirectoryExists();
    bool migrateExistingModels(const QString& oldPath, const QString& newPath);
    
    QJsonObject m_config;
    QString m_configFilePath;
    
    // Default values
    static constexpr const char* CONFIG_FILE_NAME = "config.json";
    static constexpr const char* KEY_MODELS_DIRECTORY = "models_directory";
    static constexpr const char* KEY_APP_STATE = "application_state";
    static constexpr const char* KEY_AUDIO_CONFIG = "audio_configuration";
};

#endif // CONFIGMANAGER_H
