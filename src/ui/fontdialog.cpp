#include "fontdialog.h"
#include "menuicons.h"

#include <QFontComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QFrame>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

FontDialog::FontDialog(const QString &family, const FontSizes &sizes, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Font Configuration");
    setMinimumWidth(380);

    m_familyBox = new QFontComboBox;
    m_familyBox->setCurrentFont(QFont(family));
    m_familyBox->setMinimumHeight(30);

    m_spToolbar      = makeSpinBox(sizes.toolbar);
    m_spServerHeader = makeSpinBox(sizes.serverHeader);
    m_spSidebar      = makeSpinBox(sizes.sidebar);
    m_spChat         = makeSpinBox(sizes.chat);
    m_spNickList     = makeSpinBox(sizes.nickList);
    m_spNickDock     = makeSpinBox(sizes.nickDock);
    m_spTopicBar     = makeSpinBox(sizes.topicBar);
    m_spInputNick    = makeSpinBox(sizes.inputNick);
    m_spInput        = makeSpinBox(sizes.input);
    m_spTyping       = makeSpinBox(sizes.typing);
    m_spEmoji        = makeSpinBox(sizes.emoji);

    m_preview = new QLabel("The quick brown fox — AaBbCc 0123");
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_preview->setContentsMargins(8, 6, 8, 6);
    m_preview->setMinimumHeight(36);

    auto updatePreview = [this]{
        m_preview->setFont(QFont(m_familyBox->currentFont().family(), m_spChat->value()));
    };
    connect(m_familyBox, &QFontComboBox::currentFontChanged, this, updatePreview);
    connect(m_spChat, QOverload<int>::of(&QSpinBox::valueChanged), this, updatePreview);
    updatePreview();

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setIcon(MenuIcons::confirm());
    buttons->button(QDialogButtonBox::Cancel)->setIcon(MenuIcons::close());
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Family row
    auto *familyRow = new QHBoxLayout;
    familyRow->addWidget(new QLabel("Family:"));
    familyRow->addWidget(m_familyBox, 1);

    // Sizes — two columns to halve the visual rows
    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(6);

    struct SizeRow { const char *l1; QSpinBox *s1; const char *l2; QSpinBox *s2; };
    const SizeRow rows[] = {
        { "Chat",        m_spChat,         "Input",        m_spInput        },
        { "Sidebar",     m_spSidebar,      "Nick List",    m_spNickList     },
        { "Topic Bar",   m_spTopicBar,     "Nick Label",   m_spInputNick    },
        { "Server Name", m_spServerHeader, "Typing",       m_spTyping       },
        { "Toolbar",     m_spToolbar,      "Users Title",  m_spNickDock     },
        { "Chat Emoji",  m_spEmoji,        "",             nullptr          },
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

QString   FontDialog::selectedFamily() const { return m_familyBox->currentFont().family(); }
FontSizes FontDialog::selectedSizes()  const
{
    return { m_spToolbar->value(), m_spServerHeader->value(), m_spSidebar->value(),
             m_spChat->value(), m_spNickList->value(), m_spNickDock->value(),
             m_spTopicBar->value(), m_spInputNick->value(), m_spInput->value(),
             m_spTyping->value(), m_spEmoji->value() };
}
