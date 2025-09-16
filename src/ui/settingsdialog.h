#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QDialogButtonBox;
class QTabWidget;
class QLineEdit;
QT_END_NAMESPACE

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

private slots:
    void onChangeModelDirectory();
    void onApply();
    void onRestoreDefaults();

private:
    void setupUi();
    void loadSettings();
    void saveSettings();
    
    // Storage tab widgets
    QLabel *m_modelDirLabel;
    QPushButton *m_changeModelDirButton;
    QLabel *m_modelDirPathLabel;
    
    // Dialog buttons
    QDialogButtonBox *m_buttonBox;
    QPushButton *m_restoreDefaultsButton;
    
    // Tab widget for future expansion
    QTabWidget *m_tabWidget;
};

#endif // SETTINGSDIALOG_H
