/**
 * @file qpainter.h
 * @brief QPainter — framebuffer drawing engine
 *
 * Draws directly to the 32-bit (XRGB8888) framebuffer.
 * Color format: 0x00RRGGBB
 */

#ifndef QPAINTER_H
#define QPAINTER_H

#include "stdint_compat.h"

class QPainter {
public:
    QPainter(uint32_t *framebuffer, int width, int height, int pitch);

    // --- Color ---
    void setColor(uint32_t rgba);
    uint32_t color() const { return m_color; }

    // --- Primitives ---
    void drawPixel(int x, int y);
    void fillRect(int x, int y, int w, int h);
    void drawRect(int x, int y, int w, int h);
    void drawLine(int x1, int y1, int x2, int y2);

    // --- Text (built-in 8x12 bitmap font) ---
    void drawText(int x, int y, const char *text);
    int  textWidth(const char *text) const;   // in pixels
    static int charWidth() { return 8; }
    static int charHeight() { return 8; }

    // --- Full screen ---
    void clear(uint32_t color);

    // --- Clip ---
    void setClipRect(int x, int y, int w, int h);
    void clearClip();

private:
    void drawChar(int x, int y, char ch);
    bool isClipped(int x, int y) const;

    uint32_t *m_fb;
    int  m_width;
    int  m_height;
    int  m_pitch;       // bytes per row
    int  m_pitchPx;     // pixels per row (pitch / 4)
    uint32_t m_color;

    // Clip region
    bool m_clipEnabled;
    int  m_clipX, m_clipY, m_clipW, m_clipH;
};

// Commonly used colors (XRGB8888)
#define COLOR_BLACK       0x00000000
#define COLOR_WHITE       0x00FFFFFF
#define COLOR_RED         0x00FF0000
#define COLOR_GREEN       0x0000FF00
#define COLOR_BLUE        0x000000FF
#define COLOR_DARK_GRAY   0x00404040
#define COLOR_GRAY        0x00808080
#define COLOR_LIGHT_GRAY  0x00C0C0C0
#define COLOR_DARK_BLUE   0x00000080
#define COLOR_NAVY        0x00004080

#endif // QPAINTER_H
