#include <QtTest/QtTest>
#include "irc/ircparser.h"

class TstIrcParser : public QObject
{
    Q_OBJECT

private slots:
    void basicPrivmsg();
    void serverPrefix();
    void noPrefix();
    void multiParam();
    void trailingOnly();
    void ircv3Tags();
    void tagUnescape();
    void serverTimeTag();
    void emptyLine();
    void crlfStripped();
    void numericCommand();
    void tagsThenNoPrefix();
    void malformedTagsOnly();
};

void TstIrcParser::basicPrivmsg()
{
    const auto m = IrcParser::parse(":nick!user@host PRIVMSG #chan :hello world");
    QVERIFY(m.isValid());
    QCOMPARE(m.command,   QStringLiteral("PRIVMSG"));
    QCOMPARE(m.nick,      QStringLiteral("nick"));
    QCOMPARE(m.user,      QStringLiteral("user"));
    QCOMPARE(m.host,      QStringLiteral("host"));
    QCOMPARE(m.params.size(), 2);
    QCOMPARE(m.params[0], QStringLiteral("#chan"));
    QCOMPARE(m.trailing,  QStringLiteral("hello world"));
}

void TstIrcParser::serverPrefix()
{
    const auto m = IrcParser::parse(":irc.example.org 001 mynick :Welcome to IRC");
    QVERIFY(m.isValid());
    QCOMPARE(m.command, QStringLiteral("001"));
    QCOMPARE(m.prefix,  QStringLiteral("irc.example.org"));
    QCOMPARE(m.nick,    QStringLiteral("irc.example.org"));
    QVERIFY(m.user.isEmpty());
    QVERIFY(m.host.isEmpty());
    QCOMPARE(m.params.size(), 2);
    QCOMPARE(m.params[0], QStringLiteral("mynick"));
    QCOMPARE(m.trailing,  QStringLiteral("Welcome to IRC"));
}

void TstIrcParser::noPrefix()
{
    const auto m = IrcParser::parse("PING :token123");
    QVERIFY(m.isValid());
    QCOMPARE(m.command, QStringLiteral("PING"));
    QVERIFY(m.prefix.isEmpty());
    QCOMPARE(m.params.size(), 1);
    QCOMPARE(m.trailing, QStringLiteral("token123"));
}

void TstIrcParser::multiParam()
{
    const auto m = IrcParser::parse(":nick!u@h MODE #chan +o targetuser");
    QVERIFY(m.isValid());
    QCOMPARE(m.command, QStringLiteral("MODE"));
    QCOMPARE(m.params.size(), 3);
    QCOMPARE(m.params[0], QStringLiteral("#chan"));
    QCOMPARE(m.params[1], QStringLiteral("+o"));
    QCOMPARE(m.params[2], QStringLiteral("targetuser"));
    QVERIFY(m.trailing.isEmpty());
}

void TstIrcParser::trailingOnly()
{
    // Command with no mid-params, only trailing
    const auto m = IrcParser::parse(":nick!u@h JOIN :#uplink");
    QVERIFY(m.isValid());
    QCOMPARE(m.command, QStringLiteral("JOIN"));
    QCOMPARE(m.params.size(), 1);
    QCOMPARE(m.trailing, QStringLiteral("#uplink"));
}

void TstIrcParser::ircv3Tags()
{
    const auto m = IrcParser::parse("@account=alice;msgid=abc123 :nick!u@h PRIVMSG #c :hi");
    QVERIFY(m.isValid());
    QCOMPARE(m.tags.value(QStringLiteral("account")), QStringLiteral("alice"));
    QCOMPARE(m.tags.value(QStringLiteral("msgid")),   QStringLiteral("abc123"));
    QCOMPARE(m.command,  QStringLiteral("PRIVMSG"));
    QCOMPARE(m.trailing, QStringLiteral("hi"));
}

void TstIrcParser::tagUnescape()
{
    // Tag value: a\:b\sc\\d\re\nf\z
    // Expected:  a ; b   c \ d CR e LF f \ z  (unknown \z preserves backslash per current behaviour)
    const auto m = IrcParser::parse("@k=a\\:b\\sc\\\\d\\re\\nf\\z :s PING");
    QVERIFY(m.isValid());
    const QString expected = QStringLiteral("a;b c\\d\re\nf\\z");
    QCOMPARE(m.tags.value(QStringLiteral("k")), expected);
}

void TstIrcParser::serverTimeTag()
{
    const auto m = IrcParser::parse("@time=2026-01-15T10:30:00.000Z :s PING");
    QVERIFY(m.isValid());
    QVERIFY(m.serverTime.isValid());
    QCOMPARE(m.serverTime.date(),         QDate(2026, 1, 15));
    QCOMPARE(m.serverTime.time().hour(),   10);
    QCOMPARE(m.serverTime.time().minute(), 30);
}

void TstIrcParser::emptyLine()
{
    QVERIFY(!IrcParser::parse(QString()).isValid());
    QVERIFY(!IrcParser::parse(QStringLiteral("\r\n")).isValid());
}

void TstIrcParser::crlfStripped()
{
    const auto m = IrcParser::parse(":nick!u@h PRIVMSG #c :text\r\n");
    QVERIFY(m.isValid());
    QCOMPARE(m.trailing, QStringLiteral("text"));
}

void TstIrcParser::numericCommand()
{
    const auto m = IrcParser::parse(":server 332 nick #chan :the channel topic");
    QVERIFY(m.isValid());
    QCOMPARE(m.command,   QStringLiteral("332"));
    QCOMPARE(m.params.size(), 3);
    QCOMPARE(m.params[0], QStringLiteral("nick"));
    QCOMPARE(m.params[1], QStringLiteral("#chan"));
    QCOMPARE(m.trailing,  QStringLiteral("the channel topic"));
}

void TstIrcParser::tagsThenNoPrefix()
{
    const auto m = IrcParser::parse("@msgid=xyz PING :token");
    QVERIFY(m.isValid());
    QCOMPARE(m.tags.value(QStringLiteral("msgid")), QStringLiteral("xyz"));
    QCOMPARE(m.command,  QStringLiteral("PING"));
    QCOMPARE(m.trailing, QStringLiteral("token"));
    QVERIFY(m.prefix.isEmpty());
}

void TstIrcParser::malformedTagsOnly()
{
    // Tags with no space → no command → invalid
    QVERIFY(!IrcParser::parse(QStringLiteral("@key=val")).isValid());
}

QTEST_GUILESS_MAIN(TstIrcParser)
#include "tst_ircparser.moc"
