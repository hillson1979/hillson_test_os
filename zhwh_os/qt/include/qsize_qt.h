/**
 * @file qsize_qt.h
 * @brief QSize — 2D size class (standalone, freestanding C++)
 */
#ifndef QSIZE_QT_H
#define QSIZE_QT_H

class QSize {
public:
    QSize() : m_w(0), m_h(0) {}
    QSize(int w, int h) : m_w(w), m_h(h) {}

    int width()  const { return m_w; }
    int height() const { return m_h; }
    void setWidth(int w)  { m_w = w; }
    void setHeight(int h) { m_h = h; }

    QSize &operator+=(const QSize &s) { m_w += s.m_w; m_h += s.m_h; return *this; }
    QSize &operator-=(const QSize &s) { m_w -= s.m_w; m_h -= s.m_h; return *this; }
    QSize &operator*=(int c) { m_w *= c; m_h *= c; return *this; }
    QSize &operator/=(int c) { m_w /= c; m_h /= c; return *this; }

    friend inline bool operator==(const QSize &a, const QSize &b) { return a.m_w == b.m_w && a.m_h == b.m_h; }
    friend inline bool operator!=(const QSize &a, const QSize &b) { return !(a == b); }
    friend inline QSize operator+(const QSize &a, const QSize &b) { return QSize(a.m_w + b.m_w, a.m_h + b.m_h); }
    friend inline QSize operator-(const QSize &a, const QSize &b) { return QSize(a.m_w - b.m_w, a.m_h - b.m_h); }
    friend inline QSize operator*(const QSize &s, int c) { return QSize(s.m_w * c, s.m_h * c); }
    friend inline QSize operator*(int c, const QSize &s) { return QSize(s.m_w * c, s.m_h * c); }

    QSize expandedTo(const QSize &s) const {
        return QSize(m_w > s.m_w ? m_w : s.m_w, m_h > s.m_h ? m_h : s.m_h);
    }
    QSize boundedTo(const QSize &s) const {
        return QSize(m_w < s.m_w ? m_w : s.m_w, m_h < s.m_h ? m_h : s.m_h);
    }

    bool isEmpty()  const { return m_w <= 0 || m_h <= 0; }
    bool isNull()   const { return m_w == 0 && m_h == 0; }
    bool isValid()  const { return m_w > 0 && m_h > 0; }

private:
    int m_w, m_h;
};

#endif // QSIZE_QT_H
