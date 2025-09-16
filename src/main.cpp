#include <QApplication>
#include <QStyleFactory>
#include <QSettings>
#include <QDebug>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application metadata
    QCoreApplication::setOrganizationName("qwhisper");
    QCoreApplication::setApplicationName("qwhisper");
    QCoreApplication::setApplicationVersion("1.0.0");
    
    // Set application style
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // High DPI support is automatic in Qt6, no need to set attributes
    
    // Create and show main window
    MainWindow window;
    window.show();
    
    return app.exec();
}
