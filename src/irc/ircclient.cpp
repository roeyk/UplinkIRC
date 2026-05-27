#include "ircclient.h"
#include "ircparser.h"

#include <QSslSocket>

IrcClient::IrcClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QSslSocket(this))
    , m_nick("UplinkUser")
    , m_user("uplink")
    , m_realname("UplinkIRC")
{
    connect(m_socket, &QSslSocket::connected,    this, &IrcClient::onConnected);
    connect(m_socket, &QSslSocket::disconnected, this, &IrcClient::onDisconnected);
    connect(m_socket, &QSslSocket::readyRead,    this, &IrcClient::onReadyRead);
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &IrcClient::onError);
}

IrcClient::~IrcClient() = default;

void IrcClient::connectToServer(const QString &host, quint16 port, bool ssl)
{
    m_host = host;
    m_port = port;
    if (ssl)
        m_socket->connectToHostEncrypted(host, port);
    else
        m_socket->connectToHost(host, port);
}

void IrcClient::disconnect()
{
    sendRaw("QUIT :UplinkIRC");
    m_socket->disconnectFromHost();
}

void IrcClient::sendRaw(const QString &line)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) return;
    m_socket->write((line + "\r\n").toUtf8());
}

void IrcClient::join(const QString &channel)    { sendRaw("JOIN " + channel); }
void IrcClient::part(const QString &channel, const QString &reason)
{
    sendRaw(reason.isEmpty() ? "PART " + channel : "PART " + channel + " :" + reason);
}
void IrcClient::privmsg(const QString &target, const QString &text) { sendRaw("PRIVMSG " + target + " :" + text); }
void IrcClient::setNick(const QString &nick)    { m_nick = nick; sendRaw("NICK " + nick); }

void IrcClient::onConnected()
{
    sendRaw("CAP LS 302");
    sendRaw("NICK " + m_nick);
    sendRaw("USER " + m_user + " 0 * :" + m_realname);
    emit connected(m_host);
}

void IrcClient::onDisconnected()
{
    emit disconnected(m_host);
}

void IrcClient::onReadyRead()
{
    m_buffer += QString::fromUtf8(m_socket->readAll());
    int idx;
    while ((idx = m_buffer.indexOf('\n')) != -1) {
        QString line = m_buffer.left(idx).trimmed();
        m_buffer.remove(0, idx + 1);
        emit rawReceived(line);
        processLine(line);
    }
}

void IrcClient::onError(QAbstractSocket::SocketError)
{
    emit disconnected(m_host);
}

void IrcClient::processLine(const QString &line)
{
    IrcMessage msg = IrcParser::parse(line);
    if (!msg.isValid()) return;

    if (msg.command == "PING") {
        sendRaw("PONG :" + (msg.params.isEmpty() ? QString{} : msg.params.last()));
        return;
    }

    if (msg.command == "PRIVMSG" && msg.params.size() >= 2) {
        emit messageReceived(m_host, msg.params[0], msg.nick, msg.trailing);
        return;
    }

    // TODO: handle JOIN, PART, QUIT, NICK, MODE, NOTICE, numerics, CAP negotiation
}
