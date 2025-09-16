#include "settingsdialog.h"
#include "../config/configmanager.h"
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Settings"));
    setModal(true);
    setMinimumWidth(500);
    
    setupUi();
    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Create tab widget for organized settings
    m_tabWidget = new QTabWidget(this);
    
    // Storage Tab
    QWidget *storageTab = new QWidget();
    QVBoxLayout *storageLayout = new QVBoxLayout(storageTab);
    
    // Model Storage Group
    QGroupBox *modelStorageGroup = new QGroupBox(tr("Model Storage Location"), storageTab);
    QGridLayout *modelStorageLayout = new QGridLayout(modelStorageGroup);
    
    m_modelDirLabel = new QLabel(tr("Models Directory:"), this);
    m_modelDirPathLabel = new QLabel(this);
    m_modelDirPathLabel->setWordWrap(true);
    m_modelDirPathLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 5px; border: 1px solid #ccc; }");
    
    m_changeModelDirButton = new QPushButton(tr("Change..."), this);
    m_changeModelDirButton->setMaximumWidth(100);
    
    modelStorageLayout->addWidget(m_modelDirLabel, 0, 0);
    modelStorageLayout->addWidget(m_modelDirPathLabel, 1, 0, 1, 2);
    modelStorageLayout->addWidget(m_changeModelDirButton, 2, 0);
    
    // Add information label
    QLabel *infoLabel = new QLabel(tr("Note: Changing the model directory will not move existing models. "
                                      "You will be prompted to migrate them if any are found."), this);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("QLabel { color: #666; font-style: italic; }");
    modelStorageLayout->addWidget(infoLabel, 3, 0, 1, 2);
    
    storageLayout->addWidget(modelStorageGroup);
    storageLayout->addStretch();
    
    // Add tabs
    m_tabWidget->addTab(storageTab, tr("Storage"));
    
    // Dialog buttons
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, this);
    
    m_restoreDefaultsButton = new QPushButton(tr("Restore Defaults"), this);
    m_buttonBox->addButton(m_restoreDefaultsButton, QDialogButtonBox::ResetRole);
    
    // Add widgets to main layout
    mainLayout->addWidget(m_tabWidget);
    mainLayout->addWidget(m_buttonBox);
    
    // Connect signals
    connect(m_changeModelDirButton, &QPushButton::clicked, this, &SettingsDialog::onChangeModelDirectory);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &SettingsDialog::onApply);
    connect(m_restoreDefaultsButton, &QPushButton::clicked, this, &SettingsDialog::onRestoreDefaults);
    
    // Save settings when OK is clicked
    connect(this, &QDialog::accepted, this, &SettingsDialog::saveSettings);
}

void SettingsDialog::loadSettings()
{
    // Load current model directory from ConfigManager
    QString currentDir = ConfigManager::instance().getModelsDirectory();
    m_modelDirPathLabel->setText(currentDir);
}

void SettingsDialog::saveSettings()
{
    // Settings are saved immediately when changed, so nothing to do here
    // This is kept for future expansion when we have more settings
}

void SettingsDialog::onChangeModelDirectory()
{
    // Get current directory
    QString currentDir = ConfigManager::instance().getModelsDirectory();
    
    // Open directory selection dialog
    QString newDir = QFileDialog::getExistingDirectory(this,
        tr("Select Models Directory"),
        currentDir,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (!newDir.isEmpty() && newDir != currentDir) {
        // Validate the directory
        if (ConfigManager::instance().isValidModelsDirectory(newDir)) {
            // Check if there are existing models in the old directory
            QDir oldDir(currentDir);
            QStringList filters;
            filters << "*.bin";
            QFileInfoList existingModels = oldDir.entryInfoList(filters, QDir::Files);
            
            bool shouldMigrate = false;
            if (!existingModels.isEmpty()) {
                // Ask user if they want to migrate existing models
                QMessageBox::StandardButton reply = QMessageBox::question(this,
                    tr("Migrate Existing Models"),
                    tr("Found %1 model(s) in the current directory.\n\n"
                       "Would you like to move them to the new location?\n\n"
                       "Note: This may take some time for large models.")
                       .arg(existingModels.size()),
                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
                
                if (reply == QMessageBox::Cancel) {
                    return;
                }
                shouldMigrate = (reply == QMessageBox::Yes);
            }
            
            // Update the configuration
            ConfigManager::instance().setModelsDirectory(newDir);
            m_modelDirPathLabel->setText(newDir);
            
            // Migrate models if requested
            if (shouldMigrate) {
                int migrated = 0;
                int failed = 0;
                
                for (const QFileInfo& modelInfo : existingModels) {
                    QString oldPath = modelInfo.absoluteFilePath();
                    QString newPath = QDir(newDir).filePath(modelInfo.fileName());
                    
                    if (QFile::exists(newPath)) {
                        // File already exists in new location
                        continue;
                    }
                    
                    if (QFile::rename(oldPath, newPath)) {
                        migrated++;
                    } else {
                        // Try to copy if move fails
                        if (QFile::copy(oldPath, newPath)) {
                            migrated++;
                            QFile::remove(oldPath);
                        } else {
                            failed++;
                        }
                    }
                }
                
                // Show migration results
                if (failed == 0) {
                    QMessageBox::information(this,
                        tr("Migration Complete"),
                        tr("Successfully migrated %1 model(s) to the new directory.")
                        .arg(migrated));
                } else {
                    QMessageBox::warning(this,
                        tr("Migration Partially Complete"),
                        tr("Migrated %1 model(s) successfully.\n"
                           "%2 model(s) could not be migrated and remain in the old directory.")
                        .arg(migrated).arg(failed));
                }
            }
            
            QMessageBox::information(this,
                tr("Settings Updated"),
                tr("Model directory has been updated to:\n%1").arg(newDir));
        } else {
            QMessageBox::critical(this,
                tr("Invalid Directory"),
                tr("The selected directory is not valid or not writable.\n"
                   "Please choose a different directory."));
        }
    }
}

void SettingsDialog::onApply()
{
    saveSettings();
    QMessageBox::information(this, tr("Settings Applied"), 
                           tr("Settings have been applied successfully."));
}

void SettingsDialog::onRestoreDefaults()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        tr("Restore Defaults"),
        tr("Are you sure you want to restore all settings to their default values?\n\n"
           "This will reset the model directory to:\n%1")
           .arg(QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath("models")),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Reset to default configuration
        ConfigManager::instance().createDefaultConfig();
        ConfigManager::instance().saveConfig();
        
        // Reload the UI
        loadSettings();
        
        QMessageBox::information(this,
            tr("Defaults Restored"),
            tr("All settings have been restored to their default values."));
    }
}
