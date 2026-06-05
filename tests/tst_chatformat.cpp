#include <QtTest/QtTest>
#include "ui/chatrenderer.h"

using namespace ChatRenderer;

class TstChatFormat : public QObject
{
    Q_OBJECT

private slots:
    void htmlAttrQuote();
    void htmlAttrEntities();
    void ircToHtmlPlain();
    void ircToHtmlEscaping();
    void ircToHtmlBold();
    void ircToHtmlItalic();
    void ircToHtmlUnderline();
    void ircToHtmlColorFg();
    void ircToHtmlColorFgBg();
    void ircToHtmlResetPlain();
    void ircToHtmlResetColor();
    void linkifyHttp();
    void linkifyHttps();
    void linkifyFtpSkipped();
    void linkifyNoUrl();
};

void TstChatFormat::htmlAttrQuote()
{
    QCOMPARE(htmlAttr(QStringLiteral("it's")), QStringLiteral("it&#39;s"));
}

void TstChatFormat::htmlAttrEntities()
{
    QCOMPARE(htmlAttr(QStringLiteral("<a & b>")), QStringLiteral("&lt;a &amp; b&gt;"));
}

void TstChatFormat::ircToHtmlPlain()
{
    QCOMPARE(ircToHtml(QStringLiteral("hello world")), QStringLiteral("hello world"));
    QVERIFY(ircToHtml(QString()).isEmpty());
}

void TstChatFormat::ircToHtmlEscaping()
{
    QCOMPARE(ircToHtml(QStringLiteral("<b>x</b>")), QStringLiteral("&lt;b&gt;x&lt;/b&gt;"));
    QCOMPARE(ircToHtml(QStringLiteral("a & b")),    QStringLiteral("a &amp; b"));
    QCOMPARE(ircToHtml(QStringLiteral("say \"hi\"")), QStringLiteral("say &quot;hi&quot;"));
}

void TstChatFormat::ircToHtmlBold()
{
    const QString r = ircToHtml(QStringLiteral("\x02""bold\x02""normal"));
    QVERIFY(r.contains(QStringLiteral("font-weight:bold;")));
    QVERIFY(r.contains(QStringLiteral("bold</span>")));
    QVERIFY(r.endsWith(QStringLiteral("normal")));
}

void TstChatFormat::ircToHtmlItalic()
{
    const QString r = ircToHtml(QStringLiteral("\x1D""italic\x1D"));
    QVERIFY(r.contains(QStringLiteral("font-style:italic;")));
    QVERIFY(r.contains(QStringLiteral("italic</span>")));
}

void TstChatFormat::ircToHtmlUnderline()
{
    const QString r = ircToHtml(QStringLiteral("\x1F""under\x1F"));
    QVERIFY(r.contains(QStringLiteral("text-decoration:underline;")));
}

void TstChatFormat::ircToHtmlColorFg()
{
    // IRC color 4 = #FF0000; bare \x03 followed by space resets
    const QString r = ircToHtml(QStringLiteral("\x03""4red\x03 more"));
    QVERIFY(r.contains(QStringLiteral("color:#FF0000;")));
    QVERIFY(r.contains(QStringLiteral("red</span>")));
    QVERIFY(r.endsWith(QStringLiteral(" more")));
}

void TstChatFormat::ircToHtmlColorFgBg()
{
    // fg=4 (#FF0000), bg=2 (#00007F)
    const QString r = ircToHtml(QStringLiteral("\x03""4,2text\x0F"));
    QVERIFY(r.contains(QStringLiteral("color:#FF0000;")));
    QVERIFY(r.contains(QStringLiteral("background-color:#00007F;")));
}

void TstChatFormat::ircToHtmlResetPlain()
{
    // \x0F resets bold; text after it must not be inside a span
    const QString r = ircToHtml(QStringLiteral("\x02""bold\x0F""normal"));
    const int pos = r.indexOf(QStringLiteral("normal"));
    QVERIFY(pos != -1);
    QVERIFY(!r.mid(pos).contains(QStringLiteral("<span")));
}

void TstChatFormat::ircToHtmlResetColor()
{
    // Color followed by \x0F; text after reset has no color span
    const QString r = ircToHtml(QStringLiteral("\x03""4red\x0F""plain"));
    QVERIFY(r.contains(QStringLiteral("color:#FF0000;")));
    const int pos = r.indexOf(QStringLiteral("plain"));
    QVERIFY(pos != -1);
    QVERIFY(!r.mid(pos).contains(QStringLiteral("<span")));
}

void TstChatFormat::linkifyHttp()
{
    const QString r = linkifyHtml(QStringLiteral("visit http://example.com today"));
    QVERIFY(r.contains(QStringLiteral("<a href=\"http://example.com\">")));
}

void TstChatFormat::linkifyHttps()
{
    const QString r = linkifyHtml(QStringLiteral("see https://example.com/path?q=1 for details"));
    QVERIFY(r.contains(QStringLiteral("href=\"https://example.com/path?q=1\"")));
}

void TstChatFormat::linkifyFtpSkipped()
{
    const QString r = linkifyHtml(QStringLiteral("ftp://example.com/file"));
    QVERIFY(!r.contains(QStringLiteral("<a ")));
}

void TstChatFormat::linkifyNoUrl()
{
    QCOMPARE(linkifyHtml(QStringLiteral("no url here")), QStringLiteral("no url here"));
}

QTEST_GUILESS_MAIN(TstChatFormat)
#include "tst_chatformat.moc"
