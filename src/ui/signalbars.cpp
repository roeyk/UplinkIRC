#include "signalbars.h"
#include <QPainter>
#include <QTimer>

static constexpr int kBars = 4;
static constexpr int kBarW = 3;
static constexpr int kGap  = 2;
static constexpr int kMinH = 4;
static constexpr int kMaxH = 13;

SignalBars::SignalBars(QWidget *parent) : QWidget(parent)
{
    setFixedSize(sizeHint());

    m_flashTimer = new QTimer(this);
    m_flashTimer->setInterval(500);
    connect(m_flashTimer, &QTimer::timeout, this, [this]{
        m_flashOn = !m_flashOn;
        update();
    });
}

QSize SignalBars::sizeHint() const
{
    return QSize(kBars * kBarW + (kBars - 1) * kGap, kMaxH);
}

bool SignalBars::isFlashing() const
{
    return m_state == State::Connecting
        || m_state == State::Reconnecting
        || m_state == State::Disconnected;
}

void SignalBars::setState(State state)
{
    if (m_state == state) return;
    m_state = state;

    switch (state) {
    case State::Connecting:    setToolTip("Connecting…");    break;
    case State::Reconnecting:  setToolTip("Reconnecting…");  break;
    case State::Disconnected:  setToolTip("Disconnected");   break;
    case State::None:          setToolTip({});               break;
    case State::Connected:     break; // tooltip set by setLatency
    }

    if (isFlashing()) {
        m_flashOn = true;
        m_flashTimer->start();
    } else {
        m_flashTimer->stop();
        m_flashOn = true;
    }
    update();
}

void SignalBars::setLatency(int ms)
{
    if (ms < 50)       m_litBars = 4;
    else if (ms < 150) m_litBars = 3;
    else if (ms < 300) m_litBars = 2;
    else               m_litBars = 1;

    setToolTip(QString("%1 ms").arg(ms));
    setState(State::Connected);
}

void SignalBars::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QColor dim = QColor("#444444");
    const int h = height();

    QColor litColor;
    int litBars = 0;

    switch (m_state) {
        case State::Connected:
            litColor = QColor("#50e050");
            litBars  = m_litBars;
            break;
        case State::Connecting:
        case State::Reconnecting:
            litColor = QColor("#5080e0");
            litBars  = 2;
            break;
        case State::Disconnected:
            litColor = QColor("#e05050");
            litBars  = 1;
            break;
        case State::None:
            break;
    }

    for (int i = 0; i < kBars; ++i) {
        int barH = kMinH + (kMaxH - kMinH) * i / (kBars - 1);
        int x    = i * (kBarW + kGap);
        int y    = h - barH;

        bool lit = (i < litBars) && (m_state != State::None);
        QColor color = (lit && m_flashOn) ? litColor : dim;

        p.setBrush(color);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(x, y, kBarW, barH, 1, 1);
    }
}
