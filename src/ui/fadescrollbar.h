#pragma once
#include <QScrollBar>

class QTimer;
class QPropertyAnimation;
class QGraphicsOpacityEffect;

class FadeScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    explicit FadeScrollBar(Qt::Orientation orientation, QWidget *parent = nullptr);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void wake();
    void scheduleFade();

    QGraphicsOpacityEffect *m_effect;
    QTimer                 *m_hideTimer;
    QPropertyAnimation     *m_anim;
};
