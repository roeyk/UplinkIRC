#pragma once
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QApplication>
#include <QPalette>
#include <QSvgRenderer>

// Material Symbols Outlined icons, colorized to the current palette at runtime.

namespace MenuIcons {

inline QIcon fromSvg(const QString &path, const QColor &color = {})
{
    const QColor col = color.isValid() ? color
                                       : QApplication::palette().color(QPalette::WindowText);
    QSvgRenderer renderer(path);
    QPixmap pix(32, 32);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        renderer.render(&p);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(pix.rect(), col);
    }
    pix.setDevicePixelRatio(2.0);
    return QIcon(pix);
}

inline QIcon about          (const QColor &c = {}) { return fromSvg(":/icons/mi-info.svg",               c); }
inline QIcon documentation  (const QColor &c = {}) { return fromSvg(":/icons/mi-menu-book.svg",          c); }
inline QIcon servers        (const QColor &c = {}) { return fromSvg(":/icons/mi-dns.svg",                c); }
inline QIcon fontConfig     (const QColor &c = {}) { return fromSvg(":/icons/mi-font-download.svg",      c); }
inline QIcon theme          (const QColor &c = {}) { return fromSvg(":/icons/mi-palette.svg",            c); }
inline QIcon appIcon        (const QColor &c = {}) { return fromSvg(":/icons/mi-widgets.svg",            c); }
inline QIcon topicBar       (const QColor &c = {}) { return fromSvg(":/icons/mi-view-headline.svg",      c); }
inline QIcon nickInInput    (const QColor &c = {}) { return fromSvg(":/icons/mi-badge.svg",              c); }
inline QIcon emojiButton    (const QColor &c = {}) { return fromSvg(":/icons/mi-emoji-emotions.svg",     c); }
inline QIcon typingIndicator(const QColor &c = {}) { return fromSvg(":/icons/mi-pending.svg",            c); }
inline QIcon connStatus     (const QColor &c = {}) { return fromSvg(":/icons/mi-signal-cellular-alt.svg",c); }
inline QIcon coloredNicks   (const QColor &c = {}) { return fromSvg(":/icons/mi-format-color-text.svg",  c); }
inline QIcon checkForUpdates(const QColor &c = {}) { return fromSvg(":/icons/mi-new-releases.svg",       c); }
inline QIcon exit           (const QColor &c = {}) { return fromSvg(":/icons/mi-logout.svg",             c); }
inline QIcon preferences    (const QColor &c = {}) { return fromSvg(":/icons/mi-tune.svg",               c); }
inline QIcon eye            (const QColor &c = {}) { return fromSvg(":/icons/mi-visibility.svg",          c); }
inline QIcon eyeOff         (const QColor &c = {}) { return fromSvg(":/icons/mi-visibility-off.svg",      c); }
inline QIcon confirm        (const QColor &c = {}) { return fromSvg(":/icons/mi-check.svg",               c); }
inline QIcon close          (const QColor &c = {}) { return fromSvg(":/icons/mi-close.svg",               c); }
inline QIcon mention        (const QColor &c = {}) { return fromSvg(":/icons/mi-lightbulb-2.svg",         c); }
inline QIcon unread         (const QColor &c = {}) { return fromSvg(":/icons/mi-forum.svg",               c); }

} // namespace MenuIcons
