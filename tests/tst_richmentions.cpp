#include <QtTest/QtTest>

#include "search/richsearch.h"

#include <QTimeZone>

class TstRichMentions : public QObject
{
    Q_OBJECT

private slots:
    void rendersMentionsAcrossLoadedChannels();
    void mentionsAcceptsSearchFilters();
    void mentionsReportsFilterErrors();
    void mentionsDefaultScopeIsLocal();
    void mentionsGlobalScopePrefixesNetworkChannel();
};

// Creates a single loaded IRC message for mention-rendering tests.
//
// Test cases call this helper when constructing synthetic channel history.
// `nick` is the sender to evaluate against origin filters, `text` is the body
// scanned by mention/text selectors, and `minute` keeps timestamps distinct.
// The returned Message has no side effects; it is inserted into test sessions.
static Message mentionMessage(const QString &nick, const QString &text, int minute)
{
    return Message::make(
        MessageType::Privmsg,
        nick,
        text,
        QDateTime(QDate(2026, 6, 17), QTime(12, minute), QTimeZone::UTC));
}

// Removes generated match-highlighting controls for tests that assert the
// underlying display text. Highlight-specific tests inspect the raw string.
static QString withoutHighlightControls(QString line)
{
    line.replace(QStringLiteral("\x03""01,08"), QString());
    line.replace(QChar(u'\x03'), QString());
    return line;
}

// Builds one in-memory server session containing one mention line.
//
// Mention tests use this to create local and global scope fixtures without IRC
// traffic. `name` and `host` identify the network, `channel` identifies the
// loaded buffer, and `sender` identifies who mentioned the current user. The
// returned session is consumed by RichSearch::renderMentions.
static ServerSession sessionWithMention(
    const QString &name,
    const QString &host,
    const QString &channel,
    const QString &sender)
{
    ServerSession session;
    session.name = name;
    session.host = host;
    session.nick = QStringLiteral("roey");
    session.getOrCreate(channel).messages << mentionMessage(
        sender,
        QStringLiteral("roey: check this"),
        0);
    return session;
}

// Verifies that local mention rendering scans multiple loaded channels on the
// active network and includes message/notice mention lines.
void TstRichMentions::rendersMentionsAcrossLoadedChannels()
{
    ServerSession session = sessionWithMention(
        QStringLiteral("Libera"),
        QStringLiteral("irc.libera.chat"),
        QStringLiteral("#one"),
        QStringLiteral("alice"));
    session.getOrCreate(QStringLiteral("#two")).messages << Message::make(
        MessageType::Notice,
        QStringLiteral("bob"),
        QStringLiteral("ping roey"),
        QDateTime(QDate(2026, 6, 17), QTime(12, 2), QTimeZone::UTC));

    const QStringList lines = RichSearch::renderMentions(
        QList<ServerSession>{session},
        ServerId{QStringLiteral("irc.libera.chat")},
        QStringLiteral("roey"),
        QString(),
        QDateTime(QDate(2026, 6, 17), QTime(12, 3), QTimeZone::UTC));

    QCOMPARE(lines.size(), 2);
    QVERIFY(lines[0].contains(QStringLiteral("#one")));
    QVERIFY(lines[1].contains(QStringLiteral("#two")));
}

// Verifies that /mentions accepts the shared /search filters and preserves the
// legacy local output shape when no global scope is requested.
void TstRichMentions::mentionsAcceptsSearchFilters()
{
    ServerSession session = sessionWithMention(
        QStringLiteral("Libera"),
        QStringLiteral("irc.libera.chat"),
        QStringLiteral("#one"),
        QStringLiteral("alice"));
    session.getOrCreate(QStringLiteral("#one")).messages << mentionMessage(
        QStringLiteral("bob"),
        QStringLiteral("roey: unrelated"),
        1);

    const QStringList lines = RichSearch::renderMentions(
        QList<ServerSession>{session},
        ServerId{QStringLiteral("irc.libera.chat")},
        QStringLiteral("roey"),
        QStringLiteral("--notimestamp origin=alice text=check"),
        QDateTime(QDate(2026, 6, 17), QTime(12, 2), QTimeZone::UTC));

    QCOMPARE(lines.size(), 1);
    QCOMPARE(withoutHighlightControls(lines.first()),
             QStringLiteral("#one  <alice> roey: check this"));
    QVERIFY(lines.first().contains(QStringLiteral("\x03""01,08roey\x03")));
    QVERIFY(lines.first().contains(QStringLiteral("\x03""01,08check\x03")));
}

// Verifies that invalid user-supplied search syntax is surfaced as an error
// instead of being ignored or widened into a broader mention scan.
void TstRichMentions::mentionsReportsFilterErrors()
{
    ServerSession session;
    session.host = QStringLiteral("irc.libera.chat");
    session.nick = QStringLiteral("roey");

    const auto result = RichSearch::renderMentionResult(
        QList<ServerSession>{session},
        ServerId{QStringLiteral("irc.libera.chat")},
        QStringLiteral("roey"), QStringLiteral("text="));

    QVERIFY(!result.ok);
    QCOMPARE(result.error, QStringLiteral("expected selector value"));
}

// Verifies that /mentions defaults to local scope: only the active network's
// loaded channels are searched unless the user explicitly asks for global.
void TstRichMentions::mentionsDefaultScopeIsLocal()
{
    const ServerSession libera = sessionWithMention(
        QStringLiteral("Libera"), QStringLiteral("irc.libera.chat"),
        QStringLiteral("#one"), QStringLiteral("alice"));
    const ServerSession oftc = sessionWithMention(
        QStringLiteral("OFTC"), QStringLiteral("irc.oftc.net"),
        QStringLiteral("#two"), QStringLiteral("bob"));

    const QStringList lines = RichSearch::renderMentions(
        QList<ServerSession>{libera, oftc},
        ServerId{QStringLiteral("irc.libera.chat")},
        QStringLiteral("roey"), QStringLiteral("--notimestamp"),
        QDateTime(QDate(2026, 6, 17), QTime(12, 3), QTimeZone::UTC));

    QCOMPARE(lines.size(), 1);
    QCOMPARE(withoutHighlightControls(lines.first()),
             QStringLiteral("#one  <alice> roey: check this"));
}

// Verifies that global scope searches all loaded networks and prefixes each
// result with network/channel so similarly named channels remain clear.
void TstRichMentions::mentionsGlobalScopePrefixesNetworkChannel()
{
    const ServerSession libera = sessionWithMention(
        QStringLiteral("Libera"), QStringLiteral("irc.libera.chat"),
        QStringLiteral("#one"), QStringLiteral("alice"));
    const ServerSession oftc = sessionWithMention(
        QStringLiteral("OFTC"), QStringLiteral("irc.oftc.net"),
        QStringLiteral("#two"), QStringLiteral("bob"));

    const QStringList lines = RichSearch::renderMentions(
        QList<ServerSession>{libera, oftc},
        ServerId{QStringLiteral("irc.libera.chat")},
        QStringLiteral("roey"), QStringLiteral("scope=global --notimestamp"),
        QDateTime(QDate(2026, 6, 17), QTime(12, 3), QTimeZone::UTC));

    QStringList plainLines;
    for (const QString &line : lines)
        plainLines << withoutHighlightControls(line);

    QVERIFY(plainLines.contains(QStringLiteral("Libera/#one: <alice> roey: check this")));
    QVERIFY(plainLines.contains(QStringLiteral("OFTC/#two: <bob> roey: check this")));
}

QTEST_MAIN(TstRichMentions)
#include "tst_richmentions.moc"
