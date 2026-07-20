/**
 * @file qwidget.cpp
 * @brief QWidget, QLabel, QPushButton implementation
 *
 * Coordinate system:
 *  - m_x, m_y are relative to parent widget
 *  - render() converts to absolute screen coordinates before painting
 */

#include "qwidget.h"
#include "qpainter.h"

extern "C" {
#include "libuser_minimal.h"
}

static char *strdup_simple(const char *s) {
    if (!s) return nullptr;
    unsigned int len = 0;
    while (s[len]) len++;
    char *d = new char[len + 1];
    for (unsigned int i = 0; i <= len; i++) d[i] = s[i];
    return d;
}

// ============================================================
//  QWidget
// ============================================================
QWidget::QWidget(QWidget *parent, const char *name)
    : QObject(parent, name)
    , m_x(0), m_y(0), m_w(100), m_h(30)
    , m_visible(true)
    , m_bgColor(COLOR_LIGHT_GRAY)
{
}

QWidget::~QWidget() {
}

void QWidget::setGeometry(int x, int y, int w, int h) {
    m_x = x; m_y = y; m_w = w; m_h = h;
}

void QWidget::setVisible(bool v) {
    m_visible = v;
}

int QWidget::childCount() const {
    int n = 0;
    QObject *c = firstChild();
    while (c) { n++; c = c->nextSibling(); }
    return n;
}

void QWidget::paintEvent(QPainter *painter) {
    painter->setColor(m_bgColor);
    painter->fillRect(m_x, m_y, m_w, m_h);
}

void QWidget::render(QPainter *painter, int parentAbsX, int parentAbsY) {
    if (!m_visible) return;

    // Convert to absolute screen coordinates
    int savedX = m_x, savedY = m_y;
    m_x += parentAbsX;
    m_y += parentAbsY;

    // 1. Paint self
    paintEvent(painter);

    // 2. Collect children in z-order (firstChild = most recent = topmost)
    int n = childCount();
    if (n > 0) {
        QObject **kids = (QObject **)(new char[n * sizeof(QObject *)]);
        QObject *c = firstChild();
        for (int i = n - 1; i >= 0; i--) {
            kids[i] = c;
            c = c->nextSibling();
        }
        // Paint oldest → newest (newest on top)
        int myAbsX = m_x, myAbsY = m_y;
        for (int i = 0; i < n; i++) {
            ((QWidget *)kids[i])->render(painter, myAbsX, myAbsY);
        }
        delete[] (char *)kids;
    }

    // Restore relative coordinates
    m_x = savedX;
    m_y = savedY;
}

bool QWidget::contains(int px, int py) const {
    return (px >= m_x && px < m_x + m_w &&
            py >= m_y && py < m_y + m_h);
}

// ============================================================
//  QLabel
// ============================================================
QLabel::QLabel(QWidget *parent, const char *name, const char *text)
    : QWidget(parent, name), m_text(nullptr)
{
    m_text = strdup_simple(text);
    m_bgColor = COLOR_LIGHT_GRAY;
    m_w = 120; m_h = 20;
}

QLabel::~QLabel() {
    if (m_text) delete[] m_text;
}

void QLabel::setText(const char *text) {
    if (m_text) delete[] m_text;
    m_text = strdup_simple(text);
}

void QLabel::paintEvent(QPainter *painter) {
    painter->setColor(m_bgColor);
    painter->fillRect(m_x, m_y, m_w, m_h);
    if (m_text) {
        painter->setColor(COLOR_BLACK);
        painter->drawText(m_x + 4, m_y + 6, m_text);
    }
}

// ============================================================
//  QPushButton
// ============================================================
QPushButton::QPushButton(QWidget *parent, const char *name, const char *text)
    : QWidget(parent, name), m_text(nullptr), m_pressed(false)
{
    m_text = strdup_simple(text);
    m_bgColor = COLOR_GRAY;
    m_w = 80; m_h = 28;
}

QPushButton::~QPushButton() {
    if (m_text) delete[] m_text;
}

void QPushButton::setText(const char *text) {
    if (m_text) delete[] m_text;
    m_text = strdup_simple(text);
}

void QPushButton::paintEvent(QPainter *painter) {
    int x = m_x, y = m_y, w = m_w, h = m_h;

    // Button face
    painter->setColor(m_pressed ? COLOR_DARK_GRAY : COLOR_GRAY);
    painter->fillRect(x + 1, y + 1, w - 2, h - 2);

    // 3D border: top-left highlight
    painter->setColor(m_pressed ? COLOR_BLACK : COLOR_LIGHT_GRAY);
    painter->drawLine(x, y, x + w - 1, y);
    painter->drawLine(x, y, x, y + h - 1);

    // Bottom-right shadow
    painter->setColor(m_pressed ? COLOR_LIGHT_GRAY : COLOR_BLACK);
    painter->drawLine(x, y + h - 1, x + w - 1, y + h - 1);
    painter->drawLine(x + w - 1, y, x + w - 1, y + h - 1);

    // Text centered
    if (m_text) {
        const char *p = m_text;
        int len = 0;
        while (*p++) len++;
        int tw = len * QPainter::charWidth();
        int tx = x + (w - tw) / 2;
        int ty = y + (h - QPainter::charHeight()) / 2;
        painter->setColor(COLOR_WHITE);
        painter->drawText(tx, ty, m_text);
    }
}

void QPushButton::click() {
    emitSignal("clicked", nullptr);
}

bool QPushButton::mousePress(int px, int py) {
    if (contains(px, py)) {
        m_pressed = true;
        click();
        m_pressed = false;
        return true;
    }
    return false;
}
