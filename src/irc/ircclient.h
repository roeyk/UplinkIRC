#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QSslSocket>

class IrcClient : public QObject
{
    Q_OBJECT

public:
    explicit IrcClient(QObject *parent = nullptr);
    ~IrcClient() override;

    void connectToServer(const QString &host, quint16 port, bool ssl = true);
    void disconnect();
    void sendRaw(const QString &line);

    void join(const QString &channel);
    void part(const QString &channel, const QString &reason = {});
    void privmsg(const QString &target, const QString &text);
    void setNick(const QString &nick);

signals:
    void connected(const QString &server);
    void disconnected(const QString &server);
    void messageReceived(const QString &server, const QString &channel, const QString &nick, const QString &text);
    void rawReceived(const QString &line);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError error);

private:
    void processLine(const QString &line);

    QSslSocket *m_socket;
    QString     m_host;
    quint16     m_port{6697};
    QString     m_nick;
    QString     m_user;
    QString     m_realname;
    QString     m_buffer;
};
