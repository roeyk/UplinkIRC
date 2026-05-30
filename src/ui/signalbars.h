#pragma once
#include <QWidget>

class QTimer;

class SignalBars : public QWidget
{
    Q_OBJECT
public:
    enum class State { None, Connecting, Reconnecting, Connected, Disconnected };

    explicit SignalBars(QWidget *parent = nullptr);
    void setState(State state);
    void setLatency(int ms);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    State  m_state{State::None};
    int    m_litBars{4};
    bool   m_flashOn{true};
    QTimer *m_flashTimer{nullptr};

    bool isFlashing() const;
};
