#ifndef WHISPERMODELS_H
#define WHISPERMODELS_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <cstddef>

class WhisperModels : public QObject
{
    Q_OBJECT

public:
    explicit WhisperModels(QObject *parent = nullptr);
    ~WhisperModels();
    
    static QStringList availableModels();
    static QString modelPath(const QString &modelName);
    static bool isModelDownloaded(const QString &modelName);
    static QString modelDescription(const QString &modelName);
    static size_t getModelMemoryRequirement(const QString &modelName);
};

#endif // WHISPERMODELS_H
