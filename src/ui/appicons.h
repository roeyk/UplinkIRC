#pragma once

#include <QIcon>
#include <QString>

namespace AppIcons {

inline QIcon iconForChoice(const QString &choice)
{
    if (choice == "light-default") return QIcon(":/icons/app-icon-light-default.svg");
    if (choice == "light")         return QIcon(":/icons/app-icon-light.svg");
    if (choice == "avatar")        return QIcon(":/icons/app-icon-avatar.svg");
    return QIcon(":/icons/app-icon-dark.svg");
}

inline QIcon appIcon(const QString &choice = "dark") { return iconForChoice(choice); }
inline QIcon aboutBanner() { return QIcon(":/icons/banner.svg"); }

} // namespace AppIcons
