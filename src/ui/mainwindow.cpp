#include "mainwindow.h"
#include "ui/commanddispatcher.h"
#include "irc/ircclient.h"
#include "irc/dccsend.h"
#include "irc/dccreceive.h"
#include "ui/trayicon.h"
#include "ui/aboutdialog.h"
#include "ui/channellistdialog.h"
#include "ui/docsdialog.h"
#include "ui/fontdialog.h"
#include "ui/preferencesdialog.h"
#include "ui/serverdialog.h"
#include "ui/manageserversdialog.h"
#include "ui/appicons.h"
#include "ui/themeloader.h"
#include "ui/linkpreview.h"
#include "ui/emojipicker.h"
#include "ui/emojidata.h"
#include "ui/menuicons.h"
#include "ui/signalbars.h"
#include "ui/fadescrollbar.h"
#include "ui/channelpane.h"
#include "ui/chatrenderer.h"
#include "ui/chatview.h"
#include "config/config.h"
#include "net/addresscheck.h"

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QToolBar>
#include <QToolButton>
#include <QMenu>
#include <QAction>
#include <QTreeWidget>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTextBrowser>
#include <QDesktopServices>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>
#include <QSplitter>
#include <QTextCharFormat>
#include <QScrollBar>
#include <QInputDialog>
#include <QSysInfo>
#include <QThread>
#include <QTimer>
#include <QSettings>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QProcess>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>
#include "version.h"
#include <QRegularExpression>
#include <QToolTip>
#include <QCursor>
#include <QMouseEvent>
#include <QTextDocument>
#include <QTextFrame>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextFragment>
#include <QBuffer>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QScreen>
#include <QRandomGenerator>
#include <QShortcut>
#include <QStyledItemDelegate>
#if defined(Q_OS_WIN)
#  include <windows.h>
#endif

class FixedRowDelegate : public QStyledItemDelegate {
    int m_height;
public:
    explicit FixedRowDelegate(int height, QObject *parent = nullptr)
        : QStyledItemDelegate(parent), m_height(height) {}
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
    {
        QSize s = QStyledItemDelegate::sizeHint(option, index);
        s.setHeight(m_height);
        return s;
    }
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        const QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        opt.icon = QIcon();
        opt.decorationSize = QSize(0, 0);
        QStyledItemDelegate::paint(painter, opt, index);
        if (!icon.isNull()) {
            const int sz = 14;
            const QFontMetrics fm(opt.font);
            const int textEnd = opt.rect.left() + opt.fontMetrics.horizontalAdvance(opt.text)
                                + fm.horizontalAdvance(QLatin1Char(' '));
            QRect r(textEnd,
                    opt.rect.top() + (opt.rect.height() - sz) / 2,
                    sz, sz);
            icon.paint(painter, r);
        }
    }
};

class NickDelegate : public QStyledItemDelegate {
    QColor m_accent;
    QColor m_hover;
    QColor m_activeText;
public:
    explicit NickDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent) {}

    void setColors(const QColor &accent, const QColor &hover, const QColor &activeText) {
        m_accent     = accent;
        m_hover      = hover;
        m_activeText = activeText;
    }

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &) const override
    {
        return QSize(option.rect.width(), 16);
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        const QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        opt.icon = QIcon();
        opt.decorationSize = QSize(0, 0);

        const bool selected = opt.state & QStyle::State_Selected;
        const bool hovered  = opt.state & QStyle::State_MouseOver;

        const QWidget *w    = opt.widget;
        const QStyle  *s    = w ? w->style() : QApplication::style();
        const QRect textRect = s->subElementRect(QStyle::SE_ItemViewItemText, &opt, w);
        const QFontMetrics fm(opt.font);
        const int textW      = fm.horizontalAdvance(opt.text);
        const int textMargin = s->pixelMetric(QStyle::PM_FocusFrameHMargin, &opt, w) + 1;
        constexpr int hPad   = 8;
        constexpr int vPad   = 1;
        constexpr int iconSz = 14;
        constexpr int iconGap = 2;
        const int pillX      = textRect.x() + textMargin - hPad;
        const int iconExtra  = icon.isNull() ? 0 : (iconGap + iconSz);

        if (selected || hovered) {
            const QColor bg = selected ? m_accent : m_hover;
            if (bg.isValid()) {
                QRect r(pillX,
                        opt.rect.y() + vPad,
                        qMin(textW + hPad * 2 + iconExtra, opt.rect.right() - pillX),
                        opt.rect.height() - vPad * 2);
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setPen(Qt::NoPen);
                painter->setBrush(bg);
                painter->drawRoundedRect(r, 6.0, 6.0);
                painter->restore();
            }
        }

        opt.state &= ~(QStyle::State_HasFocus | QStyle::State_MouseOver);
        if (selected && m_activeText.isValid()) {
            opt.palette.setColor(QPalette::All, QPalette::Highlight,       QColor(Qt::transparent));
            opt.palette.setColor(QPalette::All, QPalette::HighlightedText, m_activeText);
        }
        QStyledItemDelegate::paint(painter, opt, index);

        if (!icon.isNull()) {
            const int iconX = textRect.x() + textMargin + textW + iconGap;
            QRect r(iconX,
                    opt.rect.top() + (opt.rect.height() - iconSz) / 2,
                    iconSz, iconSz);
            icon.paint(painter, r);
        }
    }
};

class SidebarDelegate : public QStyledItemDelegate {
    QColor m_accent;
    QColor m_hover;
    QColor m_activeText;
public:
    explicit SidebarDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent) {}

    void setColors(const QColor &accent, const QColor &hover, const QColor &activeText) {
        m_accent     = accent;
        m_hover      = hover;
        m_activeText = activeText;
    }

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
    {
        QSize s = QStyledItemDelegate::sizeHint(option, index);
        s.setHeight(26);
        return s;
    }


    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        const QIcon indicator = qvariant_cast<QIcon>(index.data(Qt::UserRole + 2));
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        opt.icon = QIcon();
        opt.decorationSize = QSize(0, 0);

        const bool selected = opt.state & QStyle::State_Selected;
        const bool hovered  = opt.state & QStyle::State_MouseOver;

        const QWidget *w    = opt.widget;
        const QStyle  *s    = w ? w->style() : QApplication::style();
        const QRect textRect = s->subElementRect(QStyle::SE_ItemViewItemText, &opt, w);
        const QFontMetrics fm(opt.font);
        const int textW      = fm.horizontalAdvance(opt.text);
        const int textMargin = s->pixelMetric(QStyle::PM_FocusFrameHMargin, &opt, w) + 1;
        constexpr int hPad   = 8;
        constexpr int vPad   = 2;
        constexpr int iconSz = 14;
        constexpr int iconGap = 2;
        const int pillX      = textRect.x() + textMargin - hPad;
        const int iconExtra  = indicator.isNull() ? 0 : (iconGap + iconSz);

        if (selected || hovered) {
            const QColor bg = selected ? m_accent : m_hover;
            if (bg.isValid()) {
                QRect r(pillX,
                        opt.rect.y() + vPad,
                        qMin(textW + hPad * 2 + iconExtra, opt.rect.right() - pillX),
                        opt.rect.height() - vPad * 2);
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setPen(Qt::NoPen);
                painter->setBrush(bg);
                painter->drawRoundedRect(r, 10.0, 10.0);
                painter->restore();
            }
        }

        opt.state &= ~(QStyle::State_HasFocus | QStyle::State_MouseOver);
        if (selected && m_activeText.isValid()) {
            opt.palette.setColor(QPalette::All, QPalette::Highlight,       QColor(Qt::transparent));
            opt.palette.setColor(QPalette::All, QPalette::HighlightedText, m_activeText);
        }
        QStyledItemDelegate::paint(painter, opt, index);

        if (!indicator.isNull()) {
            const int iconX = textRect.x() + textMargin + textW + iconGap;
            QRect r(iconX,
                    opt.rect.top() + (opt.rect.height() - iconSz) / 2,
                    iconSz, iconSz);
            indicator.paint(painter, r);
        }
    }
};

// Minimum width of the topic bar left zone — wide enough to always show the
// hamburger (22) + gear (22) + right margin (4) even when the sidebar is closed.
static constexpr int kBtnZoneMinW    = 60;
static constexpr int kDefaultWindowW  = 900;
static constexpr int kDefaultWindowH  = 650;
static constexpr int kInputHistoryCap = 100;
static constexpr int kMaxExtraPanes   = 3;
static constexpr int kRenderWindow    = 150; // messages rendered per channel view
static constexpr int kRenderChunk     = 50;  // messages loaded per scroll-to-top


// Clips all child widgets to a rounded rect via a bitmap mask.
class RoundedPane : public QWidget {
public:
    explicit RoundedPane(QWidget *parent = nullptr) : QWidget(parent) {}
protected:
    void resizeEvent(QResizeEvent *e) override {
        QWidget::resizeEvent(e);
        if (size().isEmpty()) return;
        QBitmap bm(size());
        bm.fill(Qt::color0);
        QPainter p(&bm);
        p.setBrush(Qt::color1);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect(), 10, 10);
        setMask(bm);
    }
};

static ChatLine buildReactionLine(const QHash<QString, QSet<QString>> &rx, const QString &msgid)
{
    QTextCharFormat fmt;
    fmt.setForeground(QColor("#888888"));
    QString text;
    QList<ChatSegment> segs;
    for (auto it = rx.constBegin(); it != rx.constEnd(); ++it) {
        ChatSegment seg;
        seg.start  = static_cast<int>(text.size());
        const QString piece = it.key() + "(" + QString::number(it.value().size()) + ") ";
        seg.length = static_cast<int>(piece.size());
        seg.format = fmt;
        text += piece;
        segs.append(seg);
    }
    ChatLine line;
    line.text     = text;
    line.segments = segs;
    line.id       = "rx:" + msgid;
    line.role     = ChatLineRole::Reaction;
    return line;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

MainWindow::MainWindow(SessionModel *model, const Config &cfg, QWidget *parent)
    : QMainWindow(parent)
    , m_model(model)
    , m_config(cfg)
{
    // Init UI toggles from config
    m_showNickPrefix = cfg.ui.showNickPrefix;
    m_showTopic      = cfg.ui.showTopic;
    m_showEmojiBtn   = cfg.ui.showEmojiButton;

    setWindowTitle("Uplink");
    setWindowIcon(AppIcons::appIcon(m_config.ui.appIcon));
    resize(kDefaultWindowW, kDefaultWindowH);

    ThemeLoader::apply(m_config.ui.theme);
    m_theme = ThemeLoader::load(m_config.ui.theme);
    setDockOptions(QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks);

    setupToolbar();
    setupSidebar();
    setupNickPanel();
    setupChatArea();
    setupInputBar();
    connectModel();
    applyFontSizes();

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        m_tray = new TrayIcon(model, this);
        m_tray->setBaseIcon(AppIcons::trayIcon());
        m_tray->setNotificationsEnabled(m_config.ui.notifications);
    }

    auto *ctrlW = new QShortcut(QKeySequence("Ctrl+W"), this);
    connect(ctrlW, &QShortcut::activated, this, [this]{
        if (m_tray && m_tray->isVisible()) hide();
    });

    auto *ctrlF = new QShortcut(QKeySequence::Find, this);
    connect(ctrlF, &QShortcut::activated, this, &MainWindow::showSearchBar);

    statusBar()->hide();

    QSettings settings("uplink", "uplink");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    setWindowState(windowState() & ~Qt::WindowMaximized);
    // If the restored normal-mode size fills ≥80% of the screen the right edge
    // ends up at or near the screen boundary — WM can't offer a grab handle.
    // Reset to default so the window always opens with visible resize margins.
    if (auto *scr = QGuiApplication::primaryScreen()) {
        if (width() > scr->availableGeometry().width() * 8 / 10)
            resize(kDefaultWindowW, kDefaultWindowH);
    }
    // Pre-show width cap: limits WM_NORMAL_HINTS.max_width so the WM cannot map
    // the window wider than our target.  Released in the post-show timer.
    // Skipped on Windows — Windows uses max size to gate the maximize button,
    // so setting it this tight would grey out the title-bar maximize control.
#if !defined(Q_OS_WIN)
    setMaximumWidth(width() + 20);
#endif

    if (settings.contains("nickSplitter"))
        m_chatSplitter->restoreState(settings.value("nickSplitter").toByteArray());
    if (settings.contains("sidebarWidth"))
        m_sidebarExpandedWidth = settings.value("sidebarWidth").toInt();

    const QStringList savedPanes       = settings.value("panes").toStringList();
    const int         savedPrimarySlot = settings.value("primarySlot", 0).toInt();

    QTimer::singleShot(0, this, [this]{
        // Release the pre-show width cap.
        setMaximumWidth(QWIDGETSIZE_MAX);

        auto *scr = screen() ? screen() : QGuiApplication::primaryScreen();
        const bool qtMaximized = windowState() & Qt::WindowMaximized;
        const bool tooWide     = scr && width() > scr->availableGeometry().width() * 8 / 10;

        if (qtMaximized) {
            setWindowState(Qt::WindowNoState);
        } else if (tooWide) {
            // KWin session-restored a wider-than-screen geometry that Qt doesn't
            // surface as Maximized. Cycle state so KWin clears the saved size.
            setWindowState(Qt::WindowMaximized);
            setWindowState(Qt::WindowNoState);
        }

        if (qtMaximized || tooWide) {
            QTimer::singleShot(100, this, &MainWindow::correctStartupGeometry);
            return;
        }

        // Normal path: clamp position/size immediately.
        if (scr) {
            const QRect avail = scr->availableGeometry();
            QRect w = geometry();
            if (w.height() > avail.height() - 60) w.setHeight(avail.height() - 80);
            if (w.right()  > avail.right())  w.moveRight(avail.right());
            if (w.left()   < avail.left())   w.moveLeft(avail.left());
            if (w.bottom() > avail.bottom()) w.moveBottom(avail.bottom());
            if (w.top()    < avail.top())    w.moveTop(avail.top());
            setGeometry(w);
        }
        const int total = m_mainSplitter->width();
        if (total > 0)
            m_mainSplitter->setSizes({m_sidebarExpandedWidth, total - m_sidebarExpandedWidth});
        if (m_topicLeft) m_topicLeft->setFixedWidth(m_sidebarExpandedWidth);
    });

    if (!savedPanes.isEmpty()) {
        QTimer::singleShot(0, this, [this, savedPanes, savedPrimarySlot]{
            for (const QString &k : savedPanes) {
                const qsizetype sep = k.indexOf('|');
                if (sep < 0) continue;
                openChannelPane(k.left(sep), k.mid(sep + 1));
            }
            m_primarySlot = qBound(0, savedPrimarySlot, static_cast<int>(m_orderedPanes.size()));
            rebuildPaneLayout();
        });
    }

    connect(m_mainSplitter, &QSplitter::splitterMoved, this, [this](int, int){
        const int w = m_mainSplitter->sizes().value(0);
        if (m_sidebarExpanded && w > 0)
            m_sidebarExpandedWidth = w;
        if (m_topicLeft) m_topicLeft->setFixedWidth(qMax(kBtnZoneMinW, w));
    });

    connect(qApp, &QApplication::aboutToQuit, this, [this]{
        QSettings s("uplink", "uplink");
        s.setValue("geometry", saveGeometry());
        s.setValue("windowState", saveState());
        s.setValue("nickSplitter", m_chatSplitter->saveState());
        s.setValue("sidebarWidth", m_sidebarExpandedWidth);
        QStringList paneList;
        for (auto *p : std::as_const(m_orderedPanes))
            paneList << p->key();
        s.setValue("panes", paneList);
        s.setValue("primarySlot", m_primarySlot);
    });

    m_dispatcher = new CommandDispatcher(m_model, &m_config, this, this);
    connect(m_dispatcher, &CommandDispatcher::switchChannel,  this, &MainWindow::switchToChannel);
    connect(m_dispatcher, &CommandDispatcher::focusInput,     this, [this]{ if (m_input) m_input->setFocus(); });
    connect(m_dispatcher, &CommandDispatcher::clearChat,      this, [this]{ if (m_chatView) m_chatView->clear(); });
    connect(m_dispatcher, &CommandDispatcher::openChannelList,this, &MainWindow::openChannelList);
    connect(m_dispatcher, &CommandDispatcher::replyBarCleared, this, &MainWindow::clearReplyBar);
}

MainWindow::~MainWindow() = default;

void MainWindow::correctStartupGeometry()
{
    QScreen *scr = screen() ? screen() : QGuiApplication::primaryScreen();
    if (scr) {
        const QRect avail = scr->availableGeometry();
        QRect w = geometry();
        if (w.width() > avail.width() * 8 / 10) w.setWidth(kDefaultWindowW);
        if (w.height() > avail.height() - 60)   w.setHeight(avail.height() - 80);
        if (w.right()  > avail.right())  w.moveRight(avail.right());
        if (w.left()   < avail.left())   w.moveLeft(avail.left());
        if (w.bottom() > avail.bottom()) w.moveBottom(avail.bottom());
        if (w.top()    < avail.top())    w.moveTop(avail.top());
        setMinimumSize(1, 1);
        setGeometry(w);
    }
    const int total = m_mainSplitter->width();
    if (total > 0)
        m_mainSplitter->setSizes({m_sidebarExpandedWidth, total - m_sidebarExpandedWidth});
    if (m_topicLeft) m_topicLeft->setFixedWidth(m_sidebarExpandedWidth + 8);
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

void MainWindow::setupToolbar()
{
    auto *tb = addToolBar("Main");
    tb->setMovable(false);
    tb->setFloatable(false);
    tb->setContentsMargins(0, 0, 0, 0);
    if (tb->layout())
        tb->layout()->setContentsMargins(0, 0, 0, 0);
    tb->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tb, &QWidget::customContextMenuRequested, this, [](const QPoint&){});

    m_hamburger = new QToolButton;
    m_hamburger->setFixedSize(28, 28);
    m_hamburger->setAutoRaise(true);
    m_hamburger->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_hamburger, &QWidget::customContextMenuRequested, this, [](const QPoint&){});
    m_hamburger->setObjectName("hamburger");
    tb->hide();

    connect(m_hamburger, &QToolButton::clicked, this, [this]{
        auto *menu = new QMenu(m_hamburger);
        menu->setAttribute(Qt::WA_DeleteOnClose);
        const QColor ic(m_theme.valid ? m_theme.text : "#ffffff");

        menu->addAction(MenuIcons::about(ic), "About Uplink", this, [this]{
            if (!m_aboutDialog) m_aboutDialog = new AboutDialog(this);
            m_aboutDialog->showCentered();
        });

        menu->addAction(MenuIcons::checkForUpdates(ic), "Check for Updates", this, [this]{
            auto *nam = new QNetworkAccessManager(this);
            QNetworkRequest req(QUrl("https://api.github.com/repos/noderelay/UplinkIRC/releases/latest"));
            req.setRawHeader("User-Agent", "Uplink/" UPLINK_VERSION);
            auto *reply = nam->get(req);
            connect(reply, &QNetworkReply::finished, this, [this, reply, nam]{
                reply->deleteLater();
                nam->deleteLater();
                if (reply->error() != QNetworkReply::NoError) {
                    QMessageBox::warning(this, "Update Check",
                        "Could not check for updates:\n" + reply->errorString());
                    return;
                }
                const QByteArray body = reply->readAll();
                const QRegularExpression re(R"_("tag_name"\s*:\s*"v?(\d+)\.(\d+)\.(\d+)")_");
                const auto m = re.match(body);
                if (!m.hasMatch()) {
                    QMessageBox::warning(this, "Update Check", "Could not parse release info.");
                    return;
                }
                const int maj = m.captured(1).toInt();
                const int min = m.captured(2).toInt();
                const int pat = m.captured(3).toInt();
                const bool newer = (maj != UPLINK_VERSION_MAJOR) ? maj > UPLINK_VERSION_MAJOR
                                 : (min != UPLINK_VERSION_MINOR) ? min > UPLINK_VERSION_MINOR
                                 : pat > UPLINK_VERSION_PATCH;
                if (newer) {
                    QMessageBox::information(this, "Update Available",
                        QString("Uplink v%1.%2.%3 is available.\nYou are running v" UPLINK_VERSION ".")
                            .arg(maj).arg(min).arg(pat));
                } else {
                    QMessageBox::information(this, "Up to Date",
                        "You are running the latest version (v" UPLINK_VERSION ").");
                }
            });
        });

        menu->addAction(MenuIcons::documentation(ic), "Documentation", this, [this]{
            if (!m_docsDialog)
                m_docsDialog = new DocsDialog(this);
            m_docsDialog->show();
            m_docsDialog->raise();
            m_docsDialog->activateWindow();
        });

        menu->addAction(MenuIcons::fontConfig(ic), "Preferences", this, [this]{
            if (!m_prefsDialog) {
                m_prefsDialog = new PreferencesDialog(m_config, this);
                connectPreferences();
            }
            m_prefsDialog->show();
            m_prefsDialog->raise();
            m_prefsDialog->activateWindow();
        });

        menu->addSeparator();

        menu->addAction(MenuIcons::servers(ic), "Open Config", this, []{
            QDesktopServices::openUrl(QUrl::fromLocalFile(Config::defaultPath()));
        });

        menu->addAction(MenuIcons::connStatus(ic), "Reload Config", this, []{
            QProcess::startDetached(QCoreApplication::applicationFilePath(),
                                    QCoreApplication::arguments());
            QCoreApplication::quit();
        });

        menu->addSeparator();
        menu->addAction(MenuIcons::exit(ic), "Quit Uplink", this, []{ QCoreApplication::quit(); });

        QPoint pos = m_hamburger->mapToGlobal(QPoint(0, m_hamburger->height()));
        menu->exec(pos);
    });
}

static QIcon makeTopicIcon(const QColor &color);
static QIcon makeMenuIcon(const QColor &color);

void MainWindow::connectPreferences()
{
    connect(m_prefsDialog, &PreferencesDialog::themeChanged, this, [this](const QString &name){
        m_config.ui.theme = name;
        ThemeLoader::apply(name);
        m_theme = ThemeLoader::load(name);
        if (m_chatView && m_theme.valid)
            m_chatView->setColors(QColor(m_theme.text), QColor(m_theme.background),
                                  QColor(m_theme.accent),
                                  QColor(m_theme.background),
                                  QColor(m_theme.border));
        if (m_sidebarDelegate && m_theme.valid)
            m_sidebarDelegate->setColors(QColor(m_theme.accent),
                                         QColor(m_theme.border),
                                         QColor(m_theme.text));
        if (m_nickDelegate && m_theme.valid)
            m_nickDelegate->setColors(QColor(m_theme.accent),
                                      QColor(m_theme.border),
                                      QColor(m_theme.text));
        if (m_theme.valid) {
            if (m_primaryTopicBtn) {
                const bool on = m_primaryTopicBtn->isChecked();
                m_primaryTopicBtn->setIcon(makeTopicIcon(
                    QColor(on ? m_theme.accent : m_theme.placeholder)));
            }
            for (auto *pane : std::as_const(m_panes)) {
                auto *nd = new NickDelegate(pane->nickList());
                nd->setColors(QColor(m_theme.accent),
                              QColor(m_theme.border),
                              QColor(m_theme.text));
                pane->nickList()->setItemDelegate(nd);
                pane->setTopicIcon(
                    makeTopicIcon(QColor(m_theme.placeholder)),
                    makeTopicIcon(QColor(m_theme.accent)));
            }
        }
        Config::save(m_config, Config::defaultPath());
    });

    connect(m_prefsDialog, &PreferencesDialog::fontConfigRequested, this, [this]{
        FontDialog dlg(m_config.ui.fontFamily, m_config.ui.fontSizes, this);
        if (dlg.exec() == QDialog::Accepted) {
            m_config.ui.fontFamily = dlg.selectedFamily();
            m_config.ui.fontSizes  = dlg.selectedSizes();
            Config::save(m_config, Config::defaultPath());
            QFont appFont(m_config.ui.fontFamily);
            appFont.setStyleHint(QFont::Monospace);
            QApplication::setFont(appFont);
            applyFontSizes();
        }
    });


    connect(m_prefsDialog, &PreferencesDialog::appIconChanged, this, [this](const QString &key){
        m_config.ui.appIcon = key;
        setWindowIcon(AppIcons::appIcon(key));
        if (m_tray) m_tray->setBaseIcon(AppIcons::appIcon(key));
        Config::save(m_config, Config::defaultPath());
    });

    connect(m_prefsDialog, &PreferencesDialog::topicBarToggled, this, [this](bool on){
        m_showTopic = on;
        m_config.ui.showTopic = on;
        m_topicDisplay->setVisible(on);
        if (m_primaryTopicBtn) {
            m_primaryTopicBtn->setChecked(on);
            m_primaryTopicBtn->setText(on ? QStringLiteral("▾") : QStringLiteral("▸"));
        }
        Config::save(m_config, Config::defaultPath());
    });

    connect(m_prefsDialog, &PreferencesDialog::nickPrefixToggled, this, [this](bool on){
        m_showNickPrefix = on;
        m_config.ui.showNickPrefix = on;
        m_nickPrefix->setVisible(on);
        for (auto *p : std::as_const(m_orderedPanes))
            p->setNickVisible(on);
        Config::save(m_config, Config::defaultPath());
    });

    connect(m_prefsDialog, &PreferencesDialog::emojiBtnToggled, this, [this](bool on){
        m_showEmojiBtn = on;
        m_config.ui.showEmojiButton = on;
        m_emojiBtn->setVisible(on);
        Config::save(m_config, Config::defaultPath());
    });

    connect(m_prefsDialog, &PreferencesDialog::sendBtnToggled, this, [this](bool on){
        m_config.ui.showSendButton = on;
        m_sendBtn->setVisible(on);
        Config::save(m_config, Config::defaultPath());
    });

    connect(m_prefsDialog, &PreferencesDialog::typingIndicatorToggled, this, [this](bool on){
        m_config.ui.typingIndicator = on;
        Config::save(m_config, Config::defaultPath());
        if (!on) {
            m_typingOutTimer->stop();
            m_typingActive = false;
            m_typingNicks.clear();
            for (auto *t : std::as_const(m_typingNickTimers)) { t->stop(); t->deleteLater(); }
            m_typingNickTimers.clear();
            m_typingLabel->setVisible(false);
        } else {
            m_typingLabel->setText("");
            m_typingLabel->setVisible(true);
        }
    });


    connect(m_prefsDialog, &PreferencesDialog::notificationsToggled, this, [this](bool on){
        m_config.ui.notifications = on;
        Config::save(m_config, Config::defaultPath());
        if (m_tray)
            m_tray->setNotificationsEnabled(on);
    });

    connect(m_prefsDialog, &PreferencesDialog::coloredNicksToggled, this, [this](bool on){
        m_config.ui.coloredNicks = on;
        Config::save(m_config, Config::defaultPath());
        if (!m_model->activeHost().isEmpty() && !m_model->activeChannel().isEmpty())
            refreshNickList(m_model->activeHost().str(), m_model->activeChannel().str());
    });

    connect(m_prefsDialog, &PreferencesDialog::hangingIndentToggled, this, [this](bool on){
        m_config.ui.hangingIndent = on;
        Config::save(m_config, Config::defaultPath());
    });

    connect(m_prefsDialog, &PreferencesDialog::loggingToggled, this, [this](bool on){
        m_config.ui.logMessages = on;
        Config::save(m_config, Config::defaultPath());
    });

    connect(m_prefsDialog, &PreferencesDialog::linkPreviewsToggled, this, [this](bool on){
        m_config.ui.linkPreviews = on;
        Config::save(m_config, Config::defaultPath());
    });

    connect(m_prefsDialog, &PreferencesDialog::nickBracketsChanged, this, [this](const QString &br){
        m_config.ui.nickBrackets = br;
        Config::save(m_config, Config::defaultPath());
    });

    connect(m_prefsDialog, &PreferencesDialog::manageServersRequested, this, [this]{
        ManageServersDialog dlg(m_config.servers, this);
        if (dlg.exec() != QDialog::Accepted) return;
        const QList<ServerConfig> updated = dlg.servers();
        for (const ServerConfig &old : m_config.servers) {
            const bool stillPresent = std::any_of(updated.begin(), updated.end(),
                [&](const ServerConfig &s){ return s.host == old.host; });
            if (!stillPresent)
                m_model->removeServer(ServerId{old.host});
        }
        for (const ServerConfig &sc : updated) {
            const ServerConfig *existing = nullptr;
            for (const ServerConfig &old : m_config.servers)
                if (old.host == sc.host) { existing = &old; break; }
            if (!existing) {
                m_model->addServer(sc);
            } else if (*existing != sc) {
                m_model->updateServer(ServerId{existing->host}, sc);
            }
        }
        m_config.servers = updated;
        Config::save(m_config, Config::defaultPath(), true);
    });

    connect(m_prefsDialog, &PreferencesDialog::aboutRequested, this, [this]{
        if (!m_aboutDialog) m_aboutDialog = new AboutDialog(this);
        m_aboutDialog->showCentered();
    });

    connect(m_prefsDialog, &PreferencesDialog::docsRequested, this, [this]{
        if (!m_docsDialog)
            m_docsDialog = new DocsDialog(this);
        m_docsDialog->show();
        m_docsDialog->raise();
        m_docsDialog->activateWindow();
    });

    connect(m_prefsDialog, &PreferencesDialog::profileSetRequested,
            this, [this](const QString &displayName, const QString &avatarUrl) {
        const QString oldAvatarUrl = m_config.profileAvatarUrl;
        m_config.profileDisplayName = displayName;
        m_config.profileAvatarUrl   = avatarUrl;
        Config::save(m_config, Config::defaultPath());
        // Evict stale cached avatar so the new one is fetched and displayed
        if (!oldAvatarUrl.isEmpty() && oldAvatarUrl != avatarUrl)
            m_avatarCache.remove(oldAvatarUrl);
        QStringList sent, skipped;
        for (const auto &sess : m_model->sessions()) {
            if (!sess.connected) continue;
            // Always update local nickMeta for own nick so tooltip reflects new values
            if (!displayName.isEmpty())
                m_model->onUserMetaChanged(ServerId{sess.host}, sess.nick, "display-name", displayName);
            if (!avatarUrl.isEmpty())
                m_model->onUserMetaChanged(ServerId{sess.host}, sess.nick, "avatar", avatarUrl);
            auto *cl = m_model->clientFor(ServerId{sess.host});
            if (!cl || !cl->hasCap("draft/metadata-2")) {
                skipped << sess.name;
                continue;
            }
            m_model->sendRaw(ServerId{sess.host}, "METADATA * SET display-name :" + displayName);
            const bool localPath = avatarUrl.startsWith('/') || QUrl(avatarUrl).isLocalFile();
            if (!localPath)
                m_model->sendRaw(ServerId{sess.host}, "METADATA * SET avatar :" + avatarUrl);
            sent << sess.name;
        }
        if (!avatarUrl.isEmpty()) fetchAvatar(avatarUrl);
        const QString activeHost = m_model->activeHost().str();
        const QString activeChan = m_model->activeChannel().str();
        if (!sent.isEmpty())
            m_model->localMessage(ServerId{activeHost}, BufferId{activeChan},
                "Profile sent to: " + sent.join(", "));
        if (!skipped.isEmpty())
            m_model->localMessage(ServerId{activeHost}, BufferId{activeChan},
                "Skipped (no draft/metadata-2 support): " + skipped.join(", "));
        if (sent.isEmpty() && skipped.isEmpty())
            m_model->localMessage(ServerId{activeHost}, BufferId{activeChan},
                "No connected servers to send profile to.");
    });
}


static QIcon makeTopicIcon(const QColor &color)
{
    const int sz = 14;
    QPixmap pix(sz, sz);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    QPen pen(color, 1.3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    // Speech bubble body
    p.drawRoundedRect(QRectF(1.0, 1.0, 11.0, 8.5), 2.0, 2.0);
    // Tail pointing bottom-left
    QPolygonF tail;
    tail << QPointF(2.5, 9.5) << QPointF(1.0, 13.0) << QPointF(5.5, 9.5);
    p.drawPolyline(tail);
    // Two text lines inside the bubble
    p.setPen(QPen(color, 1.0, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(3.5, 4.0), QPointF(9.5, 4.0));
    p.drawLine(QPointF(3.5, 6.5), QPointF(7.5, 6.5));
    return QIcon(pix);
}

static QIcon makeGearIcon(int angleDeg, const QColor &color)
{
    QSvgRenderer renderer(QStringLiteral(":/icons/settings.svg"));
    QPixmap pix(20, 20);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    if (angleDeg != 0) {
        p.translate(10, 10);
        p.rotate(angleDeg);
        p.translate(-10, -10);
    }
    renderer.render(&p);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(pix.rect(), color);
    p.end();
    return QIcon(pix);
}

void MainWindow::applyFontSizes()
{
    const QString &fam = m_config.ui.fontFamily;
    const FontSizes &fs = m_config.ui.fontSizes;

    auto makeFont = [&](int pt) {
        QFont f;
        f.setFamilies({fam,
                       QStringLiteral("Noto Color Emoji"),
                       QStringLiteral("Segoe UI Emoji"),
                       QStringLiteral("Apple Color Emoji")});
        f.setPointSize(pt);
        return f;
    };

    if (m_appLabel) {
        QFont f = makeFont(fs.toolbar);
        f.setBold(true);
        m_appLabel->setFont(f);
    }
    if (m_sidebar) {
        m_sidebar->setFont(makeFont(fs.sidebar));
        // Update server header items with their dedicated font size
        for (int i = 0; i < m_sidebar->topLevelItemCount(); ++i) {
            auto *srv = m_sidebar->topLevelItem(i);
            QFont f = makeFont(fs.serverHeader);
            f.setBold(true);
            srv->setFont(0, f);
        }
    }
    if (m_chatView)       m_chatView->setFont(makeFont(fs.chat));
    if (m_nickList)       m_nickList->setFont(makeFont(fs.nickList));
    if (m_nickPanel)      m_nickPanel->setFont(makeFont(fs.nickDock));
    if (m_nickToggleBtn)
        m_nickToggleBtn->setIcon(
            makeGearIcon(0, QColor(m_theme.valid ? m_theme.text : "#ffffff")));
    if (m_sidebarToggleBtn)
        m_sidebarToggleBtn->setIcon(
            makeGearIcon(0, QColor(m_theme.valid ? m_theme.text : "#ffffff")));
    if (m_hamburger)
        m_hamburger->setIcon(
            makeMenuIcon(QColor(m_theme.valid ? m_theme.text : "#ffffff")));
    if (m_topicLabel)    m_topicLabel->setFont(makeFont(fs.topicBar));
    if (m_modesLabel)    m_modesLabel->setFont(makeFont(fs.topicBar));
    if (m_userInfoLabel) m_userInfoLabel->setFont(makeFont(fs.topicBar));
    if (m_nickPrefix)   m_nickPrefix->setFont(makeFont(fs.inputNick));
    if (m_input)        m_input->setFont(makeFont(fs.input));
    for (auto *p : std::as_const(m_orderedPanes)) {
        const QFont chatFont = makeFont(fs.chat);
        p->chatView()->setFont(chatFont);
        p->nickList()->setFont(makeFont(fs.nickList));
        p->setTopicFont(makeFont(fs.topicBar));
        p->setInputFont(makeFont(fs.inputNick), makeFont(fs.input));
    }
    if (m_typingLabel) {
        QFont f = makeFont(fs.typing);
        f.setItalic(true);
        m_typingLabel->setFont(f);
    }
}

static QIcon makeMenuIcon(const QColor &color)
{
    QSvgRenderer renderer(QStringLiteral(":/icons/menu.svg"));
    QPixmap pix(20, 20);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    renderer.render(&p);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(pix.rect(), color);
    p.end();
    return QIcon(pix);
}

static QIcon makeConnectedIcon()
{
    return MenuIcons::connectedServer();
}

void MainWindow::setupSidebar()
{
    m_sidebar = new QTreeWidget;
    m_sidebar->setVerticalScrollBar(new FadeScrollBar(Qt::Vertical, m_sidebar));
    m_sidebar->setHeaderHidden(true);
    m_sidebar->setRootIsDecorated(false);
    m_sidebar->setItemsExpandable(false);
    m_sidebar->setIndentation(8);
    m_sidebar->setMinimumWidth(112);
    m_sidebar->setObjectName("sidebar");
    m_sidebarDelegate = new SidebarDelegate(m_sidebar);
    if (m_theme.valid)
        m_sidebarDelegate->setColors(QColor(m_theme.accent),
                                     QColor(m_theme.border),
                                     QColor(m_theme.text));
    m_sidebar->setItemDelegate(m_sidebarDelegate);

    connect(m_sidebar, &QTreeWidget::itemClicked,
            this, [this](QTreeWidgetItem *, int){ onSidebarSelectionChanged(); });
    m_sidebar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_sidebar, &QTreeWidget::customContextMenuRequested,
            this, &MainWindow::onSidebarContextMenu);

    m_sidebarToggleBtn = new QToolButton;
    m_sidebarToggleBtn->setFixedSize(28, 28);
    m_sidebarToggleBtn->setAutoRaise(true);
    m_sidebarToggleBtn->setObjectName("sidebarToggleBtn");
    m_sidebarToggleBtn->setToolTip(tr("Toggle channel list"));
    m_sidebarToggleBtn->setIcon(makeGearIcon(0, QColor(m_theme.valid ? m_theme.text : "#ffffff")));
    connect(m_sidebarToggleBtn, &QToolButton::clicked, this, [this]{
        m_sidebarExpanded = !m_sidebarExpanded;
        m_sidebar->setVisible(m_sidebarExpanded);
        const QList<int> sizes = m_mainSplitter->sizes();
        const int total = sizes[0] + sizes[1];
        if (m_sidebarExpanded) {
            m_mainSplitter->setSizes({m_sidebarExpandedWidth, total - m_sidebarExpandedWidth});
            if (m_topicLeft) m_topicLeft->setFixedWidth(m_sidebarExpandedWidth);
            if (auto *l = qobject_cast<QVBoxLayout *>(m_rightContent->layout()))
                l->setContentsMargins(8, 0, 8, 8);
        } else {
            m_mainSplitter->setSizes({0, total});
            if (m_topicLeft) m_topicLeft->setFixedWidth(kBtnZoneMinW);
            if (auto *l = qobject_cast<QVBoxLayout *>(m_rightContent->layout()))
                l->setContentsMargins(8, 0, 8, 8);
        }
    });

    m_sidebarPanel = new QWidget;
    m_sidebarPanel->setObjectName("sidebarPanel");
    auto *vbox = new QVBoxLayout(m_sidebarPanel);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(0);

    vbox->addWidget(m_sidebar, 1);
}

void MainWindow::setupNickPanel()
{
    m_nickList = new QListWidget;
    m_nickList->setVerticalScrollBar(new FadeScrollBar(Qt::Vertical, m_nickList));
    m_nickList->setSpacing(0);
    m_nickDelegate = new NickDelegate(m_nickList);
    if (m_theme.valid)
        m_nickDelegate->setColors(QColor(m_theme.accent),
                                  QColor(m_theme.border),
                                  QColor(m_theme.text));
    m_nickList->setItemDelegate(m_nickDelegate);
    m_nickList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_nickList, &QListWidget::customContextMenuRequested,
            this, &MainWindow::onNickListContextMenu);

    m_nickCountLabel = new QLabel(QStringLiteral("0 users"));
    m_nickCountLabel->setObjectName("nickCountLabel");
    m_nickCountLabel->setContentsMargins(4, 0, 4, 0);

    m_nickToggleBtn = new QToolButton;
    m_nickToggleBtn->setFixedSize(28, 28);
    m_nickToggleBtn->setAutoRaise(true);
    m_nickToggleBtn->setToolTip(tr("Toggle user list"));
    m_nickToggleBtn->setIcon(makeGearIcon(0, QColor(m_theme.valid ? m_theme.text : "#ffffff")));

    m_gearTimer = new QTimer(this);
    m_gearTimer->setInterval(16);
    connect(m_gearTimer, &QTimer::timeout, this, [this]{
        m_gearAngle += 12;
        m_nickToggleBtn->setIcon(
            makeGearIcon(m_gearAngle, m_nickToggleBtn->palette().color(QPalette::WindowText)));
        if (m_gearAngle < 360) return;

        m_gearTimer->stop();
        m_gearAngle = 0;
        m_nickExpanded = !m_nickExpanded;
        m_nickList->setVisible(m_nickExpanded);

        m_nickToggleBtn->setIcon(
            makeGearIcon(0, m_nickToggleBtn->palette().color(QPalette::WindowText)));
    });

    connect(m_nickToggleBtn, &QToolButton::clicked, this, [this]{
        if (m_gearTimer->isActive()) return;
        m_gearAngle = 0;
        m_gearTimer->start();
    });

    auto *header = new QWidget;
    header->setObjectName("nickPanelHeader");
    auto *hbox = new QHBoxLayout(header);
    hbox->setContentsMargins(2, 2, 2, 2);
    hbox->setSpacing(2);
    hbox->addWidget(m_nickToggleBtn);
    hbox->addWidget(m_nickCountLabel, 1);

    m_nickPanel = new QWidget;
    m_nickPanel->setObjectName("nickPanel");
    m_nickPanel->setMinimumWidth(24);
    auto *vbox = new QVBoxLayout(m_nickPanel);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(0);
    vbox->addWidget(header);
    vbox->addWidget(m_nickList, 100);
    vbox->addStretch(1);
}

void MainWindow::setupChatArea()
{
    // Single full-width bar spanning above sidebar AND chat, with border below
    m_topicBar = new QWidget;
    m_topicBar->setObjectName("topicBar");
    auto *tHbox = new QHBoxLayout(m_topicBar);
    tHbox->setContentsMargins(0, 0, 0, 0);
    tHbox->setSpacing(0);

    // Left zone — mirrors sidebar width: hamburger left, gear right
    m_topicLeft = new QWidget;
    m_topicLeft->setObjectName("topicLeftZone");
    m_topicLeft->setFixedWidth(m_sidebarExpandedWidth);
    auto *tlHbox = new QHBoxLayout(m_topicLeft);
    tlHbox->setContentsMargins(0, 4, 4, 4);
    tlHbox->setSpacing(0);
    tlHbox->addWidget(m_hamburger);
    tlHbox->addStretch(1);
    tlHbox->addWidget(m_sidebarToggleBtn);

    // Right zone — mirrors chat area: signal bars + channel info
    auto *topicRight = new QWidget;
    topicRight->setObjectName("topicRightZone");
    auto *trHbox = new QHBoxLayout(topicRight);
    trHbox->setContentsMargins(8, 4, 8, 4);
    trHbox->setSpacing(6);

    m_topicLabel = new QLabel;
    m_topicLabel->setObjectName("channelLabel");
    m_modesLabel = new QLabel;
    m_modesLabel->setObjectName("modesLabel");
    m_userInfoLabel = new QLabel;
    m_userInfoLabel->setObjectName("userInfoLabel");
    m_signalBars = new SignalBars(topicRight);

    trHbox->addWidget(m_signalBars);
    trHbox->addWidget(m_topicLabel);
    trHbox->addWidget(m_userInfoLabel);
    trHbox->addWidget(m_modesLabel, 1);

    tHbox->addWidget(m_topicLeft);
    tHbox->addWidget(topicRight, 1);

    // Right content — holds the panes splitter only
    m_rightContent = new QWidget;
    m_rightContent->setObjectName("rightContent");
    auto *vbox     = new QVBoxLayout(m_rightContent);
    vbox->setContentsMargins(8, 0, 8, 8);
    vbox->setSpacing(0);

    // Primary panel — first column in the panes splitter
    m_primaryPanel = new QWidget;
    auto *primaryVbox = new QVBoxLayout(m_primaryPanel);
    primaryVbox->setContentsMargins(0, 0, 0, 0);
    primaryVbox->setSpacing(0);

    // Primary panel header: hidden until the first extra pane is opened
    m_primaryHeader = new QWidget;
    m_primaryHeader->setObjectName("paneHeader");
    m_primaryHeader->setVisible(true);
    auto *primaryHeader = m_primaryHeader;
    {
        auto *hbox = new QHBoxLayout(primaryHeader);
        hbox->setContentsMargins(6, 3, 4, 3);
        hbox->setSpacing(6);

        m_primaryTopicBtn = new QToolButton;
        m_primaryTopicBtn->setObjectName("topicToggle");
        m_primaryTopicBtn->setCheckable(true);
        m_primaryTopicBtn->setChecked(m_showTopic);
        m_primaryTopicBtn->setText({});
        m_primaryTopicBtn->setIcon(makeTopicIcon(
            QColor(m_theme.valid ? m_theme.placeholder : "#888888")));
        m_primaryTopicBtn->setIconSize(QSize(14, 14));
        m_primaryTopicBtn->setAutoRaise(false);
        connect(m_primaryTopicBtn, &QToolButton::toggled, this, [this](bool on){
            m_topicDisplay->setVisible(on);
            if (m_theme.valid)
                m_primaryTopicBtn->setIcon(makeTopicIcon(
                    QColor(on ? m_theme.accent : m_theme.placeholder)));
        });

        m_primaryPaneLabel = new QLabel;
        m_primaryPaneLabel->setObjectName("paneChannelLabel");
        {
            QFont f = m_primaryPaneLabel->font();
            f.setBold(true);
            m_primaryPaneLabel->setFont(f);
        }

        m_primaryCloseBtn = new QToolButton;
        m_primaryCloseBtn->setText(QStringLiteral("✕"));
        m_primaryCloseBtn->setFixedSize(16, 16);
        m_primaryCloseBtn->setStyleSheet(
            "QToolButton { background: transparent; border: none; padding: 0px; }"
            "QToolButton:hover { color: palette(highlight); }"
        );
        m_primaryCloseBtn->setVisible(false);
        connect(m_primaryCloseBtn, &QToolButton::clicked, this, [this]{
            m_primaryPanel->hide();
        });

        hbox->addWidget(m_primaryTopicBtn);
        hbox->addWidget(m_primaryPaneLabel, 1);
        hbox->addWidget(m_primaryCloseBtn);
    }
    // chatSection holds the header, topic, and chat+nick splitter.
    // It lives to the right of the sidebar in m_mainSplitter so those
    // widgets only span the chat column, not the sidebar column.
    auto *chatSection = new QWidget;
    auto *chatVbox    = new QVBoxLayout(chatSection);
    chatVbox->setContentsMargins(0, 0, 0, 0);
    chatVbox->setSpacing(0);
    chatVbox->addWidget(primaryHeader);

    // Topic display — shown below header when Show Topic is on
    m_topicDisplay = new QWidget;
    auto *tdHbox   = new QHBoxLayout(m_topicDisplay);
    tdHbox->setContentsMargins(8, 3, 8, 3);
    m_topicText = new QLabel;
    m_topicText->setObjectName("topicText");
    m_topicText->setWordWrap(true);
    m_topicText->setTextFormat(Qt::RichText);
    m_topicText->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_topicText->setOpenExternalLinks(false);
    connect(m_topicText, &QLabel::linkActivated, this, [](const QString &link){
        const QUrl u(link);
        const QString s = u.scheme().toLower();
        if (s == "http" || s == "https")
            QDesktopServices::openUrl(u);
    });
    tdHbox->addWidget(m_topicText);
    m_topicDisplay->setObjectName("topicDisplay");
    m_topicDisplay->setVisible(m_showTopic);
    chatVbox->addWidget(m_topicDisplay);

    // Chat view
    m_chatView = new ChatView;
    if (m_theme.valid)
        m_chatView->setColors(QColor(m_theme.text), QColor(m_theme.background),
                              QColor(m_theme.accent), QColor(m_theme.background),
                              QColor(m_theme.border));
    connect(m_chatView, &ChatView::anchorActivated,
            this, [this](const QString &anchor, const QPoint &gp, Qt::MouseButton btn){
        if (btn == Qt::LeftButton) {
            if (anchor.startsWith(QLatin1String("evgrp:"))) {
                toggleEventGroupInView(m_chatView, anchor.mid(6),
                                       m_model->activeHost().str(), m_model->activeChannel().str());
                return;
            }
            QString href = anchor;
            if (href.startsWith("url:"))  href = href.mid(4);
            if (href.startsWith("preview:")) href = href.mid(8);
            const QUrl u(href);
            const QString s = u.scheme().toLower();
            if (s == "http" || s == "https") QDesktopServices::openUrl(u);
        } else if (btn == Qt::RightButton) {
            handleChatViewContextMenu(m_chatView, anchor, gp,
                                      m_model->activeHost().str(), m_model->activeChannel().str());
        }
    });
    connect(m_chatView, &ChatView::anchorHovered, this, [this](const QString &anchor){
        if (anchor.isEmpty()) {
            if (!m_hoveredUrl.isEmpty()) {
                m_hoveredUrl.clear();
                QToolTip::hideText();
                statusBar()->clearMessage();
            }
            return;
        }
        if (anchor.startsWith("nick:")) {
            if (m_hoveredUrl == anchor) return;
            m_hoveredUrl = anchor;
            const QString nick = anchor.mid(5);
            const QString tip = nickTooltip(nick, m_model->activeHost().str());
            if (!tip.isEmpty()) {
                m_hoverGlobalPos = QCursor::pos();
                QToolTip::showText(m_hoverGlobalPos, tip, m_chatView->viewport());
            }
            return;
        }
        QString href = anchor;
        if (href.startsWith("url:"))     href = href.mid(4);
        if (href.startsWith("preview:")) href = href.mid(8);
        if (href == m_hoveredUrl) return;
        m_hoveredUrl = href;
        const QUrl url(href);
        statusBar()->showMessage(url.host());
        m_hoverGlobalPos = QCursor::pos();
        QToolTip::showText(m_hoverGlobalPos, url.host(), m_chatView->viewport());
        if (m_config.ui.linkPreviews) m_linkPreview->fetchHover(url);
    });
    connect(m_chatView, &ChatView::loadOlderRequested, this, &MainWindow::loadOlderMessages);
    m_chatView->installEventFilter(this);

    m_linkPreview = new LinkPreview(this);

    m_previewWatchdog = new QTimer(this);
    m_previewWatchdog->setSingleShot(true);
    connect(m_previewWatchdog, &QTimer::timeout, this, [this]{
        m_previewFetchBusy = false;
        processPreviewQueue();
    });

    connect(m_linkPreview, &LinkPreview::titleReady, this, [this](const QUrl &url, const QString &title){
        if (url.toString() != m_hoveredUrl) return;
        const QString display = title.length() > 80 ? title.left(79) + QChar(0x2026) : title;
        statusBar()->showMessage(display);
        QToolTip::showText(m_hoverGlobalPos, display, m_chatView->viewport());
    });

    connect(m_linkPreview, &LinkPreview::cardReady, this,
            [this](const QUrl &pageUrl, const QString &title, const QPixmap &thumbnail){
        const QString urlStr = pageUrl.toString();
        m_previewWatchdog->stop();
        m_previewFetchBusy = false;

        auto it = m_previewChannels.find(urlStr);
        if (it == m_previewChannels.end()) {
            processPreviewQueue();
            return;
        }
        const QString host    = it->host;
        const QString channel = it->channel;
        const QString msgid   = it->msgid;
        m_previewChannels.erase(it);
        processPreviewQueue();

        auto *ch = m_model->channel(ServerId{host}, BufferId{channel});
        if (!ch) return;

        // Thumbnail scaled to max 100 px wide
        QPixmap thumb;
        if (!thumbnail.isNull())
            thumb = thumbnail.scaledToWidth(qMin(thumbnail.width(), 240),
                                            Qt::SmoothTransformation);

        Channel::PreviewCard card;
        card.title     = title.left(120);
        card.domain    = pageUrl.host();
        card.pageUrl   = urlStr;
        card.thumbnail = thumb;
        ch->addPreview(urlStr, card);

        auto makeCardLine = [&]() -> ChatLine {
            ChatLine line;
            line.id   = "preview:" + urlStr;
            line.role = ChatLineRole::PreviewCard;
            line.image = thumb;
            line.text = card.title + "\n" + card.domain;
            QTextCharFormat titleFmt;
            titleFmt.setFontWeight(QFont::Bold);
            ChatSegment titleSeg;
            titleSeg.start  = 0;
            titleSeg.length = static_cast<int>(card.title.size());
            titleSeg.format = titleFmt;
            titleSeg.anchor = "preview:" + urlStr;
            line.segments.append(titleSeg);
            QTextCharFormat domainFmt;
            domainFmt.setForeground(QColor("#888888"));
            ChatSegment domainSeg;
            domainSeg.start  = static_cast<int>(card.title.size()) + 1;
            domainSeg.length = static_cast<int>(card.domain.size());
            domainSeg.format = domainFmt;
            line.segments.append(domainSeg);
            return line;
        };

        const bool isActive = (host == m_model->activeHost().str() &&
                               channel.toLower() == m_model->activeChannel().str().toLower());
        if (isActive) {
            const bool atBottom = m_chatView->isAtBottom();
            if (!msgid.isEmpty() && m_chatView->findLine(msgid) >= 0)
                m_chatView->insertAfter(msgid, makeCardLine());
            else
                m_chatView->appendLine(makeCardLine());
            if (atBottom) m_chatView->scrollToBottom();
        }

        const QString paneKey = host + "|" + channel.toLower();
        if (auto *pane = m_panes.value(paneKey)) {
            const bool atBottom = pane->chatView()->isAtBottom();
            ChatView *cv = pane->chatView();
            if (!msgid.isEmpty() && cv->findLine(msgid) >= 0)
                cv->insertAfter(msgid, makeCardLine());
            else
                cv->appendLine(makeCardLine());
            if (atBottom) pane->chatView()->scrollToBottom();
        }
    });

    m_chatSplitter = new QSplitter(Qt::Horizontal);
    m_chatSplitter->setHandleWidth(0);
    m_chatSplitter->addWidget(m_chatView);
    m_chatSplitter->addWidget(m_nickPanel);
    m_chatSplitter->setStretchFactor(0, 1);
    m_chatSplitter->setStretchFactor(1, 0);
    chatVbox->addWidget(m_chatSplitter, 1);

    // Sidebar sits alongside chatSection so header/topic stay in the chat
    // column while only the inputBar (added by setupInputBar) spans full width,
    // letting the RoundedPane clip only outer corners.
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    m_mainSplitter->setHandleWidth(0);
    m_mainSplitter->addWidget(m_sidebarPanel);
    m_mainSplitter->addWidget(chatSection);
    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 1);
    m_mainSplitter->setMinimumSize(1, 1);
    primaryVbox->addWidget(m_mainSplitter, 1);

    // setupInputBar will append search/reply/typing/input into primaryVbox

    m_panesSplitter = new QSplitter(Qt::Horizontal);
    m_panesSplitter->setHandleWidth(2);
    m_panesSplitter->addWidget(m_primaryPanel);
    m_panesSplitter->setStretchFactor(0, 1);

    auto *chatWrapper = new RoundedPane;
    auto *cwLayout    = new QVBoxLayout(chatWrapper);
    cwLayout->setContentsMargins(0, 0, 0, 0);
    cwLayout->setSpacing(0);
    cwLayout->addWidget(m_panesSplitter);
    vbox->addWidget(chatWrapper, 1);

    auto *outer      = new QWidget;
    auto *outerVbox  = new QVBoxLayout(outer);
    outerVbox->setContentsMargins(0, 0, 0, 0);
    outerVbox->setSpacing(0);
    outerVbox->addWidget(m_topicBar);
    outerVbox->addWidget(m_rightContent, 1);

    setCentralWidget(outer);
}

void MainWindow::ensureEmojiPicker()
{
    if (m_emojiPicker) return;
    m_emojiPicker = new EmojiPicker(this);
    connect(m_emojiPicker, &EmojiPicker::emojiSelected, this, [this](const QString &emoji){
        if (!m_pendingReactMsgid.isEmpty()) {
            m_model->sendReact(ServerId{m_pendingReactHost}, BufferId{m_pendingReactChannel},
                               m_pendingReactMsgid, emoji);
            m_pendingReactMsgid.clear();
            m_pendingReactHost.clear();
            m_pendingReactChannel.clear();
            return;
        }
        QTextCursor tc = m_input->textCursor();
        tc.insertText(emoji);
        m_input->setTextCursor(tc);
        m_input->setFocus();
    });
}

void MainWindow::setupInputBar()
{
    auto *bar  = new QWidget;
    auto *hbox = new QHBoxLayout(bar);
    hbox->setContentsMargins(4, 3, 4, 8);
    hbox->setSpacing(4);

    m_nickPrefix = new QLabel;
    m_nickPrefix->setStyleSheet("font-weight: bold; padding-right: 4px;");
    m_nickPrefix->setVisible(m_showNickPrefix);

    m_input = new QPlainTextEdit;
    m_input->setPlaceholderText("Type a message...");
    m_input->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_input->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_input->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_input->document()->setDocumentMargin(2);
    m_input->setFixedHeight(m_input->fontMetrics().lineSpacing() + 10);
    m_input->installEventFilter(this);

    m_emojiBtn = new QPushButton("😊");
    m_emojiBtn->setFixedSize(30, 30);
    m_emojiBtn->setStyleSheet("QPushButton { padding: 0; font-size: 16px; }");
    m_emojiBtn->setVisible(m_showEmojiBtn);
    m_emojiBtn->setToolTip("Emoji picker");

    // Send button floats inside the right edge of the input widget.
    m_sendBtn = new QToolButton(m_input);
    m_sendBtn->setFixedSize(28, 28);
    m_sendBtn->setAutoRaise(true);
    m_sendBtn->setToolTip("Send");
    m_sendBtn->setIcon(MenuIcons::send({}, 26));
    m_sendBtn->setIconSize(QSize(26, 26));
    m_sendBtn->setEnabled(false);
    m_sendBtn->setVisible(m_config.ui.showSendButton);
    connect(m_sendBtn, &QToolButton::clicked, this, &MainWindow::onInputSubmit);

    // Push text content left so it doesn't flow under the floating button
    {
        QTextFrameFormat fmt = m_input->document()->rootFrame()->frameFormat();
        fmt.setRightMargin(36);
        m_input->document()->rootFrame()->setFrameFormat(fmt);
    }

    hbox->addWidget(m_nickPrefix);
    hbox->addWidget(m_input, 1);
    hbox->addWidget(m_emojiBtn);

    bar->setObjectName("inputBar");

    m_typingLabel = new QLabel;
    m_typingLabel->setObjectName("typingLabel");
    m_typingLabel->setContentsMargins(8, 2, 8, 2);
    m_typingLabel->setVisible(m_config.ui.typingIndicator);

    // Search bar (Ctrl+F)
    m_searchBar = new QWidget;
    m_searchBar->setObjectName("searchBar");
    {
        auto *shbox = new QHBoxLayout(m_searchBar);
        shbox->setContentsMargins(4, 2, 4, 2);
        shbox->setSpacing(4);
        m_searchInput = new QLineEdit;
        m_searchInput->setPlaceholderText("Search in buffer…");
        m_searchInput->installEventFilter(this);
        connect(m_searchInput, &QLineEdit::textChanged, this, [this](const QString &text){
            if (text.isEmpty()) m_chatView->clearFind();
            else m_chatView->findText(text, false);
        });
        auto *closeBtn = new QToolButton;
        closeBtn->setText("✕");
        closeBtn->setFixedSize(22, 22);
        closeBtn->setAutoRaise(true);
        connect(closeBtn, &QToolButton::clicked, this, [this]{
            m_searchBar->hide();
            m_chatView->clearFind();
            m_input->setFocus();
        });
        shbox->addWidget(m_searchInput, 1);
        shbox->addWidget(closeBtn);
    }
    m_searchBar->hide();

    // Reply indicator bar
    m_replyBar = new QWidget;
    m_replyBar->setObjectName("replyBar");
    {
        auto *rhbox = new QHBoxLayout(m_replyBar);
        rhbox->setContentsMargins(8, 2, 4, 2);
        rhbox->setSpacing(4);
        m_replyLabel = new QLabel;
        m_replyLabel->setObjectName("replyLabel");
        auto *closeBtn = new QToolButton;
        closeBtn->setText("✕");
        closeBtn->setFixedSize(18, 18);
        closeBtn->setAutoRaise(true);
        connect(closeBtn, &QToolButton::clicked, this, &MainWindow::clearReplyBar);
        rhbox->addWidget(m_replyLabel, 1);
        rhbox->addWidget(closeBtn);
    }
    m_replyBar->hide();

    auto *layout = qobject_cast<QVBoxLayout *>(m_primaryPanel->layout());
    layout->addWidget(m_searchBar);
    layout->addWidget(m_replyBar);
    layout->addWidget(m_typingLabel);
    layout->addWidget(bar);

    connect(m_emojiBtn, &QPushButton::clicked, this, [this]{
        const QPoint anchor = m_emojiBtn->mapToGlobal(
            QPoint(m_emojiBtn->width(), m_emojiBtn->height()));
        ensureEmojiPicker();
        m_emojiPicker->showAt(anchor);
    });

    // Emoji inline autocomplete list (child widget, no focus steal)
    m_emojiCompleter = new QListWidget(this);
    m_emojiCompleter->setObjectName("emojiCompleter");
    m_emojiCompleter->setFocusPolicy(Qt::NoFocus);
    m_emojiCompleter->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_emojiCompleter->setFrameShape(QFrame::StyledPanel);
    m_emojiCompleter->hide();
    connect(m_emojiCompleter, &QListWidget::itemClicked, this, [this](QListWidgetItem *item){
        commitEmojiAutocomplete(m_emojiCompleter->row(item));
    });

    // Inactivity timer: sends typing=paused after 5s with no keypresses
    m_typingOutTimer = new QTimer(this);
    m_typingOutTimer->setSingleShot(true);
    m_typingOutTimer->setInterval(5000);
    connect(m_typingOutTimer, &QTimer::timeout, this, [this]{
        if (!m_config.ui.typingIndicator) return;
        const QString host = m_model->activeHost().str();
        const QString ch   = m_model->activeChannel().str();
        if (ch.isEmpty() || ch == "(server)") return;
        m_typingActive = false;
        m_model->sendTyping(ServerId{host}, BufferId{ch}, "paused");
    });

    connect(m_input, &QPlainTextEdit::textChanged, this, [this]{
        const QString text = m_input->toPlainText();
        m_sendBtn->setEnabled(!text.trimmed().isEmpty());
        checkEmojiAutocomplete(text);
        // Auto-resize: 1 to 4 lines
        const int lineH = m_input->fontMetrics().lineSpacing();
        const int margins = m_input->contentsMargins().top() + m_input->contentsMargins().bottom() + 8;
        const int lines = qMin(4, static_cast<int>(text.count('\n')) + 1);
        m_input->setFixedHeight(lines * lineH + margins);
        if (!m_config.ui.typingIndicator) return;
        const QString host = m_model->activeHost().str();
        const QString ch   = m_model->activeChannel().str();
        if (ch.isEmpty() || ch == "(server)") return;
        if (!text.isEmpty()) {
            if (!m_typingActive) {
                m_typingActive = true;
                m_model->sendTyping(ServerId{host}, BufferId{ch}, "active");
            }
            m_typingOutTimer->start();
        } else {
            m_typingOutTimer->stop();
            if (m_typingActive) {
                m_typingActive = false;
                m_model->sendTyping(ServerId{host}, BufferId{ch}, "done");
            }
        }
    });

    QTimer::singleShot(0, this, [this]{ repositionSendBtn(); });
}

void MainWindow::connectModel()
{
    connect(m_model, &SessionModel::serverAdded, this,
            [this](ServerId id){ onServerAdded(id.str()); });
    connect(m_model, &SessionModel::serverConnected, this,
            [this](ServerId id){ onServerConnected(id.str()); });
    connect(m_model, &SessionModel::serverDisconnected, this,
            [this](ServerId id){ onServerDisconnected(id.str()); });
    connect(m_model, &SessionModel::channelAdded, this,
            [this](ServerId h, BufferId ch){ onChannelAdded(h.str(), ch.str()); });
    connect(m_model, &SessionModel::channelRemoved, this,
            [this](ServerId h, BufferId ch){ onChannelRemoved(h.str(), ch.str()); });
    connect(m_model, &SessionModel::messageAdded, this,
            [this](ServerId h, BufferId ch, const Message &msg){ onMessageAdded(h.str(), ch.str(), msg); });
    connect(m_model, &SessionModel::topicChanged, this,
            [this](ServerId h, BufferId ch, const QString &t){ onTopicChanged(h.str(), ch.str(), t); });
    connect(m_model, &SessionModel::modesChanged, this, [this](ServerId h, BufferId ch){
        if (h == m_model->activeHost() && ch.str().toLower() == m_model->activeChannel().str().toLower())
            refreshTopicBar(h.str(), ch.str());
    });
    connect(m_model, &SessionModel::nickListChanged, this,
            [this](ServerId h, BufferId ch){ onNickListChanged(h.str(), ch.str()); });
    connect(m_model, &SessionModel::nickAdded, this,
            [this](ServerId h, BufferId ch, const QString &n){ onNickAdded(h.str(), ch.str(), n); });
    connect(m_model, &SessionModel::nickRemoved, this,
            [this](ServerId h, BufferId ch, const QString &n){ onNickRemoved(h.str(), ch.str(), n); });
    connect(m_model, &SessionModel::nickRenamed, this,
            [this](ServerId h, BufferId ch, const QString &o, const QString &n){ onNickRenamed(h.str(), ch.str(), o, n); });
    connect(m_model, &SessionModel::unreadChanged, this,
            [this](ServerId h, BufferId ch, int c){ onUnreadChanged(h.str(), ch.str(), c); });
    connect(m_model, &SessionModel::reactionsChanged, this,
            [this](ServerId h, BufferId ch, const QString &id){ onReactionsChanged(h.str(), ch.str(), id); });
    connect(m_model, &SessionModel::selfNickChanged, this,
            [this](ServerId h, const QString &n){ onSelfNickChanged(h.str(), n); });
    connect(m_model, &SessionModel::typingReceived, this,
            [this](ServerId h, BufferId ch, const QString &n, const QString &s){ onTypingReceived(h.str(), ch.str(), n, s); });
    connect(m_model, &SessionModel::messageRedacted, this,
            [this](ServerId h, BufferId ch, const QString &id){ onMessageRedacted(h.str(), ch.str(), id); });
    connect(m_model, &SessionModel::userMetaChanged, this,
            [this](ServerId, const QString &, const QString &key, const QString &value) {
        if (key == QLatin1String("avatar")) fetchAvatar(value);
    });

    connect(m_model, &SessionModel::sslFingerprintPrompt, this,
            [this](ServerId host, const QString &fp)
    {
        QMessageBox box(this);
        box.setWindowTitle("Untrusted Certificate");
        box.setText(host.str() + " is using a self-signed certificate.");
        box.setInformativeText("SHA-256 fingerprint:\n" + fp + "\n\nTrust this certificate?");
        auto *pinBtn  = box.addButton("Pin Certificate", QMessageBox::AcceptRole);
        auto *onceBtn = box.addButton("Accept Once",     QMessageBox::AcceptRole);
        box.addButton("Reject",          QMessageBox::RejectRole);
        box.exec();
        if (box.clickedButton() == pinBtn)
            m_model->pinCertificate(host, fp);
        else if (box.clickedButton() == onceBtn)
            m_model->acceptCertificateOnce(host, fp);
        // Reject: connection already aborted in IrcClient::onSslErrors
    });

    connect(m_model, &SessionModel::dccSendReceived, this,
            [this](ServerId, const QString &fromNick,
                   const QString &filename, quint32 ip, quint16 port, qint64 filesize)
    {
        const QString sizeStr = filesize >= 1024*1024
            ? QString::number(filesize / (1024*1024)) + " MB"
            : QString::number(filesize / 1024) + " KB";
        const QString ipStr = QHostAddress(ip).toString();

        const int ret = QMessageBox::question(this, "Incoming DCC File",
            "Sender: " + fromNick + "\n"
            "File: " + filename + "\n"
            "Size: " + sizeStr + "\n"
            "Address: " + ipStr + ":" + QString::number(port) + "\n\n"
            "DCC connects directly to the sender and may reveal your IP address.\n"
            "Only accept files from people you trust.\n\nAccept?",
            QMessageBox::Yes | QMessageBox::No);
        if (ret != QMessageBox::Yes) return;

        const QString savePath = QFileDialog::getSaveFileName(
            this, "Save File", QFileInfo(filename).fileName());
        if (savePath.isEmpty()) return;

        auto *dcc  = new DccReceive(savePath, ip, port, filesize, this);
        auto *prog = new QProgressDialog("Receiving " + filename + " from " + fromNick,
                                          "Cancel", 0, filesize > INT_MAX ? INT_MAX : static_cast<int>(filesize), this);
        prog->setWindowModality(Qt::NonModal);
        prog->setAttribute(Qt::WA_DeleteOnClose);

        connect(dcc, &DccReceive::progress, prog, [prog, filesize](qint64 received, qint64){
            prog->setValue(static_cast<int>(filesize > INT_MAX
                ? received * INT_MAX / filesize : received));
        });
        connect(dcc, &DccReceive::finished, this, [this, prog, dcc](const QString &path){
            prog->setValue(prog->maximum());
            dcc->deleteLater();
            QMessageBox::information(this, "DCC", "File received:\n" + path);
        });
        connect(dcc, &DccReceive::error, this, [this, prog, dcc](const QString &msg){
            prog->close();
            dcc->deleteLater();
            QMessageBox::warning(this, "DCC Error", msg);
        });
        connect(prog, &QProgressDialog::canceled, dcc, [dcc]{ dcc->cancel(); dcc->deleteLater(); });

        dcc->start();
        prog->show();
    });

    connect(m_model, &SessionModel::dccPassiveOfferReceived, this,
            [this](ServerId server, const QString &fromNick,
                   const QString &filename, quint32 senderIp, qint64 filesize, const QString &token)
    {
        const QString sizeStr = filesize >= 1024*1024
            ? QString::number(filesize / (1024*1024)) + " MB"
            : QString::number(filesize / 1024) + " KB";
        const QString ipStr = QHostAddress(senderIp).toString();

        const int ret = QMessageBox::question(this, "Incoming DCC File (Passive)",
            "Sender: " + fromNick + "\n"
            "File: " + filename + "\n"
            "Size: " + sizeStr + "\n"
            "Sender address: " + ipStr + "\n\n"
            "DCC connects directly to the sender and may reveal your IP address.\n"
            "Only accept files from people you trust.\n\nAccept?",
            QMessageBox::Yes | QMessageBox::No);
        if (ret != QMessageBox::Yes) return;

        const QString savePath = QFileDialog::getSaveFileName(
            this, "Save File", QFileInfo(filename).fileName());
        if (savePath.isEmpty()) return;

        auto *dcc = new DccReceive(savePath, 0, 0, filesize, this);
        if (!dcc->listenPassive(senderIp)) { dcc->deleteLater(); return; }

        IrcClient *client = m_model->clientFor(server);
        const quint32 ourIp   = client ? client->localIpv4() : 0;
        const quint16 ourPort = dcc->listenPort();
        QString fn = QFileInfo(filename).fileName().replace(' ', '_');
        fn.remove(QRegularExpression("[\\x00-\\x1f\\x7f]"));
        if (fn.isEmpty()) fn = QStringLiteral("file");
        fn = fn.left(180);

        m_model->sendRaw(server,
            "PRIVMSG " + fromNick + " :\x01""DCC SEND "
            + fn + " " + QString::number(ourIp)
            + " " + QString::number(ourPort)
            + " " + QString::number(filesize)
            + " " + token + "\x01");

        auto *prog = new QProgressDialog("Receiving " + filename + " from " + fromNick,
                                          "Cancel", 0, filesize > INT_MAX ? INT_MAX : static_cast<int>(filesize), this);
        prog->setWindowModality(Qt::NonModal);
        prog->setAttribute(Qt::WA_DeleteOnClose);

        connect(dcc, &DccReceive::progress, prog, [prog, filesize](qint64 received, qint64){
            prog->setValue(static_cast<int>(filesize > INT_MAX ? received * INT_MAX / filesize : received));
        });
        connect(dcc, &DccReceive::finished, this, [this, prog, dcc](const QString &path){
            prog->setValue(prog->maximum());
            dcc->deleteLater();
            QMessageBox::information(this, "DCC", "File received:\n" + path);
        });
        connect(dcc, &DccReceive::error, this, [this, prog, dcc](const QString &msg){
            prog->close();
            dcc->deleteLater();
            QMessageBox::warning(this, "DCC Error", msg);
        });
        connect(prog, &QProgressDialog::canceled, dcc, [dcc]{ dcc->cancel(); dcc->deleteLater(); });
        prog->show();
    });

    connect(m_model, &SessionModel::dccPassiveSendReply, this,
            [this](ServerId, const QString &, const QString &,
                   quint32 ip, quint16 port, qint64, const QString &token)
    {
        DccSend *dcc = m_pendingPassiveSends.take(token);
        if (dcc) {
            if (isPrivateAddress(QHostAddress(ip))) {
                QMessageBox::warning(this, "DCC", "Blocked: remote address is private or reserved.");
                dcc->deleteLater();
            } else {
                dcc->connectOut(ip, port);
            }
        }
    });

    connect(m_model, &SessionModel::pingRtt, this, [this](ServerId host, int ms){
        if (m_signalBars && host == m_model->activeHost())
            m_signalBars->setLatency(ms);
    });
    connect(m_model, &SessionModel::serverReconnecting, this, [this](ServerId host){
        if (m_signalBars && host == m_model->activeHost())
            m_signalBars->setState(SignalBars::State::Reconnecting);
    });
}

// ---------------------------------------------------------------------------
// Event filter — Tab completion + input history
// ---------------------------------------------------------------------------

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (!m_chatView)
        return QMainWindow::eventFilter(obj, event);

    if (obj == m_searchInput && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Escape) {
            m_searchBar->hide();
            m_chatView->clearFind();
            m_input->setFocus();
            return true;
        }
        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
            doSearch(ke->modifiers() & Qt::ShiftModifier);
            return true;
        }
    }

    // Check if obj is a pane input bar
    if (event->type() == QEvent::KeyPress) {
        for (auto *pane : std::as_const(m_orderedPanes)) {
            if (obj == pane->input()) {
                auto *ke = static_cast<QKeyEvent *>(event);
                if (ke->key() == Qt::Key_Tab) {
                    handleTabComplete(pane->input(), pane->host(), pane->channel());
                    return true;
                }
                // Non-Tab resets the completion cycle
                m_tabActive = false;
                m_tabCandidates.clear();
                break;
            }
        }
    }

    if (obj == m_input && event->type() == QEvent::Resize) {
        repositionSendBtn();
        return QMainWindow::eventFilter(obj, event);
    }

    if (obj != m_input || event->type() != QEvent::KeyPress)
        return QMainWindow::eventFilter(obj, event);

    auto *ke = static_cast<QKeyEvent *>(event);

    // Emoji autocomplete navigation takes priority when popup is visible
    if (m_emojiCompleter->isVisible()) {
        if (ke->key() == Qt::Key_Escape) {
            hideEmojiAutocomplete();
            return true;
        }
        if (ke->key() == Qt::Key_Up) {
            const int cur = m_emojiCompleter->currentRow();
            m_emojiCompleter->setCurrentRow(qMax(0, cur - 1));
            return true;
        }
        if (ke->key() == Qt::Key_Down) {
            const int cur = m_emojiCompleter->currentRow();
            m_emojiCompleter->setCurrentRow(
                qMin(m_emojiCompleter->count() - 1, cur + 1));
            return true;
        }
        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter ||
            ke->key() == Qt::Key_Tab) {
            const int row = m_emojiCompleter->currentRow();
            if (row >= 0) {
                commitEmojiAutocomplete(row);
                return true;
            }
        }
    }

    if (ke->key() == Qt::Key_F && (ke->modifiers() & Qt::ControlModifier)) {
        showSearchBar();
        return true;
    }

    if (ke->key() == Qt::Key_Escape && !m_pendingReplyMsgid.isEmpty()) {
        clearReplyBar();
        return true;
    }

    if (ke->key() == Qt::Key_Tab) {
        handleTabComplete(m_input, m_model->activeHost().str(), m_model->activeChannel().str());
        return true;
    }

    // Any non-Tab key resets nick completion cycle
    m_tabActive = false;
    m_tabCandidates.clear();

    if (ke->key() == Qt::Key_Up && !m_emojiCompleter->isVisible()) {
        if (m_input->textCursor().blockNumber() == 0) {
            handleHistoryUp();
            return true;
        }
    }
    if (ke->key() == Qt::Key_Down && !m_emojiCompleter->isVisible()) {
        if (m_input->textCursor().blockNumber() == m_input->document()->blockCount() - 1) {
            handleHistoryDown();
            return true;
        }
    }

    if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
        if (ke->modifiers() & Qt::ShiftModifier)
            return false; // let QPlainTextEdit insert newline
        onInputSubmit();
        return true;
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::repositionSendBtn()
{
    if (!m_sendBtn || !m_input) return;
    const int pad = 3;
    const int x = m_input->width()  - m_sendBtn->width()  - pad;
    const int y = (m_input->height() - m_sendBtn->height()) / 2;
    m_sendBtn->move(x, y);
    m_sendBtn->raise();
}

void MainWindow::handleTabComplete(QPlainTextEdit *input, const QString &host, const QString &channel)
{
    const QTextCursor tc = input->textCursor();
    const QString text = tc.block().text();
    const int pos = tc.positionInBlock();

    if (!m_tabActive) {
        // Start a new cycle: derive prefix from text before cursor
        const qsizetype wordStart = text.lastIndexOf(' ', pos - 1) + 1;
        const QString prefix = text.mid(wordStart, pos - wordStart);
        if (prefix.isEmpty()) return;

        m_tabPrefix    = prefix;
        m_tabWordStart = static_cast<int>(wordStart);
        m_tabCandidates.clear();
        m_tabCandidateIndex = 0;
        m_tabActive = true;

        if (prefix.startsWith('/')) {
            static const QStringList commands = {
                "/away", "/back", "/ban", "/caps", "/clear", "/ctcp",
                "/deop", "/devoice", "/invite", "/j", "/join",
                "/close", "/kick", "/leave", "/me", "/mode", "/motd", "/msg",
                "/nick", "/notice", "/op", "/part", "/ping", "/time",
                "/quit", "/quote", "/raw", "/sysinfo", "/topic",
                "/unban", "/version", "/voice", "/whois",
            };
            for (const QString &cmd : commands)
                if (cmd.startsWith(prefix, Qt::CaseInsensitive))
                    m_tabCandidates << cmd;
        } else {
            auto *ch = m_model->channel(ServerId{host}, BufferId{channel});
            if (ch) {
                for (const auto &e : std::as_const(ch->nicks))
                    if (e.nick.startsWith(prefix, Qt::CaseInsensitive))
                        m_tabCandidates << e.nick;
                m_tabCandidates.sort(Qt::CaseInsensitive);
            }
        }
    }
    // else: continuing a cycle — use stored m_tabWordStart and m_tabPrefix as-is

    if (m_tabCandidates.isEmpty()) return;

    const QString completed = m_tabCandidates[m_tabCandidateIndex];
    m_tabCandidateIndex = static_cast<int>((m_tabCandidateIndex + 1) % m_tabCandidates.size());

    // Nicks at line start get ": ", everything else at end-of-line gets " "
    QString suffix;
    if (pos == static_cast<int>(text.length()))
        suffix = (m_tabWordStart == 0 && !completed.startsWith('/'))
            ? QStringLiteral(": ") : QStringLiteral(" ");

    const int blockStart = tc.block().position();
    QTextCursor editCursor = input->textCursor();
    editCursor.setPosition(blockStart + m_tabWordStart);
    editCursor.setPosition(blockStart + pos, QTextCursor::KeepAnchor);
    editCursor.insertText(completed + suffix);
    input->setTextCursor(editCursor);
}

void MainWindow::handleHistoryUp()
{
    if (m_inputHistory.isEmpty()) return;
    if (m_historyIndex == -1)
        m_historyDraft = m_input->toPlainText();
    m_historyIndex = qMin(m_historyIndex + 1, static_cast<int>(m_inputHistory.size()) - 1);
    m_input->setPlainText(m_inputHistory[m_historyIndex]);
    m_input->moveCursor(QTextCursor::End);
}

void MainWindow::handleHistoryDown()
{
    if (m_historyIndex == -1) return;
    m_historyIndex--;
    if (m_historyIndex < 0) {
        m_historyIndex = -1;
        m_input->setPlainText(m_historyDraft);
    } else {
        m_input->setPlainText(m_inputHistory[m_historyIndex]);
    }
    m_input->moveCursor(QTextCursor::End);
}

// ---------------------------------------------------------------------------
// Emoji inline autocomplete
// ---------------------------------------------------------------------------

void MainWindow::checkEmojiAutocomplete(const QString &text)
{
    const int cursorPos = m_input->textCursor().position();
    const QString before = text.left(cursorPos);

    // Auto-substitute a completed :shortcode: when the closing colon is just typed
    if (cursorPos > 0 && text[cursorPos - 1] == ':') {
        const QString beforeColon = before.chopped(1);  // drop the closing ':'
        const qsizetype openColon = beforeColon.lastIndexOf(':');
        if (openColon >= 0) {
            const QString code = beforeColon.mid(openColon + 1);
            static const QRegularExpression wordOnly(R"(^\w+$)");
            if (wordOnly.match(code).hasMatch()) {
                const QString emoji = emojiByCode().value(code);
                if (!emoji.isEmpty()) {
                    QTextCursor tc = m_input->textCursor();
                    tc.setPosition(static_cast<int>(openColon));
                    tc.setPosition(cursorPos, QTextCursor::KeepAnchor);
                    tc.insertText(emoji);
                    m_input->setTextCursor(tc);
                    hideEmojiAutocomplete();
                    return;
                }
            }
        }
    }

    // Find a bare :word pattern ending at cursor — minimum 1 char after colon
    const qsizetype colon = before.lastIndexOf(':');
    if (colon < 0) { hideEmojiAutocomplete(); return; }

    const QString word = before.mid(colon + 1);
    // Only trigger if word is all word-chars and at least 1 char
    static const QRegularExpression wordRe(R"(^\w+$)");
    if (word.isEmpty() || !wordRe.match(word).hasMatch()) {
        hideEmojiAutocomplete();
        return;
    }

    const auto matches = emojiMatching(word);
    if (matches.isEmpty()) { hideEmojiAutocomplete(); return; }

    m_emojiTriggerPos = static_cast<int>(colon);

    m_emojiCompleter->clear();
    const int shown = static_cast<int>(qMin(matches.size(), qsizetype(8)));
    for (int i = 0; i < shown; ++i) {
        const auto &e = matches[i];
        auto *item = new QListWidgetItem(e.ch + "  " + e.shortcode);
        item->setData(Qt::UserRole, e.ch);
        m_emojiCompleter->addItem(item);
    }
    m_emojiCompleter->setCurrentRow(0);

    // Size to fit content
    const int itemH  = m_emojiCompleter->sizeHintForRow(0) + 2;
    const int popupH = itemH * shown + 4;
    const int popupW = 220;

    // Position above the input bar in main-window coords
    const QPoint inputTL = m_input->mapTo(this, QPoint(0, 0));

    // Align left edge with colon position approximation using font metrics
    const int charW  = m_input->fontMetrics().averageCharWidth();
    const int colonX = m_input->contentsMargins().left() + static_cast<int>(colon) * charW;
    const QPoint colonLocal = m_input->mapTo(this, QPoint(colonX, 0));

    int px = qMax(inputTL.x(), colonLocal.x());
    int py = inputTL.y() - popupH - 2;
    if (py < 0) py = inputTL.y() + m_input->height() + 2;

    // Clamp horizontally
    if (px + popupW > width()) px = width() - popupW;

    m_emojiCompleter->setGeometry(px, py, popupW, popupH);
    m_emojiCompleter->show();
    m_emojiCompleter->raise();
}

void MainWindow::commitEmojiAutocomplete(int row)
{
    if (row < 0 || row >= m_emojiCompleter->count()) return;

    const QString emoji = m_emojiCompleter->item(row)->data(Qt::UserRole).toString();
    hideEmojiAutocomplete();

    // Replace :word (from trigger pos to cursor) with the emoji
    const int end = m_input->textCursor().position();
    QTextCursor tc = m_input->textCursor();
    tc.setPosition(m_emojiTriggerPos);
    tc.setPosition(end, QTextCursor::KeepAnchor);
    tc.insertText(emoji);
    m_input->setTextCursor(tc);
    m_input->setFocus();
}

void MainWindow::hideEmojiAutocomplete()
{
    m_emojiCompleter->hide();
    m_emojiCompleter->clear();
    m_emojiTriggerPos = -1;
}

// ---------------------------------------------------------------------------
// Sidebar helpers
// ---------------------------------------------------------------------------

QTreeWidgetItem *MainWindow::findServerItem(const QString &host) const
{
    for (int i = 0; i < m_sidebar->topLevelItemCount(); ++i) {
        auto *item = m_sidebar->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toString() == host)
            return item;
    }
    return nullptr;
}

QTreeWidgetItem *MainWindow::findChannelItem(const QString &host, const QString &channel) const
{
    auto *srv = findServerItem(host);
    if (!srv) return nullptr;
    if (channel == "(server)") return srv;
    for (int i = 0; i < srv->childCount(); ++i) {
        auto *item = srv->child(i);
        if (item->data(0, Qt::UserRole + 1).toString().toLower() == channel.toLower())
            return item;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Model → UI slots
// ---------------------------------------------------------------------------

void MainWindow::onServerAdded(const QString &host)
{
    if (findServerItem(host)) return;
    QString label = host;
    for (const auto &sc : std::as_const(m_config.servers))
        if (sc.host == host && !sc.name.isEmpty()) { label = sc.name; break; }
    auto *item = new QTreeWidgetItem(m_sidebar);
    item->setText(0, label.toUpper());
    item->setData(0, Qt::UserRole,     host);
    item->setData(0, Qt::UserRole + 1, QString("(server)"));
    item->setExpanded(true);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    QFont f(m_config.ui.fontFamily, m_config.ui.fontSizes.serverHeader);
    f.setBold(true);
    item->setFont(0, f);
    item->setForeground(0, QColor("#6c7086"));

    if (m_signalBars && (m_model->activeHost().isEmpty() || host == m_model->activeHost().str()))
        m_signalBars->setState(SignalBars::State::Connecting);
}

void MainWindow::onServerConnected(const QString &host)
{
    auto *item = findServerItem(host);
    if (item)
        item->setData(0, Qt::UserRole + 2, QVariant::fromValue(makeConnectedIcon()));
    if (m_signalBars && host == m_model->activeHost().str())
        m_signalBars->setState(SignalBars::State::Connected);

    if (!m_config.profileDisplayName.isEmpty() || !m_config.profileAvatarUrl.isEmpty()) {
        auto *cl = m_model->clientFor(ServerId{host});
        if (cl && cl->hasCap("draft/metadata-2")) {
            m_model->sendRaw(ServerId{host}, "METADATA * SET display-name :" + m_config.profileDisplayName);
            const bool localPath = m_config.profileAvatarUrl.startsWith('/')
                                   || QUrl(m_config.profileAvatarUrl).isLocalFile();
            if (!localPath)
                m_model->sendRaw(ServerId{host}, "METADATA * SET avatar :" + m_config.profileAvatarUrl);
        }
        // Local file avatars are never sent to the server, so seed nickMeta + cache manually.
        if (!m_config.profileAvatarUrl.isEmpty()) {
            const bool localPath = m_config.profileAvatarUrl.startsWith('/')
                                   || QUrl(m_config.profileAvatarUrl).isLocalFile();
            if (localPath) {
                if (auto *sess = m_model->session(ServerId{host}); sess && !sess->nick.isEmpty())
                    m_model->onUserMetaChanged(ServerId{host}, sess->nick, "avatar", m_config.profileAvatarUrl);
                fetchAvatar(m_config.profileAvatarUrl);
            }
        }
    }
}

void MainWindow::onServerDisconnected(const QString &host)
{
    auto *item = findServerItem(host);
    if (item)
        item->setData(0, Qt::UserRole + 2, QVariant());
    if (m_signalBars && host == m_model->activeHost().str())
        m_signalBars->setState(SignalBars::State::Disconnected);

    // Prune typing state for all channels on this host
    const QString prefix = host + "|";
    for (auto it = m_typingNickTimers.begin(); it != m_typingNickTimers.end(); ) {
        if (it.key().startsWith(prefix)) {
            it.value()->stop();
            it.value()->deleteLater();
            it = m_typingNickTimers.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = m_typingNicks.begin(); it != m_typingNicks.end(); )
        it = it.key().startsWith(prefix) ? m_typingNicks.erase(it) : ++it;

    if (auto *sess = m_model->session(ServerId{host})) {
        for (const auto &ch : std::as_const(sess->channels))
            for (const QString &bn : ch.botNicks)
                m_botIconIdx.remove(bn);
        for (const QString &bn : sess->botNicks)
            m_botIconIdx.remove(bn);
    }
}

void MainWindow::onChannelAdded(const QString &host, const QString &channel)
{
    if (findChannelItem(host, channel)) return;
    auto *srv = findServerItem(host);
    if (!srv) return;
    auto *item = new QTreeWidgetItem(srv);
    item->setText(0, channel);
    item->setData(0, Qt::UserRole,     host);
    item->setData(0, Qt::UserRole + 1, channel);

    m_sidebar->setCurrentItem(item);
    switchToChannel(host, channel);
}

void MainWindow::onChannelRemoved(const QString &host, const QString &channel)
{
    auto *item = findChannelItem(host, channel);
    if (item) delete item;
    closeChannelPane(host, channel);
}

static bool isCondensable(const Message &msg, const QString &selfNick)
{
    switch (msg.type) {
    case MessageType::Join:
    case MessageType::Part:
    case MessageType::Quit:
    case MessageType::Nick:
    case MessageType::Kick:
        return selfNick.isEmpty()
            || msg.nick.compare(selfNick, Qt::CaseInsensitive) != 0;
    default:
        return false;
    }
}

// Collect the tail run of consecutive condensable messages from ch.messages
// that share the same calendar day as the last message.
static QList<Message> collectEventGroup(const Channel *ch, const QString &selfNick)
{
    QList<Message> group;
    if (ch->messages.isEmpty()) return group;
    const QDate day = ch->messages.last().timestamp.toLocalTime().date();
    for (qsizetype i = ch->messages.size() - 1; i >= 0; --i) {
        const auto &m = ch->messages[i];
        if (!isCondensable(m, selfNick)) break;
        if (m.timestamp.toLocalTime().date() != day) break;
        group.prepend(m);
    }
    return group;
}

void MainWindow::toggleEventGroupInView(ChatView *view, const QString &groupId,
                                         const QString &host, const QString &channel)
{
    auto *ch = m_model->channel(ServerId{host}, BufferId{channel});
    if (!ch) return;

    const qint64 targetMs = groupId.toLongLong();
    const QString selfNick = m_model->selfNick(ServerId{host});
    int start = -1;
    for (int i = 0; i < ch->messages.size(); ++i) {
        if (ch->messages[i].timestamp.toMSecsSinceEpoch() == targetMs
            && isCondensable(ch->messages[i], selfNick)) {
            start = i;
            break;
        }
    }
    if (start < 0) return;

    const QDate day = ch->messages[start].timestamp.toLocalTime().date();
    int j = start + 1;
    while (j < ch->messages.size()
           && isCondensable(ch->messages[j], selfNick)
           && ch->messages[j].timestamp.toLocalTime().date() == day)
        ++j;

    QList<Message> group(ch->messages.cbegin() + start, ch->messages.cbegin() + j);

    const bool expand = !m_expandedEventGroups.contains(groupId);
    if (expand)
        m_expandedEventGroups.insert(groupId);
    else
        m_expandedEventGroups.remove(groupId);

    ChatRenderer::Context ctx;
    ctx.coloredNicks = m_config.ui.coloredNicks;
    ctx.nickBrackets = m_config.ui.nickBrackets;
    ctx.emojiPt      = m_config.ui.fontSizes.emoji;
    ctx.chatPt       = m_config.ui.fontSizes.chat;
    ctx.validTheme   = m_theme.valid;
    ctx.themeText    = m_theme.text;
    ctx.selfNickRe   = m_selfNickRe;
    ctx.channel      = ch;

    const bool atBottom = view->isAtBottom();
    view->replaceLine("evgrp:" + groupId,
                      ChatRenderer::formatEventGroupLine(group, ctx, groupId, expand));
    if (atBottom) view->scrollToBottom();
}

void MainWindow::handleChatViewContextMenu(ChatView *view, const QString &anchor,
                                            const QPoint &globalPos,
                                            const QString &host, const QString &channel)
{
    if (anchor.startsWith("nick:")) {
        showNickContextMenu(anchor.mid(5), globalPos);
        return;
    }

    if (anchor.startsWith("msgid:")) {
        const QString msgid = anchor.mid(6);
        QMenu menu(view->viewport());
        connect(menu.addAction("Reply"), &QAction::triggered, this,
                [this, msgid, host, channel]{
            auto *ch = m_model->channel(ServerId{host}, BufferId{channel});
            QString origNick;
            if (ch) for (const auto &m : std::as_const(ch->messages))
                if (m.msgid == msgid) { origNick = m.nick; break; }
            m_pendingReplyMsgid = msgid;
            if (m_replyLabel) m_replyLabel->setText("↩ " + (origNick.isEmpty() ? msgid : origNick));
            if (m_replyBar) m_replyBar->show();
            if (m_input)    m_input->setFocus();
        });
        connect(menu.addAction("React"), &QAction::triggered, this,
                [this, msgid, host, channel, globalPos]{
            m_pendingReactMsgid    = msgid;
            m_pendingReactHost     = host;
            m_pendingReactChannel  = channel;
            ensureEmojiPicker();
            m_emojiPicker->showAt(globalPos);
        });
        {
            auto *cl = m_model->clientFor(ServerId{host});
            auto *ch = m_model->channel(ServerId{host}, BufferId{channel});
            if (cl && cl->hasCap("draft/message-redaction") && ch) {
                QString msgNick;
                for (const auto &m : std::as_const(ch->messages))
                    if (m.msgid == msgid) { msgNick = m.nick; break; }
                if (msgNick == m_model->selfNick(ServerId{host})) {
                    connect(menu.addAction("Delete"), &QAction::triggered, this,
                            [this, msgid, host, channel]{
                        m_model->sendRedact(ServerId{host}, BufferId{channel}, msgid);
                    });
                }
            }
        }
        menu.exec(globalPos);
        return;
    }

    if (anchor.startsWith("preview:")) {
        const QString url = anchor.mid(8);
        QMenu menu(view->viewport());
        connect(menu.addAction("Open URL"), &QAction::triggered, this, [url]{
            const QUrl u(url);
            if (u.scheme().toLower() == "http" || u.scheme().toLower() == "https")
                QDesktopServices::openUrl(u);
        });
        auto *ch = m_model->channel(ServerId{host}, BufferId{channel});
        if (ch && ch->previews.contains(url)) {
            connect(menu.addAction("Hide Preview"), &QAction::triggered, this,
                    [this, url, host, channel]{
                auto *inner = m_model->channel(ServerId{host}, BufferId{channel});
                if (inner) inner->hiddenPreviews.insert(url);
                refreshChatView(host, channel);
            });
        }
        menu.exec(globalPos);
        return;
    }

    if (!anchor.isEmpty()) {
        // URL or other anchor
        QString href = anchor;
        if (href.startsWith("url:")) href = href.mid(4);
        QMenu menu(view->viewport());
        connect(menu.addAction("Copy URL"), &QAction::triggered,
                this, [href]{ QApplication::clipboard()->setText(href); });
        connect(menu.addAction("Open URL"), &QAction::triggered, this, [href]{
            const QUrl u(href);
            if (u.scheme().toLower() == "http" || u.scheme().toLower() == "https")
                QDesktopServices::openUrl(u);
        });
        auto *ch = m_model->channel(ServerId{host}, BufferId{channel});
        const bool isHidden   = ch && ch->hiddenPreviews.contains(href);
        const bool hasPreview = ch && ch->previews.contains(href);
        if (isHidden) {
            connect(menu.addAction("Show Preview"), &QAction::triggered, this,
                    [this, href, host, channel]{
                auto *inner = m_model->channel(ServerId{host}, BufferId{channel});
                if (inner) inner->hiddenPreviews.remove(href);
                refreshChatView(host, channel);
            });
        } else {
            auto *hideAction = menu.addAction("Hide Preview");
            hideAction->setEnabled(hasPreview);
            connect(hideAction, &QAction::triggered, this, [this, href, host, channel]{
                auto *inner = m_model->channel(ServerId{host}, BufferId{channel});
                if (inner) inner->hiddenPreviews.insert(href);
                refreshChatView(host, channel);
            });
        }
        menu.exec(globalPos);
    }
}

void MainWindow::onMessageAdded(const QString &host, const QString &channel, const Message &msg)
{
    const QString selfNick = m_model->selfNick(ServerId{host});

    auto makeCtx = [&](Channel *ch) {
        ChatRenderer::Context ctx;
        ctx.coloredNicks = m_config.ui.coloredNicks;
        ctx.nickBrackets = m_config.ui.nickBrackets;
        ctx.emojiPt      = m_config.ui.fontSizes.emoji;
        ctx.chatPt       = m_config.ui.fontSizes.chat;
        ctx.validTheme   = m_theme.valid;
        ctx.themeText    = m_theme.text;
        ctx.selfNickRe   = m_selfNickRe;
        ctx.channel      = ch;
        return ctx;
    };

    auto appendToView = [&](ChatView *view, Channel *ch) {
        if (isCondensable(msg, selfNick)) {
            const QList<Message> group = collectEventGroup(ch, selfNick);
            if (group.isEmpty()) return;
            const QString groupId = QString::number(group.first().timestamp.toMSecsSinceEpoch());
            const bool grpExpanded = m_expandedEventGroups.contains(groupId);
            const QString blockId  = "evgrp:" + groupId;
            const ChatLine line    = ChatRenderer::formatEventGroupLine(group, makeCtx(ch), groupId, grpExpanded);
            if (view->findLine(blockId) >= 0)
                view->replaceLine(blockId, line);
            else
                view->appendLine(line);
        } else {
            view->appendLine(ChatRenderer::formatMessageLine(msg, makeCtx(ch)));
        }
        view->scrollToBottom();
    };

    if (host == m_model->activeHost().str() &&
        channel.toLower() == m_model->activeChannel().str().toLower())
    {
        auto *ch = m_model->channel(ServerId{host}, BufferId{channel});
        if (ch) {
            if (isCondensable(msg, selfNick)) {
                appendToView(m_chatView, ch);
            } else {
                appendMessage(msg, m_config.ui.linkPreviews);
            }
        }
    }

    const QString paneKey = host + "|" + channel.toLower();
    if (auto *pane = m_panes.value(paneKey)) {
        auto *pCh = m_model->channel(ServerId{host}, BufferId{channel});
        if (pCh) appendToView(pane->chatView(), pCh);
    }

    if (m_config.ui.notifications && m_tray && !isActiveWindow()
        && (msg.type == MessageType::Privmsg || msg.type == MessageType::Action))
    {
        const QString myNick = m_model->selfNick(ServerId{host});
        const bool isPM = !channel.startsWith('#') && !channel.startsWith('&');
        const bool isMention = !isPM && !myNick.isEmpty()
                               && msg.text.contains(myNick, Qt::CaseInsensitive);
        if (isPM || isMention)
            m_tray->setNotify(true);
    }
}

void MainWindow::onTopicChanged(const QString &host, const QString &channel, const QString &topic)
{
    if (host == m_model->activeHost().str() &&
        channel.toLower() == m_model->activeChannel().str().toLower())
        refreshTopicBar(host, channel);

    const QString paneKey = host + "|" + channel.toLower();
    if (auto *pane = m_panes.value(paneKey))
        pane->setTopic(ChatRenderer::linkifyTopic(topic));
}

void MainWindow::onNickListChanged(const QString &host, const QString &channel)
{
    scheduleNickRefresh(host, channel);
}

void MainWindow::scheduleNickRefresh(const QString &host, const QString &channel)
{
    const QString key = host + "|" + channel.toLower();
    if (m_nickRefreshPending.contains(key)) return;
    m_nickRefreshPending.insert(key);
    QTimer::singleShot(50, this, [this, host, channel, key] {
        m_nickRefreshPending.remove(key);
        const bool isActive = (host == m_model->activeHost().str() &&
                               channel.toLower() == m_model->activeChannel().str().toLower());
        if (isActive) {
            refreshNickList(host, channel);
            refreshTopicBar(host, channel);
        }
        if (auto *pane = m_panes.value(key))
            refreshPaneNickList(pane);
    });
}

void MainWindow::onUnreadChanged(const QString &host, const QString &channel, int count)
{
    auto *item = findChannelItem(host, channel);
    if (!item) return;
    QString label = channel;
    if (channel == "(server)") {
        label = host;
        for (const auto &sc : std::as_const(m_config.servers))
            if (sc.host == host && !sc.name.isEmpty()) { label = sc.name; break; }
    }
    if (channel == "(server)") {
        const bool connected = [&]{
            auto *s = m_model->session(ServerId{host}); return s && s->connected;
        }();
        if (connected) {
            const QColor col = count > 0 ? QColor("#e06c75") : QColor();
            item->setData(0, Qt::UserRole + 2, QVariant::fromValue(MenuIcons::connectedServer(col)));
        }
        item->setText(0, label);
    } else {
        if (count > 0 && m_model->hasMention(ServerId{host}, BufferId{channel}))
            item->setData(0, Qt::UserRole + 2, QVariant::fromValue(MenuIcons::mention(QColor("#FFD700"))));
        else if (count > 0)
            item->setData(0, Qt::UserRole + 2, QVariant::fromValue(MenuIcons::unread()));
        else
            item->setData(0, Qt::UserRole + 2, QVariant());
        item->setText(0, label);
    }
}

void MainWindow::onReactionsChanged(const QString &host, const QString &channel, const QString &msgid)
{
    auto updateView = [&](ChatView *view, Channel *ch) {
        if (view->findLine(msgid) < 0) return;
        const auto rxIt = ch->reactions.constFind(msgid);
        const bool hasReactions = rxIt != ch->reactions.constEnd() && !rxIt->isEmpty();
        const QString rxId = "rx:" + msgid;
        if (!hasReactions) {
            view->removeLine(rxId);
            return;
        }
        const ChatLine rxLine = buildReactionLine(*rxIt, msgid);
        if (view->findLine(rxId) >= 0)
            view->replaceLine(rxId, rxLine);
        else
            view->insertAfter(msgid, rxLine);
    };

    if (host == m_model->activeHost().str() &&
        channel.toLower() == m_model->activeChannel().str().toLower()) {
        auto *ch = m_model->channel(ServerId{host}, BufferId{channel});
        if (ch) updateView(m_chatView, ch);
    }

    for (auto *pane : std::as_const(m_panes)) {
        if (pane->host() != host || pane->channel().toLower() != channel.toLower()) continue;
        auto *pCh = m_model->channel(ServerId{host}, BufferId{channel});
        if (pCh) updateView(pane->chatView(), pCh);
    }
}

void MainWindow::onSelfNickChanged(const QString &host, const QString &nick)
{
    if (host == m_model->activeHost().str()) {
        m_nickPrefix->setText(nick);
        m_selfNickRe = nick.isEmpty() ? QRegularExpression{}
            : QRegularExpression("(\\b" + QRegularExpression::escape(nick) + "\\b)",
                                 QRegularExpression::CaseInsensitiveOption);
    }

    for (auto *pane : std::as_const(m_panes))
        if (pane->host() == host)
            pane->setNick(nick);
}

void MainWindow::onMessageRedacted(const QString &host, const QString &channel, const QString &msgid)
{
    auto makeCtx = [&](Channel *ch) {
        ChatRenderer::Context ctx;
        ctx.coloredNicks = m_config.ui.coloredNicks;
        ctx.nickBrackets = m_config.ui.nickBrackets;
        ctx.emojiPt      = m_config.ui.fontSizes.emoji;
        ctx.chatPt       = m_config.ui.fontSizes.chat;
        ctx.validTheme   = m_theme.valid;
        ctx.themeText    = m_theme.text;
        ctx.selfNickRe   = m_selfNickRe;
        ctx.channel      = ch;
        return ctx;
    };

    auto updateView = [&](ChatView *view, Channel *ch) {
        if (view->findLine(msgid) < 0) return;
        for (const auto &msg : std::as_const(ch->messages)) {
            if (msg.msgid == msgid) {
                view->replaceLine(msgid, ChatRenderer::formatMessageLine(msg, makeCtx(ch)));
                break;
            }
        }
    };

    if (host == m_model->activeHost().str() &&
        channel.toLower() == m_model->activeChannel().str().toLower()) {
        auto *ch = m_model->channel(ServerId{host}, BufferId{channel});
        if (ch) updateView(m_chatView, ch);
    }

    for (auto *pane : std::as_const(m_panes)) {
        if (pane->host() != host || pane->channel().toLower() != channel.toLower()) continue;
        auto *pCh = m_model->channel(ServerId{host}, BufferId{channel});
        if (pCh) updateView(pane->chatView(), pCh);
    }
}

void MainWindow::onTypingReceived(const QString &host, const QString &channel,
                                   const QString &nick, const QString &state)
{
    if (!m_config.ui.typingIndicator) return;

    const QString key      = host + "|" + channel;
    const QString timerKey = host + "|" + channel + "|" + nick;

    if (state == "active" || state == "paused") {
        m_typingNicks[key].insert(nick);

        if (m_typingNickTimers.contains(timerKey)) {
            m_typingNickTimers[timerKey]->start(6000);
        } else {
            auto *t = new QTimer(this);
            t->setSingleShot(true);
            connect(t, &QTimer::timeout, this, [this, key, timerKey, nick]{
                m_typingNicks[key].remove(nick);
                if (auto *timer = m_typingNickTimers.value(timerKey)) {
                    m_typingNickTimers.remove(timerKey);
                    timer->deleteLater();
                }
                updateTypingLabel();
            });
            m_typingNickTimers.insert(timerKey, t);
            t->start(6000);
        }
    } else {
        m_typingNicks[key].remove(nick);
        if (auto *t = m_typingNickTimers.value(timerKey)) {
            t->stop();
            t->deleteLater();
            m_typingNickTimers.remove(timerKey);
        }
    }

    updateTypingLabel();
}


void MainWindow::updateTypingLabel()
{
    const QString key = m_model->activeHost().str() + "|" + m_model->activeChannel().str();
    const QSet<QString> &typers = m_typingNicks.value(key);

    if (!m_config.ui.typingIndicator) {
        m_typingLabel->setVisible(false);
        return;
    }

    if (typers.isEmpty()) {
        m_typingLabel->setText("");
        return;
    }

    QStringList names(typers.begin(), typers.end());
    QString text;
    if (names.size() == 1)
        text = names[0] + " is typing...";
    else if (names.size() == 2)
        text = names[0] + " and " + names[1] + " are typing...";
    else
        text = QString::number(names.size()) + " people are typing...";

    m_typingLabel->setText(text);
}

// ---------------------------------------------------------------------------
// UI → Model
// ---------------------------------------------------------------------------

void MainWindow::onSidebarSelectionChanged()
{
    auto *item = m_sidebar->currentItem();
    if (!item) return;
    const QString host    = item->data(0, Qt::UserRole).toString();
    const QString channel = item->data(0, Qt::UserRole + 1).toString();
    if (host.isEmpty() || channel.isEmpty()) return;
    switchToChannel(host, channel);
}

// ---------------------------------------------------------------------------
// Input dispatch
// ---------------------------------------------------------------------------

void MainWindow::onInputSubmit()
{
    const QString raw  = m_input->toPlainText();
    const QString text = raw.trimmed();
    if (text.isEmpty()) return;

    // Push to history (newest first, skip consecutive duplicates)
    if (m_inputHistory.isEmpty() || m_inputHistory.first() != text) {
        m_inputHistory.prepend(text);
        if (m_inputHistory.size() > kInputHistoryCap)
            m_inputHistory.removeLast();
    }
    m_historyIndex = -1;
    m_tabActive = false;
    m_tabCandidates.clear();
    hideEmojiAutocomplete();

    m_input->clear();

    const QString host    = m_model->activeHost().str();
    const QString channel = m_model->activeChannel().str();
    if (host.isEmpty() || channel.isEmpty()) return;

    // Stop typing notification on send
    m_typingOutTimer->stop();
    if (m_typingActive && m_config.ui.typingIndicator && channel != "(server)") {
        m_typingActive = false;
        m_model->sendTyping(ServerId{host}, BufferId{channel}, "done");
    }

    dispatchInput(raw, host, channel);
}

void MainWindow::dispatchInput(const QString &text, const QString &host, const QString &channel)
{
    if (text.startsWith('/')) {
        m_dispatcher->dispatch(text, ServerId{host}, BufferId{channel}, m_pendingReplyMsgid);
        return;
    }

    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty() || channel == "(server)") return;

    // Substitute :shortcode: patterns before sending
    static const QRegularExpression shortcodeRe(R"(:(\w+):)");
    QString outText = trimmed;
    qsizetype offset = 0;
    auto it = shortcodeRe.globalMatch(trimmed);
    while (it.hasNext()) {
        const auto m = it.next();
        const QString emoji = emojiByCode().value(m.captured(1));
        if (!emoji.isEmpty()) {
            outText.replace(m.capturedStart() + offset, m.capturedLength(), emoji);
            offset += emoji.length() - m.capturedLength();
        }
    }

    const QString replyMsgid = m_pendingReplyMsgid;
    clearReplyBar();
    m_model->sendMessage(ServerId{host}, BufferId{channel}, outText, replyMsgid);
}

// ---------------------------------------------------------------------------
// View helpers
// ---------------------------------------------------------------------------

void MainWindow::switchToChannel(const QString &host, const QString &channel)
{
    m_primaryPanel->setVisible(true);

    if (m_primaryPaneLabel) {
        if (channel == "(server)") {
            QString serverName = host;
            for (const auto &sc : std::as_const(m_config.servers))
                if (sc.host == host && !sc.name.isEmpty()) { serverName = sc.name; break; }
            m_primaryPaneLabel->setText(serverName);
        } else {
            m_primaryPaneLabel->setText(channel);
        }
    }

    clearReplyBar();
    m_model->setActive(ServerId{host}, BufferId{channel});
    refreshChatView(host, channel);
    refreshNickList(host, channel);
    refreshTopicBar(host, channel);

    if (auto *sess = m_model->session(ServerId{host})) {
        m_nickPrefix->setText(sess->nick);
        m_selfNickRe = sess->nick.isEmpty() ? QRegularExpression{}
            : QRegularExpression("(\\b" + QRegularExpression::escape(sess->nick) + "\\b)",
                                 QRegularExpression::CaseInsensitiveOption);
    }

    if (m_signalBars) {
        auto *sess = m_model->session(ServerId{host});
        if (!sess)
            m_signalBars->setState(SignalBars::State::None);
        else if (sess->connected)
            m_signalBars->setState(SignalBars::State::Connected);
        else
            m_signalBars->setState(SignalBars::State::Disconnected);
    }

    setWindowTitle("Uplink — " + channel + " @ " + host);
    updateTypingLabel();
}

void MainWindow::openChannelList(const QString &host)
{
    if (m_channelListDialog && m_channelListDialog->host() == host) {
        m_channelListDialog->show();
        m_channelListDialog->raise();
        m_channelListDialog->activateWindow();
        return;
    }

    if (m_channelListDialog)
        m_channelListDialog->deleteLater();

    m_channelListDialog = new ChannelListDialog(host, this);

    connect(m_model, &SessionModel::channelListEntry,
            m_channelListDialog, [this, host](ServerId h, BufferId ch, int u, const QString &t) {
        if (h.str() == host)
            m_channelListDialog->addEntry(ch.str(), u, t);
    });
    connect(m_model, &SessionModel::channelListEnd,
            m_channelListDialog, [this, host](ServerId h, int total) {
        if (h.str() == host)
            m_channelListDialog->onListEnd(total);
    });
    connect(m_channelListDialog, &ChannelListDialog::joinRequested,
            this, [this](const QString &h, const QString &channel) {
        m_model->sendRaw(ServerId{h}, "JOIN " + channel);
    });
    connect(m_channelListDialog, &ChannelListDialog::refreshRequested,
            this, [this](const QString &h) {
        m_channelListDialog->reset();
        m_model->sendRaw(ServerId{h}, "LIST");
    });

    m_model->sendRaw(ServerId{host}, "LIST");
    m_channelListDialog->show();
}

void MainWindow::onSidebarContextMenu(const QPoint &pos)
{
    auto *item = m_sidebar->itemAt(pos);
    if (!item) return;

    const QString host    = item->data(0, Qt::UserRole).toString();
    const QString channel = item->data(0, Qt::UserRole + 1).toString();

    // Heap-allocate and use popup() instead of exec() to avoid a Qt/Wayland
    // issue where the pending button-release from the triggering click is
    // delivered into exec()'s event loop, immediately selecting the first item.
    auto *menu = new QMenu(this);
    connect(menu, &QMenu::aboutToHide, menu, &QObject::deleteLater);

    if (channel == "(server)") {
        auto *sess = m_model->session(ServerId{host});
        if (sess && sess->connected) {
            menu->addAction("Disconnect", this, [this, host]{
                if (auto *cl = m_model->clientFor(ServerId{host}))
                    cl->quit();
            });
        } else {
            menu->addAction("Reconnect", this, [this, host]{
                auto *cl = m_model->clientFor(ServerId{host});
                if (!cl) return;
                for (const auto &sc : std::as_const(m_config.servers)) {
                    if (sc.host == host) { cl->connectToServer(sc); break; }
                }
            });
        }
    } else if (channel.startsWith('#') || channel.startsWith('&')) {
        const QString paneKey = host + "|" + channel.toLower();
        if (!m_panes.contains(paneKey)) {
            menu->addAction("Open in Pane", this, [this, host, channel]{
                openChannelPane(host, channel);
            });
        } else {
            menu->addAction("Close Pane", this, [this, host, channel]{
                closeChannelPane(host, channel);
            });
        }
        menu->addSeparator();
        menu->addAction("Rejoin", this, [this, host, channel]{
            m_model->sendPart(ServerId{host}, BufferId{channel});
            QTimer::singleShot(500, this, [this, host, channel]{
                m_model->sendJoin(ServerId{host}, BufferId{channel});
            });
        });
        menu->addAction("Leave", this, [this, host, channel]{
            m_model->sendPart(ServerId{host}, BufferId{channel});
        });
        menu->addAction("Close", this, [this, host, channel]{
            m_model->closeBuffer(ServerId{host}, BufferId{channel});
        });
    } else if (!channel.isEmpty() && channel != "(server)") {
        // PM / user query
        menu->addAction("Close Query", this, [this, host, channel]{
            m_model->closeBuffer(ServerId{host}, BufferId{channel});
        });
    }

    if (!menu->actions().isEmpty())
        menu->popup(m_sidebar->viewport()->mapToGlobal(pos));
    else
        menu->deleteLater();
}

// ---------------------------------------------------------------------------
// Channel panes
// ---------------------------------------------------------------------------

void MainWindow::openChannelPane(const QString &host, const QString &channel)
{
    const QString key = host + "|" + channel.toLower();
    if (m_panes.contains(key)) return;
    if (m_orderedPanes.size() >= kMaxExtraPanes) return;

    auto *pane = new ChannelPane(host, channel, this);
    if (m_theme.valid)
        pane->chatView()->setColors(QColor(m_theme.text), QColor(m_theme.background),
                                    QColor(m_theme.accent), QColor(m_theme.background),
                                    QColor(m_theme.border));

    pane->input()->installEventFilter(this);
    connect(pane->chatView(), &ChatView::anchorActivated, this,
            [this, pane](const QString &anchor, const QPoint &gp, Qt::MouseButton /*btn*/){
        if (anchor.startsWith(QLatin1String("evgrp:")))
            toggleEventGroupInView(pane->chatView(), anchor.mid(6),
                                   pane->host(), pane->channel());
        else
            handleChatViewContextMenu(pane->chatView(), anchor, gp,
                                      pane->host(), pane->channel());
    });
    connect(pane->chatView(), &ChatView::anchorHovered, this,
            [this, pane](const QString &anchor){
        if (anchor.startsWith("nick:")) {
            const QString nick = anchor.mid(5);
            const QString tip = nickTooltip(nick, pane->host());
            if (!tip.isEmpty())
                QToolTip::showText(QCursor::pos(), tip, pane->chatView()->viewport());
        } else {
            QToolTip::hideText();
        }
    });

    if (auto *sess = m_model->session(ServerId{host}))
        pane->setNick(sess->nick);
    pane->setNickVisible(m_showNickPrefix);
    {
        const FontSizes &fs = m_config.ui.fontSizes;
        const QString   &fam = m_config.ui.fontFamily;
        auto makeFont = [&](int pt){ QFont f(fam,pt); f.setStyleHint(QFont::Monospace); return f; };
        const QFont chatFont = makeFont(fs.chat);
        pane->chatView()->setFont(chatFont);
        pane->nickList()->setFont(makeFont(fs.nickList));
        {
            auto *nd = new NickDelegate(pane->nickList());
            if (m_theme.valid)
                nd->setColors(QColor(m_theme.accent),
                              QColor(m_theme.border),
                              QColor(m_theme.text));
            pane->nickList()->setItemDelegate(nd);
        }
        pane->setTopicFont(makeFont(fs.topicBar));
        pane->setInputFont(makeFont(fs.inputNick), makeFont(fs.input));
    }

    if (auto *ch = m_model->channel(ServerId{host}, BufferId{channel}))
        pane->setTopic(ChatRenderer::linkifyTopic(ch->topic));

    connect(pane, &ChannelPane::closeRequested, this, [this, host, channel]{
        closeChannelPane(host, channel);
    });
    connect(pane, &ChannelPane::inputSubmitted, this, [this, host, channel](const QString &text){
        dispatchInput(text, host, channel);
    });
    connect(pane, &ChannelPane::dragActive, this, [this](const QString &sourceKey, const QPoint &gp){
        ChannelPane *target = paneAt(gp);
        if (target == m_panes.value(sourceKey)) target = nullptr;
        if (m_dragHighlighted && m_dragHighlighted != target) {
            m_dragHighlighted->setDragHighlight(false);
            m_dragHighlighted = nullptr;
        }
        if (target && !m_dragHighlighted) {
            target->setDragHighlight(true);
            m_dragHighlighted = target;
        }
    });
    connect(pane, &ChannelPane::dragDropped, this, [this](const QString &sourceKey, const QPoint &gp){
        if (m_dragHighlighted) {
            m_dragHighlighted->setDragHighlight(false);
            m_dragHighlighted = nullptr;
        }
        ChannelPane *source = m_panes.value(sourceKey);
        if (!source) return;
        ChannelPane *target = paneAt(gp);
        if (target && target != source) {
            // pane ↔ pane swap
            const qsizetype fromIdx = m_orderedPanes.indexOf(source);
            const qsizetype toIdx   = m_orderedPanes.indexOf(target);
            if (fromIdx >= 0 && toIdx >= 0) {
                m_orderedPanes.swapItemsAt(fromIdx, toIdx);
                rebuildPaneLayout();
            }
        } else if (!target && isOverPrimary(gp)) {
            // pane ↔ primary swap: move primary to source's old slot
            const qsizetype srcIdx  = m_orderedPanes.indexOf(source);
            if (srcIdx >= 0) {
                // combined slot of source (accounts for primary's current position)
                const int paneSlot = static_cast<int>(srcIdx < m_primarySlot ? srcIdx : srcIdx + 1);
                m_primarySlot = paneSlot;
                rebuildPaneLayout();
            }
        }
    });

    m_panes[key] = pane;
    m_orderedPanes.append(pane);
    m_primaryHeader->setVisible(true);
    m_primaryCloseBtn->setVisible(true);

    if (m_theme.valid) {
        pane->setTopicIcon(
            makeTopicIcon(QColor(m_theme.placeholder)),
            makeTopicIcon(QColor(m_theme.accent)));
    }

    rebuildPaneLayout();
    refreshPaneChatView(pane);
    refreshPaneNickList(pane);
}

ChannelPane *MainWindow::paneAt(const QPoint &globalPos) const
{
    QWidget *w = QApplication::widgetAt(globalPos);
    while (w) {
        if (auto *p = qobject_cast<ChannelPane *>(w)) return p;
        w = w->parentWidget();
    }
    return nullptr;
}

bool MainWindow::isOverPrimary(const QPoint &globalPos) const
{
    QWidget *w = QApplication::widgetAt(globalPos);
    while (w) {
        if (w == m_primaryPanel) return true;
        w = w->parentWidget();
    }
    return false;
}

void MainWindow::closeChannelPane(const QString &host, const QString &channel)
{
    const QString key = host + "|" + channel.toLower();
    auto *pane = m_panes.take(key);
    if (!pane) return;

    m_orderedPanes.removeOne(pane);
    m_primarySlot = qMin(m_primarySlot, static_cast<int>(m_orderedPanes.size()));
    pane->setParent(nullptr); // detach before rebuild
    pane->deleteLater();

    if (m_orderedPanes.isEmpty()) {
        m_primaryCloseBtn->setVisible(false);
        m_primaryPanel->setVisible(true);
    }

    rebuildPaneLayout();
}

void MainWindow::rebuildPaneLayout()
{
    // Collect widgets in display order, inserting primary at m_primarySlot.
    QList<QWidget*> widgets;
    int pi = 0;
    const qsizetype nSlots = 1 + m_orderedPanes.size();
    for (int i = 0; i < nSlots; i++) {
        if (i == m_primarySlot)
            widgets.append(m_primaryPanel);
        else
            widgets.append(m_orderedPanes[pi++]);
    }

    // Detach all pane widgets from wherever they currently live
    for (auto *w : std::as_const(widgets))
        if (w->parentWidget())
            w->setParent(nullptr);

    // Remove and delete any nested splitters left in m_panesSplitter
    while (m_panesSplitter->count() > 0) {
        auto *w = m_panesSplitter->widget(0);
        w->setParent(nullptr);
        if (auto *s = qobject_cast<QSplitter *>(w))
            delete s;
    }

    auto makeVert = []() -> QSplitter * {
        auto *s = new QSplitter(Qt::Vertical);
        s->setHandleWidth(2);
        return s;
    };

    const qsizetype n = widgets.size();
    if (n <= 2) {
        // 1 or 2 panes: flat horizontal
        for (auto *w : std::as_const(widgets)) {
            m_panesSplitter->addWidget(w);
            w->show();
        }
    } else if (n == 3) {
        // primary full-height left, two panes stacked right
        m_panesSplitter->addWidget(widgets[0]);
        widgets[0]->show();
        auto *right = makeVert();
        right->addWidget(widgets[1]);
        right->addWidget(widgets[2]);
        widgets[1]->show();
        widgets[2]->show();
        m_panesSplitter->addWidget(right);
    } else { // n == 4  (2×2 grid)
        auto *left = makeVert();
        left->addWidget(widgets[0]);
        left->addWidget(widgets[1]);
        widgets[0]->show();
        widgets[1]->show();
        auto *right = makeVert();
        right->addWidget(widgets[2]);
        right->addWidget(widgets[3]);
        widgets[2]->show();
        widgets[3]->show();
        m_panesSplitter->addWidget(left);
        m_panesSplitter->addWidget(right);
    }

    // Equalize top-level columns
    const int total = m_panesSplitter->width();
    if (total > 0 && m_panesSplitter->count() > 0) {
        const int each = total / m_panesSplitter->count();
        QList<int> sizes(m_panesSplitter->count(), each);
        m_panesSplitter->setSizes(sizes);
    }
}

void MainWindow::refreshPaneChatView(ChannelPane *pane)
{
    pane->chatView()->clear();
    auto *ch = m_model->channel(ServerId{pane->host()}, BufferId{pane->channel()});
    if (!ch) return;

    ChatRenderer::Context ctx;
    ctx.coloredNicks = m_config.ui.coloredNicks;
    ctx.nickBrackets = m_config.ui.nickBrackets;
    ctx.emojiPt      = m_config.ui.fontSizes.emoji;
    ctx.chatPt       = m_config.ui.fontSizes.chat;
    ctx.validTheme   = m_theme.valid;
    ctx.themeText    = m_theme.text;
    ctx.selfNickRe   = m_selfNickRe;
    ctx.channel      = ch;

    const QString selfNick = m_model->selfNick(ServerId{pane->host()});

    static const QRegularExpression urlRe(
        R"(https?://[^\s<>"]+)",
        QRegularExpression::CaseInsensitiveOption);

    for (int i = 0; i < ch->messages.size(); ) {
        const auto &msg = ch->messages[i];
        if (isCondensable(msg, selfNick)) {
            const QDate day = msg.timestamp.toLocalTime().date();
            int j = i + 1;
            while (j < ch->messages.size()
                   && isCondensable(ch->messages[j], selfNick)
                   && ch->messages[j].timestamp.toLocalTime().date() == day)
                ++j;
            QList<Message> group(ch->messages.cbegin() + i, ch->messages.cbegin() + j);
            const QString groupId = QString::number(group.first().timestamp.toMSecsSinceEpoch());
            const bool grpExpanded = m_expandedEventGroups.contains(groupId);
            pane->chatView()->appendLine(
                ChatRenderer::formatEventGroupLine(group, ctx, groupId, grpExpanded));
            i = j;
        } else {
            pane->chatView()->appendLine(ChatRenderer::formatMessageLine(msg, ctx));
            if (!msg.msgid.isEmpty()) {
                auto rxIt = ch->reactions.constFind(msg.msgid);
                if (rxIt != ch->reactions.constEnd() && !rxIt->isEmpty())
                    pane->chatView()->appendLine(buildReactionLine(*rxIt, msg.msgid));
            }
            const bool isText = (msg.type == MessageType::Privmsg ||
                                 msg.type == MessageType::Action  ||
                                 msg.type == MessageType::Notice);
            if (isText && !ch->previews.isEmpty()) {
                auto uit = urlRe.globalMatch(msg.text);
                while (uit.hasNext()) {
                    const QString urlStr = QUrl(uit.next().captured(0)).toString();
                    const auto p = ch->previews.constFind(urlStr);
                    if (p == ch->previews.constEnd() || ch->hiddenPreviews.contains(urlStr))
                        continue;
                    ChatLine card;
                    card.role   = ChatLineRole::PreviewCard;
                    card.id     = "preview:" + urlStr;
                    card.image  = p->thumbnail;
                    card.text   = p->title + "\n" + p->domain;
                    ChatSegment seg; seg.start = 0; seg.length = card.text.size();
                    seg.anchor = "preview:" + urlStr;
                    card.segments.append(seg);
                    pane->chatView()->appendLine(card);
                }
            }
            ++i;
        }
    }

    pane->chatView()->scrollToBottom();
}

void MainWindow::refreshPaneNickList(ChannelPane *pane)
{
    pane->nickList()->clear();
    auto *ch   = m_model->channel(ServerId{pane->host()}, BufferId{pane->channel()});
    if (!ch) return;
    auto *sess = m_model->session(ServerId{pane->host()});

    for (const auto &e : std::as_const(ch->nicks))
        pane->nickList()->addItem(makeNickItem(e, ch, sess));
}

void MainWindow::onNickListContextMenu(const QPoint &pos)
{
    auto *item = m_nickList->itemAt(pos);
    if (!item) return;
    const QString nick = item->data(Qt::UserRole).toString();
    if (nick.isEmpty()) return;
    showNickContextMenu(nick, m_nickList->mapToGlobal(pos));
}

void MainWindow::showNickContextMenu(const QString &nick, const QPoint &globalPos)
{
    const QString host    = m_model->activeHost().str();
    const QString channel = m_model->activeChannel().str();
    if (nick.isEmpty() || host.isEmpty()) return;

    QMenu menu(this);

    QAction *title = menu.addAction(nick);
    title->setEnabled(false);
    QFont tf = title->font();
    tf.setBold(true);
    title->setFont(tf);
    menu.addSeparator();

    // ── Common actions ────────────────────────────────────────────────────────
    connect(menu.addAction("Message"), &QAction::triggered, this, [this, host, nick]{
        m_model->openPM(ServerId{host}, nick);
        switchToChannel(host, nick);
        if (m_input) m_input->setFocus();
    });

    connect(menu.addAction("Whois"), &QAction::triggered, this, [this, host, nick]{
        m_model->sendRaw(ServerId{host}, "WHOIS " + nick);
    });

    connect(menu.addAction("Copy Nick"), &QAction::triggered, this, [nick]{
        qApp->clipboard()->setText(nick);
    });

    menu.addSeparator();

    // ── Ignore ▶ ─────────────────────────────────────────────────────────────
    {
        const QString key = nick.toLower();
        const IgnoreTypes curFlags = m_model->ignoreFlags(key);

        auto *ignoreSub = new QMenu("Ignore", &menu);
        ignoreSub->setIcon(MenuIcons::eyeOff());

        auto makeTypeAction = [&](const QString &label, IgnoreType type) {
            auto *act = ignoreSub->addAction(label);
            act->setCheckable(true);
            act->setChecked(bool(curFlags & type));
            connect(act, &QAction::triggered, this, [this, host, key, type](bool checked) {
                IgnoreTypes flags = m_model->ignoreFlags(key);
                if (checked) flags |= type;
                else         flags &= ~IgnoreTypes(type);
                m_config.ignoreList.removeIf([&](const IgnoreEntry &e){ return e.nick == key; });
                if (flags) {
                    m_model->setIgnore(key, flags);
                    m_config.ignoreList.append({key, flags});
                } else {
                    m_model->clearIgnore(key);
                    m_model->localMessage(ServerId{host}, m_model->activeChannel(),
                                          "No longer ignoring " + key);
                }
                Config::save(m_config, Config::defaultPath());
            });
        };

        makeTypeAction("Private Messages", IgnoreType::PM);
        makeTypeAction("Notices",          IgnoreType::Notice);
        makeTypeAction("Invites",          IgnoreType::Invite);

        if (curFlags) {
            ignoreSub->addSeparator();
            connect(ignoreSub->addAction(MenuIcons::close(), "Unignore All"),
                    &QAction::triggered, this, [this, host, key] {
                m_model->clearIgnore(key);
                m_config.ignoreList.removeIf([&](const IgnoreEntry &e){ return e.nick == key; });
                Config::save(m_config, Config::defaultPath());
                m_model->localMessage(ServerId{host}, m_model->activeChannel(),
                                      "No longer ignoring " + key);
            });
        }

        menu.addMenu(ignoreSub);
    }

    menu.addSeparator();

    // ── CTCP ▶ ───────────────────────────────────────────────────────────────
    {
        auto *ctcpSub = new QMenu("CTCP", &menu);

        connect(ctcpSub->addAction("Ping"), &QAction::triggered, this, [this, host, nick]{
            const qint64 ts = QDateTime::currentMSecsSinceEpoch();
            m_model->sendRaw(ServerId{host}, "PRIVMSG " + nick + " :\x01PING " + QString::number(ts) + "\x01");
        });

        connect(ctcpSub->addAction("Version"), &QAction::triggered, this, [this, host, nick]{
            m_model->sendRaw(ServerId{host}, "PRIVMSG " + nick + " :\x01VERSION\x01");
        });

        menu.addMenu(ctcpSub);
    }

    // ── DCC ▶ ────────────────────────────────────────────────────────────────
    {
        auto *dccSub = new QMenu("DCC", &menu);

        connect(dccSub->addAction("Send File"), &QAction::triggered, this, [this, host, nick]{
            const QString path = QFileDialog::getOpenFileName(this, "Send File to " + nick);
            if (path.isEmpty()) return;

            IrcClient *client = m_model->clientFor(ServerId{host});
            if (!client) return;

            const quint32 localIp = client->localIpv4();
            auto *dcc = new DccSend(path, this);
            if (!dcc->listen(localIp ? QHostAddress(localIp) : QHostAddress::Any)) {
                dcc->deleteLater(); return;
            }

            const quint32 ip   = localIp;
            const quint16 port = dcc->port();
            const QString fn   = dcc->filename();
            const qint64  size = dcc->filesize();

            m_model->sendRaw(ServerId{host},
                "PRIVMSG " + nick + " :\x01""DCC SEND "
                + fn + " " + QString::number(ip)
                + " " + QString::number(port)
                + " " + QString::number(size) + "\x01");

            auto *prog = new QProgressDialog("Sending " + fn + " to " + nick,
                                              "Cancel", 0, size > INT_MAX ? INT_MAX : static_cast<int>(size), this);
            prog->setWindowModality(Qt::NonModal);
            prog->setAttribute(Qt::WA_DeleteOnClose);

            connect(dcc, &DccSend::progress, prog, [prog, size](qint64 sent, qint64){
                prog->setValue(static_cast<int>(size > INT_MAX ? sent * INT_MAX / size : sent));
            });
            connect(dcc, &DccSend::finished, prog, [prog, dcc]{
                prog->setValue(prog->maximum());
                dcc->deleteLater();
            });
            connect(dcc, &DccSend::error, this, [this, prog, dcc](const QString &msg){
                prog->close();
                dcc->deleteLater();
                QMessageBox::warning(this, "DCC Error", msg);
            });
            connect(prog, &QProgressDialog::canceled, dcc, [dcc]{ dcc->cancel(); dcc->deleteLater(); });

            prog->show();
        });

        connect(dccSub->addAction("Send File (Passive)"), &QAction::triggered, this, [this, host, nick]{
            const QString path = QFileDialog::getOpenFileName(this, "Send File to " + nick + " (Passive)");
            if (path.isEmpty()) return;

            auto *dcc = new DccSend(path, this);
            const QString token = dcc->initPassive();
            if (token.isEmpty()) { dcc->deleteLater(); return; }

            const QString fn   = dcc->filename();
            const qint64  size = dcc->filesize();

            m_pendingPassiveSends.insert(token, dcc);
            m_model->sendRaw(ServerId{host},
                "PRIVMSG " + nick + " :\x01""DCC SEND "
                + fn + " 0 0"
                + " " + QString::number(size)
                + " " + token + "\x01");

            auto *prog = new QProgressDialog("Waiting for " + nick + " to accept...",
                                              "Cancel", 0, size > INT_MAX ? INT_MAX : static_cast<int>(size), this);
            prog->setWindowModality(Qt::NonModal);
            prog->setAttribute(Qt::WA_DeleteOnClose);

            connect(dcc, &DccSend::progress, prog, [prog, size](qint64 sent, qint64){
                prog->setValue(static_cast<int>(size > INT_MAX ? sent * INT_MAX / size : sent));
            });
            connect(dcc, &DccSend::finished, prog, [prog, dcc]{
                prog->setValue(prog->maximum());
                dcc->deleteLater();
            });
            connect(dcc, &DccSend::error, this, [this, prog, dcc, token](const QString &msg){
                prog->close();
                m_pendingPassiveSends.remove(token);
                dcc->deleteLater();
                QMessageBox::warning(this, "DCC Error", msg);
            });
            connect(prog, &QProgressDialog::canceled, dcc, [this, dcc, token]{
                m_pendingPassiveSends.remove(token);
                dcc->cancel();
                dcc->deleteLater();
            });
            prog->show();
        });

        menu.addMenu(dccSub);
    }

    menu.addSeparator();

    // ── Chan Ops ▶ ───────────────────────────────────────────────────────────
    {
        auto *opSub = new QMenu("Chan Ops", &menu);

        connect(opSub->addAction("Give Op"), &QAction::triggered, this, [this, host, channel, nick]{
            if (!channel.isEmpty() && channel != "(server)")
                m_model->sendRaw(ServerId{host}, "MODE " + channel + " +o " + nick);
        });
        connect(opSub->addAction("Take Op"), &QAction::triggered, this, [this, host, channel, nick]{
            if (!channel.isEmpty() && channel != "(server)")
                m_model->sendRaw(ServerId{host}, "MODE " + channel + " -o " + nick);
        });
        connect(opSub->addAction("Give Voice"), &QAction::triggered, this, [this, host, channel, nick]{
            if (!channel.isEmpty() && channel != "(server)")
                m_model->sendRaw(ServerId{host}, "MODE " + channel + " +v " + nick);
        });
        connect(opSub->addAction("Take Voice"), &QAction::triggered, this, [this, host, channel, nick]{
            if (!channel.isEmpty() && channel != "(server)")
                m_model->sendRaw(ServerId{host}, "MODE " + channel + " -v " + nick);
        });

        opSub->addSeparator();

        connect(opSub->addAction("Invite"), &QAction::triggered, this, [this, host, channel, nick]{
            bool ok;
            const QString target = QInputDialog::getText(
                this, "Invite " + nick, "Channel:", QLineEdit::Normal,
                (channel.isEmpty() || channel == "(server)") ? QString() : channel, &ok);
            if (!ok || target.isEmpty()) return;
            m_model->sendRaw(ServerId{host}, "INVITE " + nick + " " + target);
        });
        connect(opSub->addAction("Kick"), &QAction::triggered, this, [this, host, channel, nick]{
            if (channel.isEmpty() || channel == "(server)") return;
            bool ok;
            QString reason = QInputDialog::getText(this, "Kick " + nick, "Reason:", QLineEdit::Normal, {}, &ok);
            if (!ok) return;
            m_model->sendRaw(ServerId{host}, "KICK " + channel + " " + nick + (reason.isEmpty() ? QString() : " :" + reason));
        });
        connect(opSub->addAction("Ban"), &QAction::triggered, this, [this, host, channel, nick]{
            if (!channel.isEmpty() && channel != "(server)")
                m_model->sendRaw(ServerId{host}, "MODE " + channel + " +b " + nick + "!*@*");
        });
        connect(opSub->addAction("Kick && Ban"), &QAction::triggered, this, [this, host, channel, nick]{
            if (channel.isEmpty() || channel == "(server)") return;
            bool ok;
            QString reason = QInputDialog::getText(this, "Kick & Ban " + nick, "Reason:", QLineEdit::Normal, {}, &ok);
            if (!ok) return;
            m_model->sendRaw(ServerId{host}, "MODE " + channel + " +b " + nick + "!*@*");
            m_model->sendRaw(ServerId{host}, "KICK " + channel + " " + nick + (reason.isEmpty() ? QString() : " :" + reason));
        });

        menu.addMenu(opSub);
    }

    menu.exec(globalPos);
}

void MainWindow::enqueuePreview(const QUrl &url, const QString &host, const QString &channel, const QString &msgid)
{
    const QString key = url.toString();
    if (key.isEmpty()) return;
    if (m_previewChannels.contains(key)) return;
    if (m_previewQueue.size() >= 10) return;
    if (m_previewChannels.size() >= 100) return;
    m_previewChannels.insert(key, {host, channel, msgid});
    m_previewQueue.enqueue(url);
    processPreviewQueue();
}

void MainWindow::processPreviewQueue()
{
    if (m_previewFetchBusy || m_previewQueue.isEmpty()) return;
    m_previewFetchBusy = true;
    m_previewWatchdog->start(20000);
    m_linkPreview->fetch(m_previewQueue.dequeue());
}

void MainWindow::refreshChatView(const QString &host, const QString &channel, bool resetToLatest)
{
    m_chatView->clear();
    auto *ch = m_model->channel(ServerId{host}, BufferId{channel});
    if (!ch) return;

    const QString   key   = host + '\t' + channel;
    const qsizetype total = ch->messages.size();

    if (resetToLatest || !m_renderStart.contains(key))
        m_renderStart[key] = static_cast<int>(qMax(qsizetype(0), total - kRenderWindow));
    const int startIdx = m_renderStart.value(key, 0);

    static const QRegularExpression urlRe(
        R"(https?://[^\s<>"]+)",
        QRegularExpression::CaseInsensitiveOption);

    ChatRenderer::Context ctx;
    ctx.coloredNicks = m_config.ui.coloredNicks;
    ctx.nickBrackets = m_config.ui.nickBrackets;
    ctx.emojiPt      = m_config.ui.fontSizes.emoji;
    ctx.chatPt       = m_config.ui.fontSizes.chat;
    ctx.validTheme   = m_theme.valid;
    ctx.themeText    = m_theme.text;
    ctx.selfNickRe   = m_selfNickRe;
    ctx.channel      = ch;

    if (startIdx > 0) {
        ChatLine status = ChatRenderer::makeStatusLine(
            QString("── %1 older messages ──").arg(startIdx));
        status.id = "status:older";
        m_chatView->appendLine(status);
    }

    const QString selfNick = m_model->selfNick(ServerId{host});
    for (int i = startIdx; i < ch->messages.size(); ) {
        const auto &msg = ch->messages[i];
        if (isCondensable(msg, selfNick)) {
            const QDate day = msg.timestamp.toLocalTime().date();
            int j = i + 1;
            while (j < ch->messages.size()
                   && isCondensable(ch->messages[j], selfNick)
                   && ch->messages[j].timestamp.toLocalTime().date() == day)
                ++j;
            QList<Message> group(ch->messages.cbegin() + i, ch->messages.cbegin() + j);
            const QString groupId = QString::number(group.first().timestamp.toMSecsSinceEpoch());
            const bool grpExpanded = m_expandedEventGroups.contains(groupId);
            m_chatView->appendLine(ChatRenderer::formatEventGroupLine(group, ctx, groupId, grpExpanded));
            i = j;
        } else {
            m_chatView->appendLine(ChatRenderer::formatMessageLine(msg, ctx));
            if (!msg.msgid.isEmpty()) {
                auto rxIt = ch->reactions.constFind(msg.msgid);
                if (rxIt != ch->reactions.constEnd() && !rxIt->isEmpty())
                    m_chatView->appendLine(buildReactionLine(*rxIt, msg.msgid));
            }
            const bool isText = (msg.type == MessageType::Privmsg ||
                                 msg.type == MessageType::Action  ||
                                 msg.type == MessageType::Notice);
            if (isText && !ch->previews.isEmpty()) {
                auto it = urlRe.globalMatch(msg.text);
                while (it.hasNext()) {
                    const QString urlStr = QUrl(it.next().captured(0)).toString();
                    const auto p = ch->previews.constFind(urlStr);
                    if (p == ch->previews.constEnd() || ch->hiddenPreviews.contains(urlStr))
                        continue;
                    ChatLine card;
                    card.role   = ChatLineRole::PreviewCard;
                    card.id     = "preview:" + urlStr;
                    card.image  = p->thumbnail;
                    card.text   = p->title + "\n" + p->domain;
                    ChatSegment seg; seg.start = 0; seg.length = card.text.size();
                    seg.anchor = "preview:" + urlStr;
                    card.segments.append(seg);
                    m_chatView->appendLine(card);
                }
            }
            ++i;
        }
    }

    if (resetToLatest)
        QTimer::singleShot(0, this, [this]{ m_chatView->scrollToBottom(); });
}

void MainWindow::loadOlderMessages()
{
    if (m_loadingOlder) return;
    const QString host    = m_model->activeHost().str();
    const QString chName  = m_model->activeChannel().str();
    if (host.isEmpty() || chName.isEmpty()) return;

    const QString key = host + '\t' + chName;
    if (!m_renderStart.contains(key) || m_renderStart[key] == 0) return;

    auto *ch = m_model->channel(ServerId{host}, BufferId{chName});
    if (!ch) return;

    m_loadingOlder = true;

    const int prevStart = m_renderStart[key];
    m_renderStart[key]  = qMax(0, prevStart - kRenderChunk);
    const int newStart  = m_renderStart[key];

    ChatRenderer::Context ctx;
    ctx.coloredNicks = m_config.ui.coloredNicks;
    ctx.nickBrackets = m_config.ui.nickBrackets;
    ctx.emojiPt      = m_config.ui.fontSizes.emoji;
    ctx.chatPt       = m_config.ui.fontSizes.chat;
    ctx.validTheme   = m_theme.valid;
    ctx.themeText    = m_theme.text;
    ctx.selfNickRe   = m_selfNickRe;
    ctx.channel      = ch;

    const QString selfNick = m_model->selfNick(ServerId{host});
    QList<ChatLine> older;

    if (newStart > 0) {
        ChatLine status = ChatRenderer::makeStatusLine(
            QString("── %1 older messages ──").arg(newStart));
        status.id = "status:older";
        older.append(status);
    }

    for (int i = newStart; i < prevStart; ) {
        const auto &msg = ch->messages[i];
        if (isCondensable(msg, selfNick)) {
            const QDate day = msg.timestamp.toLocalTime().date();
            int j = i + 1;
            while (j < prevStart
                   && isCondensable(ch->messages[j], selfNick)
                   && ch->messages[j].timestamp.toLocalTime().date() == day)
                ++j;
            QList<Message> group(ch->messages.cbegin() + i, ch->messages.cbegin() + j);
            const QString groupId    = QString::number(group.first().timestamp.toMSecsSinceEpoch());
            const bool    grpExpanded = m_expandedEventGroups.contains(groupId);
            older.append(ChatRenderer::formatEventGroupLine(group, ctx, groupId, grpExpanded));
            i = j;
        } else {
            older.append(ChatRenderer::formatMessageLine(msg, ctx));
            if (!msg.msgid.isEmpty()) {
                auto rxIt = ch->reactions.constFind(msg.msgid);
                if (rxIt != ch->reactions.constEnd() && !rxIt->isEmpty())
                    older.append(buildReactionLine(*rxIt, msg.msgid));
            }
            ++i;
        }
    }

    // Remove the existing "older messages" sentinel before prepending new batch
    m_chatView->removeLine("status:older");
    m_chatView->prependLines(std::move(older));

    QTimer::singleShot(0, this, [this]{ m_loadingOlder = false; });
}

QListWidgetItem *MainWindow::makeNickItem(const NickEntry &e, const Channel *ch,
                                           const ServerSession *sess)
{
    const bool isBot = ch->botNicks.contains(e.nick.toLower())
                    || (sess && sess->botNicks.contains(e.nick.toLower()));
    auto *item = new QListWidgetItem(e.display());
    if (isBot) {
        const QString key = e.nick.toLower();
        if (!m_botIconIdx.contains(key))
            m_botIconIdx[key] = QRandomGenerator::global()->bounded(2);
        const QString svgPath = m_botIconIdx[key] == 0
            ? QStringLiteral(":/icons/mi-smart-toy.svg")
            : QStringLiteral(":/icons/mi-alien.svg");
        item->setIcon(MenuIcons::fromSvg(svgPath,
                                         QColor(m_theme.valid ? m_theme.accent : "#5588ff")));
    }
    item->setData(Qt::UserRole, e.nick);
    {
        const NickMeta *meta = nullptr;
        if (sess) {
            auto it = sess->nickMeta.constFind(e.nick.toLower());
            if (it != sess->nickMeta.constEnd())
                meta = &it.value();
        }
        const bool hasAvatarImage = meta && !meta->avatarUrl.isEmpty()
                                    && m_avatarCache.contains(meta->avatarUrl);
        if (hasAvatarImage) {
            QByteArray bytes;
            QBuffer buf(&bytes);
            buf.open(QIODevice::WriteOnly);
            m_avatarCache[meta->avatarUrl].save(&buf, "PNG");
            const QString b64 = QString::fromLatin1(bytes.toBase64());
            QStringList lines;
            if (!meta->displayName.isEmpty())
                lines << "Name:" + meta->displayName.toHtmlEscaped();
            if (!e.account.isEmpty())
                lines << "Account: " + e.account.toHtmlEscaped();
            item->setToolTip(
                QString("<html><body><table><tr>"
                        "<td><img src='data:image/png;base64,%1' width='32' height='32'></td>"
                        "<td style='padding-left:6px;vertical-align:middle'>%2</td>"
                        "</tr></table></body></html>")
                    .arg(b64, lines.join("<br>")));
        } else {
            QStringList tips;
            if (meta && !meta->displayName.isEmpty())
                tips << "Name:" + meta->displayName;
            if (!e.account.isEmpty())
                tips << "Account: " + e.account;
            if (meta && !meta->avatarUrl.isEmpty())
                tips << "Avatar: " + meta->avatarUrl;
            if (!tips.isEmpty())
                item->setToolTip(tips.join('\n'));
        }
    }
    if (m_config.ui.coloredNicks)
        item->setForeground(ChatRenderer::nickColor(e.nick));
    return item;
}

QString MainWindow::nickTooltip(const QString &nick, const QString &host) const
{
    const ServerSession *sess = const_cast<SessionModel *>(m_model)->session(ServerId{host});
    const NickMeta *meta = nullptr;
    QString account;
    if (sess) {
        auto it = sess->nickMeta.constFind(nick.toLower());
        if (it != sess->nickMeta.constEnd()) meta = &it.value();
        // account comes from Channel's nicks list
        for (const auto &ch : std::as_const(sess->channels)) {
            auto ni = std::find_if(ch.nicks.cbegin(), ch.nicks.cend(),
                                   [&](const NickEntry &e){ return e.nick.toLower() == nick.toLower(); });
            if (ni != ch.nicks.cend()) { account = ni->account; break; }
        }
    }
    const bool hasImage = meta && !meta->avatarUrl.isEmpty() && m_avatarCache.contains(meta->avatarUrl);
    if (hasImage) {
        QByteArray bytes;
        QBuffer buf(const_cast<QByteArray *>(&bytes));
        buf.open(QIODevice::WriteOnly);
        m_avatarCache[meta->avatarUrl].save(&buf, "PNG");
        const QString b64 = QString::fromLatin1(bytes.toBase64());
        QStringList lines;
        if (!meta->displayName.isEmpty())
            lines << meta->displayName.toHtmlEscaped();
        if (!account.isEmpty())
            lines << account.toHtmlEscaped();
        return QString("<html><body><table><tr>"
                       "<td><img src='data:image/png;base64,%1' width='32' height='32'></td>"
                       "<td style='padding-left:6px;vertical-align:middle'>%2</td>"
                       "</tr></table></body></html>")
                   .arg(b64, lines.join("<br>"));
    }
    QStringList tips;
    if (meta && !meta->displayName.isEmpty()) tips << meta->displayName;
    if (!account.isEmpty())                   tips << account;
    return tips.join('\n');
}

int MainWindow::findNickRow(QListWidget *list, const QString &nick)
{
    const QString lower = nick.toLower();
    for (int i = 0; i < list->count(); ++i)
        if (list->item(i)->data(Qt::UserRole).toString().toLower() == lower)
            return i;
    return -1;
}

void MainWindow::onNickAdded(const QString &host, const QString &channel, const QString &nick)
{
    auto *ch   = m_model->channel(ServerId{host}, BufferId{channel});
    auto *sess = m_model->session(ServerId{host});
    if (!ch) return;
    const qsizetype row = ch->nickIndex.value(nick.toLower(), -1);
    if (row < 0) return;
    const NickEntry &e = ch->nicks[row];

    const bool isActive = (host == m_model->activeHost().str() &&
                           channel.toLower() == m_model->activeChannel().str().toLower());
    if (isActive) {
        m_nickList->insertItem(static_cast<int>(row), makeNickItem(e, ch, sess));
        if (m_nickCountLabel)
            m_nickCountLabel->setText(QString::number(ch->nicks.size()) + " users");
    }

    const QString key = host + "|" + channel.toLower();
    if (auto *pane = m_panes.value(key))
        pane->nickList()->insertItem(static_cast<int>(row), makeNickItem(e, ch, sess));
}

void MainWindow::onNickRemoved(const QString &host, const QString &channel, const QString &nick)
{
    auto *ch = m_model->channel(ServerId{host}, BufferId{channel});

    const bool isActive = (host == m_model->activeHost().str() &&
                           channel.toLower() == m_model->activeChannel().str().toLower());
    if (isActive) {
        const int row = findNickRow(m_nickList, nick);
        if (row >= 0) delete m_nickList->takeItem(row);
        if (m_nickCountLabel && ch)
            m_nickCountLabel->setText(QString::number(ch->nicks.size()) + " users");
    }

    const QString key = host + "|" + channel.toLower();
    if (auto *pane = m_panes.value(key)) {
        const int row = findNickRow(pane->nickList(), nick);
        if (row >= 0) delete pane->nickList()->takeItem(row);
    }
}

void MainWindow::onNickRenamed(const QString &host, const QString &channel,
                                const QString &oldNick, const QString &newNick)
{
    auto *ch   = m_model->channel(ServerId{host}, BufferId{channel});
    auto *sess = m_model->session(ServerId{host});
    if (!ch) return;
    const qsizetype newRow = ch->nickIndex.value(newNick.toLower(), -1);
    if (newRow < 0) return;
    const NickEntry &e = ch->nicks[newRow];

    auto apply = [&](QListWidget *list) {
        const int oldRow = findNickRow(list, oldNick);
        if (oldRow < 0) return;
        delete list->takeItem(oldRow);
        list->insertItem(static_cast<int>(newRow), makeNickItem(e, ch, sess));
    };

    const bool isActive = (host == m_model->activeHost().str() &&
                           channel.toLower() == m_model->activeChannel().str().toLower());
    if (isActive) apply(m_nickList);

    const QString key = host + "|" + channel.toLower();
    if (auto *pane = m_panes.value(key)) apply(pane->nickList());
}

void MainWindow::refreshNickList(const QString &host, const QString &channel)
{
    m_nickList->clear();
    auto *ch   = m_model->channel(ServerId{host}, BufferId{channel});
    if (!ch) return;
    auto *sess = m_model->session(ServerId{host});

    for (const auto &e : std::as_const(ch->nicks))
        m_nickList->addItem(makeNickItem(e, ch, sess));

    if (m_nickCountLabel)
        m_nickCountLabel->setText(QString::number(ch->nicks.size()) + " users");
}


void MainWindow::refreshTopicBar(const QString &host, const QString &channel)
{
    auto *ch = m_model->channel(ServerId{host}, BufferId{channel});

    QString serverName = host;
    for (const auto &sc : std::as_const(m_config.servers))
        if (sc.host == host && !sc.name.isEmpty()) { serverName = sc.name; break; }

    m_modesLabel->clear();

    if (channel == "(server)") {
        m_topicLabel->setText(serverName);
        m_userInfoLabel->clear();
        if (m_topicText) m_topicText->clear();
    } else {
        const QString modes   = ch ? ch->modes : QString();
        const QString modeStr = modes.isEmpty() ? QString() : " (" + modes + ")";
        m_topicLabel->setText(channel + modeStr);

        const qsizetype userCount = ch ? ch->nicks.size() : 0;
        m_userInfoLabel->setText(
            QString("* %1 — %2 user%3").arg(serverName).arg(userCount).arg(userCount != 1 ? "s" : ""));

        if (m_topicText)
            m_topicText->setText(ChatRenderer::linkifyTopic(ch ? ch->topic : QString()));
    }
}

QString MainWindow::msgidAtViewPos(const QPoint & /*viewPos*/) const
{
    // Phase 3: implement via ChatView hit-test
    return {};
}

void MainWindow::doSearch(bool backward)
{
    const QString text = m_searchInput ? m_searchInput->text() : QString{};
    if (text.isEmpty()) { m_chatView->clearFind(); return; }
    m_chatView->findText(text, backward);
}

void MainWindow::showSearchBar()
{
    m_searchBar->show();
    m_searchInput->setFocus();
    m_searchInput->selectAll();
}

void MainWindow::clearReplyBar()
{
    m_pendingReplyMsgid.clear();
    if (m_replyLabel) m_replyLabel->setText({});
    if (m_replyBar)   m_replyBar->hide();
}

void MainWindow::appendMessage(const Message &msg, bool autoPreview)
{
    const QString host    = m_model->activeHost().str();
    const QString channel = m_model->activeChannel().str();
    auto *ch = m_model->channel(m_model->activeHost(), m_model->activeChannel());

    ChatRenderer::Context ctx;
    ctx.coloredNicks = m_config.ui.coloredNicks;
    ctx.nickBrackets = m_config.ui.nickBrackets;
    ctx.emojiPt      = m_config.ui.fontSizes.emoji;
    ctx.chatPt       = m_config.ui.fontSizes.chat;
    ctx.validTheme   = m_theme.valid;
    ctx.themeText    = m_theme.text;
    ctx.selfNickRe   = m_selfNickRe;
    ctx.channel      = ch;

    m_chatView->appendLine(ChatRenderer::formatMessageLine(msg, ctx));

    const bool isText = (msg.type == MessageType::Privmsg ||
                         msg.type == MessageType::Action  ||
                         msg.type == MessageType::Notice);
    if (autoPreview && isText) {
        static const QRegularExpression urlRe(
            R"(https?://[^\s<>"]+)",
            QRegularExpression::CaseInsensitiveOption);
        auto it = urlRe.globalMatch(msg.text);
        while (it.hasNext()) {
            const QString urlStr = QUrl(it.next().captured(0)).toString();
            if (urlStr.isEmpty()) continue;
            if (ch) {
                const auto p = ch->previews.constFind(urlStr);
                if (p != ch->previews.constEnd() && !ch->hiddenPreviews.contains(urlStr)) {
                    ChatLine card;
                    card.role   = ChatLineRole::PreviewCard;
                    card.id     = "preview:" + urlStr;
                    card.image  = p->thumbnail;
                    card.text   = p->title + "\n" + p->domain;
                    ChatSegment seg; seg.start = 0; seg.length = card.text.size();
                    seg.anchor = "preview:" + urlStr;
                    card.segments.append(seg);
                    m_chatView->appendLine(card);
                    continue;
                }
            }
            if (!m_previewChannels.contains(urlStr))
                enqueuePreview(QUrl(urlStr), host, channel, msg.msgid);
        }
    }

    m_chatView->scrollToBottom();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_tray && m_tray->isVisible()) {
        hide();
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::PaletteChange && m_sendBtn)
        m_sendBtn->setIcon(MenuIcons::send({}, 26));

    if (event->type() == QEvent::ActivationChange && isActiveWindow() && m_tray)
        m_tray->setNotify(false);

    if (event->type() == QEvent::WindowStateChange) {
        const auto *sc = static_cast<QWindowStateChangeEvent *>(event);
        const bool wasMaximized = sc->oldState() & Qt::WindowMaximized;
        const bool isNormal     = !(windowState() & Qt::WindowMaximized);
        if (wasMaximized && isNormal) {
            QTimer::singleShot(0, this, [this]{
                auto *scr = screen() ? screen() : QGuiApplication::primaryScreen();
                if (!scr) return;
                const QRect avail = scr->availableGeometry();
                QRect w = geometry();
                if (w.width() > avail.width() - 80)
                    w.setWidth(qMin(kDefaultWindowW, avail.width() - 100));
                if (w.right()  > avail.right())  w.moveRight(avail.right());
                if (w.left()   < avail.left())   w.moveLeft(avail.left());
                setGeometry(w);
            });
        }
    }

    QMainWindow::changeEvent(event);
}

QString MainWindow::formatMessage(const Message &msg) const
{
    ChatRenderer::Context ctx;
    ctx.coloredNicks = m_config.ui.coloredNicks;
    ctx.nickBrackets = m_config.ui.nickBrackets;
    ctx.emojiPt      = m_config.ui.fontSizes.emoji;
    ctx.chatPt       = m_config.ui.fontSizes.chat;
    ctx.validTheme   = m_theme.valid;
    ctx.themeText    = m_theme.text;
    ctx.selfNickRe   = m_selfNickRe;
    ctx.channel      = m_model->channel(m_model->activeHost(), m_model->activeChannel());
    return ChatRenderer::formatMessage(msg, ctx);
}

void MainWindow::fetchAvatar(const QString &url)
{
    if (url.isEmpty() || m_avatarCache.contains(url) || m_avatarFetching.contains(url))
        return;

    auto cacheAndRefresh = [this, url](QPixmap px) {
        if (px.isNull()) return;
        px = px.scaled(36, 36, Qt::KeepAspectRatio, Qt::FastTransformation);
        static constexpr int kAvatarCacheCap = 200;
        if (!m_avatarCache.contains(url)) {
            if (m_avatarCacheOrder.size() >= kAvatarCacheCap)
                m_avatarCache.remove(m_avatarCacheOrder.takeFirst());
            m_avatarCacheOrder.append(url);
        }
        m_avatarCache.insert(url, px);
        scheduleNickRefresh(m_model->activeHost().str(), m_model->activeChannel().str());
    };

    // Local file — load directly without network
    const QUrl qurl(url);
    if (qurl.isLocalFile()) {
        cacheAndRefresh(QPixmap(qurl.toLocalFile()));
        return;
    }
    if (url.startsWith('/')) {
        cacheAndRefresh(QPixmap(url));
        return;
    }

    if (!m_avatarNam)
        m_avatarNam = new QNetworkAccessManager(this);
    m_avatarFetching.insert(url);
    QNetworkRequest req{qurl};
    req.setRawHeader("User-Agent", "Uplink/" UPLINK_VERSION);
    auto *reply = m_avatarNam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, url, cacheAndRefresh] {
        reply->deleteLater();
        m_avatarFetching.remove(url);
        if (reply->error() != QNetworkReply::NoError)
            return;
        QPixmap px;
        if (px.loadFromData(reply->readAll()))
            cacheAndRefresh(px);
    });
}
