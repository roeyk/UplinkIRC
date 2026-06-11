#include "fadescrollbar.h"

#include <QTimer>
#include <QPropertyAnimation>
#include <QEnterEvent>
#include <QPainter>
#include <QStyleOptionSlider>

static constexpr int   kHoldMs  = 3500;
static constexpr int   kFadeMs  = 300;
static constexpr qreal kVisible = 0.85;

FadeScrollBar::FadeScrollBar(Qt::Orientation orientation, QWidget *parent)
    : QScrollBar(orientation, parent)
{
    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    m_hideTimer->setInterval(kHoldMs);

    m_anim = new QPropertyAnimation(this, "opacity", this);
    m_anim->setDuration(kFadeMs);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);

    connect(m_hideTimer, &QTimer::timeout, this, [this] {
        if (underMouse()) return;
        m_anim->stop();
        m_anim->setStartValue(m_opacity);
        m_anim->setEndValue(0.0);
        m_anim->start();
    });

    connect(this, &QScrollBar::actionTriggered, this, [this](int) { wake(); });
    connect(this, &QScrollBar::sliderPressed,   this, [this]      { wake(); });
    connect(this, &QScrollBar::sliderReleased,  this, [this]      { wake(); });
}

void FadeScrollBar::wake()
{
    m_anim->stop();
    setOpacity(kVisible);
    scheduleFade();
}

void FadeScrollBar::scheduleFade()
{
    m_hideTimer->start();
}

void FadeScrollBar::enterEvent(QEnterEvent *event)
{
    QScrollBar::enterEvent(event);
    m_anim->stop();
    setOpacity(kVisible);
    m_hideTimer->stop();
}

void FadeScrollBar::leaveEvent(QEvent *event)
{
    QScrollBar::leaveEvent(event);
    if (m_opacity >= kVisible)
        scheduleFade();
}

void FadeScrollBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setOpacity(m_opacity);
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    style()->drawComplexControl(QStyle::CC_ScrollBar, &opt, &painter, this);
}
