#ifndef OUTPUTMANAGER_H
#define OUTPUTMANAGER_H

#include <QObject>
#include <QString>
#include <memory>

struct AudioConfiguration;
class FileOutput;
class WindowTyper;

class OutputManager : public QObject
{
    Q_OBJECT

public:
    explicit OutputManager(QObject *parent = nullptr);
    ~OutputManager();
    
    void updateConfiguration(const AudioConfiguration &config);
    void handleTranscription(const QString &text, qint64 timestamp);

private:
    std::unique_ptr<FileOutput> m_fileOutput;
    std::unique_ptr<WindowTyper> m_windowTyper;
    bool m_outputToClipboard;
};

#endif // OUTPUTMANAGER_H
