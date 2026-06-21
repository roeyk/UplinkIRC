#pragma once

#include "config/config.h"
#include <QDialog>

class QComboBox;
class QKeySequenceEdit;
class QLabel;
class QSlider;

class DropdownSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DropdownSettingsDialog(const UiConfig &ui, QWidget *parent = nullptr);

    QString shortcut() const;
    QString edge() const;
    int widthPercent() const;
    int heightPercent() const;
    int animationMs() const;
    int opacityPercent() const;
    int inactiveOpacityPercent() const;

private:
    void refreshCommand();

    QKeySequenceEdit *m_shortcutEdit{nullptr};
    QComboBox        *m_edgeCombo{nullptr};
    QSlider          *m_widthSlider{nullptr};
    QSlider          *m_heightSlider{nullptr};
    QSlider          *m_animationSlider{nullptr};
    QSlider          *m_opacitySlider{nullptr};
    QSlider          *m_inactiveOpacitySlider{nullptr};
    QLabel           *m_commandLabel{nullptr};
};
