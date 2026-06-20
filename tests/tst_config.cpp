#include <QtTest/QtTest>

#include "config/config.h"

#include <QTemporaryDir>
#include <QFile>

class TstConfig : public QObject
{
    Q_OBJECT

private slots:
    void loadsServerPasswordFile();
    void savesServerPasswordFile();
};

// Writes a compact test config to disk.
//
// Called by config parser tests. `dir` owns the temporary directory and `body`
// is the server-specific TOML content. Returns the generated config path. This
// helper writes only test data and does not touch the user's real config.
static QString writeConfig(QTemporaryDir *dir, const QString &body)
{
    const QString path = dir->path() + QStringLiteral("/config.toml");
    QFile file(path);

    // Test setup must fail loudly if the temporary file cannot be created.
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return {};
    file.write("[ui]\n");
    file.write("theme = \"default\"\n\n");
    file.write("[[server]]\n");
    file.write("name = \"znc\"\n");
    file.write("host = \"10.100.10.1\"\n");
    file.write("nick = \"roey\"\n");
    file.write("user = \"roey/libera\"\n");
    file.write("realname = \"roey\"\n");
    file.write(body.toUtf8());
    file.close();
    return path;
}

void TstConfig::loadsServerPasswordFile()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString path = writeConfig(
        &dir,
        QStringLiteral("password_file = \"/home/roey/.config/halloy/znc-password\"\n"
                       "password_file_first_line_only = false\n"));
    QVERIFY(!path.isEmpty());

    const Config config = Config::load(path);

    QCOMPARE(config.servers.size(), 1);
    QCOMPARE(config.servers.first().passwordFile,
             QStringLiteral("/home/roey/.config/halloy/znc-password"));
    QCOMPARE(config.servers.first().passwordFileFirstLineOnly, false);
    QCOMPARE(config.servers.first().password, QString());
}

void TstConfig::savesServerPasswordFile()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    Config config;
    ServerConfig server;
    server.name = QStringLiteral("znc");
    server.host = QStringLiteral("10.100.10.1");
    server.nick = QStringLiteral("roey");
    server.user = QStringLiteral("roey/libera");
    server.realname = QStringLiteral("roey");
    server.passwordFile = QStringLiteral("~/.config/halloy/znc-password");
    server.passwordFileFirstLineOnly = false;
    config.servers << server;

    const QString path = dir.path() + QStringLiteral("/saved.toml");
    Config::save(config, path);

    QFile file(path);

    // The saved config should preserve the file reference and avoid writing a
    // plaintext password when only password_file is configured.
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString text = QString::fromUtf8(file.readAll());
    QVERIFY(text.contains(QStringLiteral("password_file")));
    QVERIFY(text.contains(QStringLiteral("~/.config/halloy/znc-password")));
    QVERIFY(text.contains(QStringLiteral("password_file_first_line_only = false")));
    QVERIFY(!text.contains(QStringLiteral("password          =")));
}

QTEST_MAIN(TstConfig)
#include "tst_config.moc"
