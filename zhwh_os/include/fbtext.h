// Simple framebuffer text drawing (kernel use)
#ifndef FBTEXT_H
#define FBTEXT_H
#include "font8x8.h"
static inline void fb_draw_char(volatile uint32_t *fb, int x, int y, char ch, uint32_t color, int pitch_px) {
    if (ch < 32 || ch > 126) return;
    const unsigned char *g = font8x8_data[(unsigned char)ch - 32];
    for (int r = 0; r < 8; r++) {
        unsigned char bits = g[r];
        for (int c = 0; c < 8; c++)
            if (bits & (0x80 >> c))
                fb[(y+r)*pitch_px + x + c] = color;
    }
}
static inline void fb_draw_text(volatile uint32_t *fb, int x, int y, const char *s, uint32_t color, int pitch_px) {
    while (*s) { fb_draw_char(fb, x, y, *s, color, pitch_px); x += 8; s++; }
}
#endif
