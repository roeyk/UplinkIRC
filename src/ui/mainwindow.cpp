#include "mainwindow.h"
#include "irc/ircclient.h"
#include "irc/dccsend.h"
#include "irc/dccreceive.h"
#include "ui/trayicon.h"
#include "ui/aboutdialog.h"
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
#include "ui/channelpane.h"
#include "config/config.h"

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QKeyEvent>
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
#include <QRandomGenerator>
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
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        const bool selected = opt.state & QStyle::State_Selected;
        const bool hovered  = opt.state & QStyle::State_MouseOver;

        if (selected || hovered) {
            const QColor bg = selected ? m_accent : m_hover;
            if (bg.isValid()) {
                // Use the style's actual text rect so we measure from where
                // Qt will draw the text, not the raw item rect edge.
                const QWidget *w = opt.widget;
                const QStyle  *s = w ? w->style() : QApplication::style();
                const QRect textRect = s->subElementRect(
                    QStyle::SE_ItemViewItemText, &opt, w);

                const QFontMetrics fm(opt.font);
                const int textW = fm.horizontalAdvance(opt.text);
                constexpr int hPad = 8;
                constexpr int vPad = 2;
                // Qt draws text at textRect.x() + PM_FocusFrameHMargin+1 internally;
                // shift pill origin right by that margin so padding is symmetric.
                const int textMargin = s->pixelMetric(QStyle::PM_FocusFrameHMargin, &opt, w) + 1;
                const int pillX = textRect.x() + textMargin - hPad;
                QRect r(pillX,
                        opt.rect.y() + vPad,
                        qMin(textW + hPad * 2, opt.rect.right() - pillX),
                        opt.rect.height() - vPad * 2);
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setPen(Qt::NoPen);
                painter->setBrush(bg);
                painter->drawRoundedRect(r, 10.0, 10.0);
                painter->restore();
            }
        }

        // Suppress focus rect and hover shift from the native style.
        opt.state &= ~(QStyle::State_HasFocus | QStyle::State_MouseOver);
        if (selected && m_activeText.isValid()) {
            // Keep State_Selected so Qt uses HighlightedText for the text color,
            // but make the Highlight background transparent so our rounded rect shows.
            opt.palette.setColor(QPalette::All, QPalette::Highlight,       QColor(Qt::transparent));
            opt.palette.setColor(QPalette::All, QPalette::HighlightedText, m_activeText);
        }
        QStyledItemDelegate::paint(painter, opt, index);
    }
};

// Minimum width of the topic bar left zone — wide enough to always show the
// hamburger (22) + gear (22) + right margin (4) even when the sidebar is closed.
static constexpr int kBtnZoneMinW = 60;

static QString linkifyTopic(const QString &text);

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

class ChatBrowser : public QTextBrowser {
public:
    using QTextBrowser::QTextBrowser;
protected:
    QVariant loadResource(int type, const QUrl &name) override {
        if (type == QTextDocument::ImageResource) {
            const QString s = name.toString();
            if (s.startsWith("data:image/")) {
                const qsizetype comma = s.indexOf(',');
                if (comma >= 0) {
                    QImage img;
                    img.loadFromData(QByteArray::fromBase64(s.mid(comma + 1).toLatin1()));
                    if (!img.isNull())
                        return QPixmap::fromImage(std::move(img));
                }
            }
        }
        return QTextBrowser::loadResource(type, name);
    }
};

static void insertHtmlBlock(QTextBrowser *view, const QString &html, bool hangIndent = false)
{
    QTextCursor cursor(view->document());
    cursor.movePosition(QTextCursor::End);
    if (view->document()->characterCount() > 1)
        cursor.insertBlock();
    if (hangIndent) {
        const QFontMetrics fm(view->font());
        const int indent = fm.horizontalAdvance("00:00  ");
        QTextBlockFormat fmt;
        fmt.setLeftMargin(indent);
        fmt.setTextIndent(-indent);
        cursor.setBlockFormat(fmt);
    }
    cursor.insertHtml(html);
}

// ---------------------------------------------------------------------------
// Nick color — consistent hash-based color per nick
// ---------------------------------------------------------------------------

QColor MainWindow::nickColor(const QString &nick)
{
    static const char *palette[] = {
        "#e06c75", "#98c379", "#e5c07b", "#61afef",
        "#c678dd", "#56b6c2", "#d19a66", "#7ec8a0",
        "#ff7b72", "#79c0ff", "#ffa657", "#85e89d",
        "#f78166", "#58a6ff", "#d4a0f5", "#4db5bd",
    };
    static constexpr int N = int(sizeof(palette) / sizeof(palette[0]));
    return QColor(palette[qHash(nick.toLower()) % N]);
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
    resize(1100, 700);

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

    statusBar()->hide();

    QSettings settings("LinuxDojo", "Uplink");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    if (settings.contains("nickSplitter"))
        m_chatSplitter->restoreState(settings.value("nickSplitter").toByteArray());
    if (settings.contains("sidebarWidth"))
        m_sidebarExpandedWidth = settings.value("sidebarWidth").toInt();

    const QStringList savedPanes       = settings.value("panes").toStringList();
    const int         savedPrimarySlot = settings.value("primarySlot", 0).toInt();

    QTimer::singleShot(0, this, [this]{
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
        QSettings s("LinuxDojo", "Uplink");
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
}

MainWindow::~MainWindow() = default;

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

        menu->addAction(MenuIcons::about(ic), "Check for Updates", this, [this]{
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

        menu->addAction(MenuIcons::servers(ic), "Open Config", this, [this]{
            QDesktopServices::openUrl(QUrl::fromLocalFile(Config::defaultPath()));
        });

        menu->addAction(MenuIcons::connStatus(ic), "Reload Config", this, []{
            QProcess::startDetached(QCoreApplication::applicationFilePath(),
                                    QCoreApplication::arguments());
            QCoreApplication::quit();
        });

        menu->addSeparator();
        menu->addAction(MenuIcons::exit(ic), "Exit", menu, &QMenu::close);

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
            m_chatView->document()->setDefaultStyleSheet(
                QString("a { color: %1; text-decoration: underline; }").arg(m_theme.accent));
        if (m_sidebarDelegate && m_theme.valid)
            m_sidebarDelegate->setColors(QColor(m_theme.accent),
                                         QColor(m_theme.border),
                                         QColor(m_theme.text));
        if (m_theme.valid) {
            if (m_primaryTopicBtn) {
                const bool on = m_primaryTopicBtn->isChecked();
                m_primaryTopicBtn->setIcon(makeTopicIcon(
                    QColor(on ? m_theme.accent : m_theme.placeholder)));
            }
            for (auto *pane : std::as_const(m_panes)) {
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
            refreshNickList(m_model->activeHost(), m_model->activeChannel());
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
                m_model->removeServer(old.host);
        }
        for (const ServerConfig &sc : updated) {
            const ServerConfig *existing = nullptr;
            for (const ServerConfig &old : m_config.servers)
                if (old.host == sc.host) { existing = &old; break; }
            if (!existing) {
                m_model->addServer(sc);
            } else if (*existing != sc) {
                m_model->updateServer(existing->host, sc);
            }
        }
        m_config.servers = updated;
        Config::save(m_config, Config::defaultPath());
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
        QFont f(fam, pt);
        f.setStyleHint(QFont::Monospace);
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
        p->chatView()->document()->setDefaultFont(chatFont);
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
    QPixmap pm(10, 10);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor("#a6adc8"));
    p.setPen(Qt::NoPen);
    p.drawEllipse(1, 1, 8, 8);
    return QIcon(pm);
}

void MainWindow::setupSidebar()
{
    m_sidebar = new QTreeWidget;
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
                l->setContentsMargins(0, 0, 8, 8);
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
    m_nickList->setSpacing(0);
    m_nickList->setItemDelegate(new FixedRowDelegate(16, m_nickList));
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
    vbox->setContentsMargins(0, 0, 8, 8);
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
    primaryVbox->addWidget(primaryHeader);

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
    primaryVbox->addWidget(m_topicDisplay);

    // Chat view
    m_chatView = new ChatBrowser;
    m_chatView->setReadOnly(true);
    m_chatView->setLineWrapMode(QTextEdit::WidgetWidth);
    m_chatView->setOpenLinks(false);
    m_chatView->document()->setMaximumBlockCount(kMessageBufferCap + 300);
    if (m_theme.valid)
        m_chatView->document()->setDefaultStyleSheet(
            QString("a { color: %1; text-decoration: underline; }").arg(m_theme.accent));
    connect(m_chatView, &QTextBrowser::anchorClicked, this, [](const QUrl &url){
        QString s = url.scheme().toLower();
        QUrl target = url;
        if (s == "preview") {
            target = QUrl(url.toString().mid(8));
            s = target.scheme().toLower();
        }
        if (s == "http" || s == "https")
            QDesktopServices::openUrl(target);
    });

    m_linkPreview = new LinkPreview(this);

    m_previewWatchdog = new QTimer(this);
    m_previewWatchdog->setSingleShot(true);
    connect(m_previewWatchdog, &QTimer::timeout, this, [this]{
        m_previewFetchBusy = false;
        processPreviewQueue();
    });

    m_chatView->viewport()->setMouseTracking(true);
    m_chatView->viewport()->installEventFilter(this);
    m_chatView->installEventFilter(this);

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
        const QString host    = it->first;
        const QString channel = it->second;
        m_previewChannels.erase(it);
        processPreviewQueue();

        auto *ch = m_model->channel(host, channel);
        if (!ch) return;

        QString imgHtml;
        if (!thumbnail.isNull()) {
            QByteArray imgData;
            QBuffer imgBuf(&imgData);
            imgBuf.open(QIODevice::WriteOnly);
            thumbnail.save(&imgBuf, "PNG");
            const QString dataUri = "data:image/png;base64,"
                                  + QString::fromLatin1(imgData.toBase64());
            imgHtml = QString("<br/><img src=\"%1\" width=\"%2\" height=\"%3\"/>")
                .arg(dataUri).arg(thumbnail.width()).arg(thumbnail.height());
        }

        const QColor bg = m_chatView->palette().color(QPalette::Base);
        const QColor border(m_theme.border);
        const QColor fg(m_theme.text);
        const QColor sub(m_theme.timestamp);

        const QString titleEsc  = title.toHtmlEscaped().left(120);
        const QString domainEsc = pageUrl.host().toHtmlEscaped();

        const int cardLeft = m_config.ui.hangingIndent
            ? QFontMetrics(m_chatView->font()).horizontalAdvance("00:00  ")
            : 20;

        const QString cardHtml =
            QString("<table cellpadding=\"5\" cellspacing=\"0\" "
                    "style=\"margin:1px 0 3px %1px;"
                    "border-left:3px solid %2;"
                    "background-color:%3\"><tr><td>")
                .arg(cardLeft).arg(border.name(), bg.name())
            + "<a href=\"preview:" + urlStr.toHtmlEscaped()
            + "\" style=\"text-decoration:none\">"
            + QString("<span style=\"color:%1;font-weight:bold\">%2</span><br/>"
                      "<span style=\"color:%3;font-size:8pt\">%4</span>")
                .arg(fg.name(), titleEsc, sub.name(), domainEsc)
            + imgHtml
            + "</a></td></tr></table>";

        ch->addPreview(urlStr, cardHtml);

        if (host != m_model->activeHost() ||
            channel.toLower() != m_model->activeChannel().toLower())
            return;

        QScrollBar *sb = m_chatView->verticalScrollBar();
        const bool atBottom = sb->value() >= sb->maximum() - 4;

        insertHtmlBlock(m_chatView, cardHtml);

        if (atBottom)
            sb->setValue(sb->maximum());
    });

    m_chatSplitter = new QSplitter(Qt::Horizontal);
    m_chatSplitter->setHandleWidth(0);
    m_chatSplitter->addWidget(m_chatView);
    m_chatSplitter->addWidget(m_nickPanel);
    m_chatSplitter->setStretchFactor(0, 1);
    m_chatSplitter->setStretchFactor(1, 0);
    primaryVbox->addWidget(m_chatSplitter, 1);

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

    m_mainSplitter = new QSplitter(Qt::Horizontal);
    m_mainSplitter->setHandleWidth(0);
    m_mainSplitter->addWidget(m_sidebarPanel);
    m_mainSplitter->addWidget(m_rightContent);
    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 1);

    auto *outer      = new QWidget;
    auto *outerVbox  = new QVBoxLayout(outer);
    outerVbox->setContentsMargins(0, 0, 0, 0);
    outerVbox->setSpacing(0);
    outerVbox->addWidget(m_topicBar);
    outerVbox->addWidget(m_mainSplitter, 1);

    setCentralWidget(outer);
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

    m_input = new QLineEdit;
    m_input->setPlaceholderText("Type a message...");
    m_input->installEventFilter(this);

    m_emojiBtn = new QPushButton("😊");
    m_emojiBtn->setFixedSize(30, 30);
    m_emojiBtn->setStyleSheet("QPushButton { padding: 0; font-size: 16px; }");
    m_emojiBtn->setVisible(m_showEmojiBtn);
    m_emojiBtn->setToolTip("Emoji picker");

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
            if (!text.isEmpty()) {
                QTextCursor c(m_chatView->document());
                c.movePosition(QTextCursor::Start);
                m_chatView->setTextCursor(c);
                m_chatView->find(text);
            } else {
                m_chatView->setTextCursor(QTextCursor(m_chatView->document()));
            }
        });
        auto *closeBtn = new QToolButton;
        closeBtn->setText("✕");
        closeBtn->setFixedSize(22, 22);
        closeBtn->setAutoRaise(true);
        connect(closeBtn, &QToolButton::clicked, this, [this]{
            m_searchBar->hide();
            m_chatView->setTextCursor(QTextCursor(m_chatView->document()));
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

    // Emoji picker popup
    m_emojiPicker = new EmojiPicker(this);
    connect(m_emojiPicker, &EmojiPicker::emojiSelected, this, [this](const QString &emoji){
        if (!m_pendingReactMsgid.isEmpty()) {
            m_model->sendReact(m_pendingReactHost, m_pendingReactChannel,
                               m_pendingReactMsgid, emoji);
            m_pendingReactMsgid.clear();
            m_pendingReactHost.clear();
            m_pendingReactChannel.clear();
            return;
        }
        const int pos = m_input->cursorPosition();
        const QString text = m_input->text();
        m_input->setText(text.left(pos) + emoji + text.mid(pos));
        m_input->setCursorPosition(pos + static_cast<int>(emoji.length()));
        m_input->setFocus();
    });
    connect(m_emojiBtn, &QPushButton::clicked, this, [this]{
        const QPoint anchor = m_emojiBtn->mapToGlobal(
            QPoint(m_emojiBtn->width(), m_emojiBtn->height()));
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
        const QString host = m_model->activeHost();
        const QString ch   = m_model->activeChannel();
        if (ch.isEmpty() || ch == "(server)") return;
        m_typingActive = false;
        m_model->sendTyping(host, ch, "paused");
    });

    connect(m_input, &QLineEdit::textChanged, this, [this](const QString &text){
        checkEmojiAutocomplete(text);
    });

    connect(m_input, &QLineEdit::textChanged, this, [this](const QString &text){
        if (!m_config.ui.typingIndicator) return;
        const QString host = m_model->activeHost();
        const QString ch   = m_model->activeChannel();
        if (ch.isEmpty() || ch == "(server)") return;
        if (!text.isEmpty()) {
            if (!m_typingActive) {
                m_typingActive = true;
                m_model->sendTyping(host, ch, "active");
            }
            m_typingOutTimer->start();
        } else {
            m_typingOutTimer->stop();
            if (m_typingActive) {
                m_typingActive = false;
                m_model->sendTyping(host, ch, "done");
            }
        }
    });

    connect(m_input, &QLineEdit::returnPressed, this, &MainWindow::onInputSubmit);
}

void MainWindow::connectModel()
{
    connect(m_model, &SessionModel::serverAdded,       this, &MainWindow::onServerAdded);
    connect(m_model, &SessionModel::serverConnected,   this, &MainWindow::onServerConnected);
    connect(m_model, &SessionModel::serverDisconnected,this, &MainWindow::onServerDisconnected);
    connect(m_model, &SessionModel::channelAdded,      this, &MainWindow::onChannelAdded);
    connect(m_model, &SessionModel::channelRemoved,    this, &MainWindow::onChannelRemoved);
    connect(m_model, &SessionModel::messageAdded,      this, &MainWindow::onMessageAdded);
    connect(m_model, &SessionModel::topicChanged,      this, &MainWindow::onTopicChanged);
    connect(m_model, &SessionModel::modesChanged, this, [this](const QString &host, const QString &channel){
        if (host == m_model->activeHost() && channel.toLower() == m_model->activeChannel().toLower())
            refreshTopicBar(host, channel);
    });
    connect(m_model, &SessionModel::nickListChanged,   this, &MainWindow::onNickListChanged);
    connect(m_model, &SessionModel::unreadChanged,     this, &MainWindow::onUnreadChanged);
    connect(m_model, &SessionModel::reactionsChanged,  this, &MainWindow::onReactionsChanged);
    connect(m_model, &SessionModel::selfNickChanged,   this, &MainWindow::onSelfNickChanged);
    connect(m_model, &SessionModel::typingReceived,    this, &MainWindow::onTypingReceived);
    connect(m_model, &SessionModel::messageRedacted,   this, &MainWindow::onMessageRedacted);

    connect(m_model, &SessionModel::sslFingerprintPrompt, this,
            [this](const QString &host, const QString &fp)
    {
        QMessageBox box(this);
        box.setWindowTitle("Untrusted Certificate");
        box.setText(host + " is using a self-signed certificate.");
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
            [this](const QString &, const QString &fromNick,
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

        const QString savePath = QFileDialog::getSaveFileName(this, "Save File", filename);
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
            [this](const QString &server, const QString &fromNick,
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

        const QString savePath = QFileDialog::getSaveFileName(this, "Save File", filename);
        if (savePath.isEmpty()) return;

        auto *dcc = new DccReceive(savePath, 0, 0, filesize, this);
        if (!dcc->listenPassive(senderIp)) { dcc->deleteLater(); return; }

        IrcClient *client = m_model->clientFor(server);
        const quint32 ourIp   = client ? client->localIpv4() : 0;
        const quint16 ourPort = dcc->listenPort();
        const QString fn      = QFileInfo(filename).fileName().replace(' ', '_');

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
            [this](const QString &, const QString &, const QString &,
                   quint32 ip, quint16 port, qint64, const QString &token)
    {
        DccSend *dcc = m_pendingPassiveSends.take(token);
        if (dcc) dcc->connectOut(ip, port);
    });

    connect(m_model, &SessionModel::pingRtt, this, [this](const QString &host, int ms){
        if (m_signalBars && host == m_model->activeHost())
            m_signalBars->setLatency(ms);
    });
    connect(m_model, &SessionModel::serverReconnecting, this, [this](const QString &host){
        if (m_signalBars && host == m_model->activeHost())
            m_signalBars->setState(SignalBars::State::Reconnecting);
    });
}

// ---------------------------------------------------------------------------
// Event filter — Tab completion + input history
// ---------------------------------------------------------------------------

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_chatView->viewport()) {
        if (event->type() == QEvent::MouseMove) {
            auto *me = static_cast<QMouseEvent *>(event);
            const QString anchor = m_chatView->anchorAt(me->position().toPoint());
            const bool isNick = anchor.startsWith("nick:");
            if (anchor != m_hoveredUrl) {
                m_hoveredUrl = anchor;
                if (anchor.isEmpty() || isNick) {
                    QToolTip::hideText();
                    statusBar()->clearMessage();
                } else {
                    m_hoverGlobalPos = m_chatView->viewport()->mapToGlobal(
                        me->position().toPoint());
                    const QUrl url(anchor);
                    statusBar()->showMessage(url.host());
                    QToolTip::showText(m_hoverGlobalPos, url.host(),
                                       m_chatView->viewport());
                    if (m_config.ui.linkPreviews)
                        m_linkPreview->fetchHover(url);
                }
            }
        } else if (event->type() == QEvent::ContextMenu) {
            auto *ce = static_cast<QContextMenuEvent *>(event);
            const QString anchor = m_chatView->anchorAt(ce->pos());
            const QPoint globalPos = ce->globalPos();

            if (anchor.startsWith("nick:")) {
                showNickContextMenu(anchor.mid(5), globalPos);
                return true;
            }

            if (anchor.startsWith("msgid:")) {
                const QString msgid   = anchor.mid(6);
                const QString host    = m_model->activeHost();
                const QString channel = m_model->activeChannel();
                QMenu menu(m_chatView->viewport());
                connect(menu.addAction("Reply"), &QAction::triggered, this,
                        [this, msgid, host, channel]{
                    auto *ch = m_model->channel(host, channel);
                    QString origNick;
                    if (ch)
                        for (const auto &m : std::as_const(ch->messages))
                            if (m.msgid == msgid) { origNick = m.nick; break; }
                    m_pendingReplyMsgid = msgid;
                    if (m_replyLabel)
                        m_replyLabel->setText("↩ " + (origNick.isEmpty() ? msgid : origNick));
                    if (m_replyBar) m_replyBar->show();
                    if (m_input)    m_input->setFocus();
                });
                connect(menu.addAction("React"), &QAction::triggered, this,
                        [this, msgid, host, channel, globalPos]{
                    m_pendingReactMsgid    = msgid;
                    m_pendingReactHost     = host;
                    m_pendingReactChannel  = channel;
                    m_emojiPicker->showAt(globalPos);
                });
                {
                    auto *cl = m_model->clientFor(host);
                    auto *ch = m_model->channel(host, channel);
                    if (cl && cl->hasCap("draft/message-redaction") && ch) {
                        QString msgNick;
                        for (const auto &m : std::as_const(ch->messages))
                            if (m.msgid == msgid) { msgNick = m.nick; break; }
                        if (msgNick == m_model->selfNick(host)) {
                            connect(menu.addAction("Delete"), &QAction::triggered, this,
                                    [this, msgid, host, channel]{
                                m_model->sendRedact(host, channel, msgid);
                            });
                        }
                    }
                }
                if (m_chatView->textCursor().hasSelection())
                    connect(menu.addAction("Copy"), &QAction::triggered,
                            this, [this]{ m_chatView->copy(); });
                menu.exec(globalPos);
                return true;
            }

            if (anchor.startsWith("preview:")) {
                const QString url     = anchor.mid(8);
                const QString host    = m_model->activeHost();
                const QString channel = m_model->activeChannel();
                QMenu menu(m_chatView->viewport());
                connect(menu.addAction("Open URL"), &QAction::triggered, this, [url]{
                    const QUrl u(url);
                    const QString s = u.scheme().toLower();
                    if (s == "http" || s == "https")
                        QDesktopServices::openUrl(u);
                });
                auto *ch = m_model->channel(host, channel);
                if (ch && ch->previews.contains(url)) {
                    connect(menu.addAction("Hide Preview"), &QAction::triggered, this,
                            [this, url, host, channel]{
                        auto *inner = m_model->channel(host, channel);
                        if (inner) inner->hiddenPreviews.insert(url);
                        refreshChatView(host, channel);
                    });
                }
                menu.exec(globalPos);
                return true;
            }

            if (!anchor.isEmpty()) {
                const QString host    = m_model->activeHost();
                const QString channel = m_model->activeChannel();
                QMenu menu(m_chatView->viewport());

                connect(menu.addAction("Copy URL"), &QAction::triggered,
                        this, [anchor]{ QApplication::clipboard()->setText(anchor); });
                connect(menu.addAction("Open URL"), &QAction::triggered, this, [anchor]{
                    const QUrl u(anchor);
                    const QString s = u.scheme().toLower();
                    if (s == "http" || s == "https")
                        QDesktopServices::openUrl(u);
                });

                auto *ch = m_model->channel(host, channel);
                const bool isHidden  = ch && ch->hiddenPreviews.contains(anchor);
                const bool hasPreview = ch && ch->previews.contains(anchor);
                if (isHidden) {
                    auto *showAction = menu.addAction("Show Preview");
                    connect(showAction, &QAction::triggered, this, [this, anchor, host, channel]{
                        auto *inner = m_model->channel(host, channel);
                        if (inner) inner->hiddenPreviews.remove(anchor);
                        refreshChatView(host, channel);
                    });
                } else {
                    auto *hideAction = menu.addAction("Hide Preview");
                    hideAction->setEnabled(hasPreview);
                    connect(hideAction, &QAction::triggered, this, [this, anchor, host, channel]{
                        auto *inner = m_model->channel(host, channel);
                        if (inner) inner->hiddenPreviews.insert(anchor);
                        refreshChatView(host, channel);
                    });
                }

                menu.exec(globalPos);
            } else if (m_chatView->textCursor().hasSelection()) {
                QMenu menu(m_chatView->viewport());
                connect(menu.addAction("Copy"), &QAction::triggered,
                        this, [this]{ m_chatView->copy(); });

                // Find the msgid anchor in the clicked paragraph so Reply
                // is available even when right-clicking on message body text.
                QString foundMsgid;
                const QTextBlock block = m_chatView->cursorForPosition(ce->pos()).block();
                for (auto it = block.begin(); !it.atEnd(); ++it) {
                    const QString href = it.fragment().charFormat().anchorHref();
                    if (href.startsWith("msgid:")) { foundMsgid = href.mid(6); break; }
                }
                if (!foundMsgid.isEmpty()) {
                    const QString host    = m_model->activeHost();
                    const QString channel = m_model->activeChannel();
                    connect(menu.addAction("Reply"), &QAction::triggered, this,
                            [this, foundMsgid, host, channel]{
                        auto *ch = m_model->channel(host, channel);
                        QString origNick;
                        if (ch)
                            for (const auto &m : std::as_const(ch->messages))
                                if (m.msgid == foundMsgid) { origNick = m.nick; break; }
                        m_pendingReplyMsgid = foundMsgid;
                        if (m_replyLabel)
                            m_replyLabel->setText("↩ " + (origNick.isEmpty() ? foundMsgid : origNick));
                        if (m_replyBar) m_replyBar->show();
                        if (m_input)    m_input->setFocus();
                    });
                }

                menu.exec(globalPos);
            }
            return true;
        } else if (event->type() == QEvent::Leave) {
            m_hoveredUrl.clear();
            QToolTip::hideText();
            statusBar()->clearMessage();
        }
        return false;
    }

    // Pane chat view viewports — ContextMenu only (no hover/link preview in panes)
    for (auto *p : std::as_const(m_orderedPanes)) {
        if (obj != p->chatView()->viewport()) continue;
        if (event->type() != QEvent::ContextMenu) return false;
        auto *ce           = static_cast<QContextMenuEvent *>(event);
        const QString anch = p->chatView()->anchorAt(ce->pos());
        const QPoint gp    = ce->globalPos();
        const QString host = p->host();
        const QString chan  = p->channel();

        if (anch.startsWith("nick:")) {
            showNickContextMenu(anch.mid(5), gp);
            return true;
        }
        if (anch.startsWith("msgid:")) {
            const QString msgid = anch.mid(6);
            QMenu menu(p->chatView()->viewport());
            connect(menu.addAction("Reply"), &QAction::triggered, this,
                    [this, msgid, host, chan]{
                auto *ch = m_model->channel(host, chan);
                QString nick;
                if (ch) for (const auto &m : std::as_const(ch->messages))
                    if (m.msgid == msgid) { nick = m.nick; break; }
                m_pendingReplyMsgid = msgid;
                if (m_replyLabel) m_replyLabel->setText("↩ " + (nick.isEmpty() ? msgid : nick));
                if (m_replyBar)   m_replyBar->show();
                if (m_input)      m_input->setFocus();
            });
            connect(menu.addAction("React"), &QAction::triggered, this,
                    [this, msgid, host, chan, gp]{
                m_pendingReactMsgid = msgid; m_pendingReactHost = host;
                m_pendingReactChannel = chan; m_emojiPicker->showAt(gp);
            });
            auto *cl = m_model->clientFor(host);
            auto *ch = m_model->channel(host, chan);
            if (cl && cl->hasCap("draft/message-redaction") && ch) {
                QString msgNick;
                for (const auto &m : std::as_const(ch->messages))
                    if (m.msgid == msgid) { msgNick = m.nick; break; }
                if (msgNick == m_model->selfNick(host))
                    connect(menu.addAction("Delete"), &QAction::triggered, this,
                            [this, host, chan, msgid]{
                        m_model->sendRedact(host, chan, msgid, "deleted");
                    });
            }
            menu.exec(gp);
            return true;
        }
        if (p->chatView()->textCursor().hasSelection()) {
            QMenu menu(p->chatView()->viewport());
            connect(menu.addAction("Copy"), &QAction::triggered, this,
                    [p]{ p->chatView()->copy(); });
            QString foundMsgid;
            const QTextBlock blk = p->chatView()->cursorForPosition(ce->pos()).block();
            for (auto it = blk.begin(); !it.atEnd(); ++it) {
                const QString href = it.fragment().charFormat().anchorHref();
                if (href.startsWith("msgid:")) { foundMsgid = href.mid(6); break; }
            }
            if (!foundMsgid.isEmpty())
                connect(menu.addAction("Reply"), &QAction::triggered, this,
                        [this, foundMsgid, host, chan]{
                    auto *ch = m_model->channel(host, chan);
                    QString nick;
                    if (ch) for (const auto &m : std::as_const(ch->messages))
                        if (m.msgid == foundMsgid) { nick = m.nick; break; }
                    m_pendingReplyMsgid = foundMsgid;
                    if (m_replyLabel) m_replyLabel->setText("↩ " + (nick.isEmpty() ? foundMsgid : nick));
                    if (m_replyBar)   m_replyBar->show();
                    if (m_input)      m_input->setFocus();
                });
            menu.exec(gp);
            return true;
        }
        return false;
    }

    if (obj == m_chatView && event->type() == QEvent::Resize) {
        repositionTypingLabel();
        return false;
    }

    if (obj == m_chatView && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_F && (ke->modifiers() & Qt::ControlModifier)) {
            showSearchBar();
            return true;
        }
        return false;
    }

    if (obj == m_searchInput && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Escape) {
            m_searchBar->hide();
            m_chatView->setTextCursor(QTextCursor(m_chatView->document()));
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
        handleTabComplete(m_input, m_model->activeHost(), m_model->activeChannel());
        return true;
    }

    // Any non-Tab key resets nick completion cycle
    m_tabActive = false;
    m_tabCandidates.clear();

    if (ke->key() == Qt::Key_Up && !m_emojiCompleter->isVisible()) {
        handleHistoryUp();
        return true;
    }
    if (ke->key() == Qt::Key_Down && !m_emojiCompleter->isVisible()) {
        handleHistoryDown();
        return true;
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::handleTabComplete(QLineEdit *input, const QString &host, const QString &channel)
{
    const QString text = input->text();
    const int pos = input->cursorPosition();

    // Find start of word before cursor
    const qsizetype wordStart = text.lastIndexOf(' ', pos - 1) + 1;
    const QString prefix = text.mid(wordStart, pos - wordStart);

    if (prefix.isEmpty()) return;

    // Build candidate list if this is a new cycle or prefix changed
    if (!m_tabActive || prefix != m_tabPrefix) {
        m_tabPrefix = prefix;
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
            auto *ch = m_model->channel(host, channel);
            if (ch) {
                for (const auto &e : std::as_const(ch->nicks))
                    if (e.nick.startsWith(prefix, Qt::CaseInsensitive))
                        m_tabCandidates << e.nick;
            }
        }
    }

    if (m_tabCandidates.isEmpty()) return;

    const QString completed = m_tabCandidates[m_tabCandidateIndex];
    m_tabCandidateIndex = static_cast<int>((m_tabCandidateIndex + 1) % m_tabCandidates.size());

    // Commands always get a space suffix; nicks get ": " at line start, " " otherwise
    QString suffix;
    if (pos == text.length())
        suffix = (wordStart == 0 && !completed.startsWith('/'))
            ? QStringLiteral(": ") : QStringLiteral(" ");

    input->setText(text.left(wordStart) + completed + suffix + text.mid(pos));
    input->setCursorPosition(static_cast<int>(wordStart + completed.length() + suffix.length()));
}

void MainWindow::handleHistoryUp()
{
    if (m_inputHistory.isEmpty()) return;
    if (m_historyIndex == -1)
        m_historyDraft = m_input->text();
    m_historyIndex = qMin(m_historyIndex + 1, static_cast<int>(m_inputHistory.size()) - 1);
    m_input->setText(m_inputHistory[m_historyIndex]);
    m_input->end(false);
}

void MainWindow::handleHistoryDown()
{
    if (m_historyIndex == -1) return;
    m_historyIndex--;
    if (m_historyIndex < 0) {
        m_historyIndex = -1;
        m_input->setText(m_historyDraft);
    } else {
        m_input->setText(m_inputHistory[m_historyIndex]);
    }
    m_input->end(false);
}

// ---------------------------------------------------------------------------
// Emoji inline autocomplete
// ---------------------------------------------------------------------------

void MainWindow::checkEmojiAutocomplete(const QString &text)
{
    const int cursorPos = m_input->cursorPosition();
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
                    const QString newText = text.left(openColon) + emoji + text.mid(cursorPos);
                    m_input->setText(newText);
                    m_input->setCursorPosition(static_cast<int>(openColon + emoji.length()));
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
    QString text    = m_input->text();
    const int start = m_emojiTriggerPos;          // position of ':'
    const int end   = m_input->cursorPosition();  // current cursor
    m_input->setText(text.left(start) + emoji + text.mid(end));
    m_input->setCursorPosition(start + static_cast<int>(emoji.length()));
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

    if (m_signalBars && (m_model->activeHost().isEmpty() || host == m_model->activeHost()))
        m_signalBars->setState(SignalBars::State::Connecting);
}

void MainWindow::onServerConnected(const QString &host)
{
    auto *item = findServerItem(host);
    if (item)
        item->setIcon(0, makeConnectedIcon());
    if (m_signalBars && host == m_model->activeHost())
        m_signalBars->setState(SignalBars::State::Connected);
}

void MainWindow::onServerDisconnected(const QString &host)
{
    auto *item = findServerItem(host);
    if (item)
        item->setIcon(0, QIcon());
    if (m_signalBars && host == m_model->activeHost())
        m_signalBars->setState(SignalBars::State::Disconnected);
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

void MainWindow::onMessageAdded(const QString &host, const QString &channel, const Message &msg)
{
    if (host == m_model->activeHost() &&
        channel.toLower() == m_model->activeChannel().toLower())
    {
        appendMessage(msg, m_config.ui.linkPreviews);
    }

    const QString paneKey = host + "|" + channel.toLower();
    if (auto *pane = m_panes.value(paneKey)) {
        const bool isText = (msg.type == MessageType::Privmsg ||
                             msg.type == MessageType::Action  ||
                             msg.type == MessageType::Notice);
        insertHtmlBlock(pane->chatView(), formatMessage(msg),
                        isText && m_config.ui.hangingIndent);
        auto *sb = pane->chatView()->verticalScrollBar();
        sb->setValue(sb->maximum());
    }

    if (m_config.ui.notifications && m_tray && !isActiveWindow()
        && (msg.type == MessageType::Privmsg || msg.type == MessageType::Action))
    {
        const QString myNick = m_model->selfNick(host);
        const bool isPM = !channel.startsWith('#') && !channel.startsWith('&');
        const bool isMention = !isPM && !myNick.isEmpty()
                               && msg.text.contains(myNick, Qt::CaseInsensitive);
        if (isPM || isMention)
            m_tray->setNotify(true);
    }
}

void MainWindow::onTopicChanged(const QString &host, const QString &channel, const QString &topic)
{
    if (host == m_model->activeHost() &&
        channel.toLower() == m_model->activeChannel().toLower())
        refreshTopicBar(host, channel);

    const QString paneKey = host + "|" + channel.toLower();
    if (auto *pane = m_panes.value(paneKey))
        pane->setTopic(linkifyTopic(topic));
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
        const bool isActive = (host == m_model->activeHost() &&
                               channel.toLower() == m_model->activeChannel().toLower());
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
    if (channel != "(server)") {
        if (count > 0 && m_model->hasMention(host, channel))
            item->setText(0, "💡 " + label);
        else if (count > 0)
            item->setText(0, "🔥 " + label);
        else
            item->setText(0, label);
    }
}

void MainWindow::onReactionsChanged(const QString &host, const QString &channel)
{
    if (host == m_model->activeHost() &&
        channel.toLower() == m_model->activeChannel().toLower())
        refreshChatView(host, channel);
}

void MainWindow::onSelfNickChanged(const QString &host, const QString &nick)
{
    if (host == m_model->activeHost())
        m_nickPrefix->setText(nick);

    for (auto *pane : std::as_const(m_panes))
        if (pane->host() == host)
            pane->setNick(nick);
}

void MainWindow::onMessageRedacted(const QString &host, const QString &channel)
{
    if (host == m_model->activeHost() &&
        channel.toLower() == m_model->activeChannel().toLower())
        refreshChatView(host, channel);
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

void MainWindow::repositionTypingLabel()
{
    // no-op: typing label is now a layout row, not an overlay
}

void MainWindow::updateTypingLabel()
{
    const QString key = m_model->activeHost() + "|" + m_model->activeChannel();
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
// /sysinfo helpers
// ---------------------------------------------------------------------------

static QString sysinfoKernel()
{
    QProcess p;
    p.start("uname", {"-r"});
    p.waitForFinished(2000);
    const QString out = QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed();
    return out.isEmpty() ? "Unknown" : out;
}

static QString sysinfoOS()
{
#if defined(Q_OS_LINUX)
    QString name, buildId, versionId;
    QFile f("/etc/os-release");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&f);
        const auto strip = [](QString s) {
            if (s.startsWith('"') && s.endsWith('"'))
                s = s.mid(1, s.length() - 2);
            return s;
        };
        while (!in.atEnd()) {
            const QString line = in.readLine();
            if (line.startsWith("NAME=") && name.isEmpty())
                name = strip(line.mid(5));
            else if (line.startsWith("BUILD_ID=") && buildId.isEmpty())
                buildId = strip(line.mid(9));
            else if (line.startsWith("VERSION_ID=") && versionId.isEmpty())
                versionId = strip(line.mid(11));
        }
    }
    if (name.isEmpty()) name = "Linux";
    const QString ver = !buildId.isEmpty() ? buildId : versionId;
    const QString distro = ver.isEmpty() ? name : name + " " + ver;
    return QString("Linux (%1) (%2)").arg(distro, sysinfoKernel());
#elif defined(Q_OS_FREEBSD)
    QProcess p;
    p.start("uname", {"-s"});
    p.waitForFinished(2000);
    return QString("%1 (%2)").arg(QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed(),
                                  sysinfoKernel());
#else
    return QSysInfo::prettyProductName();
#endif
}

static QString sysinfoCPU()
{
#if defined(Q_OS_LINUX)
    QFile f("/proc/cpuinfo");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&f);
        while (!in.atEnd()) {
            const QString line = in.readLine();
            if (line.startsWith("model name")) {
                const qsizetype colon = line.indexOf(':');
                if (colon != -1)
                    return line.mid(colon + 1).trimmed();
            }
        }
    }
    return QSysInfo::currentCpuArchitecture();
#elif defined(Q_OS_FREEBSD) || defined(Q_OS_DARWIN)
    QProcess p;
    p.start("sysctl", {"-n", "hw.model"});
    p.waitForFinished(2000);
    const QString model = QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed();
    return model.isEmpty() ? "Unknown" : model;
#elif defined(Q_OS_WIN)
    QSettings reg(
        "HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        QSettings::NativeFormat);
    const QString name = reg.value("ProcessorNameString").toString().trimmed();
    return name.isEmpty() ? QSysInfo::currentCpuArchitecture() : name;
#else
    return QSysInfo::currentCpuArchitecture();
#endif
}

static QString sysinfoMEM()
{
#if defined(Q_OS_LINUX)
    QFile f("/proc/meminfo");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&f);
        while (!in.atEnd()) {
            const QStringList parts = in.readLine().split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 2 && parts[0] == "MemTotal:") {
                const quint64 kb = parts[1].toULongLong();
                return QString("%1 GB").arg((kb + 512 * 1024) / (1024 * 1024));
            }
        }
    }
    return "Unknown";
#elif defined(Q_OS_FREEBSD)
    QProcess p;
    p.start("sysctl", {"-n", "hw.physmem"});
    p.waitForFinished(2000);
    const quint64 total = QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed().toULongLong();
    if (total > 0)
        return QString("%1 GB").arg((total + 512ULL*1024*1024) / (1024ULL*1024*1024));
    return "Unknown";
#elif defined(Q_OS_WIN)
    MEMORYSTATUSEX ms{};
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms))
        return QString("%1 GB").arg((ms.ullTotalPhys + 512ULL*1024*1024) / (1024ULL*1024*1024));
    return "Unknown";
#else
    return "Unknown";
#endif
}

static QString sysinfoGPU()
{
#if defined(Q_OS_LINUX)
    // Try vulkaninfo first — gives device name + renderer string
    QProcess vk;
    vk.start("vulkaninfo", {"--summary"});
    if (vk.waitForFinished(3000) && vk.exitCode() == 0) {
        const QString out = QString::fromLocal8Bit(vk.readAllStandardOutput());
        QString deviceName, driverInfo;
        for (const QString &line : out.split('\n')) {
            const QString t = line.trimmed();
            if (t.startsWith("deviceName") && deviceName.isEmpty())
                deviceName = t.section('=', 1).trimmed();
            else if (t.startsWith("driverInfo") && driverInfo.isEmpty())
                driverInfo = t.section('=', 1).trimmed();
        }
        if (!deviceName.isEmpty()) {
            // driverInfo e.g. "Mesa 24.3.4 (RADV STRIX1)" → extract last parens
            const qsizetype lp = driverInfo.lastIndexOf('(');
            const qsizetype rp = driverInfo.lastIndexOf(')');
            if (lp != -1 && rp > lp)
                return QString("%1 (%2) (Vulkan)").arg(deviceName, driverInfo.mid(lp + 1, rp - lp - 1));
            return deviceName + " (Vulkan)";
        }
    }
    // Fallback: lspci
    QProcess lp;
    lp.start("lspci", {});
    if (lp.waitForFinished(2000)) {
        const QString out = QString::fromLocal8Bit(lp.readAllStandardOutput());
        for (const QString &line : out.split('\n')) {
            if (line.contains("VGA", Qt::CaseInsensitive) ||
                line.contains("3D controller", Qt::CaseInsensitive) ||
                line.contains("Display controller", Qt::CaseInsensitive)) {
                // "06:00.0 VGA compatible controller: AMD ... [Radeon 890M] (rev c8)"
                const qsizetype c2 = line.indexOf(':', line.indexOf(':') + 1);
                if (c2 != -1)
                    return line.mid(c2 + 1).section('(', 0, 0).trimmed();
            }
        }
    }
    return "Unknown";
#elif defined(Q_OS_WIN)
    QProcess p;
    p.start("powershell", {"-NoProfile", "-Command",
        "(Get-CimInstance Win32_VideoController | Select-Object -First 1).Name"});
    if (p.waitForFinished(5000)) {
        const QString out = QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed();
        if (!out.isEmpty()) return out;
    }
    return "Unknown";
#else
    return "Unknown";
#endif
}

static QString sysinfoUptime()
{
#if defined(Q_OS_LINUX)
    QFile f("/proc/uptime");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const quint64 s = static_cast<quint64>(
            QString::fromLocal8Bit(f.readLine()).section(' ', 0, 0).toDouble());
        const quint64 days    = s / 86400;
        const quint64 hours   = (s % 86400) / 3600;
        const quint64 minutes = (s % 3600) / 60;
        const quint64 seconds = s % 60;
        QStringList parts;
        if (days > 0)    parts << QString("%1 day%2").arg(days).arg(days != 1 ? "s" : "");
        if (hours > 0)   parts << QString("%1 hour%2").arg(hours).arg(hours != 1 ? "s" : "");
        if (minutes > 0) parts << QString("%1 minute%2").arg(minutes).arg(minutes != 1 ? "s" : "");
        parts << QString("%1 second%2").arg(seconds).arg(seconds != 1 ? "s" : "");
        return parts.join(' ') + " ago";
    }
    return "Unknown";
#elif defined(Q_OS_FREEBSD) || defined(Q_OS_DARWIN)
    QProcess p;
    p.start("sysctl", {"-n", "kern.boottime"});
    p.waitForFinished(2000);
    // Parse "{ sec = N, usec = N } date" — extract sec and compute elapsed
    const QString out = QString::fromLocal8Bit(p.readAllStandardOutput());
    const int eq = out.indexOf("sec = ");
    const int cm = out.indexOf(',', eq);
    if (eq != -1 && cm != -1) {
        const quint64 bootSec = out.mid(eq + 6, cm - eq - 6).trimmed().toULongLong();
        const quint64 now = QDateTime::currentSecsSinceEpoch();
        const quint64 s   = now > bootSec ? now - bootSec : 0;
        const quint64 days    = s / 86400;
        const quint64 hours   = (s % 86400) / 3600;
        const quint64 minutes = (s % 3600) / 60;
        const quint64 seconds = s % 60;
        QStringList parts;
        if (days > 0)    parts << QString("%1 day%2").arg(days).arg(days != 1 ? "s" : "");
        if (hours > 0)   parts << QString("%1 hour%2").arg(hours).arg(hours != 1 ? "s" : "");
        if (minutes > 0) parts << QString("%1 minute%2").arg(minutes).arg(minutes != 1 ? "s" : "");
        parts << QString("%1 second%2").arg(seconds).arg(seconds != 1 ? "s" : "");
        return parts.join(' ') + " ago";
    }
    return "Unknown";
#elif defined(Q_OS_WIN)
    const quint64 s = GetTickCount64() / 1000;
    const quint64 days    = s / 86400;
    const quint64 hours   = (s % 86400) / 3600;
    const quint64 minutes = (s % 3600) / 60;
    const quint64 seconds = s % 60;
    QStringList parts;
    if (days > 0)    parts << QString("%1 day%2").arg(days).arg(days != 1 ? "s" : "");
    if (hours > 0)   parts << QString("%1 hour%2").arg(hours).arg(hours != 1 ? "s" : "");
    if (minutes > 0) parts << QString("%1 minute%2").arg(minutes).arg(minutes != 1 ? "s" : "");
    parts << QString("%1 second%2").arg(seconds).arg(seconds != 1 ? "s" : "");
    return parts.join(' ') + " ago";
#else
    return "Unknown";
#endif
}

void MainWindow::onInputSubmit()
{
    const QString raw  = m_input->text();
    const QString text = raw.trimmed();
    if (text.isEmpty()) return;

    // Push to history (newest first, skip consecutive duplicates)
    if (m_inputHistory.isEmpty() || m_inputHistory.first() != text) {
        m_inputHistory.prepend(text);
        if (m_inputHistory.size() > 100)
            m_inputHistory.removeLast();
    }
    m_historyIndex = -1;
    m_tabActive = false;
    m_tabCandidates.clear();
    hideEmojiAutocomplete();

    m_input->clear();

    const QString host    = m_model->activeHost();
    const QString channel = m_model->activeChannel();
    if (host.isEmpty() || channel.isEmpty()) return;

    // Stop typing notification on send
    m_typingOutTimer->stop();
    if (m_typingActive && m_config.ui.typingIndicator && channel != "(server)") {
        m_typingActive = false;
        m_model->sendTyping(host, channel, "done");
    }

    dispatchInput(raw, host, channel);
}

void MainWindow::dispatchInput(const QString &text, const QString &host, const QString &channel)
{
    const QString trimmed = text.trimmed();
    if (text.startsWith('/')) {
        const QString cmd  = trimmed.section(' ', 0, 0).toLower();
        const QString args = trimmed.section(' ', 1);

        if (cmd == "/join") {
            m_model->sendJoin(host, args.section(' ', 0, 0), args.section(' ', 1, 1));
        } else if (cmd == "/part" || cmd == "/leave" || cmd == "/close") {
            const bool isChannel = channel.startsWith('#') || channel.startsWith('&');
            if (isChannel)
                m_model->sendPart(host, channel, args);
            else
                m_model->closeBuffer(host, channel);
        } else if (cmd == "/nick") {
            m_model->sendNick(host, args.trimmed());
        } else if (cmd == "/me") {
            m_model->sendAction(host, channel, args);
        } else if (cmd == "/msg") {
            const QString target = args.section(' ', 0, 0);
            const QString body   = args.section(' ', 1);
            m_model->sendMessage(host, target, body);
            if (!target.startsWith('#') && !target.startsWith('&'))
                switchToChannel(host, target);
        } else if (cmd == "/query") {
            const QString target = args.trimmed().section(' ', 0, 0);
            if (!target.isEmpty()) {
                m_model->openPM(host, target);
                switchToChannel(host, target);
                if (m_input) m_input->setFocus();
            }
        } else if (cmd == "/ns") {
            if (!args.isEmpty()) m_model->sendMessage(host, "NickServ", args);
        } else if (cmd == "/cs") {
            if (!args.isEmpty()) m_model->sendMessage(host, "ChanServ", args);
        } else if (cmd == "/bs") {
            if (!args.isEmpty()) m_model->sendMessage(host, "BotServ", args);
        } else if (cmd == "/ms") {
            if (!args.isEmpty()) m_model->sendMessage(host, "MemoServ", args);
        } else if (cmd == "/oper") {
            const QString user = args.section(' ', 0, 0);
            const QString pass = args.section(' ', 1);
            if (!user.isEmpty() && !pass.isEmpty())
                m_model->sendRaw(host, "OPER " + user + " :" + pass);
        } else if (cmd == "/notice") {
            const QString target = args.section(' ', 0, 0);
            const QString msg    = args.section(' ', 1);
            if (!target.isEmpty() && !msg.isEmpty())
                m_model->sendRaw(host, "NOTICE " + target + " :" + msg);
        } else if (cmd == "/quote" || cmd == "/raw") {
            m_model->sendRaw(host, args);
        } else if (cmd == "/quit") {
            if (auto *cl = m_model->clientFor(host)) cl->quit(args.isEmpty() ? "Uplink" : args);
        } else if (cmd == "/away") {
            m_model->sendRaw(host, args.isEmpty() ? "AWAY" : "AWAY :" + args);
        } else if (cmd == "/back") {
            m_model->sendRaw(host, "AWAY");
        } else if (cmd == "/motd") {
            m_model->sendRaw(host, args.isEmpty() ? "MOTD" : "MOTD " + args.trimmed());
        } else if (cmd == "/whois") {
            m_model->sendRaw(host, "WHOIS " + args.trimmed());
        } else if (cmd == "/topic") {
            QString topicTarget = channel;
            QString topicText   = args;
            if (args.startsWith('#') || args.startsWith('&')) {
                topicTarget = args.section(' ', 0, 0);
                topicText   = args.section(' ', 1);
            }
            if (topicText.isEmpty())
                m_model->sendRaw(host, "TOPIC " + topicTarget);
            else
                m_model->sendRaw(host, "TOPIC " + topicTarget + " :" + topicText);
        } else if (cmd == "/kick") {
            const QString target = args.section(' ', 0, 0);
            const QString reason = args.section(' ', 1);
            if (!target.isEmpty())
                m_model->sendRaw(host, "KICK " + channel + " " + target
                                 + (reason.isEmpty() ? "" : " :" + reason));
        } else if (cmd == "/caps") {
            auto *cl = m_model->clientFor(host);
            if (cl) {
                const QStringList caps = cl->ackedCaps();
                m_model->localMessage(host, channel,
                    caps.isEmpty() ? "No caps negotiated."
                                   : "Active caps: " + caps.join("  "));
            }
        } else if (cmd == "/version") {
            if (args.isEmpty())
                m_model->sendRaw(host, "VERSION");
            else
                m_model->sendRaw(host, "PRIVMSG " + args.trimmed() + " :\x01VERSION\x01");
        } else if (cmd == "/ctcp") {
            const QString target   = args.section(' ', 0, 0);
            const QString ctcpcmd  = args.section(' ', 1, 1).toUpper();
            const QString ctcpargs = args.section(' ', 2);
            if (!target.isEmpty() && !ctcpcmd.isEmpty()) {
                const QString ctcp = ctcpargs.isEmpty()
                    ? "\x01" + ctcpcmd + "\x01"
                    : "\x01" + ctcpcmd + " " + ctcpargs + "\x01";
                m_model->sendRaw(host, "PRIVMSG " + target + " :" + ctcp);
            }
        } else if (cmd == "/sysinfo") {
            const int ret = QMessageBox::question(this, "Share System Info",
                "This will post your OS, CPU, memory, GPU, and uptime to " + channel + ".\n\n"
                "System details can identify you. Continue?",
                QMessageBox::Yes | QMessageBox::No);
            if (ret != QMessageBox::Yes) return;
            if (!m_sysinfoCache.isEmpty()) {
                m_model->sendMessage(host, channel,
                    m_sysinfoCache + " UP: " + sysinfoUptime());
            } else if (m_sysinfoLoading) {
                m_model->localMessage(host, channel,
                    "System info is still being collected, please wait.");
            } else {
                m_sysinfoLoading = true;
                m_model->localMessage(host, channel, "Collecting system info...");

                auto *thread = QThread::create([this, host, channel]() {
                    const QString result =
                        QString("OS: %1 CPU: %2 MEM: %3 GPU: %4")
                            .arg(sysinfoOS(), sysinfoCPU(), sysinfoMEM(), sysinfoGPU());
                    QMetaObject::invokeMethod(this, [this, host, channel, result]() {
                        m_sysinfoCache   = result;
                        m_sysinfoLoading = false;
                        m_model->sendMessage(host, channel,
                            result + " UP: " + sysinfoUptime());
                    }, Qt::QueuedConnection);
                });
                connect(thread, &QThread::finished, thread, &QThread::deleteLater);

                // Hard 12 s global timeout
                auto *timer = new QTimer(this);
                timer->setSingleShot(true);
                connect(timer, &QTimer::timeout, this, [this, host, channel, thread, timer]() {
                    timer->deleteLater();
                    if (!m_sysinfoLoading) return;
                    m_sysinfoLoading = false;
                    thread->terminate();
                    m_model->localMessage(host, channel,
                        "System info collection timed out.");
                });
                connect(thread, &QThread::finished, timer, [timer]() {
                    timer->stop();
                    timer->deleteLater();
                });
                timer->start(12000);
                thread->start();
            }
        } else if (cmd == "/j") {
            m_model->sendJoin(host, args.section(' ', 0, 0), args.section(' ', 1, 1));
        } else if (cmd == "/ping") {
            const QString nick = args.trimmed().section(' ', 0, 0);
            if (!nick.isEmpty()) {
                const QString ts = QString::number(QDateTime::currentMSecsSinceEpoch());
                m_model->sendRaw(host, "PRIVMSG " + nick + " :\x01PING " + ts + "\x01");
                m_model->localMessage(host, channel, "Pinged " + nick);
            }
        } else if (cmd == "/time") {
            const QString nick = args.trimmed().section(' ', 0, 0);
            if (!nick.isEmpty()) {
                m_model->sendRaw(host, "PRIVMSG " + nick + " :\x01TIME\x01");
                m_model->localMessage(host, channel, "Querying time for " + nick);
            }
        } else if (cmd == "/invite") {
            const QString nick = args.section(' ', 0, 0);
            const QString chan = args.section(' ', 1, 1);
            if (!nick.isEmpty())
                m_model->sendRaw(host, "INVITE " + nick + " " + (chan.isEmpty() ? channel : chan));
        } else if (cmd == "/mode") {
            if (!args.isEmpty())
                m_model->sendRaw(host, "MODE " + args);
        } else if (cmd == "/op") {
            const QString nick = args.trimmed().section(' ', 0, 0);
            if (!nick.isEmpty())
                m_model->sendRaw(host, "MODE " + channel + " +o " + nick);
        } else if (cmd == "/deop") {
            const QString nick = args.trimmed().section(' ', 0, 0);
            if (!nick.isEmpty())
                m_model->sendRaw(host, "MODE " + channel + " -o " + nick);
        } else if (cmd == "/voice") {
            const QString nick = args.trimmed().section(' ', 0, 0);
            if (!nick.isEmpty())
                m_model->sendRaw(host, "MODE " + channel + " +v " + nick);
        } else if (cmd == "/devoice") {
            const QString nick = args.trimmed().section(' ', 0, 0);
            if (!nick.isEmpty())
                m_model->sendRaw(host, "MODE " + channel + " -v " + nick);
        } else if (cmd == "/ban") {
            const QString mask = args.trimmed().section(' ', 0, 0);
            if (!mask.isEmpty())
                m_model->sendRaw(host, "MODE " + channel + " +b " + mask);
        } else if (cmd == "/unban") {
            const QString mask = args.trimmed().section(' ', 0, 0);
            if (!mask.isEmpty())
                m_model->sendRaw(host, "MODE " + channel + " -b " + mask);
        } else if (cmd == "/react") {
            const QString emoji = args.trimmed();
            if (!emoji.isEmpty() && !m_pendingReplyMsgid.isEmpty()) {
                m_model->sendReact(host, channel, m_pendingReplyMsgid, emoji);
                clearReplyBar();
            } else if (emoji.isEmpty()) {
                m_model->localMessage(host, channel, "Usage: /react <emoji> (set a reply target first)");
            } else {
                m_model->localMessage(host, channel, "No message selected — right-click a timestamp to reply first");
            }
        } else if (cmd == "/ignore") {
            const QString nick = args.trimmed().toLower();
            if (!nick.isEmpty()) {
                m_model->ignoreNick(nick);
                if (!m_config.ignoredNicks.contains(nick))
                    m_config.ignoredNicks.append(nick);
                Config::save(m_config, Config::defaultPath());
                m_model->localMessage(host, channel, "Now ignoring " + nick);
            }
        } else if (cmd == "/unignore") {
            const QString nick = args.trimmed().toLower();
            if (!nick.isEmpty()) {
                m_model->unignoreNick(nick);
                m_config.ignoredNicks.removeAll(nick);
                Config::save(m_config, Config::defaultPath());
                m_model->localMessage(host, channel, "No longer ignoring " + nick);
            }
        } else if (cmd == "/ignored") {
            if (m_config.ignoredNicks.isEmpty()) {
                m_model->localMessage(host, channel, "Ignore list is empty.");
            } else {
                m_model->localMessage(host, channel,
                    "Ignored nicks: " + m_config.ignoredNicks.join(", "));
            }
        } else if (cmd == "/monitor") {
            const QString sub  = args.section(' ', 0, 0).toLower();
            const QString nick = args.section(' ', 1, 1).trimmed();
            if (sub == "add" && !nick.isEmpty()) {
                if (!m_config.monitorList.contains(nick, Qt::CaseInsensitive))
                    m_config.monitorList.append(nick);
                Config::save(m_config, Config::defaultPath());
                m_model->monitorAdd(host, nick);
                m_model->localMessage(host, channel, "Added " + nick + " to monitor list");
            } else if ((sub == "del" || sub == "remove") && !nick.isEmpty()) {
                m_config.monitorList.removeIf([&](const QString &n){
                    return n.compare(nick, Qt::CaseInsensitive) == 0;
                });
                Config::save(m_config, Config::defaultPath());
                m_model->monitorRemove(host, nick);
                m_model->localMessage(host, channel, "Removed " + nick + " from monitor list");
            } else if (sub == "list") {
                m_model->localMessage(host, channel,
                    m_config.monitorList.isEmpty()
                        ? "Monitor list is empty"
                        : "Monitor list: " + m_config.monitorList.join(", "));
            } else if (sub == "clear") {
                m_config.monitorList.clear();
                Config::save(m_config, Config::defaultPath());
                m_model->monitorClear(host);
                m_model->localMessage(host, channel, "Monitor list cleared");
            } else if (sub == "status") {
                m_model->monitorStatus(host);
            } else {
                m_model->localMessage(host, channel,
                    "Usage: /monitor add|del|list|clear|status [nick]");
            }
        } else if (cmd == "/clear") {
            if (m_chatView) m_chatView->clear();
        } else if (cmd == "/help") {
            const QStringList lines = {
                "Available commands:",
                "  /join <channel> [key]       — join a channel",
                "  /j <channel> [key]          — alias for /join",
                "  /part [message]             — leave the current channel",
                "  /nick <newnick>             — change your nick",
                "  /me <action>                — send an action (/me waves)",
                "  /msg <target> <message>     — send a private message",
                "  /query <nick>               — open a PM buffer without sending",
                "  /ns <text>                  — message NickServ",
                "  /cs <text>                  — message ChanServ",
                "  /bs <text>                  — message BotServ",
                "  /ms <text>                  — message MemoServ",
                "  /oper <user> <pass>         — IRC operator login",
                "  /notice <target> <message>  — send a NOTICE",
                "  /topic [text]               — show or set the channel topic",
                "  /kick <nick> [reason]       — kick a user",
                "  /invite <nick> [#channel]   — invite a user to a channel",
                "  /mode <target> <flags>      — set channel or user modes",
                "  /op <nick>                  — give op (+o)",
                "  /deop <nick>                — remove op (-o)",
                "  /voice <nick>               — give voice (+v)",
                "  /devoice <nick>             — remove voice (-v)",
                "  /ban <mask>                 — ban a mask (+b)",
                "  /unban <mask>               — remove a ban (-b)",
                "  /ping <nick>                — CTCP PING a user",
                "  /time <nick>                — query a user's local time",
                "  /away [message]             — set away status",
                "  /back                       — clear away status",
                "  /whois <nick>               — request WHOIS info",
                "  /motd [server]              — request the MOTD",
                "  /version [nick]             — request VERSION (nick optional)",
                "  /ctcp <target> <cmd> [args] — send a CTCP request",
                "  /sysinfo                    — post client/system info to channel",
                "  /react <emoji>              — react to the currently selected message",
                "  /ignore <nick>              — suppress messages from nick",
                "  /unignore <nick>            — stop ignoring nick",
                "  /ignored                    — list ignored nicks",
                "  /monitor add|del|list|clear|status [nick]  — watch list (MONITOR)",
                "  /clear                      — clear the chat buffer",
                "  /quote <raw>  /raw <raw>    — send a raw IRC line",
                "  /quit [message]             — disconnect from server",
            };
            for (const QString &line : lines)
                appendMessage(Message::make(MessageType::Server, "", line));
        } else {
            // Pass unknown /CMD args directly as raw IRC (e.g. /REHASH, /SAMODE)
            m_model->sendRaw(host, text.mid(1));
        }
        return;
    }

    if (trimmed.isEmpty() || channel == "(server)") return;

    // Substitute any remaining :shortcode: patterns before sending
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
    m_model->sendMessage(host, channel, outText, replyMsgid);
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
    m_model->setActive(host, channel);
    refreshChatView(host, channel);
    refreshNickList(host, channel);
    refreshTopicBar(host, channel);

    if (auto *sess = m_model->session(host))
        m_nickPrefix->setText(sess->nick);

    if (m_signalBars) {
        auto *sess = m_model->session(host);
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
        auto *sess = m_model->session(host);
        if (sess && sess->connected) {
            menu->addAction("Disconnect", this, [this, host]{
                if (auto *cl = m_model->clientFor(host))
                    cl->quit();
            });
        } else {
            menu->addAction("Reconnect", this, [this, host]{
                auto *cl = m_model->clientFor(host);
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
            m_model->sendPart(host, channel);
            QTimer::singleShot(500, this, [this, host, channel]{
                m_model->sendJoin(host, channel);
            });
        });
        menu->addAction("Leave", this, [this, host, channel]{
            m_model->sendPart(host, channel);
        });
        menu->addAction("Close", this, [this, host, channel]{
            m_model->closeBuffer(host, channel);
        });
    } else if (!channel.isEmpty() && channel != "(server)") {
        // PM / user query
        menu->addAction("Close Query", this, [this, host, channel]{
            m_model->closeBuffer(host, channel);
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
    if (m_orderedPanes.size() >= 3) return; // max 4 total (primary + 3)

    auto *pane = new ChannelPane(host, channel, this);
    if (m_theme.valid)
        pane->chatView()->document()->setDefaultStyleSheet(
            QString("a { color: %1; text-decoration: underline; }").arg(m_theme.accent));

    pane->chatView()->viewport()->installEventFilter(this);
    pane->input()->installEventFilter(this);

    if (auto *sess = m_model->session(host))
        pane->setNick(sess->nick);
    pane->setNickVisible(m_showNickPrefix);
    {
        const FontSizes &fs = m_config.ui.fontSizes;
        const QString   &fam = m_config.ui.fontFamily;
        auto makeFont = [&](int pt){ QFont f(fam,pt); f.setStyleHint(QFont::Monospace); return f; };
        const QFont chatFont = makeFont(fs.chat);
        pane->chatView()->setFont(chatFont);
        pane->chatView()->document()->setDefaultFont(chatFont);
        pane->nickList()->setFont(makeFont(fs.nickList));
        pane->nickList()->setItemDelegate(new FixedRowDelegate(16, pane->nickList()));
        pane->setTopicFont(makeFont(fs.topicBar));
        pane->setInputFont(makeFont(fs.inputNick), makeFont(fs.input));
    }

    if (auto *ch = m_model->channel(host, channel))
        pane->setTopic(linkifyTopic(ch->topic));

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

    auto makeVert = [this]() -> QSplitter * {
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
    auto *ch = m_model->channel(pane->host(), pane->channel());
    if (!ch) return;

    for (const auto &msg : std::as_const(ch->messages)) {
        const bool isText = (msg.type == MessageType::Privmsg ||
                             msg.type == MessageType::Action  ||
                             msg.type == MessageType::Notice);
        insertHtmlBlock(pane->chatView(), formatMessage(msg),
                        isText && m_config.ui.hangingIndent);

        if (!msg.msgid.isEmpty()) {
            auto rxIt = ch->reactions.constFind(msg.msgid);
            if (rxIt != ch->reactions.constEnd()) {
                const int ept = m_config.ui.fontSizes.emoji;
                QString rxHtml = QString("<span style='font-size:%1pt; color:#888;'>").arg(ept);
                for (auto it = rxIt->constBegin(); it != rxIt->constEnd(); ++it) {
                    rxHtml += it.key().toHtmlEscaped()
                              + QStringLiteral("<span style='font-size:8pt'>(")
                              + QString::number(it.value().size())
                              + QStringLiteral(")</span> ");
                }
                rxHtml += QStringLiteral("</span>");
                insertHtmlBlock(pane->chatView(), rxHtml);
            }
        }
    }

    auto *sb = pane->chatView()->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void MainWindow::refreshPaneNickList(ChannelPane *pane)
{
    pane->nickList()->clear();
    auto *ch   = m_model->channel(pane->host(), pane->channel());
    if (!ch) return;
    auto *sess = m_model->session(pane->host());

    for (const auto &e : std::as_const(ch->nicks)) {
        const bool isBot = ch->botNicks.contains(e.nick.toLower())
                        || (sess && sess->botNicks.contains(e.nick.toLower()));
        if (isBot && !m_botIcons.contains(e.nick.toLower()))
            m_botIcons[e.nick.toLower()] = QRandomGenerator::global()->bounded(2)
                                           ? QStringLiteral("🤖") : QStringLiteral("👾");
        const QString label = isBot
            ? e.display() + " " + m_botIcons[e.nick.toLower()]
            : e.display();
        auto *item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, e.nick);
        if (!e.account.isEmpty())
            item->setToolTip("Account: " + e.account);
        if (m_config.ui.coloredNicks)
            item->setForeground(nickColor(e.nick));
        pane->nickList()->addItem(item);
    }
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
    const QString host    = m_model->activeHost();
    const QString channel = m_model->activeChannel();
    if (nick.isEmpty() || host.isEmpty()) return;

    QMenu menu(this);

    QAction *title = menu.addAction(nick);
    title->setEnabled(false);
    QFont tf = title->font();
    tf.setBold(true);
    title->setFont(tf);
    menu.addSeparator();

    connect(menu.addAction("Message"), &QAction::triggered, this, [this, host, nick]{
        m_model->openPM(host, nick);
        switchToChannel(host, nick);
        if (m_input) m_input->setFocus();
    });

    connect(menu.addAction("Send File"), &QAction::triggered, this, [this, host, nick]{
        const QString path = QFileDialog::getOpenFileName(this, "Send File to " + nick);
        if (path.isEmpty()) return;

        IrcClient *client = m_model->clientFor(host);
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

        m_model->sendRaw(host,
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

    connect(menu.addAction("Send File (Passive)"), &QAction::triggered, this, [this, host, nick]{
        const QString path = QFileDialog::getOpenFileName(this, "Send File to " + nick + " (Passive)");
        if (path.isEmpty()) return;

        auto *dcc = new DccSend(path, this);
        const QString token = dcc->initPassive();
        if (token.isEmpty()) { dcc->deleteLater(); return; }

        const QString fn   = dcc->filename();
        const qint64  size = dcc->filesize();

        m_pendingPassiveSends.insert(token, dcc);
        m_model->sendRaw(host,
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

    connect(menu.addAction("Whois"), &QAction::triggered, this, [this, host, nick]{
        m_model->sendRaw(host, "WHOIS " + nick);
    });

    connect(menu.addAction("Invite"), &QAction::triggered, this, [this, host, channel, nick]{
        bool ok;
        const QString target = QInputDialog::getText(
            this, "Invite " + nick, "Channel:", QLineEdit::Normal,
            (channel.isEmpty() || channel == "(server)") ? QString() : channel, &ok);
        if (!ok || target.isEmpty()) return;
        m_model->sendRaw(host, "INVITE " + nick + " " + target);
    });

    connect(menu.addAction("Give Op"), &QAction::triggered, this, [this, host, channel, nick]{
        if (!channel.isEmpty() && channel != "(server)")
            m_model->sendRaw(host, "MODE " + channel + " +o " + nick);
    });

    connect(menu.addAction("Take Op"), &QAction::triggered, this, [this, host, channel, nick]{
        if (!channel.isEmpty() && channel != "(server)")
            m_model->sendRaw(host, "MODE " + channel + " -o " + nick);
    });

    connect(menu.addAction("Give Voice"), &QAction::triggered, this, [this, host, channel, nick]{
        if (!channel.isEmpty() && channel != "(server)")
            m_model->sendRaw(host, "MODE " + channel + " +v " + nick);
    });

    connect(menu.addAction("Take Voice"), &QAction::triggered, this, [this, host, channel, nick]{
        if (!channel.isEmpty() && channel != "(server)")
            m_model->sendRaw(host, "MODE " + channel + " -v " + nick);
    });

    connect(menu.addAction("Version"), &QAction::triggered, this, [this, host, nick]{
        m_model->sendRaw(host, "PRIVMSG " + nick + " :\x01VERSION\x01");
    });

    connect(menu.addAction("Ping"), &QAction::triggered, this, [this, host, nick]{
        const qint64 ts = QDateTime::currentMSecsSinceEpoch();
        m_model->sendRaw(host, "PRIVMSG " + nick + " :\x01PING " + QString::number(ts) + "\x01");
    });

    connect(menu.addAction("Copy Nick"), &QAction::triggered, this, [nick]{
        qApp->clipboard()->setText(nick);
    });

    menu.addSeparator();

    if (m_model->isIgnored(nick)) {
        connect(menu.addAction("Unignore"), &QAction::triggered, this, [this, host, nick]{
            m_model->unignoreNick(nick);
            const QString key = nick.toLower();
            m_config.ignoredNicks.removeAll(key);
            Config::save(m_config, Config::defaultPath());
            m_model->localMessage(host, m_model->activeChannel(), "No longer ignoring " + key);
        });
    } else {
        connect(menu.addAction("Ignore"), &QAction::triggered, this, [this, host, nick]{
            const QString key = nick.toLower();
            m_model->ignoreNick(key);
            if (!m_config.ignoredNicks.contains(key))
                m_config.ignoredNicks.append(key);
            Config::save(m_config, Config::defaultPath());
            m_model->localMessage(host, m_model->activeChannel(), "Now ignoring " + key);
        });
    }

    menu.addSeparator();

    connect(menu.addAction("Kick"), &QAction::triggered, this, [this, host, channel, nick]{
        if (channel.isEmpty() || channel == "(server)") return;
        bool ok;
        QString reason = QInputDialog::getText(this, "Kick " + nick, "Reason:", QLineEdit::Normal, {}, &ok);
        if (!ok) return;
        m_model->sendRaw(host, "KICK " + channel + " " + nick + (reason.isEmpty() ? QString() : " :" + reason));
    });

    connect(menu.addAction("Ban"), &QAction::triggered, this, [this, host, channel, nick]{
        if (!channel.isEmpty() && channel != "(server)")
            m_model->sendRaw(host, "MODE " + channel + " +b " + nick + "!*@*");
    });

    connect(menu.addAction("Kick && Ban"), &QAction::triggered, this, [this, host, channel, nick]{
        if (channel.isEmpty() || channel == "(server)") return;
        bool ok;
        QString reason = QInputDialog::getText(this, "Kick & Ban " + nick, "Reason:", QLineEdit::Normal, {}, &ok);
        if (!ok) return;
        m_model->sendRaw(host, "MODE " + channel + " +b " + nick + "!*@*");
        m_model->sendRaw(host, "KICK " + channel + " " + nick + (reason.isEmpty() ? QString() : " :" + reason));
    });

    menu.exec(globalPos);
}

void MainWindow::enqueuePreview(const QUrl &url, const QString &host, const QString &channel)
{
    const QString key = url.toString();
    if (key.isEmpty()) return;
    if (m_previewChannels.contains(key)) return;
    if (m_previewQueue.size() >= 10) return;
    if (m_previewChannels.size() >= 100) return;
    m_previewChannels.insert(key, {host, channel});
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

void MainWindow::refreshChatView(const QString &host, const QString &channel)
{
    m_chatView->clear();
    auto *ch = m_model->channel(host, channel);
    if (!ch) return;

    static const QRegularExpression urlRe(
        R"(https?://[^\s<>"]+)",
        QRegularExpression::CaseInsensitiveOption);

    for (const auto &msg : std::as_const(ch->messages)) {
        appendMessage(msg);
        if (!ch->previews.isEmpty() &&
            (msg.type == MessageType::Privmsg ||
             msg.type == MessageType::Action  ||
             msg.type == MessageType::Notice)) {
            auto it = urlRe.globalMatch(msg.text);
            while (it.hasNext()) {
                const QString urlStr = QUrl(it.next().captured(0)).toString();
                const auto p = ch->previews.constFind(urlStr);
                if (p != ch->previews.constEnd() && !ch->hiddenPreviews.contains(urlStr))
                    insertHtmlBlock(m_chatView, p.value());
            }
        }
        if (!msg.msgid.isEmpty()) {
            auto rxIt = ch->reactions.constFind(msg.msgid);
            if (rxIt != ch->reactions.constEnd()) {
                const int ept = m_config.ui.fontSizes.emoji;
                QString rxHtml = QString("<span style='font-size:%1pt; color:#888;'>").arg(ept);
                for (auto it = rxIt->constBegin(); it != rxIt->constEnd(); ++it) {
                    rxHtml += it.key().toHtmlEscaped()
                              + QStringLiteral("<span style='font-size:8pt'>(")
                              + QString::number(it.value().size())
                              + QStringLiteral(")</span> ");
                }
                rxHtml += QStringLiteral("</span>");
                insertHtmlBlock(m_chatView, rxHtml);
            }
        }
    }
}

void MainWindow::refreshNickList(const QString &host, const QString &channel)
{
    m_nickList->clear();
    auto *ch   = m_model->channel(host, channel);
    if (!ch) return;
    auto *sess = m_model->session(host);

    for (const auto &e : std::as_const(ch->nicks)) {
        const bool isBot = (ch->botNicks.contains(e.nick.toLower()))
                        || (sess && sess->botNicks.contains(e.nick.toLower()));
        if (isBot && !m_botIcons.contains(e.nick.toLower()))
            m_botIcons[e.nick.toLower()] = QRandomGenerator::global()->bounded(2)
                                           ? QStringLiteral("🤖") : QStringLiteral("👾");
        const QString label = isBot
            ? e.display() + " " + m_botIcons[e.nick.toLower()]
            : e.display();
        auto *item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, e.nick);
        if (!e.account.isEmpty())
            item->setToolTip("Account: " + e.account);
        if (m_config.ui.coloredNicks)
            item->setForeground(nickColor(e.nick));
        m_nickList->addItem(item);
    }

    if (m_nickCountLabel)
        m_nickCountLabel->setText(QString::number(ch->nicks.size()) + " users");
}

// Escapes a server-controlled value for use inside a single-quoted HTML attribute.
// toHtmlEscaped() covers &, <, >, " — we additionally encode ' to close that gap.
static QString htmlAttr(const QString &s)
{
    QString out = s.toHtmlEscaped();
    out.replace(QLatin1Char('\''), QLatin1String("&#39;"));
    return out;
}

static QString linkifyTopic(const QString &text)
{
    static const QRegularExpression urlRe(
        R"((https?://[^\s<>"]+))",
        QRegularExpression::CaseInsensitiveOption);
    QString result = text.toHtmlEscaped();
    result.replace(urlRe, R"(<a href="\1">\1</a>)");
    return result;
}

static QString linkifyHtml(const QString &html)
{
    static const QRegularExpression urlRe(
        R"((https?://[^\s<>"]+))",
        QRegularExpression::CaseInsensitiveOption);
    QString result = html;
    result.replace(urlRe, R"(<a href="\1">\1</a>)");
    return result;
}

void MainWindow::refreshTopicBar(const QString &host, const QString &channel)
{
    auto *ch = m_model->channel(host, channel);

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
            m_topicText->setText(linkifyTopic(ch ? ch->topic : QString()));
    }
}

QString MainWindow::msgidAtViewPos(const QPoint &viewPos) const
{
    QTextCursor cur = m_chatView->cursorForPosition(viewPos);
    QTextBlock block = cur.block();
    for (auto it = block.begin(); !it.atEnd(); ++it) {
        const QTextFragment frag = it.fragment();
        const QTextCharFormat fmt = frag.charFormat();
        if (fmt.isAnchor()) {
            const QString href = fmt.anchorHref();
            if (href.startsWith("msgid:"))
                return href.mid(6);
        }
    }
    return {};
}

void MainWindow::doSearch(bool backward)
{
    const QString text = m_searchInput->text();
    if (text.isEmpty()) return;
    QTextDocument::FindFlags flags;
    if (backward) flags |= QTextDocument::FindBackward;
    if (!m_chatView->find(text, flags)) {
        QTextCursor c(m_chatView->document());
        c.movePosition(backward ? QTextCursor::End : QTextCursor::Start);
        m_chatView->setTextCursor(c);
        m_chatView->find(text, flags);
    }
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
    const bool isText = (msg.type == MessageType::Privmsg ||
                         msg.type == MessageType::Action  ||
                         msg.type == MessageType::Notice);
    insertHtmlBlock(m_chatView, formatMessage(msg), isText && m_config.ui.hangingIndent);

    if (autoPreview &&
        (msg.type == MessageType::Privmsg ||
         msg.type == MessageType::Action  ||
         msg.type == MessageType::Notice)) {
        static const QRegularExpression urlRe(
            R"(https?://[^\s<>"]+)",
            QRegularExpression::CaseInsensitiveOption);
        auto it = urlRe.globalMatch(msg.text);
        while (it.hasNext()) {
            const QString urlStr = QUrl(it.next().captured(0)).toString();
            if (urlStr.isEmpty() || m_previewChannels.contains(urlStr)) continue;
            enqueuePreview(QUrl(urlStr),
                m_model->activeHost(), m_model->activeChannel());
        }
    }

    auto *sb = m_chatView->verticalScrollBar();
    sb->setValue(sb->maximum());
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

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange && isActiveWindow() && m_tray)
        m_tray->setNotify(false);
    QMainWindow::changeEvent(event);
}

static QString ircToHtml(const QString &raw)
{
    static const char* const kColors[] = {
        "#FFFFFF","#000000","#00007F","#009300",
        "#FF0000","#7F0000","#9C009C","#FC7F00",
        "#FFFF00","#00FC00","#009393","#00FFFF",
        "#0000FC","#FF00FF","#7F7F7F","#D2D2D2"
    };

    struct Fmt {
        bool bold{false}, italic{false}, underline{false}, strike{false};
        int  fg{-1}, bg{-1};
        bool operator==(const Fmt &o) const {
            return bold==o.bold && italic==o.italic && underline==o.underline
                && strike==o.strike && fg==o.fg && bg==o.bg;
        }
    };

    Fmt     cur;
    int     openSpans = 0;
    QString out;
    out.reserve(raw.size() * 2);

    auto closeAll = [&] {
        for (int i = 0; i < openSpans; ++i) out += "</span>";
        openSpans = 0;
    };

    auto applyFmt = [&](const Fmt &next) {
        if (next == cur) return;
        closeAll();
        cur = next;
        QString style;
        if (cur.bold)      style += "font-weight:bold;";
        if (cur.italic)    style += "font-style:italic;";
        QString td;
        if (cur.underline) td += "underline ";
        if (cur.strike)    td += "line-through ";
        if (!td.isEmpty()) style += "text-decoration:" + td.trimmed() + ";";
        if (cur.fg >= 0 && cur.fg < 16) style += QString("color:%1;").arg(kColors[cur.fg]);
        if (cur.bg >= 0 && cur.bg < 16) style += QString("background-color:%1;").arg(kColors[cur.bg]);
        if (!style.isEmpty()) {
            out += "<span style='" + style + "'>";
            openSpans = 1;
        }
    };

    int i = 0;
    const qsizetype len = raw.size();
    while (i < len) {
        const ushort c = raw[i].unicode();
        if (c == 0x02) {                          // bold
            Fmt n = cur; n.bold = !cur.bold; applyFmt(n); ++i;
        } else if (c == 0x1D) {                   // italic
            Fmt n = cur; n.italic = !cur.italic; applyFmt(n); ++i;
        } else if (c == 0x1F) {                   // underline
            Fmt n = cur; n.underline = !cur.underline; applyFmt(n); ++i;
        } else if (c == 0x1E) {                   // strikethrough
            Fmt n = cur; n.strike = !cur.strike; applyFmt(n); ++i;
        } else if (c == 0x16) {                   // reverse fg/bg
            Fmt n = cur; std::swap(n.fg, n.bg); applyFmt(n); ++i;
        } else if (c == 0x0F || (c == 0x03 && i+1 < len && !raw[i+1].isDigit() && raw[i+1] != ',')) {
            // reset all (\x0F) or bare \x03
            applyFmt({}); ++i;
        } else if (c == 0x03) {                   // color \x03[fg[,bg]]
            ++i;
            Fmt n = cur;
            if (i < len && raw[i].isDigit()) {
                int fg = raw[i++].digitValue();
                if (i < len && raw[i].isDigit()) fg = fg * 10 + raw[i++].digitValue();
                n.fg = fg;
                if (i < len && raw[i] == ',' && i+1 < len && raw[i+1].isDigit()) {
                    ++i;
                    int bg = raw[i++].digitValue();
                    if (i < len && raw[i].isDigit()) bg = bg * 10 + raw[i++].digitValue();
                    n.bg = bg;
                }
            } else {
                n.fg = -1; n.bg = -1;
            }
            applyFmt(n);
        } else if (c == 0x11) {                   // monospace — skip control char
            ++i;
        } else {
            if      (raw[i] == '<') out += "&lt;";
            else if (raw[i] == '>') out += "&gt;";
            else if (raw[i] == '&') out += "&amp;";
            else if (raw[i] == '"') out += "&quot;";
            else                    out += raw[i];
            ++i;
        }
    }
    closeAll();
    return out;
}

static QString wrapEmojiHtml(const QString &html, int ptSize)
{
    const QString open  = QString("<span style='font-size:%1pt'>").arg(ptSize);
    const QString close = QStringLiteral("</span>");

    QString result;
    result.reserve(html.size() * 2);

    qsizetype i = 0;
    while (i < html.size()) {
        if (html[i] == '<') {
            qsizetype end = html.indexOf('>', i);
            if (end == -1) { result += html.mid(i); break; }
            result += html.mid(i, end - i + 1);
            i = end + 1;
            continue;
        }

        // Supplementary-plane emoji: surrogate pair
        if (html[i].isHighSurrogate() && i + 1 < html.size() && html[i+1].isLowSurrogate()) {
            const uint cp = QChar::surrogateToUcs4(html[i], html[i+1]);
            if (cp >= 0x1F000 && cp <= 0x1FAFF) {
                result += open;
                result += html[i]; result += html[i+1];
                i += 2;
                if (i < html.size() && html[i].unicode() == 0xFE0F)
                    result += html[i++];
                result += close;
            } else {
                result += html[i++]; result += html[i++];
            }
            continue;
        }

        // BMP misc symbols and dingbats (☀ ★ ✓ etc.)
        const ushort c = html[i].unicode();
        if (c >= 0x2300 && c <= 0x27BF) {
            result += open + html[i];
            ++i;
            if (i < html.size() && html[i].unicode() == 0xFE0F)
                result += html[i++];
            result += close;
            continue;
        }

        result += html[i++];
    }
    return result;
}

QString MainWindow::formatMessage(const Message &msg) const
{
    const QDateTime local = msg.timestamp.toLocalTime();
    const bool sameDay = local.date() == QDate::currentDate();
    const QString ts = sameDay
        ? local.toString("hh:mm")
        : local.toString("MM/dd hh:mm");

    auto wrap = [](const QString &color, const QString &text) {
        return QString("<span style='color:%1'>%2</span>").arg(color, text.toHtmlEscaped());
    };
    const int eventPt = qMax(7, qRound(m_config.ui.fontSizes.chat * 0.82));
    auto wrapEvent = [&eventPt](const QString &color, const QString &text) {
        return QString("<span style='color:%1; font-size:%2pt'>%3</span>")
            .arg(color, QString::number(eventPt), text.toHtmlEscaped());
    };

    QString html;
    // Timestamp span — double as msgid anchor when present
    const QString tsSpan = msg.msgid.isEmpty()
        ? QString("<span style='color:gray'>%1</span>").arg(ts)
        : QString("<a href='msgid:%1' style='color:gray;text-decoration:none'>%2</a>")
            .arg(htmlAttr(msg.msgid), ts);

    if (msg.redacted) {
        html = tsSpan + " <span style='color:gray;font-style:italic'>[message deleted]</span>";
        if (msg.isHistory)
            html = "<span style='opacity:0.55'>" + html + "</span>";
        return html;
    }

    switch (msg.type) {
    case MessageType::Privmsg: {
        const QString color = m_config.ui.coloredNicks
            ? nickColor(msg.nick).name()
            : (m_theme.valid ? m_theme.text : QStringLiteral("#cccccc"));
        const QString &br = m_config.ui.nickBrackets;
        QString nickOpen, nickClose;
        if (!br.isEmpty()) {
            if (br.length() % 2 == 0) {
                nickOpen  = br.left(br.length() / 2).toHtmlEscaped();
                nickClose = br.mid(br.length() / 2).toHtmlEscaped();
            } else {
                nickOpen  = QString(br.front()).toHtmlEscaped();
                nickClose = QString(br.back()).toHtmlEscaped();
            }
        }
        const QString nickDisplay = nickOpen + msg.nick.toHtmlEscaped() + nickClose;
        const QString titleAttr = msg.account.isEmpty()
            ? QString()
            : " title='account: " + htmlAttr(msg.account) + "'";
        const QString nickAnchor  = QString("<a href='nick:%1'%2 style='color:%3; text-decoration:none; font-weight:bold'>%4</a>")
            .arg(htmlAttr(msg.nick), titleAttr, color, nickDisplay);
        QString textHtml = wrapEmojiHtml(linkifyHtml(ircToHtml(msg.text)), m_config.ui.fontSizes.emoji);
        const QString sn = m_model->selfNick(m_model->activeHost());
        if (!sn.isEmpty()) {
            QRegularExpression snRe("(\\b" + QRegularExpression::escape(sn) + "\\b)",
                                    QRegularExpression::CaseInsensitiveOption);
            textHtml.replace(snRe, "<span style='color:red;font-weight:bold'>\\1</span>");
        }
        // Reply reference
        QString replySpan;
        if (!msg.replyTo.isEmpty()) {
            auto *ch = m_model->channel(m_model->activeHost(), m_model->activeChannel());
            QString origNick;
            if (ch) {
                for (const auto &orig : std::as_const(ch->messages))
                    if (orig.msgid == msg.replyTo) { origNick = orig.nick; break; }
            }
            replySpan = origNick.isEmpty()
                ? "<span style='color:#6c7086;font-size:small'>↩</span> "
                : QString("<span style='color:#6c7086;font-size:small'>↩ %1</span> ")
                    .arg(origNick.toHtmlEscaped());
        }
        html = QString("%1 %2%3 %4").arg(tsSpan, replySpan, nickAnchor, textHtml);
        break;
    }
    case MessageType::Action: {
        const QString aTitleAttr = msg.account.isEmpty()
            ? QString()
            : " title='account: " + htmlAttr(msg.account) + "'";
        const QString actionNick = QString("<a href='nick:%1'%2 style='color:inherit; text-decoration:none'>%1</a>")
            .arg(htmlAttr(msg.nick), aTitleAttr);
        html = QString("%1 <i>* %2 %3</i>")
            .arg(tsSpan, actionNick, wrapEmojiHtml(linkifyHtml(ircToHtml(msg.text)), m_config.ui.fontSizes.emoji));
        break;
    }
    case MessageType::Notice: {
        const QString noticeNick = QString("<a href='nick:%1' style='color:inherit; text-decoration:none'>%1</a>")
            .arg(htmlAttr(msg.nick));
        html = QString("%1 <span style='color:#cc8800'>-%2- %3</span>")
            .arg(tsSpan, noticeNick, wrapEmojiHtml(linkifyHtml(ircToHtml(msg.text)), m_config.ui.fontSizes.emoji));
        break;
    }

    case MessageType::Join:
        html = wrapEvent("seagreen",  ts + " → " + msg.text); break;
    case MessageType::Part:
        html = wrapEvent("#e06b6b", ts + " ← " + msg.text); break;
    case MessageType::Quit:
        html = wrapEvent("#e06b6b", ts + " ✕ " + msg.text); break;
    case MessageType::Nick:
        html = wrapEvent("steelblue", ts + " ~ "  + msg.text); break;
    case MessageType::Kick:
        html = wrap("#e06b6b", ts + " ✕ " + msg.text); break;
    case MessageType::Topic:
        html = wrap("steelblue", ts + " ⦁ Topic: " + msg.text); break;
    case MessageType::Error:
        html = wrap("red",       ts + " !! " + msg.text); break;
    case MessageType::Server:
    default:
        html = wrap("gray",      ts + " * "  + msg.text); break;
    }

    if (msg.isHistory)
        html = "<span style='opacity:0.55'>" + html + "</span>";

    return html;
}
