#include <QtTest/QtTest>

#include "search/richsearch.h"

#include <QTimeZone>

class TstRichSearch : public QObject
{
    Q_OBJECT

private slots:
    void parsesSelectors();
    void rejectsEmptySelectorValue();
    void filtersMessages();
    void parsesBooleanExpressions();
    void treatsQuotedSelectorAsText();
    void parsesStringModifiers();
    void originTracksLoadedNickChanges();
    void originTracksKnownAccounts();
    void supportsContextAndOther();
    void rendersCommonChannels();
    void rendersCommonChannelsAcrossNetworks();
    void completesSearchArguments();
    void completesCommonArguments();
    void showsArgumentHelp();
};

static Message msg(const QString &nick, const QString &text, int minutesAgo = 0)
{
    return Message::make(
        MessageType::Privmsg,
        nick,
        text,
        QDateTime(QDate(2026, 6, 17), QTime(12, 0), QTimeZone::UTC).addSecs(-minutesAgo * 60));
}

static QString withoutHighlightControls(QString line)
{
    line.replace(QStringLiteral("\x03""01,08"), QString());
    line.replace(QChar(u'\x03'), QString());
    return line;
}

void TstRichSearch::parsesSelectors()
{
    const auto parsed = RichSearch::parse(QStringLiteral("--textonly origin=alice itext=Deploy span=2h view=tab"));
    QVERIFY(parsed.ok);
    QCOMPARE(parsed.command.options.textOnly, true);
    QCOMPARE(parsed.command.options.spanSeconds, qint64(7200));
    QCOMPARE(parsed.command.options.view, RichSearch::ResultView::Tab);
    QCOMPARE(parsed.command.predicates.size(), 2);

    const auto nicknameParsed = RichSearch::parse(QStringLiteral("nickname=alice"));
    QVERIFY(nicknameParsed.ok);
    QCOMPARE(nicknameParsed.command.predicates.first().field, RichSearch::Field::Origin);
}

void TstRichSearch::rejectsEmptySelectorValue()
{
    const auto parsed = RichSearch::parse(QStringLiteral("text="));
    QVERIFY(!parsed.ok);
    QCOMPARE(parsed.error, QStringLiteral("expected selector value"));
}

void TstRichSearch::filtersMessages()
{
    Channel channel;
    channel.messages << msg(QStringLiteral("alice"), QStringLiteral("deploy finished"));
    channel.messages << msg(QStringLiteral("bob"), QStringLiteral("nothing here"));

    const auto parsed = RichSearch::parse(QStringLiteral("origin=alice text=deploy"));
    QVERIFY(parsed.ok);

    const QStringList lines = RichSearch::renderMatches(
        channel,
        parsed.command,
        QStringLiteral("roey"),
        QDateTime(QDate(2026, 6, 17), QTime(12, 0), QTimeZone::UTC));

    QCOMPARE(lines.size(), 1);
    QVERIFY(withoutHighlightControls(lines.first()).contains(QStringLiteral("<alice> deploy finished")));
    QVERIFY(lines.first().contains(QStringLiteral("\x03""01,08deploy\x03")));
}

void TstRichSearch::parsesBooleanExpressions()
{
    Channel channel;
    channel.messages << msg(QStringLiteral("alice"), QStringLiteral("deploy failed"));
    channel.messages << msg(QStringLiteral("bob"), QStringLiteral("deploy failed"));
    channel.messages << msg(QStringLiteral("carol"), QStringLiteral("test passed"));

    const auto parsed = RichSearch::parse(QStringLiteral("(origin=alice OR origin=bob) AND NOT text=passed"));
    QVERIFY(parsed.ok);

    const QStringList lines = RichSearch::renderMatches(
        channel,
        parsed.command,
        QStringLiteral("roey"),
        QDateTime(QDate(2026, 6, 17), QTime(12, 0), QTimeZone::UTC));

    QCOMPARE(lines.size(), 2);
    QVERIFY(lines[0].contains(QStringLiteral("<alice> deploy failed")));
    QVERIFY(lines[1].contains(QStringLiteral("<bob> deploy failed")));
}

void TstRichSearch::treatsQuotedSelectorAsText()
{
    Channel channel;
    channel.messages << msg(QStringLiteral("alice"), QStringLiteral("origin=bob"));
    channel.messages << msg(QStringLiteral("bob"), QStringLiteral("plain"));

    const auto parsed = RichSearch::parse(QStringLiteral("\"origin=bob\""));
    QVERIFY(parsed.ok);

    const QStringList lines = RichSearch::renderMatches(
        channel,
        parsed.command,
        QStringLiteral("roey"),
        QDateTime(QDate(2026, 6, 17), QTime(12, 0), QTimeZone::UTC));

    QCOMPARE(lines.size(), 1);
    QVERIFY(withoutHighlightControls(lines.first()).contains(QStringLiteral("<alice> origin=bob")));
}

void TstRichSearch::parsesStringModifiers()
{
    Channel channel;
    channel.messages << msg(QStringLiteral("alice"), QStringLiteral("Deploy failed"));
    channel.messages << msg(QStringLiteral("bob"), QStringLiteral("deploy passed"));

    const auto parsed = RichSearch::parse(QStringLiteral("text=i\"deploy\" NOT text=x\"pass(ed)?\""));
    QVERIFY2(parsed.ok, qPrintable(parsed.error));

    const QStringList lines = RichSearch::renderMatches(
        channel,
        parsed.command,
        QStringLiteral("roey"),
        QDateTime(QDate(2026, 6, 17), QTime(12, 0), QTimeZone::UTC));

    QCOMPARE(lines.size(), 1);
    QVERIFY(withoutHighlightControls(lines.first()).contains(QStringLiteral("<alice> Deploy failed")));
}

void TstRichSearch::originTracksLoadedNickChanges()
{
    Channel channel;
    channel.messages << msg(QStringLiteral("alice"), QStringLiteral("before nick change"));
    channel.messages << Message::make(
        MessageType::Nick,
        QStringLiteral("alice"),
        QStringLiteral("alice is now known as alice_"),
        QDateTime(QDate(2026, 6, 17), QTime(12, 1), QTimeZone::UTC),
        false,
        QString(),
        QStringLiteral("alice_"));
    channel.messages << msg(QStringLiteral("alice_"), QStringLiteral("after nick change"));

    const auto parsed = RichSearch::parse(QStringLiteral("type=message origin=alice"));
    QVERIFY(parsed.ok);

    const QStringList lines = RichSearch::renderMatches(
        channel,
        parsed.command,
        QStringLiteral("roey"),
        QDateTime(QDate(2026, 6, 17), QTime(12, 2), QTimeZone::UTC));

    QCOMPARE(lines.size(), 2);
    QVERIFY(lines[0].contains(QStringLiteral("<alice> before nick change")));
    QVERIFY(lines[1].contains(QStringLiteral("<alice_> after nick change")));
}

void TstRichSearch::originTracksKnownAccounts()
{
    Channel channel;
    channel.messages << Message::make(
        MessageType::Privmsg,
        QStringLiteral("alice"),
        QStringLiteral("first account message"),
        QDateTime(QDate(2026, 6, 17), QTime(12, 0), QTimeZone::UTC),
        false,
        QString(),
        QString(),
        QStringLiteral("alice-account"));
    channel.messages << Message::make(
        MessageType::Privmsg,
        QStringLiteral("alice_"),
        QStringLiteral("second account message"),
        QDateTime(QDate(2026, 6, 17), QTime(12, 1), QTimeZone::UTC),
        false,
        QString(),
        QString(),
        QStringLiteral("alice-account"));

    const auto parsed = RichSearch::parse(QStringLiteral("origin=alice"));
    QVERIFY(parsed.ok);

    const QStringList lines = RichSearch::renderMatches(
        channel,
        parsed.command,
        QStringLiteral("roey"),
        QDateTime(QDate(2026, 6, 17), QTime(12, 2), QTimeZone::UTC));

    QCOMPARE(lines.size(), 2);
    QVERIFY(lines[0].contains(QStringLiteral("<alice> first account message")));
    QVERIFY(lines[1].contains(QStringLiteral("<alice_> second account message")));
}

void TstRichSearch::supportsContextAndOther()
{
    Channel channel;
    channel.messages << msg(QStringLiteral("roey"), QStringLiteral("before"));
    channel.messages << msg(QStringLiteral("alice"), QStringLiteral("needle"));
    channel.messages << msg(QStringLiteral("roey"), QStringLiteral("after"));

    const auto parsed = RichSearch::parse(QStringLiteral("--other context=1 text=needle"));
    QVERIFY(parsed.ok);

    const QStringList lines = RichSearch::renderMatches(
        channel,
        parsed.command,
        QStringLiteral("roey"),
        QDateTime(QDate(2026, 6, 17), QTime(12, 0), QTimeZone::UTC));

    QCOMPARE(lines.size(), 3);
    QVERIFY(withoutHighlightControls(lines[1]).contains(QStringLiteral("<alice> needle")));
}

void TstRichSearch::rendersCommonChannels()
{
    ServerSession session;
    session.name = QStringLiteral("Libera");
    session.host = QStringLiteral("irc.libera.chat");
    session.nick = QStringLiteral("roey");
    session.getOrCreate(QStringLiteral("#here")).setNicks({
        QStringLiteral("roey"),
        QStringLiteral("alice"),
        QStringLiteral("bob"),
    });
    session.getOrCreate(QStringLiteral("#rust")).setNicks({
        QStringLiteral("alice"),
        QStringLiteral("carol"),
    });

    const QStringList lines = RichSearch::renderCommon(
        QList<ServerSession>{session},
        ServerId{QStringLiteral("irc.libera.chat")},
        BufferId{QStringLiteral("#here")},
        RichSearch::CommonScope::Network);

    QCOMPARE(lines, QStringList({
        QStringLiteral("alice: #rust"),
        QStringLiteral("exclusive to #here: bob"),
    }));
}

void TstRichSearch::rendersCommonChannelsAcrossNetworks()
{
    ServerSession libera;
    libera.name = QStringLiteral("Libera");
    libera.host = QStringLiteral("irc.libera.chat");
    libera.nick = QStringLiteral("roey");
    libera.getOrCreate(QStringLiteral("#here")).setNicks({
        QStringLiteral("roey"),
        QStringLiteral("alice"),
        QStringLiteral("bob"),
    });

    ServerSession oftc;
    oftc.name = QStringLiteral("OFTC");
    oftc.host = QStringLiteral("irc.oftc.net");
    oftc.nick = QStringLiteral("roey");
    oftc.getOrCreate(QStringLiteral("#debian")).setNicks({
        QStringLiteral("alice"),
    });

    const QStringList lines = RichSearch::renderCommon(
        QList<ServerSession>{libera, oftc},
        ServerId{QStringLiteral("irc.libera.chat")},
        BufferId{QStringLiteral("#here")},
        RichSearch::CommonScope::Global);

    QVERIFY(lines.contains(QStringLiteral("OFTC/alice: #debian")));
    QVERIFY(lines.contains(QStringLiteral("exclusive to #here: bob")));
}

void TstRichSearch::completesSearchArguments()
{
    int wordStart = -1;
    const QString input = QStringLiteral("/search spa");
    const QStringList matches = RichSearch::completeArgument(
        input,
        input.size(),
        &wordStart);

    QCOMPARE(wordStart, 8);
    QVERIFY(matches.contains(QStringLiteral("span=")));
    QVERIFY(matches.contains(QStringLiteral("span=2d")));

    wordStart = -1;
    const QString emptyInput = QStringLiteral("/last ");
    const QStringList emptyMatches = RichSearch::completeArgument(
        emptyInput,
        emptyInput.size(),
        &wordStart);

    QCOMPARE(wordStart, 6);
    QVERIFY(emptyMatches.contains(QStringLiteral("text=")));
    QVERIFY(emptyMatches.contains(QStringLiteral("nickname=")));
    QVERIFY(emptyMatches.contains(QStringLiteral("--notimestamp")));

    wordStart = -1;
    const QString viewKeyInput = QStringLiteral("/search vi");
    const QStringList viewKeyMatches = RichSearch::completeArgument(
        viewKeyInput,
        viewKeyInput.size(),
        &wordStart);

    QCOMPARE(wordStart, 8);
    QCOMPARE(viewKeyMatches, QStringList{QStringLiteral("view=")});

    wordStart = -1;
    const QString viewValueInput = QStringLiteral("/search view=p");
    const QStringList viewValueMatches = RichSearch::completeArgument(
        viewValueInput,
        viewValueInput.size(),
        &wordStart);

    QCOMPARE(wordStart, 8);
    QCOMPARE(viewValueMatches, QStringList{QStringLiteral("view=pane")});
    QVERIFY(RichSearch::completeArgument(
        QStringLiteral("/search view="),
        QStringLiteral("/search view=").size(),
        &wordStart).isEmpty());
    QCOMPARE(RichSearch::completionChoices(
        QStringLiteral("/search view="),
        QStringLiteral("/search view=").size()),
        (QStringList{QStringLiteral("inline"), QStringLiteral("pane"), QStringLiteral("tab")}));
}

void TstRichSearch::completesCommonArguments()
{
    int wordStart = -1;
    const QString input = QStringLiteral("/common scope=n");
    const QStringList matches = RichSearch::completeArgument(
        input,
        input.size(),
        &wordStart);

    QCOMPARE(wordStart, 8);
    QCOMPARE(matches, QStringList{QStringLiteral("scope=network")});
}

void TstRichSearch::showsArgumentHelp()
{
    const QString searchHelp = RichSearch::argumentHelp(QStringLiteral("/search "));
    QVERIFY(searchHelp.contains(QStringLiteral("text=")));
    QVERIFY(searchHelp.contains(QStringLiteral("span=Nd|Nh|Nm")));

    const QString commonHelp = RichSearch::argumentHelp(QStringLiteral("/common "));
    QCOMPARE(commonHelp, QStringLiteral("/common scope=global|network"));

    QVERIFY(RichSearch::argumentHelp(QStringLiteral("/join #halloy")).isEmpty());
}

QTEST_MAIN(TstRichSearch)
#include "tst_richsearch.moc"
