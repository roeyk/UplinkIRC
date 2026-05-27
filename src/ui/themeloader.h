#pragma once

#include <QString>
#include <QStringList>

struct Theme {
    // [general]
    QString background, text, border, accent;
    // [sidebar]
    QString sidebarBg, sidebarText, sidebarActive, sidebarUnread, sidebarMention, sidebarServer;
    // [buffer]
    QString bufferBg, timestamp, serverLine, action, nickSelf;
    // [highlights]
    QString mentionBg, mentionText, keyword;
    // [nicklist]
    QString nicklistBg, nicklistText, op, halfop, voice, away;
    // [input]
    QString inputBg, inputText, placeholder, inputNick;

    bool valid{false};
};

class ThemeLoader
{
public:
    static Theme    load(const QString &name);
    static QString  toStyleSheet(const Theme &t);
    static void     apply(const QString &name);          // load + set on QApplication
    static QStringList availableThemes();                // names without .toml
    static QString  themesDir();                         // resolved search path
};
