#include "preferencesdialog.h"
#include "ui/themeloader.h"
#include "ui/menuicons.h"
#include "ui/pillbutton.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QLabel>
#include <QFrame>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>

const QList<QPair<QString,QString>> PreferencesDialog::s_iconChoices = {
    { "dark",  "Dark"  },
    { "light", "Light" },
};

const QList<QPair<QString,QString>> PreferencesDialog::s_bracketChoices = {
    { "<>",   "<nick>  (angle)"   },
    { "[]",   "[nick]  (square)"  },
    { "::::", "::nick::  (colon)" },
    { "",     "nick  (none)"      },
};

static QLabel *sectionLabel(const QString &text)
{
    auto *l = new QLabel("<b>" + text + "</b>");
    l->setContentsMargins(0, 6, 0, 2);
    return l;
}

static QFrame *makeSep()
{
    auto *f = new QFrame;
    f->setFrameShape(QFrame::HLine);
    f->setFrameShadow(QFrame::Sunken);
    return f;
}

PreferencesDialog::PreferencesDialog(const Config &cfg, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Preferences");
    setMinimumWidth(330);

    const Theme t = ThemeLoader::load(cfg.ui.theme);
    const QColor accent(t.accent);

    auto *inner = new QWidget;
    auto *vbox  = new QVBoxLayout(inner);
    vbox->setContentsMargins(8, 6, 8, 6);
    vbox->setSpacing(3);

    // ── Quick access ─────────────────────────────────────────────────────────
    {
        auto *row = new QHBoxLayout;
        auto *manageBtn = new PillButton("Manage Servers...");
        manageBtn->setIcon(MenuIcons::servers());
        manageBtn->setAccentColor(accent);
        manageBtn->setAutoDefault(false);
        connect(manageBtn, &QPushButton::clicked, this, [this]{ emit manageServersRequested(); });
        row->addWidget(manageBtn);
        auto *docsBtn = new PillButton("Documentation");
        docsBtn->setIcon(MenuIcons::documentation());
        docsBtn->setAccentColor(accent);
        docsBtn->setAutoDefault(false);
        connect(docsBtn, &QPushButton::clicked, this, [this]{ emit docsRequested(); });
        row->addWidget(docsBtn);
        vbox->addLayout(row);
    }

    // ── Appearance ────────────────────────────────────────────────────────────
    vbox->addSpacing(2);
    vbox->addWidget(makeSep());
    vbox->addWidget(sectionLabel("Appearance"));

    // Theme — compact toggle button + collapsible list
    auto *themeBtn = new PillButton(cfg.ui.theme);
    themeBtn->setCheckable(true);
    themeBtn->setAutoDefault(false);
    themeBtn->setAccentColor(accent);
    themeBtn->setLeftAlign(true);
    {
        auto *row = new QHBoxLayout;
        row->addWidget(new QLabel("Theme:"));
        row->addWidget(themeBtn, 1);
        vbox->addLayout(row);
    }

    auto *themeList = new QListWidget;
    themeList->setFrameShape(QFrame::StyledPanel);
    themeList->setFixedHeight(150);
    themeList->setVisible(false);
    for (const QString &name : ThemeLoader::availableThemes())
        themeList->addItem(name);
    {
        const auto matches = themeList->findItems(cfg.ui.theme, Qt::MatchExactly);
        if (!matches.isEmpty()) {
            themeList->setCurrentItem(matches.first());
            themeList->scrollToItem(matches.first());
        }
    }
    vbox->addWidget(themeList);

    connect(themeBtn, &QPushButton::toggled, themeList, &QWidget::setVisible);

    auto applyTheme = [this, themeBtn](QListWidgetItem *item){
        if (!item) return;
        themeBtn->setText(item->text());
        emit themeChanged(item->text());
    };
    connect(themeList, &QListWidget::itemClicked,   this, applyTheme);
    connect(themeList, &QListWidget::itemActivated, this, applyTheme);

    vbox->addSpacing(3);
    vbox->addWidget(sectionLabel("App Icon"));
    {
        auto *iconGroup = new QButtonGroup(this);
        iconGroup->setExclusive(true);
        for (int i = 0; i < s_iconChoices.size(); ++i) {
            auto *rb = new QRadioButton(s_iconChoices[i].second);
            rb->setChecked(s_iconChoices[i].first == cfg.ui.appIcon);
            iconGroup->addButton(rb, i);
            vbox->addWidget(rb);
        }
        connect(iconGroup, &QButtonGroup::idClicked, this, [this](int id){
            if (id >= 0 && id < s_iconChoices.size())
                emit appIconChanged(s_iconChoices[id].first);
        });
    }

    vbox->addSpacing(2);
    auto *fontBtn = new PillButton("Font Config...");
    fontBtn->setIcon(MenuIcons::fontConfig());
    fontBtn->setAccentColor(accent);
    fontBtn->setAutoDefault(false);
    connect(fontBtn, &QPushButton::clicked, this, [this]{ emit fontConfigRequested(); });
    vbox->addWidget(fontBtn);

    // ── Interface ─────────────────────────────────────────────────────────────
    vbox->addSpacing(2);
    vbox->addWidget(makeSep());
    vbox->addWidget(sectionLabel("Interface"));

    m_topicCheck = new QCheckBox("Show Topic Bar");
    m_topicCheck->setChecked(cfg.ui.showTopic);
    connect(m_topicCheck, &QCheckBox::toggled, this, [this](bool on){ emit topicBarToggled(on); });
    vbox->addWidget(m_topicCheck);

    m_nickPrefixCheck = new QCheckBox("Show Nick in Input");
    m_nickPrefixCheck->setChecked(cfg.ui.showNickPrefix);
    connect(m_nickPrefixCheck, &QCheckBox::toggled, this, [this](bool on){ emit nickPrefixToggled(on); });
    vbox->addWidget(m_nickPrefixCheck);

    m_emojiCheck = new QCheckBox("Show Emoji Button");
    m_emojiCheck->setChecked(cfg.ui.showEmojiButton);
    connect(m_emojiCheck, &QCheckBox::toggled, this, [this](bool on){ emit emojiBtnToggled(on); });
    vbox->addWidget(m_emojiCheck);

    m_typingCheck = new QCheckBox("Typing Indicator");
    m_typingCheck->setChecked(cfg.ui.typingIndicator);
    connect(m_typingCheck, &QCheckBox::toggled, this, [this](bool on){ emit typingIndicatorToggled(on); });
    vbox->addWidget(m_typingCheck);

    m_notificationsCheck = new QCheckBox("Tray Notifications");
    m_notificationsCheck->setChecked(cfg.ui.notifications);
    connect(m_notificationsCheck, &QCheckBox::toggled, this, [this](bool on){ emit notificationsToggled(on); });
    vbox->addWidget(m_notificationsCheck);

    m_coloredNicksCheck = new QCheckBox("Colored Nicks");
    m_coloredNicksCheck->setChecked(cfg.ui.coloredNicks);
    connect(m_coloredNicksCheck, &QCheckBox::toggled, this, [this](bool on){ emit coloredNicksToggled(on); });
    vbox->addWidget(m_coloredNicksCheck);

    m_hangingIndentCheck = new QCheckBox("Hanging Indent (wrap under message)");
    m_hangingIndentCheck->setChecked(cfg.ui.hangingIndent);
    connect(m_hangingIndentCheck, &QCheckBox::toggled, this, [this](bool on){ emit hangingIndentToggled(on); });
    vbox->addWidget(m_hangingIndentCheck);

    m_loggingCheck = new QCheckBox("Log Messages to Disk");
    m_loggingCheck->setChecked(cfg.ui.logMessages);
    connect(m_loggingCheck, &QCheckBox::toggled, this, [this](bool on){ emit loggingToggled(on); });
    vbox->addWidget(m_loggingCheck);

    m_linkPreviewsCheck = new QCheckBox("Link Previews");
    m_linkPreviewsCheck->setChecked(cfg.ui.linkPreviews);
    connect(m_linkPreviewsCheck, &QCheckBox::toggled, this, [this](bool on){ emit linkPreviewsToggled(on); });
    vbox->addWidget(m_linkPreviewsCheck);

    vbox->addSpacing(2);
    {
        auto *row = new QHBoxLayout;
        row->addWidget(new QLabel("Nick Brackets:"));
        m_bracketsCombo = new QComboBox;
        m_bracketsCombo->setStyleSheet("font-size:11px; padding:2px 4px;");
        for (const auto &[key, label] : s_bracketChoices)
            m_bracketsCombo->addItem(label);
        for (int i = 0; i < s_bracketChoices.size(); ++i) {
            if (s_bracketChoices[i].first == cfg.ui.nickBrackets) {
                m_bracketsCombo->setCurrentIndex(i);
                break;
            }
        }
        connect(m_bracketsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int idx){
            if (idx >= 0 && idx < s_bracketChoices.size())
                emit nickBracketsChanged(s_bracketChoices[idx].first);
        });
        row->addWidget(m_bracketsCombo, 1);
        vbox->addLayout(row);
    }

    // ── Profile ───────────────────────────────────────────────────────────────
    vbox->addSpacing(2);
    vbox->addWidget(makeSep());
    vbox->addWidget(sectionLabel("Profile"));

    {
        auto *note = new QLabel(
            "Your display name and avatar URL are published to the server when you connect. "
            "Other users can see them in the nick list tooltip. "
            "Requires <b>draft/metadata-2</b> support (Ergo, soju, and others).");
        note->setWordWrap(true);
        note->setStyleSheet("font-size:11px;");
        vbox->addWidget(note);
    }

    vbox->addSpacing(3);
    {
        auto *row = new QHBoxLayout;
        row->addWidget(new QLabel("Display Name:"));
        m_displayNameEdit = new QLineEdit;
        m_displayNameEdit->setPlaceholderText("e.g. Alice Smith  (leave blank to clear)");
        m_displayNameEdit->setText(cfg.profileDisplayName);
        row->addWidget(m_displayNameEdit, 1);
        vbox->addLayout(row);
    }
    {
        auto *row = new QHBoxLayout;
        row->addWidget(new QLabel("Avatar URL:"));
        m_avatarUrlEdit = new QLineEdit;
        m_avatarUrlEdit->setPlaceholderText("https://example.com/avatar.png  or  /path/to/local.png");
        m_avatarUrlEdit->setText(cfg.profileAvatarUrl);
        row->addWidget(m_avatarUrlEdit, 1);
        auto *browseBtn = new QPushButton("Browse...");
        browseBtn->setAutoDefault(false);
        connect(browseBtn, &QPushButton::clicked, this, [this] {
            const QString path = QFileDialog::getOpenFileName(
                this, "Select Avatar Image", QString(),
                "Images (*.png *.jpg *.jpeg *.gif *.ico *.webp)");
            if (!path.isEmpty())
                m_avatarUrlEdit->setText(path);
        });
        row->addWidget(browseBtn);
        vbox->addLayout(row);
    }
    {
        auto *applyBtn = new PillButton("Apply to connected servers");
        applyBtn->setAccentColor(accent);
        applyBtn->setAutoDefault(false);
        connect(applyBtn, &QPushButton::clicked, this, [this]{
            emit profileSetRequested(m_displayNameEdit->text().trimmed(),
                                     m_avatarUrlEdit->text().trimmed());
        });
        vbox->addWidget(applyBtn);
    }

    vbox->addSpacing(2);

    auto *scroll = new QScrollArea;
    scroll->setWidget(inner);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(scroll);
}

