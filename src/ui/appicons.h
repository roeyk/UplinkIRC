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

inline QString resolveIconKey(const QString &choice)
{
    if (choice == "dark")  return QStringLiteral("flat-black");
    if (choice == "light") return QStringLiteral("original-flat-shine");
    return choice;
}

inline QIcon appIcon(const QString &choice = "flat-black")
{
    const QString key = resolveIconKey(choice);
    const QString path = QStringLiteral(":/icons/%1.png").arg(key);
    QIcon icon(path);
    if (icon.isNull())
        return QIcon(":/icons/flat-black.png");
    return icon;
}
inline QIcon trayIcon()    { return appIcon(); }
inline QPixmap aboutLogo() { return QPixmap(":/icons/about-logo.png"); }
inline QIcon aboutBanner() { return QIcon(":/icons/banner.png"); }

} // namespace AppIcons
