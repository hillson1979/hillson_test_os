/**
 * @file qrect_qt.h
 * @brief QRect — 2D rectangle class (standalone, freestanding C++)
 */
#ifndef QRECT_QT_H
#define QRECT_QT_H

#include "qpoint_qt.h"
#include "qsize_qt.h"

class QRect {
public:
    QRect() : m_x1(0), m_y1(0), m_x2(-1), m_y2(-1) {} // null rect
    QRect(int x, int y, int w, int h)
        : m_x1(x), m_y1(y), m_x2(x + w - 1), m_y2(y + h - 1) {}
    QRect(const QPoint &topLeft, const QPoint &bottomRight)
        : m_x1(topLeft._x()), m_y1(topLeft._y())
        , m_x2(bottomRight._x()), m_y2(bottomRight._y()) {}
    QRect(const QPoint &topLeft, const QSize &size)
        : m_x1(topLeft._x()), m_y1(topLeft._y())
        , m_x2(topLeft._x() + size.width() - 1)
        , m_y2(topLeft._y() + size.height() - 1) {}

    // Accessors
    int x()      const { return m_x1; }
    int y()      const { return m_y1; }
    int width()  const { return m_x2 - m_x1 + 1; }
    int height() const { return m_y2 - m_y1 + 1; }
    int left()   const { return m_x1; }
    int top()    const { return m_y1; }
    int right()  const { return m_x2; }
    int bottom() const { return m_y2; }

    QPoint topLeft()     const { return QPoint(m_x1, m_y1); }
    QPoint topRight()    const { return QPoint(m_x2, m_y1); }
    QPoint bottomLeft()  const { return QPoint(m_x1, m_y2); }
    QPoint bottomRight() const { return QPoint(m_x2, m_y2); }
    QPoint center()      const { return QPoint((m_x1+m_x2)/2, (m_y1+m_y2)/2); }
    QSize  size()        const { return QSize(width(), height()); }

    void setX(int x)       { m_x2 += (x - m_x1); m_x1 = x; }
    void setY(int y)       { m_y2 += (y - m_y1); m_y1 = y; }
    void setWidth(int w)   { m_x2 = m_x1 + w - 1; }
    void setHeight(int h)  { m_y2 = m_y1 + h - 1; }
    void setRect(int x, int y, int w, int h) { m_x1=x; m_y1=y; m_x2=x+w-1; m_y2=y+h-1; }
    void setCoords(int x1, int y1, int x2, int y2) { m_x1=x1; m_y1=y1; m_x2=x2; m_y2=y2; }
    void setTopLeft(const QPoint &p)     { m_x1 = p._x(); m_y1 = p._y(); }
    void setBottomRight(const QPoint &p)  { m_x2 = p._x(); m_y2 = p._y(); }
    void setSize(const QSize &s)         { m_x2 = m_x1 + s.width() - 1; m_y2 = m_y1 + s.height() - 1; }

    void moveTopLeft(const QPoint &p)    { m_x2 += p._x() - m_x1; m_y2 += p._y() - m_y1; m_x1 = p._x(); m_y1 = p._y(); }
    void moveCenter(const QPoint &p)     { int w=width(), h=height(); m_x1=p._x()-w/2; m_y1=p._y()-h/2; m_x2=m_x1+w-1; m_y2=m_y1+h-1; }

    // Predicates
    bool isNull()   const { return m_x2 < m_x1; }
    bool isEmpty()  const { return m_x2 <= m_x1 || m_y2 <= m_y1; }
    bool isValid()  const { return m_x1 <= m_x2 && m_y1 <= m_y2; }

    bool contains(int x, int y) const { return x >= m_x1 && x <= m_x2 && y >= m_y1 && y <= m_y2; }
    bool contains(const QPoint &p) const { return contains(p._x(), p._y()); }

    // Operations
    QRect intersected(const QRect &r) const {
        int nx1 = m_x1 > r.m_x1 ? m_x1 : r.m_x1;
        int ny1 = m_y1 > r.m_y1 ? m_y1 : r.m_y1;
        int nx2 = m_x2 < r.m_x2 ? m_x2 : r.m_x2;
        int ny2 = m_y2 < r.m_y2 ? m_y2 : r.m_y2;
        if (nx1 > nx2 || ny1 > ny2) return QRect(); // null
        return QRect(QPoint(nx1, ny1), QPoint(nx2, ny2));
    }

    QRect united(const QRect &r) const {
        if (isNull()) return r;
        if (r.isNull()) return *this;
        int nx1 = m_x1 < r.m_x1 ? m_x1 : r.m_x1;
        int ny1 = m_y1 < r.m_y1 ? m_y1 : r.m_y1;
        int nx2 = m_x2 > r.m_x2 ? m_x2 : r.m_x2;
        int ny2 = m_y2 > r.m_y2 ? m_y2 : r.m_y2;
        return QRect(QPoint(nx1, ny1), QPoint(nx2, ny2));
    }

    QRect normalize() const {
        int nx1 = m_x1, nx2 = m_x2, ny1 = m_y1, ny2 = m_y2;
        if (nx1 > nx2) { int t = nx1; nx1 = nx2; nx2 = t; }
        if (ny1 > ny2) { int t = ny1; ny1 = ny2; ny2 = t; }
        return QRect(QPoint(nx1, ny1), QPoint(nx2, ny2));
    }

    void moveBy(int dx, int dy) { m_x1 += dx; m_y1 += dy; m_x2 += dx; m_y2 += dy; }

    friend inline bool operator==(const QRect &a, const QRect &b) {
        return a.m_x1 == b.m_x1 && a.m_y1 == b.m_y1 && a.m_x2 == b.m_x2 && a.m_y2 == b.m_y2;
    }
    friend inline bool operator!=(const QRect &a, const QRect &b) { return !(a == b); }

private:
    int m_x1, m_y1, m_x2, m_y2;
};

#endif // QRECT_QT_H
