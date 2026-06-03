#pragma once

#include <QGuiApplication>
#include <QIcon>
#include <QPalette>
#include <QPixmap>
#include <QString>

namespace AppIcons {

inline bool isDark()
{
    return qApp->palette().window().color().lightness() < 128;
}

inline QIcon appIcon(const QString & = {})
{
    return isDark() ? QIcon(":/icons/uplink-dark.png")
                    : QIcon(":/icons/uplink-light.png");
}
inline QIcon trayIcon()    { return appIcon(); }
inline QPixmap aboutLogo() { return QPixmap(":/icons/about-logo.png"); }
inline QIcon aboutBanner() { return QIcon(":/icons/banner.png"); }

} // namespace AppIcons
