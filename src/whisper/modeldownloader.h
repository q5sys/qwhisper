#ifndef MODELDOWNLOADER_H
#define MODELDOWNLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QFile>
#include <memory>

class QProgressDialog;
class QWidget;

class ModelDownloader : public QObject
{
    Q_OBJECT

public:
    explicit ModelDownloader(QObject *parent = nullptr);
    ~ModelDownloader();
    
    void downloadModel(const QString &modelName, QWidget *parentWidget = nullptr);
    void cancelDownload();
    
    static QString getModelUrl(const QString &modelName);
    static qint64 getModelSize(const QString &modelName);

signals:
    void downloadComplete(const QString &modelName, const QString &filePath);
    void downloadFailed(const QString &modelName, const QString &error);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();
    void onDownloadError(QNetworkReply::NetworkError error);
    void onProgressDialogCanceled();

private:
    void startDownload(const QString &url, const QString &destinationPath);
    void cleanup();
    
    std::unique_ptr<QNetworkAccessManager> m_networkManager;
    QNetworkReply *m_currentReply;
    QString m_currentModelName;
    QString m_destinationPath;
    QProgressDialog *m_progressDialog;
    QFile *m_outputFile;
};

#endif // MODELDOWNLOADER_H
