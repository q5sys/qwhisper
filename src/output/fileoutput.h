#ifndef FILEOUTPUT_H
#define FILEOUTPUT_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <memory>

class FileOutput : public QObject
{
    Q_OBJECT

public:
    explicit FileOutput(QObject *parent = nullptr);
    ~FileOutput();
    
    void setOutputFile(const QString &filePath);
    void setEnabled(bool enabled);
    bool isEnabled() const;
    void writeTranscription(const QString &text, qint64 timestamp);

private:
    QString m_filePath;
    std::unique_ptr<QFile> m_file;
    std::unique_ptr<QTextStream> m_stream;
    bool m_enabled;
};

#endif // FILEOUTPUT_H
