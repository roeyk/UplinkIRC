#include <QApplication>
#include "ui/mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("UplinkIRC");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("LinuxDojo");

    MainWindow window;
    window.show();

    return app.exec();
}
