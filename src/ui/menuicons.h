#pragma once
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QPalette>
#include <functional>

// All icons drawn at 32x32 physical pixels (DPR=2 → 16x16 logical).
// Color is pulled from the current palette so they adapt to themes.

namespace MenuIcons {

inline QIcon make(std::function<void(QPainter &, const QColor &, float)> fn,
                  const QColor &color = {})
{
    constexpr int S = 32;
    QPixmap pm(S, S);
    pm.fill(Qt::transparent);
    {
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        const QColor col = color.isValid() ? color
                                           : QApplication::palette().color(QPalette::WindowText);
        fn(p, col, float(S));
    }
    pm.setDevicePixelRatio(2.0);
    return QIcon(pm);
}

// ── ℹ About ─────────────────────────────────────────────────────────────────
inline QIcon about(const QColor &col = {}) {
    return make([](QPainter &p, const QColor &c, float s) {
        QPen pen(c, 2.0f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen); p.setBrush(Qt::NoBrush);
        p.drawEllipse(QRectF(3, 3, s - 6, s - 6));
        p.setPen(Qt::NoPen); p.setBrush(c);
        p.drawEllipse(QPointF(s * 0.5f, s * 0.32f), 2.2f, 2.2f);
        p.setPen(pen);
        p.drawLine(QPointF(s * 0.5f, s * 0.46f), QPointF(s * 0.5f, s * 0.73f));
    }, col);
}

// ── ⚙ Manage Servers ────────────────────────────────────────────────────────
inline QIcon servers(const QColor &col = {}) {
    return make([](QPainter &p, const QColor &c, float s) {
        QPen pen(c, 2.0f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen); p.setBrush(Qt::NoBrush);
        const float rackH = 6.5f, gap = 4.0f;
        const float startY = (s - 3 * rackH - 2 * gap) * 0.5f;
        for (int i = 0; i < 3; ++i) {
            float y = startY + i * (rackH + gap);
            p.drawRoundedRect(QRectF(3, y, s - 6, rackH), 2, 2);
            p.setPen(Qt::NoPen); p.setBrush(c);
            p.drawEllipse(QPointF(s - 9, y + rackH * 0.5f), 2.0f, 2.0f);
            p.setPen(pen); p.setBrush(Qt::NoBrush);
        }
    }, col);
}

// ── ≡ Documentation ─────────────────────────────────────────────────────────
inline QIcon documentation(const QColor &col = {}) {
    return make([](QPainter &p, const QColor &c, float s) {
        QPen pen(c, 2.0f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen); p.setBrush(Qt::NoBrush);
        float mid = s * 0.5f;
        p.drawRoundedRect(QRectF(3, 4, mid - 4, s - 9), 2, 2);
        p.drawRoundedRect(QRectF(mid + 1, 4, mid - 4, s - 9), 2, 2);
        p.drawLine(QPointF(mid - 1, 4), QPointF(mid - 1, s - 5));
        p.drawLine(QPointF(mid + 1, 4), QPointF(mid + 1, s - 5));
        QPen thin(c, 1.5f, Qt::SolidLine, Qt::RoundCap);
        p.setPen(thin);
        float lx = mid + 5, lw = mid - 10;
        for (int i = 0; i < 3; ++i) {
            float ly = 9 + i * 6;
            if (i == 2) lw *= 0.65f;
            p.drawLine(QPointF(lx, ly), QPointF(lx + lw, ly));
        }
    }, col);
}

// ── ✒ Font Config ────────────────────────────────────────────────────────────
inline QIcon fontConfig(const QColor &col = {}) {
    return make([](QPainter &p, const QColor &c, float s) {
        QPen pen(c, 2.2f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        float cx = s * 0.5f, top = 4.0f, bot = s - 4.0f;
        float lx = 5.0f, rx = s - 5.0f;
        p.drawLine(QPointF(cx, top), QPointF(lx, bot));
        p.drawLine(QPointF(cx, top), QPointF(rx, bot));
        float t = 0.57f;
        float cy2 = top * (1 - t) + bot * t;
        float clx  = cx * (1 - t) + lx * t;
        float crx  = cx * (1 - t) + rx * t;
        p.drawLine(QPointF(clx, cy2), QPointF(crx, cy2));
    }, col);
}

// ── ◑ Theme ──────────────────────────────────────────────────────────────────
inline QIcon theme(const QColor &col = {}) {
    return make([](QPainter &p, const QColor &c, float s) {
        QRectF r(3, 3, s - 6, s - 6);
        p.setPen(Qt::NoPen); p.setBrush(c);
        p.drawChord(r, 90 * 16, 180 * 16);
        QPen pen(c, 2.0f, Qt::SolidLine, Qt::RoundCap);
        p.setPen(pen); p.setBrush(Qt::NoBrush);
        p.drawArc(r, -90 * 16, 180 * 16);
        p.drawLine(QPointF(s * 0.5f, 3), QPointF(s * 0.5f, s - 3));
    }, col);
}

// ── ◈ App Icon ───────────────────────────────────────────────────────────────
inline QIcon appIcon(const QColor &col = {}) {
    return make([](QPainter &p, const QColor &c, float s) {
        QPen pen(c, 2.0f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen); p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(QRectF(3, 3, s - 6, s - 6), 3, 3);
        p.setPen(Qt::NoPen); p.setBrush(c);
        p.drawEllipse(QPointF(s * 0.72f, s * 0.28f), 3.0f, 3.0f);
        p.setPen(pen); p.setBrush(Qt::NoBrush);
        QPolygonF mtn;
        mtn << QPointF(6, s - 7)
            << QPointF(s * 0.38f, s * 0.4f)
            << QPointF(s * 0.58f, s * 0.56f)
            << QPointF(s * 0.76f, s * 0.43f)
            << QPointF(s - 6, s - 7);
        p.drawPolyline(mtn);
    }, col);
}

// ── ☰ Show Topic Bar ─────────────────────────────────────────────────────────
inline QIcon topicBar(const QColor &col = {}) {
    return make([](QPainter &p, const QColor &c, float s) {
        p.setPen(Qt::NoPen); p.setBrush(c);
        p.drawRoundedRect(QRectF(3, 5, s - 6, 7), 2, 2);
        QPen pen(c, 2.0f, Qt::SolidLine, Qt::RoundCap);
        p.setPen(pen);
        float lineW[] = { s - 10, s - 10, s * 0.5f };
        for (int i = 0; i < 3; ++i) {
            float ly = 18 + i * 5;
            p.drawLine(QPointF(4, ly), QPointF(4 + lineW[i], ly));
        }
    }, col);
}

// ── @ Show Nick in Input ─────────────────────────────────────────────────────
inline QIcon nickInInput(const QColor &col = {}) {
    return make([](QPainter &p, const QColor &c, float s) {
        QPen pen(c, 2.0f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen); p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(s * 0.5f, s * 0.34f), s * 0.17f, s * 0.17f);
        QPainterPath path;
        path.moveTo(4, s - 4);
        path.cubicTo(4, s * 0.63f, s - 4, s * 0.63f, s - 4, s - 4);
        p.drawPath(path);
    }, col);
}

// ── ☺ Show Emoji Button ──────────────────────────────────────────────────────
inline QIcon emojiButton(const QColor &col = {}) {
    return make([](QPainter &p, const QColor &c, float s) {
        QPen pen(c, 2.0f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen); p.setBrush(Qt::NoBrush);
        p.drawEllipse(QRectF(3, 3, s - 6, s - 6));
        p.setPen(Qt::NoPen); p.setBrush(c);
        p.drawEllipse(QPointF(s * 0.37f, s * 0.38f), 2.2f, 2.2f);
        p.drawEllipse(QPointF(s * 0.63f, s * 0.38f), 2.2f, 2.2f);
        p.setPen(pen); p.setBrush(Qt::NoBrush);
        p.drawArc(QRectF(s * 0.29f, s * 0.44f, s * 0.42f, s * 0.26f), 0, -180 * 16);
    }, col);
}

// ── ✎ Typing Indicator ───────────────────────────────────────────────────────
inline QIcon typingIndicator(const QColor &col = {}) {
    return make([](QPainter &p, const QColor &c, float s) {
        QPen pen(c, 2.0f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen); p.setBrush(Qt::NoBrush);
        QRectF bub(3, 3, s - 6, s * 0.6f);
        p.drawRoundedRect(bub, 4, 4);
        QPolygonF tail;
        tail << QPointF(9, bub.bottom())
             << QPointF(7, s - 4)
             << QPointF(17, bub.bottom());
        p.drawPolyline(tail);
        p.setPen(Qt::NoPen); p.setBrush(c);
        for (int i = 0; i < 3; ++i)
            p.drawEllipse(QPointF(s * 0.28f + i * s * 0.22f, bub.center().y()), 2.2f, 2.2f);
    }, col);
}

// ── ◉ Connection Status Bar ──────────────────────────────────────────────────
inline QIcon connStatus(const QColor &col = {}) {
    return make([](QPainter &p, const QColor &c, float s) {
        float bw = 5.5f, gap = 2.0f;
        float totalW = 4 * bw + 3 * gap;
        float startX = (s - totalW) * 0.5f;
        for (int i = 0; i < 4; ++i) {
            float bh = 5.0f + i * 5.5f;
            float bx = startX + i * (bw + gap);
            float by = s - 4.0f - bh;
            p.setPen(Qt::NoPen);
            p.setBrush(i < 2 ? QColor(c.red(), c.green(), c.blue(), 130) : c);
            p.drawRoundedRect(QRectF(bx, by, bw, bh), 1.5f, 1.5f);
        }
    }, col);
}

// ── ◐ Colored Nicks ──────────────────────────────────────────────────────────
inline QIcon coloredNicks(const QColor &col = {}) {
    return make([](QPainter &p, const QColor &c, float s) {
        QPen pen(c, 2.0f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        const float r = s * 0.12f;
        const float cx[3] = { s * 0.22f, s * 0.5f, s * 0.78f };
        for (int i = 0; i < 3; ++i) {
            if (i == 1) {
                p.setPen(Qt::NoPen); p.setBrush(c);
                p.drawEllipse(QPointF(cx[i], s * 0.33f), r, r);
            } else {
                p.setPen(pen); p.setBrush(Qt::NoBrush);
                p.drawEllipse(QPointF(cx[i], s * 0.33f), r, r);
            }
            p.setPen(pen); p.setBrush(Qt::NoBrush);
            QPainterPath path;
            float lx = cx[i] - s * 0.17f, rx = cx[i] + s * 0.17f;
            path.moveTo(lx, s - 5);
            path.quadTo(cx[i], s * 0.62f, rx, s - 5);
            p.drawPath(path);
        }
    }, col);
}

} // namespace MenuIcons
