#include "ircclient.h"
#include "ircparser.h"
#include "stsstore.h"
#include "config/config.h"
#include "version.h"

#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QFile>
#include <QNetworkProxy>
#include <QSslCertificate>
#include <QSslKey>
#include <QSslSocket>
#include <QWebSocket>
#include <QTimer>

static QString stripCrlf(QString s)
{
    s.remove('\r');
    s.remove('\n');
    return s;
}

static bool validIrcToken(const QString &s)
{
    if (s.isEmpty()) return false;
    for (QChar c : s)
        if (c.isSpace() || c == '\r' || c == '\n' || c == '\0')
            return false;
    return true;
}

static QString ircv3TagEscape(QString s)
{
    s.replace("\\", "\\\\");
    s.replace(";", "\\:");
    s.replace(" ", "\\s");
    s.replace("\r", "\\r");
    s.replace("\n", "\\n");
    return s;
}

IrcClient::IrcClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QSslSocket(this))
{
    connect(m_socket, &QSslSocket::connected,       this, &IrcClient::onConnected);
    connect(m_socket, &QSslSocket::disconnected,    this, &IrcClient::onDisconnected);
    connect(m_socket, &QSslSocket::readyRead,       this, &IrcClient::onReadyRead);
    connect(m_socket, &QSslSocket::sslErrors,       this, &IrcClient::onSslErrors);
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &IrcClient::onErrorOccurred);

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &IrcClient::doReconnect);

    m_pingTimer = new QTimer(this);
    m_pingTimer->setInterval(30000);
    connect(m_pingTimer, &QTimer::timeout, this, &IrcClient::sendPing);
}

IrcClient::~IrcClient() = default;

void IrcClient::connectToServer(const ServerConfig &cfg)
{
    m_reconnectTimer->stop();
    m_intentionalDisconnect = false;
    m_reconnectDelay = 5;

    m_host            = cfg.host;
    m_port            = cfg.port;
    m_ssl             = cfg.ssl;
    m_nick            = cfg.nick;
    m_user            = cfg.user;
    m_realname        = cfg.realname;
    m_password        = cfg.password;
    m_saslUser        = cfg.saslUser;
    m_saslPassword    = cfg.saslPassword;
    m_saslExternal    = cfg.saslExternal;
    m_saslPending     = false;
    m_utf8Only        = false;
    m_nickservPassword = cfg.nickservPassword;
    m_bouncerType     = cfg.bouncerType;
    m_bouncerNetwork  = cfg.bouncerNetwork;
    m_proxyHost           = cfg.proxyHost;
    m_proxyPort           = cfg.proxyPort;
    m_proxyUser           = cfg.proxyUser;
    m_proxyPass           = cfg.proxyPass;
    m_pinnedFingerprint   = cfg.pinnedFingerprint;
    m_useWs               = cfg.websocket;
    m_quitMessage         = cfg.quitMessage;

    if (m_useWs) {
        if (!m_wsSocket) {
            m_wsSocket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
            connect(m_wsSocket, &QWebSocket::connected,           this, &IrcClient::onConnected);
            connect(m_wsSocket, &QWebSocket::disconnected,        this, &IrcClient::onDisconnected);
            connect(m_wsSocket, &QWebSocket::textMessageReceived, this, &IrcClient::onWsTextReceived);
            connect(m_wsSocket, &QWebSocket::sslErrors,           this, &IrcClient::onSslErrors);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
            connect(m_wsSocket, &QWebSocket::errorOccurred,       this, &IrcClient::onErrorOccurred);
#else
            connect(m_wsSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
                    this, &IrcClient::onErrorOccurred);
#endif
        } else if (m_wsSocket->state() != QAbstractSocket::UnconnectedState) {
            m_wsSocket->abort();
        }
        applyProxy();
        QUrl url;
        url.setScheme(m_ssl ? "wss" : "ws");
        url.setHost(m_host);
        url.setPort(m_port);
        url.setPath("/");
        m_wsSocket->open(url);
        return;
    }

    // Apply cached STS policy — upgrade plain to TLS if one exists
    if (!m_ssl) {
        StsStore::Policy sts;
        if (StsStore::lookup(m_host, sts)) {
            m_ssl  = true;
            m_port = sts.port;
        }
    }

    if (m_saslExternal && !cfg.clientCertFile.isEmpty() && !cfg.clientKeyFile.isEmpty()) {
        QFile certFile(cfg.clientCertFile);
        if (certFile.open(QIODevice::ReadOnly)) {
            const QSslCertificate cert(certFile.readAll(), QSsl::Pem);
            if (!cert.isNull())
                m_socket->setLocalCertificate(cert);
        }
        QFile keyFile(cfg.clientKeyFile);
        if (keyFile.open(QIODevice::ReadOnly)) {
            const QByteArray keyData = keyFile.readAll();
            QSslKey key(keyData, QSsl::Rsa, QSsl::Pem);
            if (key.isNull()) key = QSslKey(keyData, QSsl::Ec, QSsl::Pem);
            if (!key.isNull())
                m_socket->setPrivateKey(key);
        }
    }

    if (sockState() != QAbstractSocket::UnconnectedState)
        sockAbort();

    applyProxy();
    if (m_ssl)
        m_socket->connectToHostEncrypted(m_host, m_port);
    else
        m_socket->connectToHost(m_host, m_port);
}

void IrcClient::applyProxy()
{
    QNetworkProxy proxy = m_proxyHost.isEmpty()
        ? QNetworkProxy(QNetworkProxy::NoProxy)
        : QNetworkProxy(QNetworkProxy::Socks5Proxy, m_proxyHost, m_proxyPort,
                        m_proxyUser, m_proxyPass);
    if (m_useWs && m_wsSocket)
        m_wsSocket->setProxy(proxy);
    else
        m_socket->setProxy(proxy);
}

// ---------------------------------------------------------------------------
// Socket abstraction helpers
// ---------------------------------------------------------------------------

void IrcClient::sockWrite(const QString &line)
{
    if (m_useWs) {
        m_wsSocket->sendTextMessage(line);
    } else {
        QByteArray buf = line.toUtf8();
        buf.append("\r\n", 2);
        m_socket->write(buf);
    }
}

void IrcClient::sockDisconnect()
{
    if (m_useWs) m_wsSocket->close();
    else         m_socket->disconnectFromHost();
}

void IrcClient::sockAbort()
{
    if (m_useWs) m_wsSocket->abort();
    else         m_socket->abort();
}

QAbstractSocket::SocketState IrcClient::sockState() const
{
    return m_useWs ? m_wsSocket->state() : m_socket->state();
}

QHostAddress IrcClient::sockLocalAddress() const
{
    return m_useWs ? m_wsSocket->localAddress() : m_socket->localAddress();
}

QString IrcClient::sockErrorString() const
{
    return m_useWs ? m_wsSocket->errorString() : m_socket->errorString();
}

bool IrcClient::isConnected() const
{
    return sockState() == QAbstractSocket::ConnectedState;
}

quint32 IrcClient::localIpv4() const
{
    bool ok = false;
    const quint32 v4 = sockLocalAddress().toIPv4Address(&ok);
    return ok ? v4 : 0;
}

void IrcClient::quit(const QString &reason)
{
    m_intentionalDisconnect = true;
    m_reconnectTimer->stop();
    const QString msg = !reason.isEmpty() ? reason
                        : (!m_quitMessage.isEmpty() ? m_quitMessage : QStringLiteral("Uplink"));
    sendRaw("QUIT :" + msg);
    sockDisconnect();
}

void IrcClient::join(const QString &channel, const QString &key)
{
    if (!validIrcToken(channel)) return;
    sendRaw(key.isEmpty() ? "JOIN " + channel : "JOIN " + channel + " " + stripCrlf(key));
}

void IrcClient::part(const QString &channel, const QString &reason)
{
    if (!validIrcToken(channel)) return;
    sendRaw(reason.isEmpty() ? "PART " + channel : "PART " + channel + " :" + stripCrlf(reason));
}

void IrcClient::privmsg(const QString &target, const QString &text, const QString &replyToMsgid)
{
    if (!validIrcToken(target)) return;
    if (sockState() != QAbstractSocket::ConnectedState) {
        emit errorMessage(m_host, "Not connected — message not sent");
        return;
    }
    if (m_utf8Only && text.contains(QChar::ReplacementCharacter))
        emit serverMessage(m_host,
            "Warning: message contains invalid UTF-8 characters — server requires UTF-8 (UTF8ONLY)");
    if (replyToMsgid.isEmpty())
        sendRaw("PRIVMSG " + target + " :" + stripCrlf(text));
    else
        sendRaw("@+draft/reply=" + ircv3TagEscape(replyToMsgid) + " PRIVMSG " + target + " :" + stripCrlf(text));
}

void IrcClient::sendMultiline(const QString &target, const QString &text,
                               const QString &replyToMsgid)
{
    if (!validIrcToken(target)) return;
    if (sockState() != QAbstractSocket::ConnectedState) {
        emit errorMessage(m_host, "Not connected — message not sent");
        return;
    }

    const QStringList lines = text.split('\n', Qt::KeepEmptyParts);

    if (!m_ackedCaps.contains("draft/multiline") || lines.size() <= 1) {
        // Fall back to separate PRIVMSGs
        for (const QString &line : lines)
            if (!line.isEmpty())
                privmsg(target, line, replyToMsgid);
        return;
    }

    const QString ref = QStringLiteral("ml%1")
        .arg(QRandomGenerator::global()->generate(), 8, 16, QLatin1Char('0'));

    sendRaw("BATCH +" + ref + " draft/multiline " + target);
    bool first = true;
    for (const QString &line : lines) {
        QString tags = "@batch=" + ref;
        if (first && !replyToMsgid.isEmpty())
            tags += ";+draft/reply=" + ircv3TagEscape(replyToMsgid);
        sendRaw(tags + " PRIVMSG " + target + " :" + stripCrlf(line));
        first = false;
    }
    sendRaw("BATCH -" + ref);
}

void IrcClient::notice(const QString &target, const QString &text)
{
    if (!validIrcToken(target)) return;
    if (sockState() != QAbstractSocket::ConnectedState) {
        emit errorMessage(m_host, "Not connected — message not sent");
        return;
    }
    sendRaw("NOTICE " + target + " :" + stripCrlf(text));
}

void IrcClient::setNick(const QString &nick)
{
    const QString clean = stripCrlf(nick).remove(' ').remove('\0');
    m_nick = clean;
    sendRaw("NICK " + clean);
}

void IrcClient::sendTyping(const QString &channel, const QString &state)
{
    if (!validIrcToken(channel)) return;
    const bool hasFinal = m_ackedCaps.contains("typing");
    const bool hasDraft = m_ackedCaps.contains("draft/typing");
    const bool hasTags  = m_ackedCaps.contains("message-tags");
    if (!hasFinal && !hasDraft && !hasTags) return;
    const QString tag = hasFinal ? "typing=" : "+typing=";
    sendRaw("@" + tag + ircv3TagEscape(state) + " TAGMSG " + channel);
}

void IrcClient::sendReact(const QString &target, const QString &msgid, const QString &emoji)
{
    if (!validIrcToken(target)) return;
    if (msgid.isEmpty() || emoji.isEmpty()) return;
    if (!m_ackedCaps.contains("message-tags")) return;
    sendRaw("@+draft/react=" + ircv3TagEscape(emoji)
            + ";+draft/reply=" + ircv3TagEscape(msgid)
            + " TAGMSG " + stripCrlf(target));
}

void IrcClient::requestHistory(const QString &target, int limit)
{
    if (!m_ackedCaps.contains("chathistory") && !m_ackedCaps.contains("draft/chathistory"))
        return;
    sendRaw(QString("CHATHISTORY LATEST %1 * %2").arg(target).arg(limit));
}

void IrcClient::markRead(const QString &target, const QDateTime &ts)
{
    if (!m_ackedCaps.contains("soju.im/read")) return;
    sendRaw("MARKREAD " + target + " timestamp=" + ts.toUTC().toString(Qt::ISODateWithMs));
}

void IrcClient::sendRedact(const QString &target, const QString &msgid, const QString &reason)
{
    if (!m_ackedCaps.contains("draft/message-redaction")) return;
    if (target.isEmpty() || msgid.isEmpty()) return;
    sendRaw("REDACT " + stripCrlf(target) + " " + stripCrlf(msgid)
            + (reason.isEmpty() ? QString() : " :" + stripCrlf(reason)));
}

void IrcClient::setMonitorList(const QStringList &nicks)
{
    m_monitorList = nicks;
    m_monitorSet.clear();
    for (const QString &n : nicks)
        m_monitorSet.insert(n.toLower());
}

void IrcClient::monitorAdd(const QString &nick)
{
    if (nick.isEmpty()) return;
    if (!m_monitorSet.contains(nick.toLower())) {
        m_monitorList.append(nick);
        m_monitorSet.insert(nick.toLower());
    }
    sendRaw("MONITOR + " + stripCrlf(nick));
}

void IrcClient::monitorRemove(const QString &nick)
{
    m_monitorSet.remove(nick.toLower());
    m_monitorList.removeIf([&](const QString &n){ return n.compare(nick, Qt::CaseInsensitive) == 0; });
    sendRaw("MONITOR - " + stripCrlf(nick));
}

void IrcClient::monitorClear()
{
    m_monitorList.clear();
    m_monitorSet.clear();
    sendRaw("MONITOR C");
}

void IrcClient::monitorStatus()
{
    sendRaw("MONITOR S");
}

static QString redactRawForLog(const QString &line)
{
    const QString cmd = line.section(' ', 0, 0).toUpper();
    if (cmd == "PASS")
        return "PASS :<redacted>";
    if (cmd == "AUTHENTICATE")
        return "AUTHENTICATE <redacted>";
    if (cmd == "OPER")
        return "OPER " + line.section(' ', 1, 1) + " :<redacted>";
    if (line.contains("NickServ", Qt::CaseInsensitive) &&
        line.contains("IDENTIFY", Qt::CaseInsensitive))
        return "<NickServ IDENTIFY redacted>";
    return line;
}

void IrcClient::sendRaw(const QString &line)
{
    if (sockState() != QAbstractSocket::ConnectedState) return;
    const QString stripped = stripCrlf(line);
    if (stripped.startsWith("AWAY :", Qt::CaseInsensitive))
        m_awayMsg = stripped.mid(6);
    else if (stripped.compare("AWAY", Qt::CaseInsensitive) == 0)
        m_awayMsg.clear();
    // Enforce 510-byte wire limit (RFC 1459: 512 bytes including CRLF).
    // left(510) counts code-points, not bytes — multi-byte chars could exceed the limit.
    QByteArray utf8 = stripped.toUtf8();
    if (utf8.size() > 510) {
        int i = 510;
        while (i > 0 && (static_cast<unsigned char>(utf8[i]) & 0xC0) == 0x80) --i;
        utf8.truncate(i);
    }
    const QString clean = QString::fromUtf8(utf8);
    emit rawReceived(">> " + redactRawForLog(clean));
    sockWrite(clean);
}

// ---------------------------------------------------------------------------
// Socket callbacks
// ---------------------------------------------------------------------------

void IrcClient::onConnected()
{
    // For SSL connections, defer until TLS handshake completes.
    if (m_ssl && !m_useWs && !m_socket->isEncrypted()) {
        connect(m_socket, &QSslSocket::encrypted, this, &IrcClient::onConnected,
                Qt::SingleShotConnection);
        return;
    }

    m_reconnectTimer->stop();
    m_reconnectDelay = 5;
    m_pingPending = false;
    m_pingTimer->start();

    sendRaw("CAP LS 302");

    // For ZNC with bouncer_network + SASL credentials, assemble the ZNC PASS format
    // so users don't have to embed it manually as "user/network:password".
    const bool zncAssembled = m_bouncerType == BouncerType::ZNC
                              && !m_bouncerNetwork.isEmpty()
                              && !m_saslUser.isEmpty()
                              && !m_saslPassword.isEmpty();
    if (zncAssembled) {
        if (!m_ssl)
            emit serverMessage(m_host, "Warning: sending server password over unencrypted connection");
        sendRaw("PASS :" + stripCrlf(m_saslUser) + "/"
                         + stripCrlf(m_bouncerNetwork) + ":"
                         + stripCrlf(m_saslPassword));
    } else if (!m_password.isEmpty()) {
        if (!m_ssl)
            emit serverMessage(m_host, "Warning: sending server password over unencrypted connection");
        sendRaw("PASS :" + stripCrlf(m_password));
    }

    const QString safeNick = stripCrlf(m_nick).remove(' ').remove('\0');
    const QString safeUser = stripCrlf(m_user).remove(' ').remove('\0');
    sendRaw("NICK " + safeNick);
    sendRaw("USER " + safeUser + " 0 * :" + stripCrlf(m_realname));
}

void IrcClient::onDisconnected()
{
    m_pingTimer->stop();
    m_pingPending = false;
    m_buffer.clear();
    m_namesBuffer.clear();
    m_requestedCaps.clear();
    m_ackedCaps.clear();
    m_capLsBuffer.clear();
    m_batches.clear();
    m_saslPending    = false;
    m_registered     = false;
    m_supportsWhox   = false;
    m_bouncerListing = false;
    m_bouncerNetBuf.clear();
    emit disconnected(m_host);
    if (!m_stsUpgrade)
        scheduleReconnect();
    m_stsUpgrade = false;
    // m_intentionalDisconnect is reset by connectToServer() on next connection
}

static constexpr qsizetype kMaxPendingBuffer = 64 * 1024;
static constexpr qsizetype kMaxIrcLine       = 8192;
static constexpr int       kMaxOpenBatches   = 8;
static constexpr int       kMaxBatchMessages = 1000;

void IrcClient::onWsTextReceived(const QString &message)
{
    // Each WebSocket text frame is one complete IRC line — no buffering needed.
    const QString line = message.trimmed();
    if (line.isEmpty() || line.size() > kMaxIrcLine) return;
    emit rawReceived(line);
    processLine(line);
}

void IrcClient::onReadyRead()
{
    const QByteArray raw = m_socket->readAll();
    if (m_utf8Only && raw.contains('\xff')) {
        // Cheap non-UTF-8 sniff: 0xFF never appears in valid UTF-8
        emit serverMessage(m_host, "Warning: server sent non-UTF-8 data (UTF8ONLY is set)");
    }
    m_buffer += raw;

    qsizetype start = 0, idx;
    while ((idx = m_buffer.indexOf('\n', start)) != -1) {
        QByteArray lineBytes = m_buffer.mid(start, idx - start);
        start = idx + 1;
        if (!lineBytes.isEmpty() && lineBytes.back() == '\r')
            lineBytes.chop(1);
        if (lineBytes.isEmpty()) continue;
        if (lineBytes.size() > kMaxIrcLine) {
            emit socketError(m_host, "Dropped oversized IRC line from server");
            continue;
        }
        const QString line = QString::fromUtf8(lineBytes);
        emit rawReceived(line);
        processLine(line);
    }
    if (start > 0)
        m_buffer = m_buffer.mid(start);

    // Only disconnect if the *remaining* partial line (no newline yet) is absurdly large
    if (m_buffer.size() > kMaxPendingBuffer) {
        emit socketError(m_host, "Server sent oversized unterminated data; disconnecting");
        m_socket->disconnectFromHost();
        m_buffer.clear();
    }
}

void IrcClient::onSslErrors(const QList<QSslError> &errors)
{
    bool allSelfSigned = true;
    for (const auto &e : errors) {
        if (e.error() != QSslError::SelfSignedCertificate &&
            e.error() != QSslError::SelfSignedCertificateInChain) {
            allSelfSigned = false;
            break;
        }
    }

    if (!allSelfSigned) {
        QStringList msgs;
        for (const auto &e : errors)
            msgs << e.errorString();
        emit socketError(m_host, "TLS error: " + msgs.join("; "));
        sockDisconnect();
        return;
    }

    const auto chain = m_useWs
        ? m_wsSocket->sslConfiguration().peerCertificateChain()
        : m_socket->peerCertificateChain();
    if (chain.isEmpty()) {
        emit socketError(m_host, "TLS error: no peer certificate");
        sockDisconnect();
        return;
    }
    const QString fp = QString::fromLatin1(
        chain.first().digest(QCryptographicHash::Sha256).toHex(':').toUpper());

    if (!m_pinnedFingerprint.isEmpty()) {
        if (m_pinnedFingerprint.compare(fp, Qt::CaseInsensitive) == 0) {
            if (m_useWs) m_wsSocket->ignoreSslErrors(errors);
            else         m_socket->ignoreSslErrors(errors);
            return;
        }
        emit socketError(m_host,
            QString("TLS: certificate fingerprint mismatch!\nPinned: %1\nGot:    %2")
                .arg(m_pinnedFingerprint, fp));
        sockDisconnect();
        return;
    }

    // No pin — abort before any credentials are sent, then ask the user.
    // The connection is re-established after the user accepts the fingerprint.
    m_intentionalDisconnect = true;
    sockAbort();
    emit sslFingerprintPrompt(m_host, fp);
}

void IrcClient::abort()
{
    m_intentionalDisconnect = true;
    if (sockState() != QAbstractSocket::UnconnectedState)
        sockDisconnect();
}

void IrcClient::reconnect()
{
    m_intentionalDisconnect = false;
    m_reconnectTimer->stop();
    m_reconnectDelay = 5;
    doReconnect();
}

void IrcClient::onErrorOccurred(QAbstractSocket::SocketError)
{
    emit socketError(m_host, sockErrorString());
    // disconnected() and scheduleReconnect() are handled by onDisconnected()
    // which Qt emits after every socket error that closes the connection.
}

void IrcClient::scheduleReconnect()
{
    if (m_intentionalDisconnect || m_reconnectTimer->isActive() || m_host.isEmpty()) return;
    emit serverMessage(m_host, QString("Reconnecting in %1s…").arg(m_reconnectDelay));
    m_reconnectTimer->start(m_reconnectDelay * 1000);
    m_reconnectDelay = qMin(m_reconnectDelay * 2, 60);
}

void IrcClient::sendPing()
{
    if (m_pingPending) {
        if (m_pingClock.elapsed() > 90000) {
            emit socketError(m_host, "Ping timeout");
            sockAbort();
        }
        return;
    }
    m_pingPending = true;
    m_pingClock.start();
    sendRaw("PING :uplink_rtt");
}

void IrcClient::doReconnect()
{
    emit reconnecting(m_host);
    emit serverMessage(m_host, "Reconnecting…");
    m_saslPending = false;
    if (sockState() != QAbstractSocket::UnconnectedState)
        sockAbort();
    applyProxy();
    if (m_useWs) {
        QUrl url;
        url.setScheme(m_ssl ? "wss" : "ws");
        url.setHost(m_host);
        url.setPort(m_port);
        url.setPath("/");
        m_wsSocket->open(url);
    } else if (m_ssl) {
        m_socket->connectToHostEncrypted(m_host, m_port);
    } else {
        m_socket->connectToHost(m_host, m_port);
    }
}

// ---------------------------------------------------------------------------
// Line processing
// ---------------------------------------------------------------------------

void IrcClient::processLine(const QString &line)
{
    IrcMessage msg = IrcParser::parse(line);
    if (!msg.isValid()) return;

    const QString &cmd = msg.command;

    if (cmd == "PING") {
        sendRaw("PONG :" + (msg.params.isEmpty() ? QString{} : msg.params.last()));
        return;
    }

    if (cmd == "PONG") {
        if (m_pingPending && !msg.params.isEmpty() && msg.params.last() == "uplink_rtt") {
            m_pingPending = false;
            emit pingRtt(m_host, static_cast<int>(m_pingClock.elapsed()));
        }
        return;
    }

    if (cmd == "CAP") {
        handleCap(msg.params, msg.trailing);
        return;
    }

    if (cmd == "AUTHENTICATE") {
        if (!msg.params.isEmpty() && msg.params[0] == "+") {
            if (m_saslExternal) {
                sendRaw("AUTHENTICATE +");
            } else {
                if (!m_ssl)
                    emit serverMessage(m_host, "Warning: sending SASL credentials over unencrypted connection");
                // For soju, the SASL username must be "user/network" to select a network
                QString saslUser = m_saslUser;
                if (m_bouncerType == BouncerType::Soju && !m_bouncerNetwork.isEmpty())
                    saslUser += "/" + m_bouncerNetwork;
                const QByteArray payload =
                    QByteArray("\0", 1) + QString(saslUser).remove(QChar(0)).toUtf8() +
                    QByteArray("\0", 1) + QString(m_saslPassword).remove(QChar(0)).toUtf8();
                sendRaw("AUTHENTICATE " + QString::fromLatin1(payload.toBase64()));
            }
        }
        return;
    }

    if (cmd == "BATCH") {
        handleBatch(msg.params);
        return;
    }

    if (cmd == "BOUNCER") {
        handleBouncer(msg.params, msg.trailing);
        return;
    }

    if (cmd == "MARKREAD" && msg.params.size() >= 2) {
        const QString target = msg.params[0];
        const QString tsTag  = msg.params[1]; // "timestamp=<iso>"
        if (tsTag.startsWith("timestamp=")) {
            const QDateTime ts = QDateTime::fromString(tsTag.mid(10), Qt::ISODateWithMs);
            emit readMarkerReceived(m_host, target, ts);
        }
        return;
    }

    bool isNumeric = false;
    cmd.toInt(&isNumeric);
    if (isNumeric) {
        handleNumeric(cmd, msg.params, msg.trailing);
        return;
    }

    // If this message belongs to an active batch, buffer it
    const QString batchRef = msg.tags.value("batch");
    if (!batchRef.isEmpty() && m_batches.contains(batchRef)) {
        auto &batch = m_batches[batchRef];
        if (batch.msgs.size() >= kMaxBatchMessages) {
            emit errorMessage(m_host, "IRC batch too large; dropping extra messages");
            return;
        }
        BatchInfo::Msg bm;
        bm.command    = cmd;
        bm.nick       = msg.nick;
        bm.params     = msg.params;
        bm.trailing   = msg.trailing;
        bm.serverTime = msg.serverTime;
        bm.msgid      = msg.tags.value("msgid");
        bm.replyTo    = msg.tags.value("+draft/reply");
        bm.concat     = msg.tags.contains("draft/multiline-concat");
        batch.msgs.append(bm);
        return;
    }

    const QDateTime serverTime = msg.serverTime;

    // PRIVMSG / CTCP
    if (cmd == "PRIVMSG" && msg.params.size() >= 1) {
        const QString target = msg.params[0];
        const QString text   = msg.params.size() >= 2 ? msg.params.last() : msg.trailing;

        const QString msgid   = msg.tags.value("msgid");
        const QString replyTo = msg.tags.value("+draft/reply");
        const QString account = msg.tags.value("account");
        if (!account.isEmpty())
            emit accountChanged(m_host, msg.nick, account);
        if (text.startsWith('\x01') && text.endsWith('\x01')) {
            const QString ctcp    = text.mid(1, text.size() - 2);
            const QString ctcpCmd = ctcp.section(' ', 0, 0).toUpper();
            if (ctcpCmd == "ACTION") {
                emit actionReceived(m_host, target,
                                    msg.nick, ctcp.mid(7), serverTime, false, msgid);
            } else if (ctcpCmd == "VERSION") {
                const QString rkey = msg.nick + ":VERSION";
                const qint64  now  = QDateTime::currentMSecsSinceEpoch();
                if (now - m_ctcpTimestamps.value(rkey, 0) >= 5000) {
                    if (m_ctcpTimestamps.size() >= 500)
                        m_ctcpTimestamps.erase(m_ctcpTimestamps.begin());
                    m_ctcpTimestamps.insert(rkey, now);
                    sendRaw("NOTICE " + msg.nick + " :\x01VERSION Uplink " UPLINK_VERSION "\x01");
                    emit serverMessage(m_host, "CTCP VERSION from " + msg.nick);
                }
            } else if (ctcpCmd == "PING") {
                const QString rkey = msg.nick + ":PING";
                const qint64  now  = QDateTime::currentMSecsSinceEpoch();
                if (now - m_ctcpTimestamps.value(rkey, 0) >= 5000) {
                    if (m_ctcpTimestamps.size() >= 500)
                        m_ctcpTimestamps.erase(m_ctcpTimestamps.begin());
                    m_ctcpTimestamps.insert(rkey, now);
                    QString payload = stripCrlf(ctcp.section(' ', 1).left(32));
                    payload.remove(QChar(1)).remove('\0'); // strip CTCP delimiters and nulls
                    sendRaw("NOTICE " + msg.nick + " :\x01PING " + payload + "\x01");
                }
            } else if (ctcpCmd == "DCC" && msg.nick != m_nick) {
                const QString dccType = ctcp.section(' ', 1, 1).toUpper();
                if (dccType == "SEND") {
                    const QString fn = ctcp.section(' ', 2, 2);
                    bool ok1, ok2, ok3;
                    const quint32 ip       = ctcp.section(' ', 3, 3).toUInt(&ok1);
                    const quint16 port     = ctcp.section(' ', 4, 4).toUShort(&ok2);
                    const qint64  filesize = ctcp.section(' ', 5, 5).toLongLong(&ok3);
                    const QString token    = ctcp.section(' ', 6, 6);
                    if (ok1 && ok2 && ok3 && !fn.isEmpty() && filesize > 0) {
                        if (port == 0 && !token.isEmpty())
                            emit dccPassiveOfferReceived(m_host, msg.nick, fn, ip, filesize, token);
                        else if (port != 0 && !token.isEmpty())
                            emit dccPassiveSendReply(m_host, msg.nick, fn, ip, port, filesize, token);
                        else if (port != 0)
                            emit dccSendReceived(m_host, msg.nick, fn, ip, port, filesize);
                        else
                            emit serverMessage(m_host, "DCC SEND from " + msg.nick + " (malformed)");
                    } else
                        emit serverMessage(m_host, "DCC SEND from " + msg.nick + " (malformed)");
                } else {
                    emit serverMessage(m_host, "DCC " + dccType + " from " + msg.nick + " (unsupported)");
                }
            } else {
                emit serverMessage(m_host, "CTCP " + ctcpCmd + " from " + msg.nick);
            }
        } else {
            emit messageReceived(m_host, target, msg.nick, text, serverTime, false, msgid, replyTo);
        }
        return;
    }

    if (cmd == "NOTICE" && msg.params.size() >= 1) {
        const QString text = msg.params.size() >= 2 ? msg.params.last() : msg.trailing;
        const QString noticeAccount = msg.tags.value("account");
        if (!noticeAccount.isEmpty())
            emit accountChanged(m_host, msg.nick, noticeAccount);
        if (text.startsWith('\x01') && text.endsWith('\x01')) {
            const QString ctcp    = text.mid(1, text.size() - 2);
            const QString ctcpCmd = ctcp.section(' ', 0, 0).toUpper();
            if (ctcpCmd == "PING") {
                bool ok = false;
                const qint64 sent = ctcp.section(' ', 1).toLongLong(&ok);
                const qint64 rtt  = QDateTime::currentMSecsSinceEpoch() - sent;
                emit ctcpPingReply(m_host, msg.nick, ok ? rtt : -1);
            } else if (ctcpCmd == "VERSION") {
                emit contextualMessage(m_host,
                    QString("VERSION reply from %1: %2").arg(msg.nick, ctcp.section(' ', 1)));
            } else if (ctcpCmd == "TIME") {
                emit ctcpTimeReply(m_host, msg.nick, ctcp.section(' ', 1));
            } else {
                emit noticeReceived(m_host, msg.params[0], msg.nick,
                                    "CTCP reply: " + ctcp, serverTime, false, {}, {});
            }
        } else {
            emit noticeReceived(m_host, msg.params[0], msg.nick, text, serverTime, false,
                                msg.tags.value("msgid"), msg.tags.value("+draft/reply"));
        }
        return;
    }

    if (cmd == "TAGMSG" && !msg.params.isEmpty()) {
        const QString target = msg.params[0];
        QString typing = msg.tags.value("typing");
        if (typing.isEmpty()) typing = msg.tags.value("+typing");
        if (!typing.isEmpty() && msg.nick != m_nick)
            emit typingReceived(m_host, target, msg.nick, typing);
        const QString emoji = msg.tags.value("+draft/react");
        if (!emoji.isEmpty()) {
            const QString replyTo = msg.tags.value("+draft/reply");
            emit reactReceived(m_host, target, msg.nick, replyTo, emoji);
        }
        return;
    }

    if (cmd == "CHGHOST" && msg.params.size() >= 2) {
        emit hostChanged(m_host, msg.nick, msg.params[0], msg.params[1]);
        return;
    }

    if (cmd == "ACCOUNT" && !msg.params.isEmpty()) {
        emit accountChanged(m_host, msg.nick, msg.params[0]);
        return;
    }

    // draft/metadata-2 — server-push notification: METADATA <target> <key> <visibility> :<value>
    // ircmsg only adds trailing colon when value has spaces — plain URLs land in params[3]
    if (cmd == "METADATA" && msg.params.size() >= 2) {
        QString target     = msg.params[0];
        if (target == QLatin1String("*")) target = m_nick;
        const QString &key = msg.params[1];
        if ((key == "display-name" || key == "avatar") && !target.startsWith('#')) {
            const QString value = !msg.trailing.isEmpty() ? msg.trailing : msg.params.value(3);
            emit userMetaChanged(m_host, target, key, value);
        }
        return;
    }

    if (cmd == "SETNAME" && !msg.trailing.isEmpty()) {
        emit setNameReceived(m_host, msg.nick, msg.trailing);
        return;
    }

    if (cmd == "REDACT" && msg.params.size() >= 2) {
        emit messageRedacted(m_host, msg.nick, msg.params[0], msg.params[1], msg.trailing);
        return;
    }

    if (cmd == "INVITE" && msg.params.size() >= 1) {
        const QString targetNick = msg.params[0];
        const QString channel    = msg.params.size() > 1 ? msg.params[1] : msg.trailing;
        emit inviteNotify(m_host, msg.nick, channel, targetNick);
        return;
    }

    if (cmd == "JOIN") {
        const QString channel = msg.params.isEmpty() ? msg.trailing : msg.params[0];
        if (msg.nick == m_nick) {
            emit serverMessage(m_host, "Joined " + channel);
            if (m_ackedCaps.contains("soju.im/no-implicit-names"))
                sendRaw("NAMES " + channel);
        }
        emit userJoined(m_host, channel, msg.nick, msg.user, msg.host);
        // extended-join: params[1] = account, trailing = realname
        if (msg.params.size() > 1) {
            const QString account = msg.params[1];
            if (!account.isEmpty() && account != "*")
                emit accountChanged(m_host, msg.nick, account);
        }
        return;
    }

    if (cmd == "PART" && !msg.params.isEmpty()) {
        emit userParted(m_host, msg.params[0], msg.nick, msg.user, msg.host, msg.trailing);
        return;
    }

    if (cmd == "QUIT") {
        emit userQuit(m_host, msg.nick, msg.user, msg.host, msg.trailing);
        return;
    }

    if (cmd == "NICK") {
        const QString newNick = msg.params.isEmpty() ? msg.trailing : msg.params[0];
        if (msg.nick.toLower() == m_nick.toLower()) {
            m_nick = newNick;
            emit selfNickChanged(m_host, newNick);
        }
        emit nickChanged(m_host, msg.nick, newNick);
        return;
    }

    if (cmd == "KICK" && msg.params.size() >= 2) {
        emit kicked(m_host, msg.params[0], msg.params[1], msg.nick, msg.trailing);
        return;
    }

    if (cmd == "MODE" && !msg.params.isEmpty()) {
        QStringList modeParts = msg.params.mid(1);
        if (!msg.trailing.isEmpty()) modeParts << msg.trailing;
        emit modesReceived(m_host, msg.params[0], modeParts.join(' '));
        return;
    }

    if (cmd == "TOPIC" && !msg.params.isEmpty()) {
        emit topicReceived(m_host, msg.params[0], msg.trailing);
        return;
    }

    if (cmd == "WALLOPS") {
        const QString text = msg.trailing.isEmpty() ? msg.params.value(0) : msg.trailing;
        emit wallopsReceived(m_host, msg.nick, text);
        return;
    }

    if (cmd == "FAIL" || cmd == "WARN" || cmd == "NOTE") {
        // params: [0]=command [1]=code [2..n-1]=context  trailing=description
        const QString triggeredBy = msg.params.value(0);
        const QString code        = msg.params.value(1);
        // Silently drop benign probe failures — these are expected on servers that
        // advertise the capability but don't support our specific keys/subcommands
        if (triggeredBy == "BOUNCER" && code == "UNKNOWN_COMMAND") {
            m_bouncerListingUnsupported = true;
            return;
        }
        if (triggeredBy == "METADATA" && code == "KEY_INVALID")
            return;
        // ACCOUNT_REQUIRED is informational — route to server buffer only, not channels
        if (code == "ACCOUNT_REQUIRED")
            return;
        const QString desc        = msg.trailing.isEmpty() ? msg.params.value(msg.params.size() - 1)
                                                           : msg.trailing;
        // Look for a channel name in the context parameters
        QString channel;
        for (int i = 2; i < msg.params.size(); ++i) {
            const QString &p = msg.params[i];
            if (!p.isEmpty() && QString("&#!+").contains(p[0])) { channel = p; break; }
        }
        const QString prefix = "[" + cmd + "] ";
        const QString text   = triggeredBy.isEmpty() || triggeredBy == "*"
            ? prefix + code + ": " + desc
            : prefix + triggeredBy + " " + code + ": " + desc;
        if (!channel.isEmpty())
            emit standardReply(m_host, channel, cmd, text);
        else if (cmd == "FAIL")
            emit errorMessage(m_host, text);
        else
            emit serverMessage(m_host, text);
        return;
    }
}

// ---------------------------------------------------------------------------
// BATCH
// ---------------------------------------------------------------------------

void IrcClient::handleBatch(const QStringList &params)
{
    if (params.isEmpty()) return;
    const QString &refArg = params[0];

    if (refArg.startsWith('+')) {
        if (m_batches.size() >= kMaxOpenBatches) {
            emit errorMessage(m_host, "Too many open IRC batches; ignoring");
            return;
        }
        const QString ref   = refArg.mid(1);
        const QString type  = params.size() > 1 ? params[1] : QString();
        const QString param = params.size() > 2 ? params.mid(2).join(' ') : QString();
        BatchInfo bi;
        bi.type  = type;
        bi.param = param;
        m_batches.insert(ref, bi);
    } else if (refArg.startsWith('-')) {
        deliverBatch(refArg.mid(1));
    }
}

void IrcClient::deliverBatch(const QString &ref)
{
    if (!m_batches.contains(ref)) return;
    const BatchInfo batch = m_batches.take(ref);
    if (batch.type == "draft/multiline") {
        // Assemble lines into a single multi-line message.
        // Lines tagged draft/multiline-concat are appended without a newline prefix.
        const QString target = batch.param;
        QString assembled;
        QString nick;
        QDateTime serverTime;
        QString msgid;
        QString replyTo;
        for (const BatchInfo::Msg &bm : batch.msgs) {
            if (bm.command != "PRIVMSG") continue;
            if (nick.isEmpty()) {
                nick       = bm.nick;
                serverTime = bm.serverTime;
                msgid      = bm.msgid;
                replyTo    = bm.replyTo;
            }
            if (!assembled.isEmpty())
                assembled += bm.concat ? QString{} : QStringLiteral("\n");
            assembled += bm.trailing;
        }
        if (!assembled.isEmpty())
            emit messageReceived(m_host, target, nick, assembled, serverTime, false, msgid, replyTo);
        return;
    }

    if (batch.type == "netsplit") {
        QStringList nicks;
        for (const auto &bm : batch.msgs)
            if (!bm.nick.isEmpty()) nicks.append(bm.nick);
        emit netsplitDetected(m_host, batch.param, nicks);
        return;
    }
    if (batch.type == "netjoin") {
        QStringList channels, nicks;
        for (const auto &bm : batch.msgs) {
            if (bm.command == "JOIN" && !bm.params.isEmpty()) {
                channels.append(bm.params[0]);
                nicks.append(bm.nick);
            }
        }
        if (!channels.isEmpty())
            emit netjoinDetected(m_host, batch.param, channels, nicks);
        return;
    }

    const bool isHistory = (batch.type == "chathistory" ||
                            batch.type == "draft/chathistory" ||
                            batch.type == "znc.in/batch/playback");

    for (const BatchInfo::Msg &bm : batch.msgs) {
        if (bm.command == "PRIVMSG" && bm.params.size() >= 1) {
            const QString target = bm.params[0];
            const QString text   = bm.params.size() >= 2 ? bm.params.last() : bm.trailing;
            if (text.startsWith('\x01') && text.endsWith('\x01')) {
                const QString ctcp = text.mid(1, text.size() - 2);
                if (ctcp.startsWith("ACTION "))
                    emit actionReceived(m_host, target, bm.nick,
                                        ctcp.mid(7), bm.serverTime, isHistory, bm.msgid);
            } else {
                emit messageReceived(m_host, target, bm.nick,
                                     text, bm.serverTime, isHistory, bm.msgid, bm.replyTo);
            }
        } else if (bm.command == "NOTICE" && bm.params.size() >= 1) {
            const QString btext = bm.params.size() >= 2 ? bm.params.last() : bm.trailing;
            emit noticeReceived(m_host, bm.params[0], bm.nick,
                                btext, bm.serverTime, isHistory, bm.msgid, bm.replyTo);
        }
    }
}

// ---------------------------------------------------------------------------
// BOUNCER (soju)
// ---------------------------------------------------------------------------

void IrcClient::handleBouncer(const QStringList &params, const QString &trailing)
{
    Q_UNUSED(trailing)
    if (params.isEmpty()) return;
    const QString sub = params[0].toUpper();

    if (sub == "NETWORK" && params.size() >= 2) {
        QHash<QString,QString> attrs;
        for (const QString &part : params[1].split(';', Qt::SkipEmptyParts)) {
            const qsizetype eq = part.indexOf('=');
            if (eq != -1) attrs.insert(part.left(eq), part.mid(eq+1));
        }
        const QString id    = attrs.value("id");
        const QString name  = attrs.value("name").isEmpty() ? id : attrs.value("name");
        const QString state = attrs.value("state");
        if (!id.isEmpty())
            m_bouncerNetBuf.append({id, name, state});
        if (!m_bouncerListing) {
            // live state-change notify (not the initial listing)
            emit bouncerNetworkReceived(m_host, id, name, state == "connected");
        }
    } else if (sub == "LISTNETWORKS" && params.size() >= 2
               && params[1].toUpper() == "END") {
        m_bouncerListing = false;
        emit bouncerNetworksListed(m_host, m_bouncerNetBuf);
        m_bouncerNetBuf.clear();
    }
}

// ---------------------------------------------------------------------------
// CAP negotiation
// ---------------------------------------------------------------------------

void IrcClient::handleCap(const QStringList &params, const QString &trailing)
{
    if (params.size() < 2) return;
    const QString subCmd = params[1].toUpper();

    if (subCmd == "LS") {
        // Buffer multi-line responses (params[2] == "*" means more lines coming)
        const bool more = (params.size() >= 3 && params[2] == "*");
        m_capLsBuffer += trailing.split(' ', Qt::SkipEmptyParts);
        if (more) return;

        const QStringList available = m_capLsBuffer;
        m_capLsBuffer.clear();

        // Servers may advertise caps as "name=value" (e.g. "sasl=PLAIN,EXTERNAL")
        auto hasAvailable = [&](const QString &cap) {
            for (const QString &a : available)
                if (a == cap || a.startsWith(cap + "="))
                    return true;
            return false;
        };

        // STS (Strict Transport Security)
        for (const QString &a : available) {
            if (a != "sts" && !a.startsWith("sts=")) continue;
            const QString val = a.startsWith("sts=") ? a.mid(4) : QString();
            quint16 stsPort     = 0;
            qint64  stsDuration = -1;
            for (const QString &kv : val.split(',', Qt::SkipEmptyParts)) {
                if (kv.startsWith("port="))
                    stsPort = kv.mid(5).toUShort();
                else if (kv.startsWith("duration="))
                    stsDuration = kv.mid(9).toLongLong();
            }
            if (stsDuration == 0) {
                StsStore::remove(m_host);
            } else if (stsDuration > 0) {
                if (m_ssl) {
                    // Already on TLS — refresh the stored policy
                    StsStore::store(m_host, m_port, stsDuration);
                } else if (stsPort > 0) {
                    // Plain connection — store policy and reconnect over TLS
                    StsStore::store(m_host, stsPort, stsDuration);
                    m_ssl  = true;
                    m_port = stsPort;
                    // m_stsUpgrade must be set BEFORE abort() — abort() may call
                    // onDisconnected() synchronously on some platforms, and that slot
                    // checks m_stsUpgrade to suppress the normal reconnect schedule.
                    // The singleShot lambda fires after onDisconnected() clears the flag,
                    // so connectToHostEncrypted() always runs in UnconnectedState.
                    m_stsUpgrade = true;
                    emit serverMessage(m_host, QString("STS: upgrading to TLS on port %1").arg(stsPort));
                    QTimer::singleShot(0, this, [this]() {
                        if (sockState() != QAbstractSocket::UnconnectedState)
                            sockAbort();
                        applyProxy();
                        m_socket->connectToHostEncrypted(m_host, m_port);
                    });
                }
            }
            break;
        }

        QStringList desired = {
            "multi-prefix", "away-notify", "server-time",
            "message-tags", "batch", "labeled-response", "typing", "draft/typing",
            "chathistory", "draft/chathistory", "chghost", "draft/react",
            "account-notify", "account-tag", "extended-join", "invite-notify", "setname",
            "userhost-in-names", "draft/message-redaction", "draft/multiline",
            "draft/metadata-2", "cap-notify",
        };

        // ZNC-specific caps
        if (m_bouncerType == BouncerType::ZNC) {
            desired << "znc.in/playback"
                    << "znc.in/self-message"
                    << "znc.in/batch";
        }

        // Soju-specific caps
        if (m_bouncerType == BouncerType::Soju) {
            desired << "soju.im/bouncer-networks"
                    << "soju.im/bouncer-networks-notify"
                    << "soju.im/read"
                    << "soju.im/no-implicit-names";
        }

        if ((!m_saslUser.isEmpty() && !m_saslPassword.isEmpty()) || m_saslExternal)
            desired << "sasl";

        QStringList want;
        for (const QString &cap : desired)
            if (hasAvailable(cap))
                want << cap;

        if (!want.isEmpty()) {
            m_requestedCaps = QSet<QString>(want.begin(), want.end());
            sendRaw("CAP REQ :" + want.join(' '));
        } else {
            sendRaw("CAP END");
        }
        return;
    }

    if (subCmd == "ACK") {
        const QStringList acked = trailing.split(' ', Qt::SkipEmptyParts);
        for (const QString &cap : acked)
            m_ackedCaps.insert(cap.startsWith('-') ? cap.mid(1) : cap);

        if (!m_registered) {
            if (acked.contains("sasl") &&
                ((!m_saslUser.isEmpty() && !m_saslPassword.isEmpty()) || m_saslExternal)) {
                m_saslPending = true;
                sendRaw(m_saslExternal ? "AUTHENTICATE EXTERNAL" : "AUTHENTICATE PLAIN");
            } else if (!m_requestedCaps.contains("sasl") || m_ackedCaps.contains("sasl")) {
                sendRaw("CAP END");
            }
            // else: sasl was requested but not yet acked — wait for the next ACK
        }


        // Request soju network list once caps are acked and SASL is not in-flight.
        // Skip if BOUNCER LISTNETWORKS was already rejected this session.
        if (m_bouncerType == BouncerType::Soju &&
            m_ackedCaps.contains("soju.im/bouncer-networks") &&
            !m_saslPending && !m_bouncerListing && !m_bouncerListingUnsupported) {
            m_bouncerListing = true;
            sendRaw("BOUNCER LISTNETWORKS");
        }
        return;
    }

    if (subCmd == "NAK") {
        if (!m_registered)
            sendRaw("CAP END");
        return;
    }

    if (subCmd == "NEW") {
        // Server added capabilities mid-session — request any we want
        const QStringList newCaps = trailing.split(' ', Qt::SkipEmptyParts);
        QStringList desired = {
            "multi-prefix", "away-notify", "server-time",
            "message-tags", "batch", "labeled-response", "typing", "draft/typing",
            "chathistory", "draft/chathistory", "chghost", "draft/react",
            "account-notify", "account-tag", "extended-join", "invite-notify", "setname",
            "userhost-in-names", "draft/message-redaction", "draft/multiline",
            "draft/metadata-2", "cap-notify",
        };
        if (m_bouncerType == BouncerType::ZNC)
            desired << "znc.in/playback" << "znc.in/self-message" << "znc.in/batch";
        if (m_bouncerType == BouncerType::Soju)
            desired << "soju.im/bouncer-networks" << "soju.im/bouncer-networks-notify"
                    << "soju.im/read" << "soju.im/no-implicit-names";
        if ((!m_saslUser.isEmpty() && !m_saslPassword.isEmpty()) || m_saslExternal)
            desired << "sasl";

        QStringList want;
        for (const QString &cap : newCaps) {
            const QString name = cap.section('=', 0, 0);
            if (desired.contains(name) && !m_ackedCaps.contains(name))
                want << name;
        }
        if (!want.isEmpty())
            sendRaw("CAP REQ :" + want.join(' '));
        return;
    }

    if (subCmd == "DEL") {
        // Server removed capabilities mid-session
        const QStringList delCaps = trailing.split(' ', Qt::SkipEmptyParts);
        for (const QString &cap : delCaps)
            m_ackedCaps.remove(cap);
        return;
    }
}

// ---------------------------------------------------------------------------
// Numeric handlers
// ---------------------------------------------------------------------------

void IrcClient::handleNumeric(const QString &cmd, const QStringList &params, const QString &trailing)
{
    const int n = cmd.toInt();

    switch (n) {
    case 1:   // RPL_WELCOME
        m_registered = true;
        emit selfNickChanged(m_host, m_nick);
        emit connected(m_host);
        emit serverMessage(m_host, trailing);
        if (!m_nickservPassword.isEmpty()) {
            if (!m_ssl)
                emit serverMessage(m_host, "Warning: sending NickServ password over unencrypted connection");
            sendRaw("PRIVMSG NickServ :IDENTIFY " + m_nickservPassword);
            emit serverMessage(m_host, "Sent NickServ IDENTIFY");
        }
        // ZNC: request playback of all buffers since last seen
        if (m_bouncerType == BouncerType::ZNC &&
            m_ackedCaps.contains("znc.in/playback")) {
            sendRaw("PRIVMSG *playback :PLAY * 0");
        }
        if (!m_monitorList.isEmpty())
            sendRaw("MONITOR + " + m_monitorList.join(','));
        if (m_ackedCaps.contains("draft/metadata-2"))
            sendRaw("METADATA * SUB display-name avatar");
        break;

    case 2:   // RPL_YOURHOST
    case 3:   // RPL_CREATED
    case 4:   // RPL_MYINFO
    case 5: { // RPL_ISUPPORT
        // params[0] = our nick, params[1..n-1] = tokens, trailing = "are supported..."
        for (int i = 1; i < params.size(); ++i) {
            if (params[i].compare("UTF8ONLY", Qt::CaseInsensitive) == 0) {
                m_utf8Only = true;
                emit serverMessage(m_host, "UTF8ONLY: server enforces UTF-8 encoding");
            } else if (params[i].compare("WHOX", Qt::CaseInsensitive) == 0) {
                m_supportsWhox = true;
            }
        }
        emit serverMessage(m_host, trailing);
        break;
    }
    case 251: case 252: case 253: case 254: case 255:
    case 265: case 266:
    case 372: // RPL_MOTD
    case 375: // RPL_MOTDSTART
    case 376: // RPL_ENDOFMOTD
    case 422: // ERR_NOMOTD
        emit serverMessage(m_host, trailing);
        break;

    case 324: // RPL_CHANNELMODEIS
        if (params.size() >= 3) {
            QStringList modeParts = params.mid(2);
            emit modesReceived(m_host, params[1], modeParts.join(' '));
        }
        break;

    case 332: // RPL_TOPIC
        if (params.size() >= 2)
            emit topicReceived(m_host, params[1], trailing);
        break;

    case 315: break; // RPL_ENDOFWHO — suppress

    case 333: // RPL_TOPICWHOTIME
        if (params.size() >= 4) {
            bool ok = false;
            const quint64 ts = params[3].toULongLong(&ok);
            if (ok)
                emit topicSetByReceived(m_host, params[1], params[2], ts);
        }
        break;

    case 353: { // RPL_NAMREPLY
        if (params.size() >= 3) {
            const QString &channel = params[2];
            const QStringList nicks = trailing.split(' ', Qt::SkipEmptyParts);
            m_namesBuffer[channel.toLower()] += nicks;
        }
        break;
    }

    case 352: { // RPL_WHOREPLY — <client> <channel> <user> <host> <server> <nick> <flags> :<hop> <real>
        if (params.size() >= 7) {
            const QString &channel = params[1];
            const QString &nick    = params[5];
            const QString &flags   = params[6];
            if (channel.startsWith('#') || channel.startsWith('&'))
                emit whoEntryReceived(m_host, channel, nick, flags);
        }
        break;
    }

    case 354: { // RPL_WHOSPCRPL — Ergo format: [me, type, channel, nick, flags, account]
        if (params.size() >= 5) {
            const QString &channel = params[2];
            const QString &nick    = params[3];
            const QString &flags   = params[4];
            const QString account = params.size() >= 6 ? params[5] : QString();
            if (channel.startsWith('#') || channel.startsWith('&'))
                emit whoEntryReceived(m_host, channel, nick, flags);
            if (!account.isEmpty() && account != "0" && account != "*")
                emit accountChanged(m_host, nick, account);
        }
        break;
    }

    case 366: { // RPL_ENDOFNAMES
        if (params.size() >= 2) {
            const QString &channel = params[1];
            emit namesReceived(m_host, channel, m_namesBuffer.take(channel.toLower()));
            emit namesDone(m_host, channel);
            sendRaw("MODE " + channel);
            if (m_supportsWhox)
                sendRaw("WHO " + channel + " %tcnfa,42");
            else
                sendRaw("WHO " + channel);
            // Request chat history on join
            if (m_ackedCaps.contains("chathistory") || m_ackedCaps.contains("draft/chathistory"))
                requestHistory(channel, 100);
        }
        break;
    }

    case 900: // RPL_LOGGEDIN
        emit serverMessage(m_host, trailing);
        break;

    case 903: // RPL_SASLSUCCESS
        emit serverMessage(m_host, "SASL authentication successful");
        m_saslPending = false;
        sendRaw("CAP END");
        break;

    case 904: case 905: case 906:
        emit serverMessage(m_host, "SASL authentication failed: " + trailing);
        m_saslPending = false;
        sendRaw("CAP END");
        break;

    case 433: // ERR_NICKNAMEINUSE
        if (!m_registered)
            setNick(m_nick + "_");
        else
            emit errorMessage(m_host, "Nickname " + m_nick + " is already in use");
        break;

    case 431: case 432:
        emit errorMessage(m_host, "Nick error: " + trailing);
        break;

    case 730: { // RPL_MONONLINE
        QStringList nicks;
        for (const QString &part : trailing.split(',', Qt::SkipEmptyParts)) {
            const QString nick = part.section('!', 0, 0).trimmed();
            if (!nick.isEmpty()) nicks << nick;
        }
        if (!nicks.isEmpty()) emit monitorOnline(m_host, nicks);
        break;
    }
    case 731: { // RPL_MONOFFLINE
        QStringList nicks;
        for (const QString &part : trailing.split(',', Qt::SkipEmptyParts)) {
            const QString nick = part.section('!', 0, 0).trimmed();
            // offline nicks have no !user@host, but handle both forms
            if (!nick.isEmpty()) nicks << nick;
        }
        if (!nicks.isEmpty()) emit monitorOffline(m_host, nicks);
        break;
    }
    case 732: // RPL_MONLIST
        emit serverMessage(m_host, "Monitor: " + trailing);
        break;
    case 733: // RPL_ENDOFMONLIST
        emit serverMessage(m_host, "End of monitor list.");
        break;
    case 734: // ERR_MONLISTFULL
        emit errorMessage(m_host, "Monitor list full: " + trailing);
        break;

    case 761: { // RPL_KEYVALUE — response to METADATA SET/GET: <client> <target> <key> <visibility> :<value>
        // params: [client, target, key, visibility, ?value], trailing = value
        // Ergo echoes target as "*" when you SET your own metadata; resolve to own nick
        // ircmsg only adds trailing colon when value has spaces — plain URLs land in params[4]
        if (params.size() >= 3) {
            QString target     = params[1];
            if (target == QLatin1String("*")) target = m_nick;
            const QString &key = params[2];
            if ((key == "display-name" || key == "avatar") && !target.startsWith('#')) {
                const QString value = !trailing.isEmpty() ? trailing : params.value(4);
                emit userMetaChanged(m_host, target, key, value);
            }
        }
        break;
    }

    case 321: // RPL_LISTSTART
        m_listBuffer.clear();
        break;

    case 322: // RPL_LIST — <client> <channel> <count> :<topic>
        if (params.size() >= 3) {
            m_listBuffer.append({params[1], params[2], trailing});
            emit channelListEntry(m_host, params[1], params[2].toInt(), trailing);
        }
        break;

    case 323: // RPL_LISTEND
        emit channelListEnd(m_host, static_cast<int>(m_listBuffer.size()));
        emit channelListReceived(m_host, m_listBuffer);
        m_listBuffer.clear();
        break;

    case 391: { // RPL_TIME — <client> <server> [<timestamp>] :<human-readable>
        const QString srv  = params.size() >= 2 ? params[1] : m_host;
        const QString time = trailing.isEmpty() ? "(no time returned)" : trailing;
        emit contextualMessage(m_host, "Time on " + srv + ": " + time);
        break;
    }

    case 211: case 212: case 213: case 214: case 215:
    case 216: case 217: case 218: case 241: case 242:
    case 243: case 244: case 249: case 250: { // STATS reply lines
        QStringList parts = params.mid(1); // skip client nick
        if (!trailing.isEmpty()) parts << trailing;
        if (!parts.isEmpty()) emit contextualMessage(m_host, parts.join(' '));
        break;
    }
    case 219: { // RPL_ENDOFSTATS
        const QString query = params.size() >= 2 ? params[1] : QString();
        const QString msg   = trailing.isEmpty() ? "End of STATS" : trailing;
        emit contextualMessage(m_host, query.isEmpty() ? msg : msg + " (" + query + ")");
        break;
    }

    case 314: { // RPL_WHOWASUSER — <client> <nick> <user> <host> * :<realname>
        if (params.size() >= 4) {
            const QString line = params[1] + " was " + params[2] + "@" + params[3]
                + (trailing.isEmpty() ? QString() : " (" + trailing + ")");
            emit contextualMessage(m_host, line);
        }
        break;
    }
    case 305: // RPL_UNAWAY
        m_away = false;
        emit awayChanged(m_host, false);
        emit contextualMessage(m_host, "You are no longer away");
        break;

    case 306: // RPL_NOWAWAY
        m_away = true;
        emit awayChanged(m_host, true);
        emit contextualMessage(m_host, m_awayMsg.isEmpty()
            ? QString("You are now away")
            : "You are now away: " + m_awayMsg);
        break;

    case 301: // RPL_AWAY — <client> <nick> :<away message>
        if (params.size() >= 2 && !trailing.isEmpty())
            emit contextualMessage(m_host, params[1] + " is away: " + trailing);
        break;

    // WHOIS/WHOWAS replies — route to active channel/window
    case 307: case 311: case 312: case 313:
    case 317: case 318: case 319: case 320: case 369:
    case 671:
        if (!trailing.isEmpty())
            emit contextualMessage(m_host, trailing);
        break;

    case 766: // ERR_NOMATCHINGKEY — metadata key not set, expected and silent
    case 768: // ERR_KEYNOTSET
        break;

    default:
        if (!trailing.isEmpty()) {
            if (n >= 400)
                emit errorMessage(m_host, trailing);
            else
                emit serverMessage(m_host, trailing);
        }
        break;
    }
}
