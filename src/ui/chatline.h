#pragma once
#include <QList>
#include <QPixmap>
#include <QString>
#include <QTextCharFormat>

enum class ChatLineRole { Message, EventGroup, PreviewCard, Reaction, StatusLine };

struct ChatSegment {
    int             start{0};
    int             length{0};
    QTextCharFormat format;
    QString         anchor; // "" | "nick:N" | "msgid:M" | "url:U" | "evgrp:G" | "preview:U"
};

struct ChatLine {
    QString             text;
    QList<ChatSegment>  segments;
    QString             id;        // msgid | "evgrp:G" | "rx:M" | ""
    ChatLineRole        role{ChatLineRole::Message};
    bool                hangIndent{false};
    QPixmap             image;     // non-null for PreviewCard lines

    // Layout cache — invalidated on resize/font change
    mutable int     cachedH{0};
    struct VisLine {
        int    charStart;
        int    charEnd;
        qreal  x, y, w, h;
    };
    mutable QList<VisLine> visLines;
};
