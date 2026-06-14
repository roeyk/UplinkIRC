#pragma once

#include <QString>
#include <QDateTime>

enum class MessageType {
    Privmsg,
    Notice,
    Action,
    Server,    // server/status text
    Reply,     // numeric command response (WHOIS, STATS, etc.)
    Wallops,   // WALLOPS broadcast
    Join,
    Part,
    Quit,
    Kick,
    Nick,
    Topic,
    Error,
};

struct Message {
    QDateTime   timestamp;
    QString     nick;
    QString     text;
    MessageType type{MessageType::Privmsg};
    bool        isHistory{false};
    bool        redacted{false};
    QString     msgid;
    QString     replyTo;
    QString     account;  // sender's NickServ account (from account-tag), empty if unknown

    static Message make(MessageType t, const QString &nick, const QString &text,
                        const QDateTime &ts = {}, bool history = false,
                        const QString &msgid = {}, const QString &replyTo = {},
                        const QString &account = {})
    {
        return { ts.isValid() ? ts : QDateTime::currentDateTime(), nick, text, t, history, false, msgid, replyTo, account };
    }
};
