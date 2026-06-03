#pragma once
#include <QPushButton>
#include <QPainter>
#include <QStyleOptionButton>

class PillButton : public QPushButton {
public:
    using QPushButton::QPushButton;

    void setAccentColor(const QColor &c) { m_accent = c; }
    void setLeftAlign(bool v)            { m_leftAlign = v; }

    QSize sizeHint() const override {
        QSize s = QPushButton::sizeHint();
        s.setHeight(qMax(28, fontMetrics().height() + 10));
        return s;
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QStyleOptionButton opt;
        initStyleOption(&opt);

        const bool hovered = opt.state & QStyle::State_MouseOver;
        const bool pressed = opt.state & QStyle::State_Sunken;
        const bool checked = opt.state & QStyle::State_On;

        QColor bg = palette().button().color();
        if (hovered || checked) bg = m_accent.isValid() ? m_accent : bg.lighter(120);
        if (pressed)            bg = bg.darker(110);

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 14, 14);

        const QColor fg = palette().buttonText().color();
        p.setPen(fg);

        if (!icon().isNull()) {
            QFontMetrics fm(font());
            const int textW  = fm.horizontalAdvance(text());
            const int iconW  = iconSize().width();
            const int gap    = 5;
            const int totalW = iconW + gap + textW;
            const int startX = m_leftAlign ? 10 : (width() - totalW) / 2;
            const int iy     = (height() - iconSize().height()) / 2;
            p.drawPixmap(startX, iy, iconW, iconSize().height(),
                         icon().pixmap(iconSize()));
            p.drawText(QRect(startX + iconW + gap, 0, textW + 2, height()),
                       Qt::AlignVCenter | Qt::AlignLeft, text());
        } else {
            const int flags = Qt::AlignVCenter | (m_leftAlign ? Qt::AlignLeft : Qt::AlignHCenter);
            p.drawText(rect().adjusted(10, 0, -10, 0), flags, text());
        }
    }

private:
    QColor m_accent;
    bool   m_leftAlign{false};
};
