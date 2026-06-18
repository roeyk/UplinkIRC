#pragma once
#include <QDialog>
#include "config/config.h"

class QListWidget;
class QPushButton;
class QDoubleSpinBox;
class QLabel;

class FontDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FontDialog(const QString &family, const FontSizes &sizes, QWidget *parent = nullptr);
    QString    selectedFamily() const;
    FontSizes  selectedSizes()  const;

private:
    QListWidget      *m_familyList;
    QPushButton      *m_familyBtn;
    QDoubleSpinBox   *m_spToolbar;
    QDoubleSpinBox   *m_spServerHeader;
    QDoubleSpinBox   *m_spSidebar;
    QDoubleSpinBox   *m_spChat;
    QDoubleSpinBox   *m_spNickList;
    QDoubleSpinBox   *m_spNickDock;
    QDoubleSpinBox   *m_spTopicBar;
    QDoubleSpinBox   *m_spTopicText;
    QDoubleSpinBox   *m_spInputNick;
    QDoubleSpinBox   *m_spInput;
    QDoubleSpinBox   *m_spTyping;
    QDoubleSpinBox   *m_spEmoji;
    QLabel           *m_preview;

    static QDoubleSpinBox *makeSpinBox(double value);
};
