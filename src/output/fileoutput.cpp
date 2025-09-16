#include "fileoutput.h"
#include <QDateTime>

FileOutput::FileOutput(QObject *parent)
    : QObject(parent)
    , m_enabled(false)
{
}

FileOutput::~FileOutput()
{
    if (m_file && m_file->isOpen()) {
        m_file->close();
    }
}

void FileOutput::setOutputFile(const QString &filePath)
{
    m_filePath = filePath;
    
    // Close existing file if open
    if (m_file && m_file->isOpen()) {
        m_file->close();
    }
    
    // Open new file
    m_file = std::make_unique<QFile>(m_filePath);
    if (m_file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_stream = std::make_unique<QTextStream>(m_file.get());
    }
}

void FileOutput::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool FileOutput::isEnabled() const
{
    return m_enabled;
}

void FileOutput::writeTranscription(const QString &text, qint64 timestamp)
{
    if (!m_enabled || !m_stream) return;
    
    QString timestampStr = QDateTime::fromMSecsSinceEpoch(timestamp).toString("hh:mm:ss");
    *m_stream << "[" << timestampStr << "] " << text << Qt::endl;
    m_stream->flush();
}
