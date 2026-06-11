#pragma once

#include <QColor>
#include <QList>
#include <QRegularExpression>
#include <QString>
#include "model/message.h"

struct Channel;

namespace ChatRenderer {

struct Context {
    bool              coloredNicks{false};
    QString           nickBrackets;
    int               emojiPt{10};
    int               chatPt{10};
    bool              validTheme{false};
    QString           themeText;
    QRegularExpression selfNickRe; // pre-compiled; invalid = no highlight
    const Channel    *channel{nullptr}; // for reply-reference lookup
};

QString formatMessage    (const Message &msg, const Context &ctx);
QString formatEventGroup (const QList<Message> &msgs, const Context &ctx,
                          const QString &groupId = {}, bool expanded = false);
QString linkifyTopic     (const QString &text);
QColor  nickColor     (const QString &nick);

// Lower-level helpers — exposed for unit tests
QString htmlAttr      (const QString &s);
QString ircToHtml     (const QString &raw);
QString linkifyHtml   (const QString &html);
QString wrapEmojiHtml (const QString &html, int ptSize);

} // namespace ChatRenderer
