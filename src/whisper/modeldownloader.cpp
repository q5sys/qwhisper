#include "modeldownloader.h"
#include "whispermodels.h"
#include <QProgressDialog>
#include <QNetworkRequest>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QDebug>
#include <QFileInfo>

ModelDownloader::ModelDownloader(QObject *parent)
    : QObject(parent)
    , m_currentReply(nullptr)
    , m_progressDialog(nullptr)
    , m_outputFile(nullptr)
{
    m_networkManager = std::make_unique<QNetworkAccessManager>(this);
}

ModelDownloader::~ModelDownloader()
{
    cleanup();
}

void ModelDownloader::downloadModel(const QString &modelName, QWidget *parentWidget)
{
    // Check if already downloading
    if (m_currentReply) {
        QMessageBox::warning(parentWidget, "Download in Progress", 
                           "A model download is already in progress. Please wait for it to complete.");
        return;
    }
    
    m_currentModelName = modelName;
    
    // Get the destination path
    m_destinationPath = WhisperModels::modelPath(modelName);
    
    // Create the models directory if it doesn't exist
    QFileInfo fileInfo(m_destinationPath);
    QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // Get the download URL
    QString url = getModelUrl(modelName);
    if (url.isEmpty()) {
        emit downloadFailed(modelName, "Invalid model name or URL not found");
        return;
    }
    
    // Create progress dialog
    m_progressDialog = new QProgressDialog(parentWidget);
    m_progressDialog->setWindowTitle(QString("Downloading %1 Model").arg(modelName));
    m_progressDialog->setLabelText(QString("Downloading %1 model...\nThis may take several minutes depending on your connection speed.").arg(modelName));
    m_progressDialog->setMinimum(0);
    m_progressDialog->setMaximum(100);
    m_progressDialog->setAutoClose(false);
    m_progressDialog->setAutoReset(false);
    m_progressDialog->setWindowModality(Qt::WindowModal);
    
    connect(m_progressDialog, &QProgressDialog::canceled, 
            this, &ModelDownloader::onProgressDialogCanceled);
    
    // Start the download
    startDownload(url, m_destinationPath);
    
    m_progressDialog->show();
}

void ModelDownloader::cancelDownload()
{
    if (m_currentReply) {
        m_currentReply->abort();
    }
    cleanup();
}

QString ModelDownloader::getModelUrl(const QString &modelName)
{
    // Hugging Face URLs for Whisper models
    QString baseUrl = "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/";
    QString fileName = QString("ggml-%1.bin").arg(modelName);
    
    // Map model names to their file names on Hugging Face
    QMap<QString, QString> modelMap;
    modelMap["tiny.en"] = "ggml-tiny.en.bin";
    modelMap["tiny"] = "ggml-tiny.bin";
    modelMap["base.en"] = "ggml-base.en.bin";
    modelMap["base"] = "ggml-base.bin";
    modelMap["small.en"] = "ggml-small.en.bin";
    modelMap["small"] = "ggml-small.bin";
    modelMap["medium.en"] = "ggml-medium.en.bin";
    modelMap["medium"] = "ggml-medium.bin";
    modelMap["large-v1"] = "ggml-large-v1.bin";
    modelMap["large-v2"] = "ggml-large-v2.bin";
    modelMap["large-v3"] = "ggml-large-v3.bin";
    modelMap["turbo"] = "ggml-large-v3-turbo.bin";
    
    if (modelMap.contains(modelName)) {
        return baseUrl + modelMap[modelName];
    }
    
    return QString();
}

qint64 ModelDownloader::getModelSize(const QString &modelName)
{
    // Return approximate model sizes in bytes
    if (modelName.contains("tiny")) {
        return 39 * 1024 * 1024;  // 39 MB
    } else if (modelName.contains("base")) {
        return 74 * 1024 * 1024;  // 74 MB
    } else if (modelName.contains("small")) {
        return 244 * 1024 * 1024;  // 244 MB
    } else if (modelName.contains("medium")) {
        return 769 * 1024 * 1024;  // 769 MB
    } else if (modelName.contains("large")) {
        return 1550 * 1024 * 1024;  // 1550 MB
    } else if (modelName.contains("turbo")) {
        return 809 * 1024 * 1024;  // 809 MB
    }
    return 100 * 1024 * 1024;  // Default 100 MB
}

void ModelDownloader::startDownload(const QString &url, const QString &destinationPath)
{
    // Create the output file
    m_outputFile = new QFile(destinationPath);
    if (!m_outputFile->open(QIODevice::WriteOnly)) {
        QString error = QString("Failed to create file: %1").arg(destinationPath);
        emit downloadFailed(m_currentModelName, error);
        cleanup();
        return;
    }
    
    // Create the network request
    QNetworkRequest request(url);
    // Note: Most modern servers handle redirects automatically
    // We'll rely on the server to provide direct download links
    request.setRawHeader("User-Agent", "QWhisper/1.0");
    
    // Start the download
    m_currentReply = m_networkManager->get(request);
    
    // Connect signals
    connect(m_currentReply, &QNetworkReply::downloadProgress,
            this, &ModelDownloader::onDownloadProgress);
    connect(m_currentReply, &QNetworkReply::finished,
            this, &ModelDownloader::onDownloadFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred,
            this, &ModelDownloader::onDownloadError);
    connect(m_currentReply, &QNetworkReply::readyRead,
            this, [this]() {
                if (m_outputFile && m_currentReply) {
                    m_outputFile->write(m_currentReply->readAll());
                }
            });
}

void ModelDownloader::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0 && m_progressDialog) {
        int progress = static_cast<int>((bytesReceived * 100) / bytesTotal);
        m_progressDialog->setValue(progress);
        
        // Update label with download progress
        QString label = QString("Downloading %1 model...\n"
                              "Progress: %2 MB / %3 MB\n"
                              "This may take several minutes depending on your connection speed.")
                              .arg(m_currentModelName)
                              .arg(bytesReceived / (1024 * 1024))
                              .arg(bytesTotal / (1024 * 1024));
        m_progressDialog->setLabelText(label);
    }
    
    emit downloadProgress(bytesReceived, bytesTotal);
}

void ModelDownloader::onDownloadFinished()
{
    if (!m_currentReply) {
        return;
    }
    
    if (m_currentReply->error() == QNetworkReply::NoError) {
        // Download successful
        if (m_outputFile) {
            m_outputFile->flush();
            m_outputFile->close();
        }
        
        if (m_progressDialog) {
            m_progressDialog->setValue(100);
            m_progressDialog->setLabelText(QString("Successfully downloaded %1 model!").arg(m_currentModelName));
            m_progressDialog->setCancelButtonText("Close");
        }
        
        emit downloadComplete(m_currentModelName, m_destinationPath);
        
        // Show success message
        if (m_progressDialog) {
            QMessageBox::information(m_progressDialog->parentWidget(), "Download Complete",
                                   QString("The %1 model has been successfully downloaded and is ready to use.")
                                   .arg(m_currentModelName));
        }
    } else {
        // Download failed
        QString error = m_currentReply->errorString();
        emit downloadFailed(m_currentModelName, error);
        
        // Delete the incomplete file
        if (m_outputFile) {
            m_outputFile->close();
            m_outputFile->remove();
        }
        
        // Show error message
        if (m_progressDialog) {
            QMessageBox::critical(m_progressDialog->parentWidget(), "Download Failed",
                                QString("Failed to download %1 model:\n%2")
                                .arg(m_currentModelName)
                                .arg(error));
        }
    }
    
    cleanup();
}

void ModelDownloader::onDownloadError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error)
    // Error will be handled in onDownloadFinished
}

void ModelDownloader::onProgressDialogCanceled()
{
    cancelDownload();
    
    // Delete the incomplete file
    if (m_outputFile && m_outputFile->isOpen()) {
        QString path = m_outputFile->fileName();
        m_outputFile->close();
        QFile::remove(path);
    }
    
    emit downloadFailed(m_currentModelName, "Download canceled by user");
}

void ModelDownloader::cleanup()
{
    if (m_currentReply) {
        m_currentReply->disconnect();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    
    if (m_outputFile) {
        if (m_outputFile->isOpen()) {
            m_outputFile->close();
        }
        delete m_outputFile;
        m_outputFile = nullptr;
    }
    
    if (m_progressDialog) {
        m_progressDialog->disconnect();
        m_progressDialog->deleteLater();
        m_progressDialog = nullptr;
    }
    
    m_currentModelName.clear();
    m_destinationPath.clear();
}
