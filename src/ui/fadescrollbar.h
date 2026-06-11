#pragma once
#include <QScrollBar>

class QTimer;
class QPropertyAnimation;

class FadeScrollBar : public QScrollBar
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
public:
    explicit FadeScrollBar(Qt::Orientation orientation, QWidget *parent = nullptr);

    qreal opacity() const { return m_opacity; }
    void  setOpacity(qreal v) { m_opacity = v; update(); }

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void wake();
    void scheduleFade();

    qreal               m_opacity  = 0.0;
    QTimer             *m_hideTimer;
    QPropertyAnimation *m_anim;
};
