#include "chatrenderer.h"
#include "model/channel.h"
#include <QDate>
#include <QDateTime>
#include <QRegularExpression>
#include <QFont>

namespace ChatRenderer {

static const QRegularExpression s_urlRe(
    R"((https?://[^\s<>"]+))",
    QRegularExpression::CaseInsensitiveOption);

QString htmlAttr(const QString &s)
{
    QString out = s.toHtmlEscaped();
    out.replace(QLatin1Char('\''), QLatin1String("&#39;"));
    return out;
}

QString linkifyTopic(const QString &text)
{
    QString result = text.toHtmlEscaped();
    result.replace(s_urlRe, R"(<a href="\1">\1</a>)");
    return result;
}

QString linkifyHtml(const QString &html)
{
    QString result = html;
    result.replace(s_urlRe, R"(<a href="\1">\1</a>)");
    return result;
}

QColor nickColor(const QString &nick)
{
    static const char *palette[] = {
        "#e06c75", "#98c379", "#e5c07b", "#61afef",
        "#c678dd", "#56b6c2", "#d19a66", "#7ec8a0",
        "#ff7b72", "#79c0ff", "#ffa657", "#85e89d",
        "#f78166", "#58a6ff", "#d4a0f5", "#4db5bd",
    };
    static constexpr int N = int(sizeof(palette) / sizeof(palette[0]));
    return QColor(palette[qHash(nick.toLower()) % N]);
}

QString ircToHtml(const QString &raw)
{
    static const char* const kColors[] = {
        "#FFFFFF","#000000","#00007F","#009300",
        "#FF0000","#7F0000","#9C009C","#FC7F00",
        "#FFFF00","#00FC00","#009393","#00FFFF",
        "#0000FC","#FF00FF","#7F7F7F","#D2D2D2"
    };

    struct Fmt {
        bool bold{false}, italic{false}, underline{false}, strike{false};
        int  fg{-1}, bg{-1};
        bool operator==(const Fmt &o) const {
            return bold==o.bold && italic==o.italic && underline==o.underline
                && strike==o.strike && fg==o.fg && bg==o.bg;
        }
    };

    Fmt     cur;
    int     openSpans = 0;
    QString out;
    out.reserve(raw.size() * 2);

    auto closeAll = [&] {
        for (int i = 0; i < openSpans; ++i) out += "</span>";
        openSpans = 0;
    };

    auto applyFmt = [&](const Fmt &next) {
        if (next == cur) return;
        closeAll();
        cur = next;
        QString style;
        if (cur.bold)      style += "font-weight:bold;";
        if (cur.italic)    style += "font-style:italic;";
        QString td;
        if (cur.underline) td += "underline ";
        if (cur.strike)    td += "line-through ";
        if (!td.isEmpty()) style += "text-decoration:" + td.trimmed() + ";";
        if (cur.fg >= 0 && cur.fg < 16) style += QString("color:%1;").arg(kColors[cur.fg]);
        if (cur.bg >= 0 && cur.bg < 16) style += QString("background-color:%1;").arg(kColors[cur.bg]);
        if (!style.isEmpty()) {
            out += "<span style='" + style + "'>";
            openSpans = 1;
        }
    };

    int i = 0;
    const qsizetype len = raw.size();
    while (i < len) {
        const ushort c = raw[i].unicode();
        if (c == 0x02) {
            Fmt n = cur; n.bold = !cur.bold; applyFmt(n); ++i;
        } else if (c == 0x1D) {
            Fmt n = cur; n.italic = !cur.italic; applyFmt(n); ++i;
        } else if (c == 0x1F) {
            Fmt n = cur; n.underline = !cur.underline; applyFmt(n); ++i;
        } else if (c == 0x1E) {
            Fmt n = cur; n.strike = !cur.strike; applyFmt(n); ++i;
        } else if (c == 0x16) {
            Fmt n = cur; std::swap(n.fg, n.bg); applyFmt(n); ++i;
        } else if (c == 0x0F || (c == 0x03 && i+1 < len && !raw[i+1].isDigit() && raw[i+1] != ',')) {
            applyFmt({}); ++i;
        } else if (c == 0x03) {
            ++i;
            Fmt n = cur;
            if (i < len && raw[i].isDigit()) {
                int fg = raw[i++].digitValue();
                if (i < len && raw[i].isDigit()) fg = fg * 10 + raw[i++].digitValue();
                n.fg = fg;
                if (i < len && raw[i] == ',' && i+1 < len && raw[i+1].isDigit()) {
                    ++i;
                    int bg = raw[i++].digitValue();
                    if (i < len && raw[i].isDigit()) bg = bg * 10 + raw[i++].digitValue();
                    n.bg = bg;
                }
            } else {
                n.fg = -1; n.bg = -1;
            }
            applyFmt(n);
        } else if (c == 0x11) {
            ++i;
        } else {
            if      (raw[i] == '<') out += "&lt;";
            else if (raw[i] == '>') out += "&gt;";
            else if (raw[i] == '&') out += "&amp;";
            else if (raw[i] == '"') out += "&quot;";
            else                    out += raw[i];
            ++i;
        }
    }
    closeAll();
    return out;
}

QString wrapEmojiHtml(const QString &html, int ptSize)
{
    const QString open  = QString("<span style='font-size:%1pt'>").arg(ptSize);
    const QString close = QStringLiteral("</span>");

    QString result;
    result.reserve(html.size() * 2);

    qsizetype i = 0;
    while (i < html.size()) {
        if (html[i] == '<') {
            qsizetype end = html.indexOf('>', i);
            if (end == -1) { result += html.mid(i); break; }
            result += html.mid(i, end - i + 1);
            i = end + 1;
            continue;
        }

        if (html[i].isHighSurrogate() && i + 1 < html.size() && html[i+1].isLowSurrogate()) {
            const uint cp = QChar::surrogateToUcs4(html[i], html[i+1]);
            if (cp >= 0x1F000 && cp <= 0x1FAFF) {
                result += open;
                result += html[i]; result += html[i+1];
                i += 2;
                if (i < html.size() && html[i].unicode() == 0xFE0F)
                    result += html[i++];
                result += close;
            } else {
                result += html[i++]; result += html[i++];
            }
            continue;
        }

        const ushort c = html[i].unicode();
        if (c >= 0x2300 && c <= 0x27BF) {
            result += open + html[i];
            ++i;
            if (i < html.size() && html[i].unicode() == 0xFE0F)
                result += html[i++];
            result += close;
            continue;
        }

        result += html[i++];
    }
    return result;
}

QString formatMessage(const Message &msg, const Context &ctx)
{
    const QDateTime local = msg.timestamp.toLocalTime();
    const bool sameDay = local.date() == QDate::currentDate();
    const QString ts = sameDay
        ? local.toString("hh:mm")
        : local.toString("MM/dd hh:mm");

    const QString dimColor = QStringLiteral("#888888");
    auto wrap = [&](const QString &color, const QString &text) {
        return QString("<span style='color:%1'>%2</span>")
            .arg(msg.isHistory ? dimColor : color, text.toHtmlEscaped());
    };
    const int eventPt = qMax(7, qRound(ctx.chatPt * 0.82));
    auto wrapEvent = [&](const QString &color, const QString &text) {
        return QString("<span style='color:%1; font-size:%2pt'>%3</span>")
            .arg(msg.isHistory ? dimColor : color, QString::number(eventPt), text.toHtmlEscaped());
    };

    QString html;
    const QString tsSpan = msg.msgid.isEmpty()
        ? QString("<span style='color:gray'>%1</span>").arg(ts)
        : QString("<a href='msgid:%1' style='color:gray;text-decoration:none'>%2</a>")
            .arg(htmlAttr(msg.msgid), ts);

    if (msg.redacted) {
        html = tsSpan + " <span style='color:gray;font-style:italic'>[message deleted]</span>";
        return html;
    }

    switch (msg.type) {
    case MessageType::Privmsg: {
        const QString color = msg.isHistory ? dimColor
            : (ctx.coloredNicks ? nickColor(msg.nick).name()
                                : (ctx.validTheme ? ctx.themeText : QStringLiteral("#cccccc")));
        const QString &br = ctx.nickBrackets;
        QString nickOpen, nickClose;
        if (!br.isEmpty()) {
            if (br.length() % 2 == 0) {
                nickOpen  = br.left(br.length() / 2).toHtmlEscaped();
                nickClose = br.mid(br.length() / 2).toHtmlEscaped();
            } else {
                nickOpen  = QString(br.front()).toHtmlEscaped();
                nickClose = QString(br.back()).toHtmlEscaped();
            }
        }
        const QString nickDisplay = nickOpen + msg.nick.toHtmlEscaped() + nickClose;
        const QString titleAttr = msg.account.isEmpty()
            ? QString()
            : " title='account: " + htmlAttr(msg.account) + "'";
        const QString nickAnchor = QString("<a href='nick:%1'%2 style='color:%3; text-decoration:none; font-weight:bold'>%4</a>")
            .arg(htmlAttr(msg.nick), titleAttr, color, nickDisplay);
        QString textHtml = wrapEmojiHtml(linkifyHtml(ircToHtml(msg.text)), ctx.emojiPt);
        textHtml.replace('\n', QLatin1String("<br>"));
        if (ctx.selfNickRe.isValid() && !msg.isHistory)
            textHtml.replace(ctx.selfNickRe, "<span style='color:red;font-weight:bold'>\\1</span>");
        if (msg.isHistory)
            textHtml = "<span style='color:" + dimColor + "'>" + textHtml + "</span>";
        QString replySpan;
        if (!msg.replyTo.isEmpty() && ctx.channel) {
            QString origNick;
            for (const auto &orig : std::as_const(ctx.channel->messages))
                if (orig.msgid == msg.replyTo) { origNick = orig.nick; break; }
            replySpan = origNick.isEmpty()
                ? "<span style='color:#6c7086;font-size:small'>↩</span> "
                : QString("<span style='color:#6c7086;font-size:small'>↩ %1</span> ")
                    .arg(origNick.toHtmlEscaped());
        }
        html = QString("%1 %2%3 %4").arg(tsSpan, replySpan, nickAnchor, textHtml);
        break;
    }
    case MessageType::Action: {
        const QString aTitleAttr = msg.account.isEmpty()
            ? QString()
            : " title='account: " + htmlAttr(msg.account) + "'";
        const QString actionNick = QString("<a href='nick:%1'%2 style='color:inherit; text-decoration:none'>%1</a>")
            .arg(htmlAttr(msg.nick), aTitleAttr);
        html = QString("%1 <span style='color:%2'><i>* %3 %4</i></span>")
            .arg(tsSpan, msg.isHistory ? dimColor : QStringLiteral("inherit"),
                 actionNick, wrapEmojiHtml(linkifyHtml(ircToHtml(msg.text)), ctx.emojiPt));
        break;
    }
    case MessageType::Notice: {
        const QString noticeNick = QString("<a href='nick:%1' style='color:inherit; text-decoration:none'>%1</a>")
            .arg(htmlAttr(msg.nick));
        const QString noticeColor = msg.isHistory ? dimColor : QStringLiteral("#cc8800");
        html = QString("%1 <span style='color:%2'>-%3- %4</span>")
            .arg(tsSpan, noticeColor, noticeNick, wrapEmojiHtml(linkifyHtml(ircToHtml(msg.text)), ctx.emojiPt));
        break;
    }

    case MessageType::Join:
        html = wrapEvent("seagreen",  ts + " → " + msg.text); break;
    case MessageType::Part:
        html = wrapEvent("#e06b6b", ts + " ← " + msg.text); break;
    case MessageType::Quit:
        html = wrapEvent("#e06b6b", ts + " ✕ " + msg.text); break;
    case MessageType::Nick:
        html = wrapEvent("steelblue", ts + " ~ "  + msg.text); break;
    case MessageType::Kick:
        html = wrap("#e06b6b", ts + " ✕ " + msg.text); break;
    case MessageType::Topic:
        html = wrap("steelblue", ts + " ⦁ Topic: " + msg.text); break;
    case MessageType::Error:
        html = wrap("red",       ts + " !! " + msg.text); break;
    case MessageType::Server:
    default:
        html = wrap("gray",      ts + " * "  + msg.text); break;
    }

    return html;
}

QString formatEventGroup(const QList<Message> &msgs, const Context &ctx,
                         const QString &groupId, bool expanded)
{
    if (msgs.isEmpty()) return {};

    const int eventPt = qMax(7, qRound(ctx.chatPt * 0.82));

    if (expanded) {
        QString collapseAnchor;
        if (!groupId.isEmpty())
            collapseAnchor = QString("<a href='evgrp:%1' style='text-decoration:none;color:gray'>▾ </a>")
                .arg(htmlAttr(groupId));

        QStringList lines;
        bool firstLine = true;
        for (const auto &msg : msgs) {
            const QDateTime mLocal = msg.timestamp.toLocalTime();
            const bool mSameDay = mLocal.date() == QDate::currentDate();
            const QString mTs = mSameDay ? mLocal.toString("hh:mm") : mLocal.toString("MM/dd hh:mm");
            const QString mTsSpan = QString("<span style='color:gray'>%1</span>").arg(mTs);

            QString color, sym;
            switch (msg.type) {
            case MessageType::Join: color = QStringLiteral("seagreen");   sym = QStringLiteral("→"); break;
            case MessageType::Part:
            case MessageType::Quit: color = QStringLiteral("#e06b6b");   sym = QStringLiteral("←"); break;
            case MessageType::Nick: color = QStringLiteral("steelblue"); sym = QStringLiteral("~");      break;
            case MessageType::Kick: color = QStringLiteral("#e06b6b");   sym = QStringLiteral("✕"); break;
            default: continue;
            }

            const QString lineHtml =
                QString("<span style='color:%1;font-size:%2pt'>%3 %4 %5</span>")
                    .arg(color, QString::number(eventPt), mTsSpan, sym, msg.text.toHtmlEscaped());

            lines << (firstLine ? collapseAnchor + lineHtml : lineHtml);
            firstLine = false;
        }
        if (lines.isEmpty()) return {};
        return lines.join(QStringLiteral("<br>"));
    }

    // Collapsed form
    const QDateTime local = msgs.front().timestamp.toLocalTime();
    const bool sameDay = local.date() == QDate::currentDate();
    const QString ts = sameDay ? local.toString("hh:mm") : local.toString("MM/dd hh:mm");
    const QString tsSpan = QString("<span style='color:gray'>%1</span>").arg(ts);

    QStringList joins, parts, kicks;
    QList<QPair<QString,QString>> nickChanges;

    for (const auto &msg : msgs) {
        switch (msg.type) {
        case MessageType::Join:  joins.append(msg.nick);                        break;
        case MessageType::Part:
        case MessageType::Quit:  parts.append(msg.nick);                        break;
        case MessageType::Nick:  nickChanges.append({msg.nick, msg.replyTo});   break;
        case MessageType::Kick:  kicks.append(msg.nick);                        break;
        default: break;
        }
    }

    // Net-change filter: nick that joins and parts in same group → suppress both
    for (qsizetype i = joins.size() - 1; i >= 0; --i) {
        if (parts.contains(joins[i])) {
            parts.removeAll(joins[i]);
            joins.removeAt(i);
        }
    }

    const qsizetype total = joins.size() + parts.size() + nickChanges.size() + kicks.size();
    const int maxNicks = 10;
    int shown = 0;
    QStringList segments;

    auto addSection = [&](const QString &color, const QString &sym, const QStringList &nicks) {
        if (nicks.isEmpty()) return;
        QStringList display;
        for (const QString &n : nicks) {
            if (shown >= maxNicks) break;
            display << QString("<span style='color:%1'>%2</span>").arg(color, n.toHtmlEscaped());
            ++shown;
        }
        if (!display.isEmpty())
            segments << sym + " " + display.join(" ");
    };

    addSection("seagreen",  "→", joins);
    addSection("#e06b6b",   "←", parts);

    if (!nickChanges.isEmpty()) {
        QStringList display;
        for (const auto &p : std::as_const(nickChanges)) {
            if (shown >= maxNicks) break;
            display << QString("<span style='color:steelblue'>%1→%2</span>")
                .arg(p.first.toHtmlEscaped(), p.second.toHtmlEscaped());
            ++shown;
        }
        if (!display.isEmpty())
            segments << "~ " + display.join(" ");
    }

    addSection("#e06b6b", "✕", kicks);

    const qsizetype overflow = total - shown;
    QString body = segments.join("  ");
    if (overflow > 0)
        body += QString("  … %1 more").arg(overflow);

    const QString expandAnchor = groupId.isEmpty()
        ? QString()
        : QString("<a href='evgrp:%1' style='text-decoration:none;color:gray'>▸ </a>")
              .arg(htmlAttr(groupId));

    return QString("<span style='font-size:%1pt'>%2%3  %4</span>")
        .arg(eventPt).arg(expandAnchor, tsSpan, body);
}

// ── ChatLine rendering ────────────────────────────────────────────────────────

namespace {

struct TextBuilder {
    QString            text;
    QList<ChatSegment> segs;

    void append(const QString &s, const QTextCharFormat &fmt, const QString &anchor = {})
    {
        if (s.isEmpty()) return;
        ChatSegment seg;
        seg.start  = static_cast<int>(text.size());
        seg.length = static_cast<int>(s.size());
        seg.format = fmt;
        seg.anchor = anchor;
        text += s;
        segs.append(seg);
    }
};

} // anonymous namespace

static void ircToSegments(const QString &raw, const QTextCharFormat &base, TextBuilder &tb)
{
    static const QColor kColors[] = {
        QColor("#FFFFFF"), QColor("#000000"), QColor("#00007F"), QColor("#009300"),
        QColor("#FF0000"), QColor("#7F0000"), QColor("#9C009C"), QColor("#FC7F00"),
        QColor("#FFFF00"), QColor("#00FC00"), QColor("#009393"), QColor("#00FFFF"),
        QColor("#0000FC"), QColor("#FF00FF"), QColor("#7F7F7F"), QColor("#D2D2D2")
    };

    struct State {
        bool bold{false}, italic{false}, underline{false}, strike{false};
        int  fg{-1}, bg{-1};
    } state;

    QString chunk;
    int i = 0;
    const int len = raw.size();

    auto flushChunk = [&]() {
        if (chunk.isEmpty()) return;
        QTextCharFormat fmt = base;
        if (state.bold)      fmt.setFontWeight(QFont::Bold);
        if (state.italic)    fmt.setFontItalic(true);
        if (state.underline) fmt.setFontUnderline(true);
        if (state.strike)    fmt.setFontStrikeOut(true);
        if (state.fg >= 0 && state.fg < 16) fmt.setForeground(kColors[state.fg]);
        if (state.bg >= 0 && state.bg < 16) fmt.setBackground(kColors[state.bg]);
        tb.append(chunk, fmt);
        chunk.clear();
    };

    while (i < len) {
        const ushort c = raw[i].unicode();
        if      (c == 0x02) { flushChunk(); state.bold      = !state.bold;      ++i; }
        else if (c == 0x1D) { flushChunk(); state.italic    = !state.italic;    ++i; }
        else if (c == 0x1F) { flushChunk(); state.underline = !state.underline; ++i; }
        else if (c == 0x1E) { flushChunk(); state.strike    = !state.strike;    ++i; }
        else if (c == 0x16) { flushChunk(); std::swap(state.fg, state.bg);      ++i; }
        else if (c == 0x0F || (c == 0x03 && i+1 < len && !raw[i+1].isDigit() && raw[i+1] != ',')) {
            flushChunk(); state = State{}; ++i;
        } else if (c == 0x03) {
            flushChunk(); ++i;
            if (i < len && raw[i].isDigit()) {
                int fg = raw[i++].digitValue();
                if (i < len && raw[i].isDigit()) fg = fg * 10 + raw[i++].digitValue();
                state.fg = fg;
                if (i < len && raw[i] == ',' && i+1 < len && raw[i+1].isDigit()) {
                    ++i;
                    int bg = raw[i++].digitValue();
                    if (i < len && raw[i].isDigit()) bg = bg * 10 + raw[i++].digitValue();
                    state.bg = bg;
                }
            } else {
                state.fg = -1; state.bg = -1;
            }
        } else if (c == 0x11) {
            ++i;
        } else {
            chunk += raw[i++];
        }
    }
    flushChunk();
}

static void linkifySegments(TextBuilder &tb, int textStart)
{
    auto it = s_urlRe.globalMatch(tb.text, textStart);
    while (it.hasNext()) {
        const auto m = it.next();
        ChatSegment seg;
        seg.start  = static_cast<int>(m.capturedStart(1));
        seg.length = static_cast<int>(m.capturedLength(1));
        seg.anchor = "url:" + tb.text.mid(seg.start, seg.length);
        tb.segs.append(seg);
    }
}

static void addSelfNickHighlight(TextBuilder &tb, int textStart, const QRegularExpression &re)
{
    if (!re.isValid()) return;
    QTextCharFormat fmt;
    fmt.setForeground(QColor(Qt::red));
    fmt.setFontWeight(QFont::Bold);
    auto it = re.globalMatch(tb.text, textStart);
    while (it.hasNext()) {
        const auto m = it.next();
        ChatSegment seg;
        seg.start  = static_cast<int>(m.capturedStart(1));
        seg.length = static_cast<int>(m.capturedLength(1));
        seg.format = fmt;
        tb.segs.append(seg);
    }
}

ChatLine formatMessageLine(const Message &msg, const Context &ctx)
{
    const QDateTime local = msg.timestamp.toLocalTime();
    const bool sameDay = local.date() == QDate::currentDate();
    const QString ts = sameDay ? local.toString("hh:mm") : local.toString("MM/dd hh:mm");

    const QColor dimColor("#888888");
    const bool   isHistory = msg.isHistory;
    TextBuilder  tb;
    const QTextCharFormat plainFmt;

    QTextCharFormat tsFmt;
    tsFmt.setForeground(dimColor);
    const QString tsAnchor = msg.msgid.isEmpty() ? QString() : ("msgid:" + msg.msgid);
    tb.append(ts, tsFmt, tsAnchor);

    if (msg.redacted) {
        QTextCharFormat f;
        f.setForeground(dimColor);
        f.setFontItalic(true);
        tb.append(" [message deleted]", f);
        ChatLine line;
        line.text       = tb.text;
        line.segments   = tb.segs;
        line.id         = msg.msgid;
        line.role       = ChatLineRole::Message;
        line.hangIndent = false;
        return line;
    }

    const bool isText = (msg.type == MessageType::Privmsg ||
                         msg.type == MessageType::Action  ||
                         msg.type == MessageType::Notice);

    int prefixEnd = 0; // char offset where body text starts
    switch (msg.type) {
    case MessageType::Privmsg: {
        const QColor nickCol = isHistory ? dimColor
            : (ctx.coloredNicks ? nickColor(msg.nick)
                                : (ctx.validTheme ? QColor(ctx.themeText) : QColor("#cccccc")));

        // Reply indicator
        if (!msg.replyTo.isEmpty() && ctx.channel) {
            QString origNick;
            for (const auto &orig : std::as_const(ctx.channel->messages))
                if (orig.msgid == msg.replyTo) { origNick = orig.nick; break; }
            QTextCharFormat f;
            f.setForeground(QColor("#6c7086"));
            tb.append(" ↩" + (origNick.isEmpty() ? " " : " " + origNick + " "), f);
        } else {
            tb.append(" ", plainFmt);
        }

        const QString &br = ctx.nickBrackets;
        QString nickOpen, nickClose;
        if (!br.isEmpty()) {
            if (br.length() % 2 == 0) {
                nickOpen  = br.left(br.length() / 2);
                nickClose = br.mid(br.length() / 2);
            } else {
                nickOpen  = QString(br.front());
                nickClose = QString(br.back());
            }
        }
        QTextCharFormat nickFmt;
        nickFmt.setFontWeight(QFont::Bold);
        nickFmt.setForeground(nickCol);
        tb.append(nickOpen + msg.nick + nickClose, nickFmt, "nick:" + msg.nick);
        tb.append(" ", plainFmt);

        prefixEnd = static_cast<int>(tb.text.size());
        QTextCharFormat base;
        if (isHistory) base.setForeground(dimColor);
        ircToSegments(msg.text, base, tb);
        if (!isHistory) {
            linkifySegments(tb, prefixEnd);
            if (ctx.selfNickRe.isValid())
                addSelfNickHighlight(tb, prefixEnd, ctx.selfNickRe);
        }
        break;
    }
    case MessageType::Action: {
        tb.append(" ", plainFmt);
        QTextCharFormat starFmt;
        starFmt.setFontItalic(true);
        if (isHistory) starFmt.setForeground(dimColor);
        tb.append("* ", starFmt);

        QTextCharFormat nickFmt;
        nickFmt.setFontItalic(true);
        if (isHistory) nickFmt.setForeground(dimColor);
        tb.append(msg.nick + " ", nickFmt, "nick:" + msg.nick);

        prefixEnd = static_cast<int>(tb.text.size());
        QTextCharFormat base;
        base.setFontItalic(true);
        if (isHistory) base.setForeground(dimColor);
        ircToSegments(msg.text, base, tb);
        if (!isHistory) linkifySegments(tb, prefixEnd);
        break;
    }
    case MessageType::Notice: {
        const QColor col = isHistory ? dimColor : QColor("#cc8800");
        tb.append(" ", plainFmt);
        QTextCharFormat f;
        f.setForeground(col);
        tb.append("-" + msg.nick + "- ", f, "nick:" + msg.nick);
        prefixEnd = static_cast<int>(tb.text.size());
        QTextCharFormat base;
        base.setForeground(col);
        ircToSegments(msg.text, base, tb);
        if (!isHistory) linkifySegments(tb, prefixEnd);
        break;
    }
    case MessageType::Join: {
        QTextCharFormat f;
        f.setForeground(isHistory ? dimColor : QColor("seagreen"));
        tb.append(" → " + msg.text, f);
        break;
    }
    case MessageType::Part: {
        QTextCharFormat f;
        f.setForeground(isHistory ? dimColor : QColor("#e06b6b"));
        tb.append(" ← " + msg.text, f);
        break;
    }
    case MessageType::Quit: {
        QTextCharFormat f;
        f.setForeground(isHistory ? dimColor : QColor("#e06b6b"));
        tb.append(" ✕ " + msg.text, f);
        break;
    }
    case MessageType::Nick: {
        QTextCharFormat f;
        f.setForeground(isHistory ? dimColor : QColor("steelblue"));
        tb.append(" ~ " + msg.text, f);
        break;
    }
    case MessageType::Kick: {
        QTextCharFormat f;
        f.setForeground(isHistory ? dimColor : QColor("#e06b6b"));
        tb.append(" ✕ " + msg.text, f);
        break;
    }
    case MessageType::Topic: {
        QTextCharFormat f;
        f.setForeground(isHistory ? dimColor : QColor("steelblue"));
        tb.append(" ⦁ Topic: " + msg.text, f);
        break;
    }
    case MessageType::Error: {
        QTextCharFormat f;
        f.setForeground(isHistory ? dimColor : QColor(Qt::red));
        tb.append(" !! " + msg.text, f);
        break;
    }
    case MessageType::Server:
    default: {
        QTextCharFormat f;
        f.setForeground(dimColor);
        tb.append(" * " + msg.text, f);
        break;
    }
    }

    ChatLine line;
    line.text       = tb.text;
    line.segments   = tb.segs;
    line.id         = msg.msgid;
    line.role       = ChatLineRole::Message;
    line.hangIndent      = isText;
    line.hangIndentChars = prefixEnd;
    return line;
}

ChatLine formatEventGroupLine(const QList<Message> &msgs, const Context &ctx,
                               const QString &groupId, bool expanded)
{
    if (msgs.isEmpty()) return {};

    TextBuilder tb;
    const QTextCharFormat plainFmt;

    if (expanded) {
        if (!groupId.isEmpty()) {
            QTextCharFormat f;
            f.setForeground(QColor(Qt::gray));
            tb.append("▾ ", f, "evgrp:" + groupId);
        }
        bool first = true;
        for (const auto &msg : msgs) {
            if (!first) tb.append("\n", plainFmt);
            const QDateTime mLocal = msg.timestamp.toLocalTime();
            const QString mTs = mLocal.date() == QDate::currentDate()
                ? mLocal.toString("hh:mm") : mLocal.toString("MM/dd hh:mm");
            QTextCharFormat tsFmt;
            tsFmt.setForeground(QColor(Qt::gray));
            tb.append(mTs + " ", tsFmt);

            QColor col;
            QString sym;
            switch (msg.type) {
            case MessageType::Join: col = QColor("seagreen");   sym = "→"; break;
            case MessageType::Part:
            case MessageType::Quit: col = QColor("#e06b6b");   sym = "←"; break;
            case MessageType::Nick: col = QColor("steelblue"); sym = "~";  break;
            case MessageType::Kick: col = QColor("#e06b6b");   sym = "✕"; break;
            default: continue;
            }
            QTextCharFormat f;
            f.setForeground(col);
            tb.append(sym + " " + msg.text, f);
            first = false;
        }
    } else {
        const QDateTime local = msgs.front().timestamp.toLocalTime();
        const QString ts = local.date() == QDate::currentDate()
            ? local.toString("hh:mm") : local.toString("MM/dd hh:mm");

        if (!groupId.isEmpty()) {
            QTextCharFormat f;
            f.setForeground(QColor(Qt::gray));
            tb.append("▸ ", f, "evgrp:" + groupId);
        }
        QTextCharFormat tsFmt;
        tsFmt.setForeground(QColor(Qt::gray));
        tb.append(ts + "  ", tsFmt);

        QStringList joins, parts, kicks;
        QList<QPair<QString,QString>> nickChanges;
        for (const auto &msg : msgs) {
            switch (msg.type) {
            case MessageType::Join:  joins.append(msg.nick);                          break;
            case MessageType::Part:
            case MessageType::Quit:  parts.append(msg.nick);                          break;
            case MessageType::Nick:  nickChanges.append({msg.nick, msg.replyTo});     break;
            case MessageType::Kick:  kicks.append(msg.nick);                          break;
            default: break;
            }
        }
        for (qsizetype i = joins.size() - 1; i >= 0; --i) {
            if (parts.contains(joins[i])) { parts.removeAll(joins[i]); joins.removeAt(i); }
        }

        const qsizetype total = joins.size() + parts.size() + nickChanges.size() + kicks.size();
        const int maxNicks = 10;
        int shown = 0;
        bool firstSec = true;

        auto addSection = [&](const QColor &col, const QString &sym, const QStringList &nicks) {
            if (nicks.isEmpty()) return;
            if (!firstSec) tb.append("  ", plainFmt);
            QTextCharFormat f;
            f.setForeground(col);
            QString text = sym + " ";
            for (const QString &n : nicks) {
                if (shown >= maxNicks) break;
                if (!text.endsWith(' ')) text += ' ';
                text += n;
                ++shown;
            }
            tb.append(text, f);
            firstSec = false;
        };

        addSection(QColor("seagreen"),  "→", joins);
        addSection(QColor("#e06b6b"),   "←", parts);

        if (!nickChanges.isEmpty()) {
            if (!firstSec) tb.append("  ", plainFmt);
            QTextCharFormat f;
            f.setForeground(QColor("steelblue"));
            QString text = "~ ";
            for (const auto &p : std::as_const(nickChanges)) {
                if (shown >= maxNicks) break;
                if (text.size() > 2) text += ' ';
                text += p.first + "→" + p.second;
                ++shown;
            }
            tb.append(text, f);
            firstSec = false;
        }

        addSection(QColor("#e06b6b"), "✕", kicks);

        const qsizetype overflow = total - shown;
        if (overflow > 0) {
            tb.append("  … " + QString::number(overflow) + " more", plainFmt);
        }
    }

    ChatLine line;
    line.text       = tb.text;
    line.segments   = tb.segs;
    line.id         = "evgrp:" + groupId;
    line.role       = ChatLineRole::EventGroup;
    line.hangIndent = false;
    return line;
}

ChatLine makeStatusLine(const QString &text)
{
    TextBuilder tb;
    QTextCharFormat f;
    f.setForeground(QColor("#888888"));
    tb.append(text, f);
    ChatLine line;
    line.text     = tb.text;
    line.segments = tb.segs;
    line.role     = ChatLineRole::StatusLine;
    return line;
}

} // namespace ChatRenderer
