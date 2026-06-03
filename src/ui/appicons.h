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

inline QIcon appIcon(const QString &choice = "dark")
{
    if (choice == "light")
        return QIcon(":/icons/uplink-light.png");
    return QIcon(":/icons/uplink-dark.png");
}
inline QIcon trayIcon()    { return appIcon(); }
inline QPixmap aboutLogo() { return QPixmap(":/icons/about-logo.png"); }
inline QIcon aboutBanner() { return QIcon(":/icons/banner.png"); }

} // namespace AppIcons
