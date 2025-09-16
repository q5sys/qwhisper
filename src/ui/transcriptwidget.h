#ifndef TRANSCRIPTWIDGET_H
#define TRANSCRIPTWIDGET_H

#include <QWidget>
#include <QDateTime>
#include <QMap>

QT_BEGIN_NAMESPACE
class QTextEdit;
class QToolBar;
class QLineEdit;
class QCheckBox;
class QLabel;
class QPushButton;
class QAction;
QT_END_NAMESPACE

class TranscriptWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TranscriptWidget(QWidget *parent = nullptr);
    ~TranscriptWidget();

    void appendTranscription(const QString &text, qint64 timestamp);
    void setAutoScroll(bool enabled);
    void setShowTimestamps(bool show);
    
public slots:
    void saveTranscript();
    void saveTranscriptAs();
    void exportAsText();
    void exportAsMarkdown();
    void exportAsRTF();
    void clearTranscript();
    void findText();
    void findNext();
    void findPrevious();
    void toggleTimestamps();
    void copySelection();
    void selectAll();
    void updateStatistics();

signals:
    void transcriptChanged();
    void statusMessage(const QString &message);

private slots:
    void onTextChanged();
    void onSearchTextChanged(const QString &text);
    void performSearch();

private:
    void setupUi();
    void createActions();
    void createToolBar();
    void highlightSearchResults();
    QString formatTimestamp(qint64 timestamp) const;
    QString getPlainTextContent() const;
    QString getMarkdownContent() const;
    QString getRTFContent() const;
    
    // UI Components
    QTextEdit *m_textEdit;
    QToolBar *m_toolBar;
    QLineEdit *m_searchBar;
    QCheckBox *m_timestampCheckBox;
    QCheckBox *m_autoScrollCheckBox;
    QLabel *m_wordCountLabel;
    QLabel *m_sessionTimeLabel;
    QPushButton *m_searchButton;
    QPushButton *m_clearSearchButton;
    
    // Actions
    QAction *m_saveAction;
    QAction *m_saveAsAction;
    QAction *m_clearAction;
    QAction *m_findAction;
    QAction *m_copyAction;
    QAction *m_selectAllAction;
    QAction *m_exportTextAction;
    QAction *m_exportMarkdownAction;
    QAction *m_exportRTFAction;
    
    // State
    bool m_autoScroll;
    bool m_showTimestamps;
    QString m_currentFilePath;
    QString m_searchText;
    QDateTime m_sessionStartTime;
    int m_wordCount;
    
    // Transcript data
    struct TranscriptEntry {
        QString text;
        QDateTime timestamp;
    };
    QList<TranscriptEntry> m_transcriptEntries;
};

#endif // TRANSCRIPTWIDGET_H
