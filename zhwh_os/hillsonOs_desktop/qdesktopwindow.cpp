/**
 * @file qdesktopwindow.cpp
 * @brief QDesktopWindow implementation
 */
#include "qdesktopwindow.h"
#include "qpainter.h"

QDesktopWindow::QDesktopWindow(QWidget *parent, const char *name, const char *title)
    : QWidget(parent, name)
{
    m_title = nullptr;
    m_content = nullptr;
    m_focused = false;
    // Allocate and copy title
    int len = 0; while (title[len]) len++;
    m_title = new char[len + 1];
    for (int i = 0; i <= len; i++) m_title[i] = title[i];
}

QDesktopWindow::~QDesktopWindow() {
    delete[] m_title;
}

void QDesktopWindow::setContent(QWidget *content) {
    m_content = content;
    if (m_content) m_content->setParent(this);
}

void QDesktopWindow::setTitle(const char *title) {
    delete[] m_title;
    int len = 0; while (title[len]) len++;
    m_title = new char[len + 1];
    for (int i = 0; i <= len; i++) m_title[i] = title[i];
}

void QDesktopWindow::setFocused(bool f) {
    m_focused = f;
}

QDesktopWindow::HitZone QDesktopWindow::hitTest(int px, int py) const {
    int lx = px - m_x;
    int ly = py - m_y;

    // Outside window
    if (lx < 0 || ly < 0 || lx >= m_w || ly >= m_h)
        return HIT_NONE;

    // Close button
    if (lx >= closeX() && lx <= closeX() + closeW() &&
        ly >= closeY() && ly <= closeY() + closeH())
        return HIT_CLOSE;

    // Title bar
    if (ly < TITLE_H + BORDER_W)
        return HIT_TITLEBAR;

    // Content area
    if (lx >= BORDER_W && lx < m_w - BORDER_W &&
        ly >= TITLE_H + BORDER_W && ly < m_h - BORDER_W)
        return HIT_CONTENT;

    // Edges for resize (check corners first)
    bool onLeft  = (lx < EDGE_SZ);
    bool onRight = (lx >= m_w - EDGE_SZ);
    bool onTop   = (ly < EDGE_SZ);
    bool onBot   = (ly >= m_h - EDGE_SZ);

    if (onTop && onLeft)   return HIT_EDGE_NW;
    if (onTop && onRight)  return HIT_EDGE_NE;
    if (onBot && onLeft)   return HIT_EDGE_SW;
    if (onBot && onRight)  return HIT_EDGE_SE;
    if (onTop)            return HIT_EDGE_N;
    if (onBot)            return HIT_EDGE_S;
    if (onLeft)           return HIT_EDGE_W;
    if (onRight)          return HIT_EDGE_E;

    return HIT_CONTENT;
}

void QDesktopWindow::paintEvent(QPainter *painter) {
    // Window background
    painter->setColor(0x00E0E0E0);
    painter->fillRect(0, 0, m_w, m_h);

    // Title bar
    uint32_t titleColor = m_focused ? 0x000060C0 : 0x00808080;
    painter->setColor(titleColor);
    painter->fillRect(BORDER_W, BORDER_W, m_w - 2*BORDER_W, TITLE_H);

    // Title text
    if (m_title) {
        painter->setColor(COLOR_WHITE);
        painter->drawText(BORDER_W + 4, BORDER_W + 4, m_title);
    }

    // Close button [X]
    int cx = closeX(), cy = closeY();
    painter->setColor(0x00CC0000);
    painter->fillRect(cx, cy, closeW(), closeH());
    painter->setColor(COLOR_WHITE);
    painter->drawText(cx + 5, cy + 3, "X");

    // Border (focused = brighter)
    uint32_t borderColor = m_focused ? 0x004080FF : 0x00606060;
    painter->setColor(borderColor);
    // Top edge
    painter->fillRect(0, 0, m_w, BORDER_W);
    // Bottom edge
    painter->fillRect(0, m_h - BORDER_W, m_w, BORDER_W);
    // Left edge
    painter->fillRect(0, 0, BORDER_W, m_h);
    // Right edge
    painter->fillRect(m_w - BORDER_W, 0, BORDER_W, m_h);

    // Content area background
    painter->setColor(COLOR_WHITE);
    int cx2 = BORDER_W;
    int cy2 = TITLE_H + BORDER_W;
    int cw2 = m_w - 2*BORDER_W;
    int ch2 = m_h - TITLE_H - 2*BORDER_W;
    painter->fillRect(cx2, cy2, cw2, ch2);

    // Render content widget
    if (m_content && m_content->isVisible()) {
        painter->setClipRect(cx2, cy2, cw2, ch2);
        painter->setColor(COLOR_BLACK);
        m_content->render(painter, m_x + cx2, m_y + cy2);
        painter->clearClip();
    }
}
