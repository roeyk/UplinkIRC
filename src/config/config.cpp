#include "config.h"
#include "keychainhelper.h"

#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QStandardPaths>
#include <QDebug>

#include <toml++/toml.hpp>

static const QString kKeychainSentinel = QStringLiteral("<keychain>");

static QString kcKey(const QString &serverName, const char *field)
{
    return serverName + QLatin1Char(':') + QLatin1String(field);
}

// Write a password to keychain and return the sentinel, or return empty.
static QString storePassword(const QString &value, const QString &serverName, const char *field)
{
    if (value.isEmpty()) {
        KeychainHelper::remove(kcKey(serverName, field));
        return {};
    }
    if (KeychainHelper::write(kcKey(serverName, field), value))
        return kKeychainSentinel;

    qWarning() << "Uplink: keychain write failed for" << serverName << field
               << "— storing in config";
    return value;
}

static QString tomlQuote(QString s)
{
    s.replace("\\", "\\\\");
    s.replace("\"", "\\\"");
    s.replace("\n", "\\n");
    s.replace("\r", "\\r");
    s.replace("\t", "\\t");
    return "\"" + s + "\"";
}

static const char *kDefaultConfig = R"(
[ui]
theme             = "default"
show_nick_prefix  = true
show_topic        = true
show_emoji_button = false
colored_nicks     = true
typing_indicator  = true
hanging_indent    = true
nick_brackets     = "<>"         # "<>" angle, "[]" square, "::::" double-colon, "" none
font_family       = "IBM Plex Mono"
font_toolbar    = 10
font_sidebar    = 10
font_chat       = 10
font_nick_list  = 10
font_nick_dock  = 10
font_topic_bar  = 10
font_input_nick = 10
font_input      = 10

[[server]]
name     = "LinuxDojo"
host     = "irc.linuxdojo.org"
port     = 6697
ssl      = true
nick     = "yournick"
user     = "yournick"
realname = "Uplink User"
channels = "#uplink"
)";

// ---------------------------------------------------------------------------

QString Config::defaultPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
           + "/.config/uplink/config.toml";
}

void Config::ensureExists(const QString &path)
{
    QFileInfo fi(path);
    if (fi.exists() && fi.size() > 0) return;

    QDir().mkpath(fi.absolutePath());
    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        f.write(kDefaultConfig);
        f.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    } else {
        qWarning() << "Uplink: could not create config at" << path;
    }
}

Config Config::load(const QString &path)
{
    ensureExists(path);

    Config cfg;

    try {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Uplink: could not open config at" << path;
            return cfg;
        }
        const std::string content = f.readAll().toStdString();
        auto tbl = toml::parse(content, path.toStdString());

        // [ui]
        if (auto ui = tbl["ui"].as_table()) {
            cfg.ui.theme           = QString::fromStdString((*ui)["theme"].value_or<std::string>("default"));
            cfg.ui.showNickPrefix  = (*ui)["show_nick_prefix"].value_or(true);
            cfg.ui.showTopic       = (*ui)["show_topic"].value_or(true);
            cfg.ui.showEmojiButton = (*ui)["show_emoji_button"].value_or(false);
            cfg.ui.coloredNicks          = (*ui)["colored_nicks"].value_or(true);
            cfg.ui.typingIndicator       = (*ui)["typing_indicator"].value_or(true);
            cfg.ui.hangingIndent         = (*ui)["hanging_indent"].value_or(true);
            cfg.ui.logMessages           = (*ui)["log_messages"].value_or(false);
            cfg.ui.appIcon               = QString::fromStdString((*ui)["app_icon"].value_or<std::string>("dark"));
            cfg.ui.nickBrackets          = QString::fromStdString((*ui)["nick_brackets"].value_or<std::string>("<>"));
            cfg.ui.notifications         = (*ui)["notifications"].value_or(true);
            cfg.ui.fontFamily            = QString::fromStdString((*ui)["font_family"].value_or<std::string>(kDefaultFontFamily));
            cfg.ui.fontSizes.toolbar      = (*ui)["font_toolbar"].value_or(10);
            cfg.ui.fontSizes.serverHeader = (*ui)["font_server_header"].value_or(9);
            cfg.ui.fontSizes.sidebar      = (*ui)["font_sidebar"].value_or(10);
            cfg.ui.fontSizes.chat         = (*ui)["font_chat"].value_or(10);
            cfg.ui.fontSizes.nickList     = (*ui)["font_nick_list"].value_or(10);
            cfg.ui.fontSizes.nickDock     = (*ui)["font_nick_dock"].value_or(10);
            cfg.ui.fontSizes.topicBar     = (*ui)["font_topic_bar"].value_or(10);
            cfg.ui.fontSizes.inputNick    = (*ui)["font_input_nick"].value_or(10);
            cfg.ui.fontSizes.input        = (*ui)["font_input"].value_or(10);
            cfg.ui.fontSizes.typing       = (*ui)["font_typing"].value_or(9);
            cfg.ui.fontSizes.emoji        = (*ui)["font_emoji"].value_or(16);
        }

        // [privacy]
        if (auto priv = tbl["privacy"].as_table())
            cfg.ui.linkPreviews = (*priv)["link_previews"].value_or(false);

        // [ignore]
        if (auto ign = tbl["ignore"].as_table()) {
            if (auto nicks = (*ign)["nicks"].as_array()) {
                for (auto &n : *nicks) {
                    if (auto v = n.value<std::string>()) {
                        const QString nick = QString::fromStdString(*v).trimmed().toLower();
                        if (!nick.isEmpty())
                            cfg.ignoredNicks.append(nick);
                    }
                }
            }
        }

        // [monitor]
        if (auto mon = tbl["monitor"].as_table()) {
            if (auto nicks = (*mon)["nicks"].as_array()) {
                for (auto &n : *nicks) {
                    if (auto v = n.value<std::string>()) {
                        const QString nick = QString::fromStdString(*v).trimmed();
                        if (!nick.isEmpty())
                            cfg.monitorList.append(nick);
                    }
                }
            }
        }

        // [[server]]
        if (auto servers = tbl["server"].as_array()) {
            for (auto &node : *servers) {
                auto *s = node.as_table();
                if (!s) continue;

                ServerConfig sc;
                sc.name     = QString::fromStdString((*s)["name"].value_or<std::string>(""));
                sc.host     = QString::fromStdString((*s)["host"].value_or<std::string>(""));
                sc.port     = static_cast<quint16>((*s)["port"].value_or(6697));
                sc.ssl      = (*s)["ssl"].value_or(true);
                sc.nick     = QString::fromStdString((*s)["nick"].value_or<std::string>(""));
                sc.user     = QString::fromStdString((*s)["user"].value_or<std::string>("uplink"));
                sc.realname = QString::fromStdString((*s)["realname"].value_or<std::string>("Uplink User"));
                sc.password     = QString::fromStdString((*s)["password"].value_or<std::string>(""));
                sc.saslUser         = QString::fromStdString((*s)["sasl_user"].value_or<std::string>(""));
                sc.saslPassword     = QString::fromStdString((*s)["sasl_password"].value_or<std::string>(""));
                sc.saslExternal     = (*s)["sasl_external"].value_or(false);
                sc.clientCertFile   = QString::fromStdString((*s)["client_cert"].value_or<std::string>(""));
                sc.clientKeyFile    = QString::fromStdString((*s)["client_key"].value_or<std::string>(""));
                sc.nickservPassword = QString::fromStdString((*s)["nickserv_password"].value_or<std::string>(""));
                const std::string bt = (*s)["bouncer"].value_or<std::string>("none");
                if      (bt == "znc")  sc.bouncerType = BouncerType::ZNC;
                else if (bt == "soju") sc.bouncerType = BouncerType::Soju;
                else                   sc.bouncerType = BouncerType::None;
                sc.bouncerNetwork = QString::fromStdString((*s)["bouncer_network"].value_or<std::string>(""));
                sc.proxyHost = QString::fromStdString((*s)["proxy_host"].value_or<std::string>(""));
                sc.proxyPort = static_cast<quint16>((*s)["proxy_port"].value_or(1080));
                sc.proxyUser = QString::fromStdString((*s)["proxy_user"].value_or<std::string>(""));
                sc.proxyPass = QString::fromStdString((*s)["proxy_pass"].value_or<std::string>(""));
                sc.pinnedFingerprint = QString::fromStdString((*s)["ssl_fingerprint"].value_or<std::string>(""));

                if (auto chans = (*s)["channel"].as_array()) {
                    for (auto &cnode : *chans) {
                        auto *c = cnode.as_table();
                        if (!c) continue;
                        ChannelConfig cc;
                        cc.name     = QString::fromStdString((*c)["name"].value_or<std::string>(""));
                        cc.password = QString::fromStdString((*c)["key"].value_or<std::string>(""));
                        if (!cc.name.isEmpty())
                            sc.channels.append(cc);
                    }
                } else {
                    // backward compat: old comma-separated string
                    const std::string chanStr = (*s)["channels"].value_or<std::string>("");
                    for (const QString &part : QString::fromStdString(chanStr).split(',', Qt::SkipEmptyParts)) {
                        const QString name = part.trimmed();
                        if (!name.isEmpty())
                            sc.channels.append({name, {}});
                    }
                }

                if (!sc.host.isEmpty())
                    cfg.servers.append(sc);
            }
        }

    } catch (const toml::parse_error &e) {
        qWarning() << "Uplink: config parse error:" << e.what();
    }

    return cfg;
}

void Config::save(const Config &cfg, const QString &path)
{
    QDir().mkpath(QFileInfo(path).absolutePath());

    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Uplink: could not open config for writing at" << path;
        return;
    }

    QTextStream out(&f);

    out << "[ui]\n";
    out << "theme             = " << tomlQuote(cfg.ui.theme)       << "\n";
    out << "show_nick_prefix  = " << (cfg.ui.showNickPrefix  ? "true" : "false") << "\n";
    out << "show_topic        = " << (cfg.ui.showTopic       ? "true" : "false") << "\n";
    out << "show_emoji_button = " << (cfg.ui.showEmojiButton ? "true" : "false") << "\n";
    out << "colored_nicks     = " << (cfg.ui.coloredNicks     ? "true" : "false") << "\n";
    out << "typing_indicator  = " << (cfg.ui.typingIndicator  ? "true" : "false") << "\n";
    out << "hanging_indent    = " << (cfg.ui.hangingIndent    ? "true" : "false") << "\n";
    out << "log_messages      = " << (cfg.ui.logMessages     ? "true" : "false") << "\n";
    out << "app_icon          = " << tomlQuote(cfg.ui.appIcon)     << "\n";
    out << "nick_brackets     = " << tomlQuote(cfg.ui.nickBrackets) << "\n";
    out << "notifications     = " << (cfg.ui.notifications    ? "true" : "false") << "\n";
    out << "font_family       = " << tomlQuote(cfg.ui.fontFamily)  << "\n";
    out << "font_toolbar       = " << cfg.ui.fontSizes.toolbar      << "\n";
    out << "font_server_header = " << cfg.ui.fontSizes.serverHeader << "\n";
    out << "font_sidebar       = " << cfg.ui.fontSizes.sidebar      << "\n";
    out << "font_chat          = " << cfg.ui.fontSizes.chat         << "\n";
    out << "font_nick_list     = " << cfg.ui.fontSizes.nickList     << "\n";
    out << "font_nick_dock     = " << cfg.ui.fontSizes.nickDock     << "\n";
    out << "font_topic_bar     = " << cfg.ui.fontSizes.topicBar     << "\n";
    out << "font_input_nick    = " << cfg.ui.fontSizes.inputNick    << "\n";
    out << "font_input         = " << cfg.ui.fontSizes.input        << "\n";
    out << "font_typing        = " << cfg.ui.fontSizes.typing       << "\n";
    out << "font_emoji         = " << cfg.ui.fontSizes.emoji        << "\n\n";

    out << "[privacy]\n";
    out << "link_previews = " << (cfg.ui.linkPreviews ? "true" : "false") << "\n\n";

    if (!cfg.ignoredNicks.isEmpty()) {
        out << "[ignore]\nnicks = [";
        bool first = true;
        for (const QString &n : cfg.ignoredNicks) {
            if (!first) out << ", ";
            out << tomlQuote(n);
            first = false;
        }
        out << "]\n\n";
    }

    if (!cfg.monitorList.isEmpty()) {
        out << "[monitor]\nnicks = [";
        bool first = true;
        for (const QString &n : cfg.monitorList) {
            if (!first) out << ", ";
            out << tomlQuote(n);
            first = false;
        }
        out << "]\n\n";
    }

    for (const auto &s : cfg.servers) {
        out << "[[server]]\n";
        out << "name     = " << tomlQuote(s.name)     << "\n";
        out << "host     = " << tomlQuote(s.host)     << "\n";
        out << "port     = " << s.port                << "\n";
        out << "ssl      = " << (s.ssl ? "true" : "false") << "\n";
        out << "nick     = " << tomlQuote(s.nick)     << "\n";
        out << "user     = " << tomlQuote(s.user)     << "\n";
        out << "realname = " << tomlQuote(s.realname) << "\n";
        const QString savedPw = storePassword(s.password, s.name, "password");
        if (!savedPw.isEmpty())
            out << "password          = " << tomlQuote(savedPw) << "\n";
        if (!s.saslUser.isEmpty())
            out << "sasl_user         = " << tomlQuote(s.saslUser) << "\n";
        const QString savedSasl = storePassword(s.saslPassword, s.name, "sasl_password");
        if (!savedSasl.isEmpty())
            out << "sasl_password     = " << tomlQuote(savedSasl) << "\n";
        if (s.saslExternal)
            out << "sasl_external     = true\n";
        if (!s.clientCertFile.isEmpty())
            out << "client_cert       = " << tomlQuote(s.clientCertFile) << "\n";
        if (!s.clientKeyFile.isEmpty())
            out << "client_key        = " << tomlQuote(s.clientKeyFile) << "\n";
        const QString savedNs = storePassword(s.nickservPassword, s.name, "nickserv_password");
        if (!savedNs.isEmpty())
            out << "nickserv_password = " << tomlQuote(savedNs) << "\n";
        if (s.bouncerType == BouncerType::ZNC)
            out << "bouncer           = \"znc\"\n";
        else if (s.bouncerType == BouncerType::Soju)
            out << "bouncer           = \"soju\"\n";
        if (!s.bouncerNetwork.isEmpty())
            out << "bouncer_network   = " << tomlQuote(s.bouncerNetwork) << "\n";
        if (!s.proxyHost.isEmpty()) {
            out << "proxy_host        = " << tomlQuote(s.proxyHost) << "\n";
            out << "proxy_port        = " << s.proxyPort << "\n";
            if (!s.proxyUser.isEmpty())
                out << "proxy_user        = " << tomlQuote(s.proxyUser) << "\n";
            const QString savedProxyPass = storePassword(s.proxyPass, s.name, "proxy_pass");
            if (!savedProxyPass.isEmpty())
                out << "proxy_pass        = " << tomlQuote(savedProxyPass) << "\n";
        }
        if (!s.pinnedFingerprint.isEmpty())
            out << "ssl_fingerprint   = " << tomlQuote(s.pinnedFingerprint) << "\n";
        const bool hasKeys = std::any_of(s.channels.begin(), s.channels.end(),
                                          [](const ChannelConfig &c){ return !c.password.isEmpty(); });
        if (hasKeys) {
            for (const auto &ch : s.channels) {
                out << "\n[[server.channel]]\n";
                out << "name = " << tomlQuote(ch.name) << "\n";
                const QString savedKey = storePassword(ch.password,
                    s.name + ":channel:" + ch.name, "key");
                if (!savedKey.isEmpty())
                    out << "key  = " << tomlQuote(savedKey) << "\n";
            }
        } else {
            QStringList names;
            for (const auto &ch : s.channels)
                names << ch.name;
            if (!names.isEmpty())
                out << "channels          = " << tomlQuote(names.join(", ")) << "\n";
        }
        out << "\n";
    }

    if (!f.commit())
        qWarning() << "Uplink: failed to commit config to" << path;
    else
        QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

bool Config::needsNickSetup() const
{
    for (const auto &s : servers)
        if (s.nick == "yournick" || s.nick.isEmpty())
            return true;
    return false;
}
