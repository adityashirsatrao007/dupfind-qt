#include <QApplication>

#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("AdityaShirsatrao"));
    QApplication::setApplicationName(QStringLiteral("DupFindQt"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    qRegisterMetaType<ScanResult>();

    MainWindow window;
    window.show();
    return app.exec();
}
