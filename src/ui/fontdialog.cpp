#include "fontdialog.h"
#include "menuicons.h"

#include <QSpinBox>
#include <QLabel>
#include <QFrame>
#include <QListWidget>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFontDatabase>

FontDialog::FontDialog(const QString &family, const FontSizes &sizes, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Font Configuration");
    setMinimumWidth(380);

    // Family — toggle button + collapsible list (no popup window)
    m_familyBtn = new QPushButton(family);
    m_familyBtn->setCheckable(true);
    m_familyBtn->setAutoDefault(false);
    m_familyBtn->setMinimumHeight(30);

    m_familyList = new QListWidget;
    m_familyList->setFrameShape(QFrame::StyledPanel);
    m_familyList->setFixedHeight(160);
    m_familyList->setVisible(false);
    for (const QString &f : QFontDatabase::families())
        m_familyList->addItem(f);
    {
        const auto matches = m_familyList->findItems(family, Qt::MatchExactly);
        if (!matches.isEmpty()) {
            m_familyList->setCurrentItem(matches.first());
            m_familyList->scrollToItem(matches.first());
        }
    }

    connect(m_familyBtn, &QPushButton::toggled, m_familyList, &QWidget::setVisible);

    auto applyFamily = [this](QListWidgetItem *item) {
        if (!item) return;
        m_familyBtn->setText(item->text());
        m_preview->setFont(QFont(item->text(), m_spChat->value()));
    };
    connect(m_familyList, &QListWidget::itemClicked,   this, applyFamily);
    connect(m_familyList, &QListWidget::itemActivated, this, applyFamily);

    m_spToolbar      = makeSpinBox(sizes.toolbar);
    m_spServerHeader = makeSpinBox(sizes.serverHeader);
    m_spSidebar      = makeSpinBox(sizes.sidebar);
    m_spChat         = makeSpinBox(sizes.chat);
    m_spNickList     = makeSpinBox(sizes.nickList);
    m_spNickDock     = makeSpinBox(sizes.nickDock);
    m_spTopicBar     = makeSpinBox(sizes.topicBar);
    m_spTopicText    = makeSpinBox(sizes.topicText);
    m_spInputNick    = makeSpinBox(sizes.inputNick);
    m_spInput        = makeSpinBox(sizes.input);
    m_spTyping       = makeSpinBox(sizes.typing);
    m_spEmoji        = makeSpinBox(sizes.emoji);

    m_preview = new QLabel("The quick brown fox — AaBbCc 0123");
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_preview->setContentsMargins(8, 6, 8, 6);
    m_preview->setMinimumHeight(36);
    m_preview->setFont(QFont(family, sizes.chat));

    connect(m_spChat, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int pt){
        m_preview->setFont(QFont(m_familyBtn->text(), pt));
    });

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setIcon(MenuIcons::confirm());
    buttons->button(QDialogButtonBox::Cancel)->setIcon(MenuIcons::close());
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Family row
    auto *familyRow = new QHBoxLayout;
    familyRow->addWidget(new QLabel("Family:"));
    familyRow->addWidget(m_familyBtn, 1);

    // Sizes — two columns
    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(6);

    struct SizeRow { const char *l1; QSpinBox *s1; const char *l2; QSpinBox *s2; };
    const SizeRow rows[] = {
        { "Chat",        m_spChat,         "Input",        m_spInput        },
        { "Sidebar",     m_spSidebar,      "Nick List",    m_spNickList     },
        { "Topic Bar",   m_spTopicBar,     "Topic Text",   m_spTopicText    },
        { "Nick Label",  m_spInputNick,    "Typing",       m_spTyping       },
        { "Toolbar",     m_spToolbar,      "Users Title",  m_spNickDock     },
        { "Server Name", m_spServerHeader, "Chat Emoji",   m_spEmoji        },
    };
    for (int r = 0; r < 6; ++r) {
        auto *la = new QLabel(QString(rows[r].l1) + ":");
        la->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        grid->addWidget(la,         r, 0);
        grid->addWidget(rows[r].s1, r, 1);
        if (rows[r].s2) {
            auto *lb = new QLabel(QString(rows[r].l2) + ":");
            lb->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            grid->addWidget(lb,          r, 2);
            grid->addWidget(rows[r].s2,  r, 3);
        }
    }
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(3, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(8);
    layout->addLayout(familyRow);
    layout->addWidget(m_familyList);
    layout->addWidget(m_preview);
    layout->addLayout(grid);
    layout->addWidget(buttons);
}

QSpinBox *FontDialog::makeSpinBox(int value)
{
    auto *sb = new QSpinBox;
    sb->setRange(6, 32);
    sb->setSuffix(" pt");
    sb->setValue(value);
    return sb;
}

QString   FontDialog::selectedFamily() const { return m_familyBtn->text(); }
FontSizes FontDialog::selectedSizes()  const
{
    return { m_spToolbar->value(), m_spServerHeader->value(), m_spSidebar->value(),
             m_spChat->value(), m_spNickList->value(), m_spNickDock->value(),
             m_spTopicBar->value(), m_spTopicText->value(), m_spInputNick->value(),
             m_spInput->value(), m_spTyping->value(), m_spEmoji->value() };
}
