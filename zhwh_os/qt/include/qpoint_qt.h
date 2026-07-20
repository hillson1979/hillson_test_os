/**
 * @file qpoint_qt.h
 * @brief QPoint — 2D point class (standalone, freestanding C++)
 */
#ifndef QPOINT_QT_H
#define QPOINT_QT_H

class QPoint {
public:
    QPoint() : m_x(0), m_y(0) {}
    QPoint(int x, int y) : m_x(x), m_y(y) {}

    int x() const { return m_x; }
    int y() const { return m_y; }
    void setX(int x) { m_x = x; }
    void setY(int y) { m_y = y; }

    QPoint &operator+=(const QPoint &p) { m_x += p.m_x; m_y += p.m_y; return *this; }
    QPoint &operator-=(const QPoint &p) { m_x -= p.m_x; m_y -= p.m_y; return *this; }
    QPoint &operator*=(int c) { m_x *= c; m_y *= c; return *this; }
    QPoint &operator*=(double c) { m_x = (int)(m_x * c); m_y = (int)(m_y * c); return *this; }
    QPoint &operator/=(int c) { m_x /= c; m_y /= c; return *this; }

    friend inline bool operator==(const QPoint &a, const QPoint &b) { return a.m_x == b.m_x && a.m_y == b.m_y; }
    friend inline bool operator!=(const QPoint &a, const QPoint &b) { return !(a == b); }
    friend inline QPoint operator+(const QPoint &a, const QPoint &b) { return QPoint(a.m_x + b.m_x, a.m_y + b.m_y); }
    friend inline QPoint operator-(const QPoint &a, const QPoint &b) { return QPoint(a.m_x - b.m_x, a.m_y - b.m_y); }
    friend inline QPoint operator*(const QPoint &p, int c) { return QPoint(p.m_x * c, p.m_y * c); }
    friend inline QPoint operator*(int c, const QPoint &p) { return QPoint(p.m_x * c, p.m_y * c); }
    friend inline QPoint operator-(const QPoint &p) { return QPoint(-p.m_x, -p.m_y); }

    int manhattanLength() const {
        int ax = m_x < 0 ? -m_x : m_x;
        int ay = m_y < 0 ? -m_y : m_y;
        return ax + ay;
    }

    bool isNull() const { return m_x == 0 && m_y == 0; }

    // Low-level access for our rendering code
    int  _x() const { return m_x; }
    int  _y() const { return m_y; }
    int &rx() { return m_x; }
    int &ry() { return m_y; }

private:
    int m_x, m_y;
};

// QPointArray stub (we don't need full QPointArray for basic widgets)
class QPointArray {
public:
    QPointArray() : m_pts(0), m_count(0) {}
    ~QPointArray() { /* no-op: uses static data */ }

    void setPoint(int i, int x, int y) { if (i < m_count) { m_pts[i].rx() = x; m_pts[i].ry() = y; } }
    QPoint point(int i) const { return (i >= 0 && i < m_count) ? m_pts[i] : QPoint(); }
    int   count() const { return m_count; }
    QPoint *data() { return m_pts; }

    static QPointArray fromRaw(QPoint *pts, int n) { QPointArray a; a.m_pts = pts; a.m_count = n; return a; }

private:
    QPoint *m_pts;
    int m_count;
};

#endif // QPOINT_QT_H
