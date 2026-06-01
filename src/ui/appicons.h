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

inline QIcon iconForChoice(const QString &choice)
{
    if (choice == "tower")
        return isDark() ? QIcon(":/icons/icon-tower.svg")
                        : QIcon(":/icons/icon-tower-light.png");
    if (choice == "hub-spoke")
        return isDark() ? QIcon(":/icons/icon-hub-spoke.svg")
                        : QIcon(":/icons/icon-hub-spoke-light.png");
    // default: node-n
    return isDark() ? QIcon(":/icons/icon-node-n.svg")
                    : QIcon(":/icons/icon-node-n-light.png");
}

inline QIcon appIcon(const QString &choice = "node-n") { return iconForChoice(choice); }
inline QIcon trayIcon() { return QIcon(":/icons/tray-icon.svg"); }
inline QPixmap aboutLogo() { return QPixmap(":/icons/about-logo.png"); }
inline QIcon aboutBanner() { return QIcon(":/icons/banner.png"); }

} // namespace AppIcons
