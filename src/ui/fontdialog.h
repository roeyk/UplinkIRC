#pragma once
#include <QDialog>
#include "config/config.h"

class QListWidget;
class QPushButton;
class QSpinBox;
class QLabel;

class FontDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FontDialog(const QString &family, const FontSizes &sizes, QWidget *parent = nullptr);
    QString    selectedFamily() const;
    FontSizes  selectedSizes()  const;

private:
    QListWidget   *m_familyList;
    QPushButton   *m_familyBtn;
    QSpinBox      *m_spToolbar;
    QSpinBox      *m_spServerHeader;
    QSpinBox      *m_spSidebar;
    QSpinBox      *m_spChat;
    QSpinBox      *m_spNickList;
    QSpinBox      *m_spNickDock;
    QSpinBox      *m_spTopicBar;
    QSpinBox      *m_spTopicText;
    QSpinBox      *m_spInputNick;
    QSpinBox      *m_spInput;
    QSpinBox      *m_spTyping;
    QSpinBox      *m_spEmoji;
    QLabel        *m_preview;

    static QSpinBox *makeSpinBox(int value);
};
