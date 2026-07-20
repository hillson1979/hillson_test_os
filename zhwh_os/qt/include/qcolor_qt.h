/**
 * @file qcolor_qt.h
 * @brief QRgb / QColor — color types (standalone, freestanding C++)
 */
#ifndef QCOLOR_QT_H
#define QCOLOR_QT_H

// QRgb: 32-bit ARGB (0xAARRGGBB) or just RGB (0x00RRGGBB)
typedef unsigned int QRgb;

inline QRgb qRgb(int r, int g, int b) {
    return (0xFFu << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
}

inline QRgb qRgba(int r, int g, int b, int a) {
    return ((a & 0xFF) << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
}

inline int qRed(QRgb rgb)   { return (rgb >> 16) & 0xFF; }
inline int qGreen(QRgb rgb) { return (rgb >> 8) & 0xFF; }
inline int qBlue(QRgb rgb)  { return rgb & 0xFF; }
inline int qAlpha(QRgb rgb) { return (rgb >> 24) & 0xFF; }

// Predefined colors
#define QRgb_Black   0xFF000000u
#define QRgb_White   0xFFFFFFFFu
#define QRgb_Red     0xFFFF0000u
#define QRgb_Green   0xFF00FF00u
#define QRgb_Blue    0xFF0000FFu
#define QRgb_Gray    0xFF808080u
#define QRgb_Yellow  0xFFFFFF00u
#define QRgb_Cyan    0xFF00FFFFu
#define QRgb_Magenta 0xFFFF00FFu

class QColor {
public:
    QColor() : m_rgb(QRgb_Black) {}
    QColor(QRgb rgb) : m_rgb(rgb) {}
    QColor(int r, int g, int b) : m_rgb(qRgb(r, g, b)) {}

    QRgb rgb()  const { return m_rgb; }
    void setRgb(QRgb rgb) { m_rgb = rgb; }
    void setRgb(int r, int g, int b) { m_rgb = qRgb(r, g, b); }

    int red()   const { return qRed(m_rgb); }
    int green() const { return qGreen(m_rgb); }
    int blue()  const { return qBlue(m_rgb); }

    bool isValid() const { return true; }
    bool operator==(const QColor &c) const { return m_rgb == c.m_rgb; }
    bool operator!=(const QColor &c) const { return m_rgb != c.m_rgb; }

private:
    QRgb m_rgb;
};

#endif // QCOLOR_QT_H
