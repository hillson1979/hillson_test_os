/**
 * @file qpainter.cpp
 * @brief QPainter implementation — framebuffer drawing
 */

#include "qpainter.h"
#include "font8x8.h"

QPainter::QPainter(uint32_t *framebuffer, int width, int height, int pitch)
    : m_fb(framebuffer)
    , m_width(width)
    , m_height(height)
    , m_pitch(pitch)
    , m_pitchPx(pitch / 4)     // pitch is in bytes, 4 bytes per pixel
    , m_color(COLOR_BLACK)
    , m_clipEnabled(false)
    , m_clipX(0), m_clipY(0), m_clipW(0), m_clipH(0)
{
}

// ============================================================
//  Color
// ============================================================
void QPainter::setColor(uint32_t rgba) {
    m_color = rgba;
}

// ============================================================
//  Pixel
// ============================================================
void QPainter::drawPixel(int x, int y) {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) return;
    if (isClipped(x, y)) return;
    m_fb[y * m_pitchPx + x] = m_color;
}

// ============================================================
//  Filled rectangle
// ============================================================
void QPainter::fillRect(int x, int y, int w, int h) {
    // Clamp to screen
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > m_width)  w = m_width  - x;
    if (y + h > m_height) h = m_height - y;
    if (w <= 0 || h <= 0) return;

    for (int row = y; row < y + h; row++) {
        if (m_clipEnabled && (row < m_clipY || row >= m_clipY + m_clipH)) {
            // Entire row is outside clip — but we need per-pixel clip
        }
        for (int col = x; col < x + w; col++) {
            if (!isClipped(col, row)) {
                m_fb[row * m_pitchPx + col] = m_color;
            }
        }
    }
}

// ============================================================
//  Outlined rectangle
// ============================================================
void QPainter::drawRect(int x, int y, int w, int h) {
    // Top and bottom edges
    for (int i = 0; i < w; i++) {
        drawPixel(x + i, y);
        drawPixel(x + i, y + h - 1);
    }
    // Left and right edges
    for (int i = 0; i < h; i++) {
        drawPixel(x, y + i);
        drawPixel(x + w - 1, y + i);
    }
}

// ============================================================
//  Line (Bresenham)
// ============================================================
void QPainter::drawLine(int x1, int y1, int x2, int y2) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    int sx = (dx > 0) ? 1 : -1;
    int sy = (dy > 0) ? 1 : -1;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;

    int err = dx - dy;
    while (true) {
        drawPixel(x1, y1);
        if (x1 == x2 && y1 == y2) break;
        int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 <  dx) { err += dx; y1 += sy; }
    }
}

// ============================================================
//  Text
// ============================================================
void QPainter::drawChar(int x, int y, char ch) {
    if (ch < 32 || ch > 126) return;
    const unsigned char *glyph = font8x8_data[(unsigned char)ch - 32];

    for (int row = 0; row < 8; row++) {
        unsigned char bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) {
                drawPixel(x + col, y + row);
            }
        }
    }
}

void QPainter::drawText(int x, int y, const char *text) {
    if (!text) return;
    int cx = x;
    while (*text) {
        if (*text == '\n') {
            cx = x;
            y += 8;
        } else {
            drawChar(cx, y, *text);
            cx += 8;  // 8px per character
        }
        text++;
    }
}

int QPainter::textWidth(const char *text) const {
    if (!text) return 0;
    int len = 0;
    while (*text++) len++;
    return len * 8;
}

// ============================================================
//  Full screen
// ============================================================
void QPainter::clear(uint32_t color) {
    int total = m_pitchPx * m_height;
    for (int i = 0; i < total; i++) {
        m_fb[i] = color;
    }
}

// ============================================================
//  Clip
// ============================================================
void QPainter::setClipRect(int x, int y, int w, int h) {
    m_clipEnabled = true;
    m_clipX = x;
    m_clipY = y;
    m_clipW = w;
    m_clipH = h;
}

void QPainter::clearClip() {
    m_clipEnabled = false;
}

bool QPainter::isClipped(int x, int y) const {
    if (!m_clipEnabled) return false;
    return (x < m_clipX || x >= m_clipX + m_clipW ||
            y < m_clipY || y >= m_clipY + m_clipH);
}
