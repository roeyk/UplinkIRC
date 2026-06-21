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
#include <QStringList>
#include "config/config.h"
#include "model/sessionmodel.h"
#include "ui/mainwindow.h"
#include "ui/themeloader.h"
#include "version.h"

// Returns the pre-existing dropdown IPC name used by the first implementation.
//
// `sendIpcCommand` still tries this name so users who have one old running
// process can toggle it once during an upgrade. New instances listen on the
// stable per-user name returned by `primaryIpcServerName`.
static QString legacyIpcServerName()
{
    const QString scope = QStandardPaths::writableLocation(
        QStandardPaths::AppConfigLocation);
    const QByteArray digest = QCryptographicHash::hash(
        scope.toUtf8(), QCryptographicHash::Sha256).toHex();
    return QStringLiteral("uplink-") + QString::fromLatin1(digest.left(16));
}

// Returns a KDE/WM-launch-stable local server name for this user's Uplink.
//
// The global shortcut command may run with a slightly different Qt standard
// path environment than a terminal-launched process. Using a per-user name
// keeps `Uplink --toggle-dropdown` pointed at the same already-running
// application instance in both launch paths.
static QString primaryIpcServerName()
{
    const QString user = qEnvironmentVariable("USER", QStringLiteral("user"));
    return QStringLiteral("uplink-") + user;
}

static QStringList ipcServerNames()
{
    QStringList names{primaryIpcServerName()};
    const QString legacy = legacyIpcServerName();
    if (!names.contains(legacy))
        names.append(legacy);
    return names;
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
    for (const QString &serverName : ipcServerNames()) {
        QLocalSocket socket;
        socket.connectToServer(serverName);

        // A short timeout keeps WM shortcut invocations responsive when Uplink
        // is not running; the caller can then start a new dropdown instance.
        if (!socket.waitForConnected(200))
            continue;

        socket.write((command + QStringLiteral("\t") + edge
                      + QStringLiteral("\n")).toUtf8());
        socket.flush();
        socket.waitForBytesWritten(200);
        socket.disconnectFromServer();
        return true;
    }

    return false;
}

int main(int argc, char *argv[])
{
#if defined(__linux__) && !defined(__MUSL__)
    mallopt(M_ARENA_MAX, 2); // cap glibc arenas; default (8×cores) wastes ~300 MiB RSS
#endif
    QApplication app(argc, argv);
    app.setApplicationName("Uplink");
    app.setApplicationVersion(UPLINK_VERSION);

    const QStringList args = app.arguments();
    const bool toggleDropdown = hasArgument(args, QStringLiteral("--toggle-dropdown"));
    const QString dropdownEdge = optionValue(args, QStringLiteral("--dropdown-edge"));

    if (toggleDropdown && sendIpcCommand(QStringLiteral("toggle-dropdown"), dropdownEdge))
        return 0;

    QPixmapCache::setCacheLimit(4096); // 4 MB — default is unlimited

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
    const QString serverName = primaryIpcServerName();
    if (!ipcServer.listen(serverName)) {
        QLocalServer::removeServer(serverName);
        ipcServer.listen(serverName);
    }

    if (toggleDropdown)
        window.showDropdown(dropdownEdge);
    else
        window.show();

    // Load config after MainWindow has connected its signals
    model->loadConfig(cfg);

    return app.exec();
}
