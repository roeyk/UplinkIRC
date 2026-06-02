#include "channelpane.h"

#include <QTextBrowser>
#include <QListWidget>
#include <QLineEdit>
#include <QLabel>
#include <QToolButton>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QFont>
#include <QSizePolicy>

ChannelPane::ChannelPane(const QString &host, const QString &channel, QWidget *parent)
    : QWidget(parent), m_host(host), m_channel(channel)
{
    auto *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(0);

    // Header: topic toggle + channel name + close button
    auto *header = new QWidget;
    header->setObjectName("paneHeader");
    auto *hbox = new QHBoxLayout(header);
    hbox->setContentsMargins(6, 3, 4, 3);
    hbox->setSpacing(6);

    m_topicToggle = new QToolButton;
    m_topicToggle->setText(QStringLiteral("▸  topic"));
    m_topicToggle->setAutoRaise(false);
    m_topicToggle->setCheckable(true);
    m_topicToggle->setStyleSheet(
        "QToolButton {"
        "  border: 1px solid palette(mid);"
        "  border-radius: 9px;"
        "  padding: 2px 10px;"
        "}"
        "QToolButton:checked {"
        "  border-color: palette(highlight);"
        "}"
    );
    {
        QFont f = m_topicToggle->font();
        f.setPointSize(13);
        m_topicToggle->setFont(f);
    }

    auto *nameLabel = new QLabel(channel);
    nameLabel->setObjectName("paneChannelLabel");
    QFont f = nameLabel->font();
    f.setBold(true);
    nameLabel->setFont(f);

    auto *closeBtn = new QToolButton;
    closeBtn->setText(QStringLiteral("✕"));
    closeBtn->setFixedSize(18, 18);
    closeBtn->setAutoRaise(true);
    connect(closeBtn, &QToolButton::clicked, this, &ChannelPane::closeRequested);

    hbox->addWidget(m_topicToggle);
    hbox->addWidget(nameLabel, 1);
    hbox->addWidget(closeBtn);
    vbox->addWidget(header);

    // Topic bar — auto-sizes to content, scrollable if very long
    m_topicBar = new QWidget;
    m_topicBar->setObjectName("topicDisplay");
    m_topicBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    auto *tbhbox = new QHBoxLayout(m_topicBar);
    tbhbox->setContentsMargins(8, 4, 8, 4);
    m_topicText = new QLabel;
    m_topicText->setObjectName("topicText");
    m_topicText->setWordWrap(true);
    m_topicText->setTextFormat(Qt::RichText);
    m_topicText->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_topicText->setOpenExternalLinks(true);
    m_topicText->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    {
        QFont f = m_topicText->font();
        f.setPointSize(qMax(7, f.pointSize() - 1));
        m_topicText->setFont(f);
    }
    tbhbox->addWidget(m_topicText);
    m_topicBar->hide();
    vbox->addWidget(m_topicBar);

    connect(m_topicToggle, &QToolButton::toggled, this, [this](bool on){
        m_topicBar->setVisible(on);
        m_topicToggle->setText(on ? QStringLiteral("▾  topic") : QStringLiteral("▸  topic"));
    });

    // Chat view + nick list
    m_chatView = new QTextBrowser;
    m_chatView->setReadOnly(true);
    m_chatView->setLineWrapMode(QTextEdit::WidgetWidth);
    m_chatView->setOpenLinks(false);

    m_nickList = new QListWidget;
    m_nickList->setSpacing(0);

    auto *nickWrapper = new QWidget;
    nickWrapper->setObjectName("nickPanel");
    auto *nwvbox = new QVBoxLayout(nickWrapper);
    nwvbox->setContentsMargins(0, 0, 0, 0);
    nwvbox->setSpacing(0);
    nwvbox->addWidget(m_nickList, 100);
    nwvbox->addStretch(1);

    auto *bodySplitter = new QSplitter(Qt::Horizontal);
    bodySplitter->setHandleWidth(0);
    bodySplitter->addWidget(m_chatView);
    bodySplitter->addWidget(nickWrapper);
    bodySplitter->setStretchFactor(0, 1);
    bodySplitter->setStretchFactor(1, 0);
    bodySplitter->setSizes({999, 120});
    vbox->addWidget(bodySplitter, 1);

    // Input bar
    auto *inputBar = new QWidget;
    inputBar->setObjectName("inputBar");
    auto *ibox = new QHBoxLayout(inputBar);
    ibox->setContentsMargins(4, 3, 4, 8);
    ibox->setSpacing(4);
    m_nickPrefix = new QLabel;
    m_nickPrefix->setStyleSheet("font-weight: bold; padding-right: 4px;");
    m_input = new QLineEdit;
    m_input->setPlaceholderText("Type a message...");
    ibox->addWidget(m_nickPrefix);
    ibox->addWidget(m_input, 1);
    vbox->addWidget(inputBar);

    connect(m_input, &QLineEdit::returnPressed, this, [this]{
        const QString text = m_input->text().trimmed();
        if (text.isEmpty()) return;
        m_input->clear();
        emit inputSubmitted(text);
    });
}

void ChannelPane::setNick(const QString &nick)
{
    if (m_nickPrefix) m_nickPrefix->setText(nick);
}

void ChannelPane::setTopic(const QString &html)
{
    if (m_topicText) m_topicText->setText(html);
    if (m_topicBar && m_topicToggle) {
        const bool hasTopic = !html.isEmpty();
        m_topicToggle->setChecked(hasTopic);
        m_topicBar->setVisible(hasTopic);
        m_topicToggle->setText(hasTopic ? QStringLiteral("▾  topic") : QStringLiteral("▸  topic"));
    }
}
