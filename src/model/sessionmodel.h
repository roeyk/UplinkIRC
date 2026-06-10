#pragma once

#include "serversession.h"
#include "config/config.h"
#include <QDateTime>
#include <QFile>
#include <QHash>
#include <QObject>
#include <QList>

class IrcClient;
class QTimer;

class SessionModel : public QObject
{
    Q_OBJECT

public:
    explicit SessionModel(QObject *parent = nullptr);

    // Create a client for each server in config and start connecting
    void loadConfig(const Config &cfg);
    void addServer(const ServerConfig &sc);
    void removeServer(const QString &host);
    void updateServer(const QString &oldHost, const ServerConfig &sc);
    void syncServers(const QList<ServerConfig> &servers);
    void closeBuffer(const QString &host, const QString &target);

    // Read access for UI
    const QList<ServerSession> &sessions() const { return m_sessions; }
    ServerSession *session(const QString &host);
    Channel       *channel(const QString &host, const QString &name);

    // Active selection — UI drives this
    void setActive(const QString &host, const QString &channel);
    QString activeHost()    const { return m_activeHost; }
    QString activeChannel() const { return m_activeChannel; }

    // Send on behalf of a session
    void sendMessage(const QString &host, const QString &target, const QString &text,
                     const QString &replyToMsgid = {});
    void sendRaw    (const QString &host, const QString &line);
    void localMessage(const QString &host, const QString &target, const QString &text);
    QString selfNick  (const QString &host);
    bool    hasMention(const QString &host, const QString &channel);
    void sendJoin   (const QString &host, const QString &channel, const QString &key = {});
    void sendPart   (const QString &host, const QString &channel, const QString &reason = {});
    void sendNick   (const QString &host, const QString &nick);
    void sendAction (const QString &host, const QString &target, const QString &text);
    void sendTyping (const QString &host, const QString &channel, const QString &state);
    void openPM    (const QString &host, const QString &nick);
    IrcClient *clientFor(const QString &host);

    void ignoreNick  (const QString &nick);
    void unignoreNick(const QString &nick);
    bool isIgnored   (const QString &nick) const;

    void sendReact(const QString &host, const QString &target,
                   const QString &msgid, const QString &emoji);
    void sendRedact(const QString &host, const QString &target,
                    const QString &msgid, const QString &reason = {});

    void monitorAdd   (const QString &host, const QString &nick);
    void monitorRemove(const QString &host, const QString &nick);
    void monitorClear (const QString &host);
    void monitorStatus(const QString &host);
    void pinCertificate(const QString &host, const QString &fingerprint);
    void acceptCertificateOnce(const QString &host, const QString &fingerprint);

signals:
    // Structural changes — sidebar needs a repaint
    void serverAdded    (const QString &host);
    void serverConnected(const QString &host);
    void serverDisconnected(const QString &host);
    void channelAdded   (const QString &host, const QString &channel);
    void channelRemoved (const QString &host, const QString &channel);

    // Event batch (condensed join/part/quit/nick/kick line — updated in-place)
    void eventBatchUpdated(const QString &host, const QString &channel,
                           const QString &batchId, const QString &html);

    // Content changes — chat view needs updating
    void messageAdded(const QString &host, const QString &channel, const Message &msg);
    void topicChanged  (const QString &host, const QString &channel, const QString &topic);
    void modesChanged  (const QString &host, const QString &channel);
    void nickListChanged(const QString &host, const QString &channel);
    void nickAdded      (const QString &host, const QString &channel, const QString &nick);
    void nickRemoved    (const QString &host, const QString &channel, const QString &nick);
    void nickRenamed    (const QString &host, const QString &channel,
                         const QString &oldNick, const QString &newNick);
    void unreadChanged   (const QString &host, const QString &channel, int count);
    void reactionsChanged(const QString &host, const QString &channel, const QString &msgid);
    void messageRedacted (const QString &host, const QString &channel, const QString &msgid);

    // Self
    void selfNickChanged(const QString &host, const QString &nick);

    // Connection quality
    void pingRtt          (const QString &host, int ms);
    void serverReconnecting(const QString &host);

    // Typing
    void typingReceived(const QString &host, const QString &channel,
                        const QString &nick, const QString &state);

    // Channel list
    void channelListEntry(const QString &host, const QString &channel, int users, const QString &topic);
    void channelListEnd  (const QString &host, int total);

    // User metadata
    void userMetaChanged (const QString &host, const QString &nick,
                          const QString &key,  const QString &value);

    // TLS cert pin
    void sslFingerprintPrompt(const QString &host, const QString &fingerprint);

    // DCC
    void dccSendReceived(const QString &server, const QString &fromNick,
                         const QString &filename, quint32 ip, quint16 port, qint64 filesize);
    void dccPassiveOfferReceived(const QString &server, const QString &fromNick,
                                  const QString &filename, quint32 ip,
                                  qint64 filesize, const QString &token);
    void dccPassiveSendReply(const QString &server, const QString &fromNick,
                              const QString &filename, quint32 ip, quint16 port,
                              qint64 filesize, const QString &token);

private:
    void attachClient(IrcClient *client, const ServerConfig &cfg);
    void spawnSession(const ServerConfig &sc, bool addToConfig);

    // IrcClient signal handlers
    void onConnected      (const QString &host);
    void onDisconnected   (const QString &host);
    void onSocketError    (const QString &host, const QString &error);
    void onMessage        (const QString &host, const QString &target,
                           const QString &nick, const QString &text,
                           const QDateTime &serverTime, bool isHistory,
                           const QString &msgid, const QString &replyTo);
    void onNotice         (const QString &host, const QString &target,
                           const QString &nick, const QString &text,
                           const QDateTime &serverTime, bool isHistory,
                           const QString &msgid, const QString &replyTo);
    void onAction         (const QString &host, const QString &target,
                           const QString &nick, const QString &text,
                           const QDateTime &serverTime, bool isHistory,
                           const QString &msgid);
    void onUserJoined     (const QString &host, const QString &channel, const QString &nick);
    void onUserParted     (const QString &host, const QString &channel,
                           const QString &nick, const QString &reason);
    void onUserQuit       (const QString &host, const QString &nick, const QString &reason);
    void onNetsplitDetected(const QString &host, const QString &servers, const QStringList &nicks);
    void onNetjoinDetected (const QString &host, const QString &servers,
                            const QStringList &channels, const QStringList &nicks);
    void onStandardReply   (const QString &host, const QString &channel,
                            const QString &severity, const QString &text);
    void onNickChanged    (const QString &host, const QString &oldNick, const QString &newNick);
    void onKicked         (const QString &host, const QString &channel, const QString &nick,
                           const QString &by,   const QString &reason);
    void onTopicReceived  (const QString &host, const QString &channel, const QString &topic);
    void onModesReceived  (const QString &host, const QString &channel, const QString &modes);
    void onNamesReceived  (const QString &host, const QString &channel, const QStringList &nicks);
    void onWhoEntry       (const QString &host, const QString &channel,
                           const QString &nick, const QString &flags);
    void onServerMessage     (const QString &host, const QString &text);
    void onErrorMessage      (const QString &host, const QString &text);
    void onContextualMessage (const QString &host, const QString &text);
    void onCtcpPingReply     (const QString &host, const QString &nick, qint64 rttMs);
    void onCtcpTimeReply  (const QString &host, const QString &nick, const QString &timeStr);
    void onSelfNickChanged(const QString &host, const QString &nick);
    void onHostChanged    (const QString &host, const QString &nick,
                           const QString &newUser, const QString &newHost);
    void onReactReceived  (const QString &host, const QString &target,
                           const QString &nick,  const QString &msgid,
                           const QString &emoji);
    void onAccountChanged (const QString &host, const QString &nick, const QString &account);
    void onUserMetaChanged(const QString &host, const QString &nick,
                           const QString &key,  const QString &value);
    void onMessageRedacted(const QString &host, const QString &senderNick,
                           const QString &target, const QString &msgid, const QString &reason);
    void onInviteNotify   (const QString &host, const QString &inviter,
                           const QString &channel, const QString &targetNick);
    void onSetNameReceived(const QString &host, const QString &nick, const QString &realname);
    void onMonitorOnline  (const QString &host, const QStringList &nicks);
    void onMonitorOffline (const QString &host, const QStringList &nicks);

    void postMessage(const QString &host, const QString &target, const Message &msg);
    void logMessage (const QString &host, const QString &target, const Message &msg);
    void emitOrUpdateBatch(const QString &host, const QString &channel, Channel &ch);
    void restartBatchTimer(const QString &host, const QString &channel);

    QList<ServerSession> m_sessions;
    QList<IrcClient *>   m_clients;
    Config               m_config;
    QSet<QString>        m_ignoredNicks;
    QHash<QString, QFile*> m_logFiles;
    QHash<QString, QTimer*> m_batchTimers;  // key: "host|channel"

    QString m_activeHost;
    QString m_activeChannel;
};
