#include "channelpane.h"

#include <QTextBrowser>
#include <QListWidget>
#include <QLineEdit>
#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QScrollBar>

ChannelPane::ChannelPane(const QString &host, const QString &channel, QWidget *parent)
    : QWidget(parent), m_host(host), m_channel(channel)
{
    auto *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(0);

    // Header: channel name + close button
    auto *header = new QWidget;
    header->setObjectName("paneHeader");
    auto *hbox = new QHBoxLayout(header);
    hbox->setContentsMargins(6, 2, 2, 2);
    hbox->setSpacing(4);
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
    hbox->addWidget(nameLabel, 1);
    hbox->addWidget(closeBtn);
    vbox->addWidget(header);

    // Chat browser + nick list
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
    if (m_nickPrefix)
        m_nickPrefix->setText(nick);
}
