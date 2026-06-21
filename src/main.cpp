#if defined(__linux__) && !defined(__MUSL__)
#include <malloc.h>
#endif
#include <QApplication>
#include <QCryptographicHash>
#include <QFont>
#include <QInputDialog>
#include <QLocalServer>
#include <QLocalSocket>
#include <QPixmapCache>
#include <QScopedPointer>
#include <QStandardPaths>
#include "config/config.h"
#include "model/sessionmodel.h"
#include "ui/mainwindow.h"
#include "ui/themeloader.h"
#include "version.h"

static QString ipcServerName()
{
    const QString scope = QStandardPaths::writableLocation(
        QStandardPaths::AppConfigLocation);
    const QByteArray digest = QCryptographicHash::hash(
        scope.toUtf8(), QCryptographicHash::Sha256).toHex();
    return QStringLiteral("uplink-") + QString::fromLatin1(digest.left(16));
}

static bool hasArgument(const QStringList &args, const QString &name)
{
    return args.contains(name);
}

static QString optionValue(const QStringList &args, const QString &name)
{
    const QString prefix = name + QStringLiteral("=");
    for (qsizetype i = 0; i < args.size(); ++i) {
        const QString &arg = args[i];
        if (arg.startsWith(prefix))
            return arg.mid(prefix.size());
        if (arg == name && i + 1 < args.size())
            return args[i + 1];
    }
    return {};
}

// Sends a command to an already-running Uplink process.
//
// Used by `--toggle-dropdown`, which lets a desktop environment or window
// manager bind a global shortcut without Uplink using desktop-specific global
// hotkey APIs. `edge` optionally overrides config for this invocation. Returns
// true only when a running instance received the command.
static bool sendIpcCommand(const QString &command, const QString &edge)
{
    QLocalSocket socket;
    socket.connectToServer(ipcServerName());

    // A short timeout keeps WM shortcut invocations responsive when Uplink is
    // not running; the caller can then start a new dropdown instance.
    if (!socket.waitForConnected(200))
        return false;

    socket.write((command + QStringLiteral("\t") + edge
                  + QStringLiteral("\n")).toUtf8());
    socket.flush();
    socket.waitForBytesWritten(200);
    socket.disconnectFromServer();
    return true;
}

int main(int argc, char *argv[])
{
#if defined(__linux__) && !defined(__MUSL__)
    mallopt(M_ARENA_MAX, 2);
    mallopt(M_TRIM_THRESHOLD, 64 * 1024);
#endif
    QApplication app(argc, argv);
    QPixmapCache::setCacheLimit(2048); // 2 MB
    app.setApplicationName("Uplink");
    app.setApplicationVersion(UPLINK_VERSION);

    const QStringList args = app.arguments();
    const bool toggleDropdown = hasArgument(args, QStringLiteral("--toggle-dropdown"));
    const QString dropdownEdge = optionValue(args, QStringLiteral("--dropdown-edge"));

    if (toggleDropdown && sendIpcCommand(QStringLiteral("toggle-dropdown"), dropdownEdge))
        return 0;

#if defined(Q_OS_WIN)
    // Use native Windows rendering as the base style.
    // Custom themes layer QSS on top; the default theme leaves it pure native.
    if (!QApplication::setStyle("windows11"))
        if (!QApplication::setStyle("windowsvista"))
            QApplication::setStyle("windows");
#endif

    const QString cfgPath = Config::defaultPath();
    Config cfg = Config::load(cfgPath);
    ThemeLoader::ensureUserThemesDir();

    QFont font(cfg.ui.fontFamily);
    font.setStyleHint(QFont::Monospace);
    app.setFont(font);

    // First-run nick setup
    if (cfg.needsNickSetup()) {
        bool ok = false;
        const QString nick = QInputDialog::getText(
            nullptr, "Uplink — Set Your Nick",
            "Choose a nickname:", QLineEdit::Normal, "", &ok);
        if (ok && !nick.trimmed().isEmpty()) {
            for (auto &s : cfg.servers)
                if (s.nick == "yournick" || s.nick.isEmpty())
                    s.nick = nick.trimmed();
            Config::save(cfg, cfgPath);
        }
    }

    QScopedPointer<SessionModel> model(new SessionModel);

    MainWindow window(model.get(), cfg);

    QLocalServer ipcServer;
    QObject::connect(&ipcServer, &QLocalServer::newConnection, &window, [&]{
        while (auto *client = ipcServer.nextPendingConnection()) {
            QObject::connect(client, &QLocalSocket::readyRead, &window, [client, &window]{
                const QString line = QString::fromUtf8(client->readAll()).trimmed();
                const QString command = line.section(u'\t', 0, 0);
                const QString edge = line.section(u'\t', 1);
                if (command == QStringLiteral("toggle-dropdown"))
                    window.toggleDropdown(edge);
            });
            QObject::connect(client, &QLocalSocket::disconnected,
                             client, &QLocalSocket::deleteLater);
        }
    });
    if (!ipcServer.listen(ipcServerName())) {
        QLocalServer::removeServer(ipcServerName());
        ipcServer.listen(ipcServerName());
    }

    if (toggleDropdown)
        window.showDropdown(dropdownEdge);
    else
        window.show();

    // Load config after MainWindow has connected its signals
    model->loadConfig(cfg);

    return app.exec();
}
