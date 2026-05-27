#include "themeloader.h"

#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDebug>

#include <toml++/toml.hpp>

// ---------------------------------------------------------------------------
// Theme search path: user config dir → exe dir → cwd
// ---------------------------------------------------------------------------

QString ThemeLoader::themesDir()
{
    const QStringList candidates = {
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/themes",
        QCoreApplication::applicationDirPath() + "/themes",
        "themes"
    };
    for (const QString &p : candidates)
        if (QDir(p).exists()) return p;
    return "themes";
}

QStringList ThemeLoader::availableThemes()
{
    QStringList names;
    const QDir dir(themesDir());
    for (const QString &f : dir.entryList({"*.toml"}, QDir::Files, QDir::Name))
        names << QFileInfo(f).baseName();
    return names;
}

// ---------------------------------------------------------------------------
// Parse
// ---------------------------------------------------------------------------

static QString str(const toml::table &tbl, std::string_view key, const char *def = "#ffffff")
{
    return QString::fromStdString(tbl[key].value_or<std::string>(def));
}

Theme ThemeLoader::load(const QString &name)
{
    const QString path = themesDir() + "/" + name + ".toml";
    Theme t;

    try {
        auto tbl = toml::parse_file(path.toStdString());

        auto get = [&](std::string_view section) -> const toml::table * {
            auto n = tbl[section];
            return n.is_table() ? n.as_table() : nullptr;
        };

        if (auto *g = get("general")) {
            t.background = str(*g, "background", "#1e1e2e");
            t.text       = str(*g, "text",       "#cdd6f4");
            t.border     = str(*g, "border",     "#313244");
            t.accent     = str(*g, "accent",     "#89b4fa");
        }
        if (auto *s = get("sidebar")) {
            t.sidebarBg      = str(*s, "background", "#181825");
            t.sidebarText    = str(*s, "text",       "#6c7086");
            t.sidebarActive  = str(*s, "active",     "#cdd6f4");
            t.sidebarUnread  = str(*s, "unread",     "#89b4fa");
            t.sidebarMention = str(*s, "mention",    "#f38ba8");
            t.sidebarServer  = str(*s, "server",     "#585b70");
        }
        if (auto *b = get("buffer")) {
            t.bufferBg   = str(*b, "background",  "#1e1e2e");
            t.timestamp  = str(*b, "timestamp",   "#6c7086");
            t.serverLine = str(*b, "server_line", "#585b70");
            t.action     = str(*b, "action",      "#cba6f7");
            t.nickSelf   = str(*b, "nick_self",   "#a6e3a1");
        }
        if (auto *h = get("highlights")) {
            t.mentionBg   = str(*h, "mention_bg",   "#2a1a2e");
            t.mentionText = str(*h, "mention_text",  "#f38ba8");
            t.keyword     = str(*h, "keyword",       "#fab387");
        }
        if (auto *n = get("nicklist")) {
            t.nicklistBg   = str(*n, "background", "#181825");
            t.nicklistText = str(*n, "text",       "#6c7086");
            t.op           = str(*n, "op",         "#89b4fa");
            t.halfop       = str(*n, "halfop",     "#cba6f7");
            t.voice        = str(*n, "voice",      "#a6e3a1");
            t.away         = str(*n, "away",       "#45475a");
        }
        if (auto *i = get("input")) {
            t.inputBg    = str(*i, "background", "#313244");
            t.inputText  = str(*i, "text",       "#cdd6f4");
            t.placeholder= str(*i, "placeholder","#6c7086");
            t.inputNick  = str(*i, "nick_color", "#a6e3a1");
        }

        t.valid = true;

    } catch (const toml::parse_error &e) {
        qWarning() << "ThemeLoader: failed to parse" << path << ":" << e.what();
    }

    return t;
}

// ---------------------------------------------------------------------------
// QSS generation
// ---------------------------------------------------------------------------

QString ThemeLoader::toStyleSheet(const Theme &t)
{
    return QString(R"(
/* ── Base ── */
QMainWindow, QDialog, QWidget {
    background-color: %1;
    color: %2;
}
QToolBar {
    background-color: %1;
    border-bottom: 1px solid %3;
    spacing: 4px;
}
QStatusBar {
    background-color: %1;
    color: %18;
    border-top: 1px solid %3;
}
QMenuBar {
    background-color: %1;
    color: %2;
}
QMenuBar::item:selected {
    background-color: %4;
    color: %2;
}
QMenu {
    background-color: %1;
    color: %2;
    border: 1px solid %3;
}
QMenu::item:selected {
    background-color: %4;
}

/* ── Sidebar (server/channel tree) ── */
QTreeWidget {
    background-color: %5;
    color: %6;
    border: none;
    outline: none;
}
QTreeWidget::item {
    padding: 2px 4px;
}
QTreeWidget::item:selected {
    background-color: %4;
    color: %7;
}
QTreeWidget::item:hover {
    background-color: %3;
}

/* ── Chat view ── */
QTextEdit {
    background-color: %10;
    color: %2;
    border: none;
    selection-background-color: %4;
}

/* ── Nick list ── */
QListWidget {
    background-color: %16;
    color: %17;
    border: none;
    outline: none;
}
QListWidget::item:selected {
    background-color: %4;
    color: %7;
}
QListWidget::item:hover {
    background-color: %3;
}

/* ── Input bar ── */
QLineEdit {
    background-color: %20;
    color: %21;
    border: none;
    border-top: 1px solid %3;
    padding: 4px 6px;
    selection-background-color: %4;
}
QLineEdit::placeholder {
    color: %22;
}

/* ── Dock widgets ── */
QDockWidget {
    color: %2;
    titlebar-close-icon: none;
}
QDockWidget::title {
    background-color: %5;
    padding: 4px;
    border-bottom: 1px solid %3;
}

/* ── Scroll bars ── */
QScrollBar:vertical {
    background: %1;
    width: 8px;
    border: none;
}
QScrollBar::handle:vertical {
    background: %3;
    border-radius: 4px;
    min-height: 20px;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar:horizontal { height: 0; }

/* ── Splitter ── */
QSplitter::handle {
    background: %3;
}

/* ── Tool buttons (hamburger) ── */
QToolButton {
    background: transparent;
    color: %2;
    border: none;
    padding: 2px 6px;
    font-size: 16px;
}
QToolButton:hover, QToolButton::menu-indicator {
    background-color: %3;
}

/* ── Labels ── */
QLabel {
    color: %2;
    background: transparent;
}

/* ── Push buttons ── */
QPushButton {
    background-color: %3;
    color: %2;
    border: 1px solid %3;
    border-radius: 3px;
    padding: 4px 12px;
}
QPushButton:hover  { background-color: %4; }
QPushButton:pressed{ background-color: %5; }

/* ── Dialog buttons ── */
QDialogButtonBox QPushButton {
    min-width: 70px;
}
)")
    .arg(t.background)    // 1
    .arg(t.text)          // 2
    .arg(t.border)        // 3
    .arg(t.accent)        // 4
    .arg(t.sidebarBg)     // 5
    .arg(t.sidebarText)   // 6
    .arg(t.sidebarActive) // 7
    .arg(t.sidebarUnread) // 8  (reserved)
    .arg(t.sidebarMention)// 9  (reserved)
    .arg(t.bufferBg)      // 10
    .arg(t.timestamp)     // 11 (reserved)
    .arg(t.serverLine)    // 12 (reserved)
    .arg(t.action)        // 13 (reserved)
    .arg(t.nickSelf)      // 14 (reserved)
    .arg(t.mentionBg)     // 15 (reserved)
    .arg(t.nicklistBg)    // 16
    .arg(t.nicklistText)  // 17
    .arg(t.sidebarServer) // 18
    .arg(t.inputBg)       // 19 (reserved)
    .arg(t.inputBg)       // 20
    .arg(t.inputText)     // 21
    .arg(t.placeholder);  // 22
}

// ---------------------------------------------------------------------------
// Apply to QApplication
// ---------------------------------------------------------------------------

void ThemeLoader::apply(const QString &name)
{
    const Theme t = load(name);
    if (!t.valid) {
        qWarning() << "ThemeLoader: could not load theme" << name;
        return;
    }
    qApp->setStyleSheet(toStyleSheet(t));
}
