#include "fadescrollbar.h"

#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QEnterEvent>

static constexpr int   kHoldMs  = 3500;
static constexpr int   kFadeMs  = 300;
static constexpr qreal kVisible = 0.85;

FadeScrollBar::FadeScrollBar(Qt::Orientation orientation, QWidget *parent)
    : QScrollBar(orientation, parent)
{
    m_effect = new QGraphicsOpacityEffect(this);
    m_effect->setOpacity(0.0);
    setGraphicsEffect(m_effect);

    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    m_hideTimer->setInterval(kHoldMs);

    m_anim = new QPropertyAnimation(m_effect, "opacity", this);
    m_anim->setDuration(kFadeMs);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);

    connect(m_hideTimer, &QTimer::timeout, this, [this] {
        if (underMouse()) { scheduleFade(); return; }
        m_anim->stop();
        m_anim->setStartValue(m_effect->opacity());
        m_anim->setEndValue(0.0);
        m_anim->start();
    });

    connect(this, &QScrollBar::valueChanged,  this, [this](int) { wake(); });
    connect(this, &QScrollBar::sliderPressed, this, [this]      { wake(); });
}

void FadeScrollBar::wake()
{
    m_anim->stop();
    m_effect->setOpacity(kVisible);
    scheduleFade();
}

void FadeScrollBar::scheduleFade()
{
    m_hideTimer->start();
}

void FadeScrollBar::enterEvent(QEnterEvent *event)
{
    QScrollBar::enterEvent(event);
    if (m_effect->opacity() > 0.0) {
        m_anim->stop();
        m_effect->setOpacity(kVisible);
        m_hideTimer->stop();
    }
}

void FadeScrollBar::leaveEvent(QEvent *event)
{
    QScrollBar::leaveEvent(event);
    if (m_effect->opacity() > 0.0)
        scheduleFade();
}
