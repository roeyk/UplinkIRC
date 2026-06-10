#include "chatrenderer.h"
#include "model/channel.h"
#include <QDate>
#include <QDateTime>
#include <QRegularExpression>

namespace ChatRenderer {

QString htmlAttr(const QString &s)
{
    QString out = s.toHtmlEscaped();
    out.replace(QLatin1Char('\''), QLatin1String("&#39;"));
    return out;
}

QString linkifyTopic(const QString &text)
{
    static const QRegularExpression urlRe(
        R"((https?://[^\s<>"]+))",
        QRegularExpression::CaseInsensitiveOption);
    QString result = text.toHtmlEscaped();
    result.replace(urlRe, R"(<a href="\1">\1</a>)");
    return result;
}

QString linkifyHtml(const QString &html)
{
    static const QRegularExpression urlRe(
        R"((https?://[^\s<>"]+))",
        QRegularExpression::CaseInsensitiveOption);
    QString result = html;
    result.replace(urlRe, R"(<a href="\1">\1</a>)");
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

    auto wrap = [](const QString &color, const QString &text) {
        return QString("<span style='color:%1'>%2</span>").arg(color, text.toHtmlEscaped());
    };
    const int eventPt = qMax(7, qRound(ctx.chatPt * 0.82));
    auto wrapEvent = [&eventPt](const QString &color, const QString &text) {
        return QString("<span style='color:%1; font-size:%2pt'>%3</span>")
            .arg(color, QString::number(eventPt), text.toHtmlEscaped());
    };

    QString html;
    const QString tsSpan = msg.msgid.isEmpty()
        ? QString("<span style='color:gray'>%1</span>").arg(ts)
        : QString("<a href='msgid:%1' style='color:gray;text-decoration:none'>%2</a>")
            .arg(htmlAttr(msg.msgid), ts);

    if (msg.redacted) {
        html = tsSpan + " <span style='color:gray;font-style:italic'>[message deleted]</span>";
        if (msg.isHistory)
            html = "<span style='opacity:0.55'>" + html + "</span>";
        return html;
    }

    switch (msg.type) {
    case MessageType::Privmsg: {
        const QString color = ctx.coloredNicks
            ? nickColor(msg.nick).name()
            : (ctx.validTheme ? ctx.themeText : QStringLiteral("#cccccc"));
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
        if (ctx.selfNickRe.isValid())
            textHtml.replace(ctx.selfNickRe, "<span style='color:red;font-weight:bold'>\\1</span>");
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
        html = QString("%1 <i>* %2 %3</i>")
            .arg(tsSpan, actionNick, wrapEmojiHtml(linkifyHtml(ircToHtml(msg.text)), ctx.emojiPt));
        break;
    }
    case MessageType::Notice: {
        const QString noticeNick = QString("<a href='nick:%1' style='color:inherit; text-decoration:none'>%1</a>")
            .arg(htmlAttr(msg.nick));
        html = QString("%1 <span style='color:#cc8800'>-%2- %3</span>")
            .arg(tsSpan, noticeNick, wrapEmojiHtml(linkifyHtml(ircToHtml(msg.text)), ctx.emojiPt));
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

    if (msg.isHistory)
        html = "<span style='opacity:0.55'>" + html + "</span>";

    return html;
}

QString formatEventGroup(const QList<Message> &msgs, const Context &ctx)
{
    if (msgs.isEmpty()) return {};

    const QDateTime local = msgs.front().timestamp.toLocalTime();
    const bool sameDay = local.date() == QDate::currentDate();
    const QString ts = sameDay ? local.toString("hh:mm") : local.toString("MM/dd hh:mm");
    const QString tsSpan = QString("<span style='color:gray'>%1</span>").arg(ts);

    const int eventPt = qMax(7, qRound(ctx.chatPt * 0.82));

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

    return QString("<span style='font-size:%1pt'>%2  %3</span>")
        .arg(eventPt).arg(tsSpan, body);
}

} // namespace ChatRenderer
