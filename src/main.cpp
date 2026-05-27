#include <QApplication>
#include "config/config.h"
#include "ui/mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("UplinkIRC");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("LinuxDojo");

    const QString cfgPath = Config::defaultPath();
    Config cfg = Config::load(cfgPath);

    MainWindow window(cfg, cfgPath);
    window.show();

    return app.exec();
}
