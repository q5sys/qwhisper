#include "outputmanager.h"
#include "fileoutput.h"
#include "windowtyper.h"
#include "../ui/configwidget.h"
#include <QClipboard>
#include <QApplication>

OutputManager::OutputManager(QObject *parent)
    : QObject(parent)
    , m_outputToClipboard(false)
{
    m_fileOutput = std::make_unique<FileOutput>(this);
    m_windowTyper = std::make_unique<WindowTyper>(this);
}

OutputManager::~OutputManager()
{
}

void OutputManager::updateConfiguration(const AudioConfiguration &config)
{
    m_outputToClipboard = config.outputToClipboard;
    
    if (config.outputToFile && !config.outputFilePath.isEmpty()) {
        m_fileOutput->setOutputFile(config.outputFilePath);
        m_fileOutput->setEnabled(true);
    } else {
        m_fileOutput->setEnabled(false);
    }
    
    // Enable/disable window typing
    m_windowTyper->setEnabled(config.outputToWindow);
}

void OutputManager::handleTranscription(const QString &text, qint64 timestamp)
{
    // Output to file if enabled
    if (m_fileOutput->isEnabled()) {
        m_fileOutput->writeTranscription(text, timestamp);
    }
    
    // Copy to clipboard if enabled
    if (m_outputToClipboard) {
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(text);
    }
    
    // Type to window if enabled
    if (m_windowTyper->isEnabled()) {
        m_windowTyper->typeText(text);
    }
}
