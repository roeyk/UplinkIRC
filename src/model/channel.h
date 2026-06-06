#pragma once

#include "message.h"
#include <QDateTime>
#include <QString>
#include <QHash>
#include <QList>
#include <QSet>
#include <QStringList>

static constexpr int kMessageBufferCap = 500;

// Nick prefix rank: ~ & @ % + (owner → admin → op → halfop → voice)
inline int prefixRank(QChar p)
{
    switch (p.unicode()) {
    case '~': return 5;
    case '&': return 4;
    case '@': return 3;
    case '%': return 2;
    case '+': return 1;
    default:  return 0;
    }
}

struct NickEntry {
    QString     nick;
    QString     account;   // NickServ account, empty if unknown or logged out
    QChar       prefix{' '};
    QSet<QChar> prefixes;

    void recomputePrefix()
    {
        prefix = ' ';
        for (QChar p : std::as_const(prefixes))
            if (prefixRank(p) > prefixRank(prefix))
                prefix = p;
    }

    QString display() const
    {
        return (prefix != ' ' && prefix != '\0') ? QString(prefix) + nick : nick;
    }

    bool operator<(const NickEntry &o) const
    {
        int ra = prefixRank(prefix), rb = prefixRank(o.prefix);
        if (ra != rb) return ra > rb;
        return nick.toLower() < o.nick.toLower();
    }
};

struct Channel {
    QString          name;
    QString          topic;
    QString          modes;
    QList<NickEntry>          nicks;
    QHash<QString, qsizetype> nickIndex; // lowercase nick → index in nicks
    QList<Message>   messages;
    QSet<QString>    botNicks;  // lowercased nicks with +B channel user mode
    QHash<QString, QString> previews;  // url → persisted card HTML
    QSet<QString>           hiddenPreviews; // urls the user manually hid
    // msgid → emoji → set of nicks who reacted (QSet deduplicates per nick)
    QHash<QString, QHash<QString, QSet<QString>>> reactions;
    int              unread{0};
    int              mentions{0};
    bool             joined{false};
    QDateTime        lastRead;  // soju.im/read marker

    void addMessage(const Message &msg)
    {
        messages.append(msg);
        while (messages.size() > kMessageBufferCap) {
            const QString oldId = messages.front().msgid;
            messages.removeFirst();
            if (!oldId.isEmpty())
                reactions.remove(oldId);
        }
    }

    void rebuildNickIndex()
    {
        nickIndex.clear();
        nickIndex.reserve(nicks.size());
        for (qsizetype i = 0; i < nicks.size(); ++i)
            nickIndex[nicks[i].nick.toLower()] = i;
    }

    void setNicks(const QStringList &raw)
    {
        nicks.clear();
        for (const QString &n : raw) {
            if (n.isEmpty()) continue;
            NickEntry e;
            int i = 0;
            while (i < n.size() && prefixRank(n[i]) > 0) {
                e.prefixes.insert(n[i]);
                ++i;
            }
            e.recomputePrefix();
            e.nick = n.mid(i);
            // userhost-in-names: strip !user@host suffix if present
            const qsizetype bang = e.nick.indexOf('!');
            if (bang != -1)
                e.nick = e.nick.left(bang);
            nicks.append(e);
        }
        std::sort(nicks.begin(), nicks.end());
        rebuildNickIndex();
    }

    void setNickAccount(const QString &nick, const QString &account)
    {
        const auto it = nickIndex.constFind(nick.toLower());
        if (it == nickIndex.constEnd()) return;
        auto &e = nicks[it.value()];
        e.account = (account == "*") ? QString() : account;
    }

    void addNick(const QString &raw)
    {
        NickEntry e;
        int i = 0;
        while (i < raw.size() && prefixRank(raw[i]) > 0) {
            e.prefixes.insert(raw[i]);
            ++i;
        }
        e.recomputePrefix();
        e.nick = raw.mid(i);
        if (nickIndex.contains(e.nick.toLower())) return;
        const qsizetype idx = std::lower_bound(nicks.cbegin(), nicks.cend(), e) - nicks.cbegin();
        nicks.insert(idx, e);
        for (auto &v : nickIndex)
            if (v >= idx) ++v;
        nickIndex[e.nick.toLower()] = idx;
    }

    void removeNick(const QString &nick)
    {
        const auto it = nickIndex.find(nick.toLower());
        if (it == nickIndex.end()) return;
        const qsizetype idx = it.value();
        nickIndex.erase(it);
        nicks.removeAt(idx);
        for (auto &v : nickIndex)
            if (v > idx) --v;
    }

    void renameNick(const QString &oldNick, const QString &newNick)
    {
        const auto it = nickIndex.constFind(oldNick.toLower());
        if (it == nickIndex.constEnd()) return;
        nicks[it.value()].nick = newNick;
        std::sort(nicks.begin(), nicks.end());
        rebuildNickIndex();
    }

    static constexpr int kPreviewCap     = 20;
    static constexpr int kPreviewHtmlCap = 8192;

    void addPreview(const QString &url, const QString &html)
    {
        if (previews.size() >= kPreviewCap) {
            hiddenPreviews.remove(previews.begin().key());
            previews.erase(previews.begin());
        }
        previews.insert(url, html.left(kPreviewHtmlCap));
    }
};
