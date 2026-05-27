#pragma once

#include <QString>
#include <QStringList>

struct IrcMessage {
    QString     prefix;   // nick!user@host or server
    QString     nick;     // parsed from prefix
    QString     user;
    QString     host;
    QString     command;
    QStringList params;
    QString     trailing;
    QString     tags;     // IRCv3 message tags (@key=value;...)

    bool isValid() const { return !command.isEmpty(); }
};

class IrcParser
{
public:
    static IrcMessage parse(const QString &raw);
};
