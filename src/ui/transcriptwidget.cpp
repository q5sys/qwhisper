#include "transcriptwidget.h"
#include <QTextEdit>
#include <QToolBar>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QAction>
#include <QMenu>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QScrollBar>
#include <QTimer>
#include <QClipboard>
#include <QApplication>
#include <QKeySequence>
#include <QFile>
#include <QRegularExpression>

TranscriptWidget::TranscriptWidget(QWidget *parent)
    : QWidget(parent)
    , m_autoScroll(true)
    , m_showTimestamps(false)
    , m_wordCount(0)
{
    m_sessionStartTime = QDateTime::currentDateTime();
    setupUi();
    createActions();
    createToolBar();
    
    // Update statistics every second
    QTimer *statsTimer = new QTimer(this);
    connect(statsTimer, &QTimer::timeout, this, &TranscriptWidget::updateStatistics);
    statsTimer->start(1000);
}

TranscriptWidget::~TranscriptWidget()
{
}

void TranscriptWidget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create toolbar (will be populated in createToolBar)
    m_toolBar = new QToolBar(this);
    mainLayout->addWidget(m_toolBar);
    
    // Create search bar
    QHBoxLayout *searchLayout = new QHBoxLayout();
    m_searchBar = new QLineEdit(this);
    m_searchBar->setPlaceholderText(tr("Search..."));
    m_searchBar->setVisible(false);
    m_searchButton = new QPushButton(tr("Find Next"), this);
    m_searchButton->setVisible(false);
    m_clearSearchButton = new QPushButton(tr("Clear"), this);
    m_clearSearchButton->setVisible(false);
    
    searchLayout->addWidget(m_searchBar);
    searchLayout->addWidget(m_searchButton);
    searchLayout->addWidget(m_clearSearchButton);
    searchLayout->addStretch();
    mainLayout->addLayout(searchLayout);
    
    // Create main text edit
    m_textEdit = new QTextEdit(this);
    m_textEdit->setAcceptRichText(true);
    m_textEdit->setFont(QFont("Consolas", 10));
    mainLayout->addWidget(m_textEdit);
    
    // Create status bar
    QHBoxLayout *statusLayout = new QHBoxLayout();
    m_wordCountLabel = new QLabel(tr("Words: 0"), this);
    m_sessionTimeLabel = new QLabel(tr("Session: 00:00:00"), this);
    m_timestampCheckBox = new QCheckBox(tr("Show Timestamps"), this);
    m_autoScrollCheckBox = new QCheckBox(tr("Auto-scroll"), this);
    m_autoScrollCheckBox->setChecked(m_autoScroll);
    
    statusLayout->addWidget(m_wordCountLabel);
    statusLayout->addWidget(new QLabel(" | ", this));
    statusLayout->addWidget(m_sessionTimeLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(m_timestampCheckBox);
    statusLayout->addWidget(m_autoScrollCheckBox);
    
    mainLayout->addLayout(statusLayout);
    
    // Connect signals
    connect(m_textEdit, &QTextEdit::textChanged, this, &TranscriptWidget::onTextChanged);
    connect(m_searchBar, &QLineEdit::textChanged, this, &TranscriptWidget::onSearchTextChanged);
    connect(m_searchBar, &QLineEdit::returnPressed, this, &TranscriptWidget::performSearch);
    connect(m_searchButton, &QPushButton::clicked, this, &TranscriptWidget::performSearch);
    connect(m_clearSearchButton, &QPushButton::clicked, [this]() {
        m_searchBar->clear();
        m_searchBar->setVisible(false);
        m_searchButton->setVisible(false);
        m_clearSearchButton->setVisible(false);
        highlightSearchResults();
    });
    connect(m_timestampCheckBox, &QCheckBox::toggled, this, &TranscriptWidget::setShowTimestamps);
    connect(m_autoScrollCheckBox, &QCheckBox::toggled, this, &TranscriptWidget::setAutoScroll);
}

void TranscriptWidget::createActions()
{
    m_saveAction = new QAction(QIcon(":/icons/save.png"), tr("&Save"), this);
    m_saveAction->setShortcut(QKeySequence::Save);
    connect(m_saveAction, &QAction::triggered, this, &TranscriptWidget::saveTranscript);
    
    m_saveAsAction = new QAction(QIcon(":/icons/save-as.png"), tr("Save &As..."), this);
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(m_saveAsAction, &QAction::triggered, this, &TranscriptWidget::saveTranscriptAs);
    
    m_clearAction = new QAction(QIcon(":/icons/clear.png"), tr("&Clear"), this);
    m_clearAction->setShortcut(QKeySequence("Ctrl+L"));
    connect(m_clearAction, &QAction::triggered, this, &TranscriptWidget::clearTranscript);
    
    m_findAction = new QAction(QIcon(":/icons/find.png"), tr("&Find..."), this);
    m_findAction->setShortcut(QKeySequence::Find);
    connect(m_findAction, &QAction::triggered, this, &TranscriptWidget::findText);
    
    m_copyAction = new QAction(QIcon(":/icons/copy.png"), tr("&Copy"), this);
    m_copyAction->setShortcut(QKeySequence::Copy);
    connect(m_copyAction, &QAction::triggered, this, &TranscriptWidget::copySelection);
    
    m_selectAllAction = new QAction(tr("Select &All"), this);
    m_selectAllAction->setShortcut(QKeySequence::SelectAll);
    connect(m_selectAllAction, &QAction::triggered, this, &TranscriptWidget::selectAll);
    
    m_exportTextAction = new QAction(tr("Export as &Text..."), this);
    connect(m_exportTextAction, &QAction::triggered, this, &TranscriptWidget::exportAsText);
    
    m_exportMarkdownAction = new QAction(tr("Export as &Markdown..."), this);
    connect(m_exportMarkdownAction, &QAction::triggered, this, &TranscriptWidget::exportAsMarkdown);
    
    m_exportRTFAction = new QAction(tr("Export as &RTF..."), this);
    connect(m_exportRTFAction, &QAction::triggered, this, &TranscriptWidget::exportAsRTF);
}

void TranscriptWidget::createToolBar()
{
    m_toolBar->addAction(m_saveAction);
    m_toolBar->addAction(m_saveAsAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_clearAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_findAction);
    m_toolBar->addAction(m_copyAction);
    m_toolBar->addSeparator();
    
    // Add export menu
    QPushButton *exportButton = new QPushButton(tr("Export"), this);
    QMenu *exportMenu = new QMenu(exportButton);
    exportMenu->addAction(m_exportTextAction);
    exportMenu->addAction(m_exportMarkdownAction);
    exportMenu->addAction(m_exportRTFAction);
    exportButton->setMenu(exportMenu);
    m_toolBar->addWidget(exportButton);
}

void TranscriptWidget::appendTranscription(const QString &text, qint64 timestamp)
{
    if (text.isEmpty()) return;
    
    // Store the entry
    TranscriptEntry entry;
    entry.text = text;
    entry.timestamp = QDateTime::fromMSecsSinceEpoch(timestamp);
    m_transcriptEntries.append(entry);
    
    // Format and append to text edit
    QString formattedText;
    if (m_showTimestamps) {
        formattedText = QString("[%1] %2").arg(formatTimestamp(timestamp), text);
    } else {
        formattedText = text;
    }
    
    // Append with formatting
    QTextCursor cursor = m_textEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    
    if (!m_textEdit->toPlainText().isEmpty()) {
        cursor.insertText("\n");
    }
    
    if (m_showTimestamps) {
        QTextCharFormat timestampFormat;
        timestampFormat.setForeground(Qt::gray);
        cursor.insertText(QString("[%1] ").arg(formatTimestamp(timestamp)), timestampFormat);
    }
    
    QTextCharFormat textFormat;
    // Use the window text color from the palette for proper theme contrast
    textFormat.setForeground(palette().color(QPalette::WindowText));
    cursor.insertText(text, textFormat);
    
    // Auto-scroll if enabled
    if (m_autoScroll) {
        QScrollBar *scrollBar = m_textEdit->verticalScrollBar();
        scrollBar->setValue(scrollBar->maximum());
    }
    
    emit transcriptChanged();
}

void TranscriptWidget::setAutoScroll(bool enabled)
{
    m_autoScroll = enabled;
}

void TranscriptWidget::setShowTimestamps(bool show)
{
    if (m_showTimestamps == show) return; // No change needed
    
    m_showTimestamps = show;
    
    // Rebuild the display without modifying the stored entries
    m_textEdit->clear();
    
    QTextCursor cursor = m_textEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    
    for (const auto &entry : m_transcriptEntries) {
        if (cursor.position() > 0) {
            cursor.insertText("\n");
        }
        
        if (m_showTimestamps) {
            QTextCharFormat timestampFormat;
            timestampFormat.setForeground(Qt::gray);
            cursor.insertText(QString("[%1] ").arg(entry.timestamp.toString("hh:mm:ss")), timestampFormat);
        }
        
        QTextCharFormat textFormat;
        textFormat.setForeground(palette().color(QPalette::WindowText));
        cursor.insertText(entry.text, textFormat);
    }
    
    // Restore auto-scroll position if enabled
    if (m_autoScroll) {
        QScrollBar *scrollBar = m_textEdit->verticalScrollBar();
        scrollBar->setValue(scrollBar->maximum());
    }
}

void TranscriptWidget::saveTranscript()
{
    if (m_currentFilePath.isEmpty()) {
        saveTranscriptAs();
        return;
    }
    
    QFile file(m_currentFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Save Error"), 
                           tr("Could not save file: %1").arg(file.errorString()));
        return;
    }
    
    QTextStream stream(&file);
    stream << getPlainTextContent();
    file.close();
    
    emit statusMessage(tr("Transcript saved to %1").arg(m_currentFilePath));
}

void TranscriptWidget::saveTranscriptAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, 
        tr("Save Transcript"), 
        QString(), 
        tr("Text Files (*.txt);;All Files (*)"));
    
    if (fileName.isEmpty()) return;
    
    m_currentFilePath = fileName;
    saveTranscript();
}

void TranscriptWidget::exportAsText()
{
    QString fileName = QFileDialog::getSaveFileName(this, 
        tr("Export as Text"), 
        QString(), 
        tr("Text Files (*.txt);;All Files (*)"));
    
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export Error"), 
                           tr("Could not export file: %1").arg(file.errorString()));
        return;
    }
    
    QTextStream stream(&file);
    stream << getPlainTextContent();
    file.close();
    
    emit statusMessage(tr("Exported to %1").arg(fileName));
}

void TranscriptWidget::exportAsMarkdown()
{
    QString fileName = QFileDialog::getSaveFileName(this, 
        tr("Export as Markdown"), 
        QString(), 
        tr("Markdown Files (*.md);;All Files (*)"));
    
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export Error"), 
                           tr("Could not export file: %1").arg(file.errorString()));
        return;
    }
    
    QTextStream stream(&file);
    stream << getMarkdownContent();
    file.close();
    
    emit statusMessage(tr("Exported to %1").arg(fileName));
}

void TranscriptWidget::exportAsRTF()
{
    QString fileName = QFileDialog::getSaveFileName(this, 
        tr("Export as RTF"), 
        QString(), 
        tr("Rich Text Files (*.rtf);;All Files (*)"));
    
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export Error"), 
                           tr("Could not export file: %1").arg(file.errorString()));
        return;
    }
    
    QTextStream stream(&file);
    stream << getRTFContent();
    file.close();
    
    emit statusMessage(tr("Exported to %1").arg(fileName));
}

void TranscriptWidget::clearTranscript()
{
    int ret = QMessageBox::question(this, tr("Clear Transcript"),
                                   tr("Are you sure you want to clear the transcript?"),
                                   QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        m_textEdit->clear();
        m_transcriptEntries.clear();
        m_wordCount = 0;
        m_sessionStartTime = QDateTime::currentDateTime();
        updateStatistics();
        emit transcriptChanged();
    }
}

void TranscriptWidget::findText()
{
    m_searchBar->setVisible(true);
    m_searchButton->setVisible(true);
    m_clearSearchButton->setVisible(true);
    m_searchBar->setFocus();
    m_searchBar->selectAll();
}

void TranscriptWidget::findNext()
{
    performSearch();
}

void TranscriptWidget::findPrevious()
{
    if (m_searchText.isEmpty()) return;
    
    QTextDocument *document = m_textEdit->document();
    QTextCursor cursor = m_textEdit->textCursor();
    
    cursor = document->find(m_searchText, cursor, QTextDocument::FindBackward);
    
    if (!cursor.isNull()) {
        m_textEdit->setTextCursor(cursor);
    }
}

void TranscriptWidget::toggleTimestamps()
{
    setShowTimestamps(!m_showTimestamps);
    m_timestampCheckBox->setChecked(m_showTimestamps);
}

void TranscriptWidget::copySelection()
{
    m_textEdit->copy();
}

void TranscriptWidget::selectAll()
{
    m_textEdit->selectAll();
}

void TranscriptWidget::updateStatistics()
{
    // Update word count
    QString text = m_textEdit->toPlainText();
    m_wordCount = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).count();
    m_wordCountLabel->setText(tr("Words: %1").arg(m_wordCount));
    
    // Update session time
    QDateTime now = QDateTime::currentDateTime();
    qint64 elapsed = m_sessionStartTime.secsTo(now);
    int hours = elapsed / 3600;
    int minutes = (elapsed % 3600) / 60;
    int seconds = elapsed % 60;
    m_sessionTimeLabel->setText(tr("Session: %1:%2:%3")
                                .arg(hours, 2, 10, QChar('0'))
                                .arg(minutes, 2, 10, QChar('0'))
                                .arg(seconds, 2, 10, QChar('0')));
}

void TranscriptWidget::onTextChanged()
{
    updateStatistics();
}

void TranscriptWidget::onSearchTextChanged(const QString &text)
{
    m_searchText = text;
    highlightSearchResults();
}

void TranscriptWidget::performSearch()
{
    if (m_searchText.isEmpty()) return;
    
    QTextDocument *document = m_textEdit->document();
    QTextCursor cursor = m_textEdit->textCursor();
    
    cursor = document->find(m_searchText, cursor);
    
    if (cursor.isNull()) {
        // Wrap around to beginning
        cursor = document->find(m_searchText);
    }
    
    if (!cursor.isNull()) {
        m_textEdit->setTextCursor(cursor);
    }
}

void TranscriptWidget::highlightSearchResults()
{
    QTextDocument *document = m_textEdit->document();
    
    // Clear previous highlights
    QTextCursor cursor(document);
    cursor.select(QTextCursor::Document);
    QTextCharFormat format;
    // Use the base color from palette instead of hardcoded white
    format.setBackground(palette().color(QPalette::Base));
    cursor.mergeCharFormat(format);
    
    if (m_searchText.isEmpty()) return;
    
    // Highlight all occurrences
    QTextCharFormat highlightFormat;
    highlightFormat.setBackground(Qt::yellow);
    
    cursor = document->find(m_searchText);
    while (!cursor.isNull()) {
        cursor.mergeCharFormat(highlightFormat);
        cursor = document->find(m_searchText, cursor);
    }
}

QString TranscriptWidget::formatTimestamp(qint64 timestamp) const
{
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(timestamp);
    return dt.toString("hh:mm:ss");
}

QString TranscriptWidget::getPlainTextContent() const
{
    QString content;
    for (const auto &entry : m_transcriptEntries) {
        if (m_showTimestamps) {
            content += QString("[%1] %2\n")
                      .arg(entry.timestamp.toString("hh:mm:ss"))
                      .arg(entry.text);
        } else {
            content += entry.text + "\n";
        }
    }
    return content;
}

QString TranscriptWidget::getMarkdownContent() const
{
    QString content = "# Transcript\n\n";
    content += QString("**Session Date:** %1\n\n")
              .arg(m_sessionStartTime.toString("yyyy-MM-dd hh:mm:ss"));
    content += QString("**Duration:** %1\n\n")
              .arg(m_sessionTimeLabel->text().mid(9)); // Remove "Session: " prefix
    content += QString("**Word Count:** %1\n\n").arg(m_wordCount);
    content += "---\n\n";
    
    for (const auto &entry : m_transcriptEntries) {
        if (m_showTimestamps) {
            content += QString("**[%1]** %2\n\n")
                      .arg(entry.timestamp.toString("hh:mm:ss"))
                      .arg(entry.text);
        } else {
            content += entry.text + "\n\n";
        }
    }
    return content;
}

QString TranscriptWidget::getRTFContent() const
{
    // Basic RTF header
    QString rtf = "{\\rtf1\\ansi\\deff0 {\\fonttbl{\\f0 Times New Roman;}}";
    rtf += "\\f0\\fs24 ";
    
    for (const auto &entry : m_transcriptEntries) {
        if (m_showTimestamps) {
            rtf += QString("[%1] %2\\par ")
                  .arg(entry.timestamp.toString("hh:mm:ss"))
                  .arg(entry.text);
        } else {
            rtf += entry.text + "\\par ";
        }
    }
    
    rtf += "}";
    return rtf;
}
