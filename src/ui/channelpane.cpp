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
#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>

ChannelPane::ChannelPane(const QString &host, const QString &channel, QWidget *parent)
    : QWidget(parent), m_host(host), m_channel(channel)
{
    auto *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(0);

    // Header: topic toggle + channel name + close button
    m_header = new QWidget;
    m_header->setObjectName("paneHeader");
    m_header->installEventFilter(this);
    auto *hbox = new QHBoxLayout(m_header);
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
    vbox->addWidget(m_header);

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
        const QString raw = m_input->text();
        if (raw.trimmed().isEmpty()) return;
        m_input->clear();
        emit inputSubmitted(raw);
    });

    setAcceptDrops(true);
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

// ---------------------------------------------------------------------------
// Drag-to-rearrange: drag from header, drop onto another pane
// ---------------------------------------------------------------------------

bool ChannelPane::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != m_header) return false;
    if (event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton)
            m_dragStartPos = me->pos();
    } else if (event->type() == QEvent::MouseMove) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (!(me->buttons() & Qt::LeftButton)) return false;
        if ((me->pos() - m_dragStartPos).manhattanLength() < QApplication::startDragDistance())
            return false;
        auto *drag = new QDrag(this);
        auto *mime = new QMimeData;
        mime->setText(key());
        drag->setMimeData(mime);
        drag->exec(Qt::MoveAction);
        return true;
    }
    return false;
}

void ChannelPane::dragEnterEvent(QDragEnterEvent *e)
{
    if (!e->mimeData()->hasText() || e->mimeData()->text() == key()) return;
    e->acceptProposedAction();
    m_header->setStyleSheet("background: palette(highlight);");
}

void ChannelPane::dragLeaveEvent(QDragLeaveEvent *)
{
    m_header->setStyleSheet({});
}

void ChannelPane::dropEvent(QDropEvent *e)
{
    m_header->setStyleSheet({});
    emit dropReceived(e->mimeData()->text());
    e->acceptProposedAction();
}
