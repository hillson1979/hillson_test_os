// vga.h
#pragma once
#include "types.h"

#define VGA_WIDTH   80
#define VGA_HEIGHT  25

enum vga_color {
    COLOR_BLACK = 0,
    COLOR_BLUE = 1,
    COLOR_GREEN = 2,
    /* 其他颜色省略... */
    COLOR_WHITE = 15
};

void vga_init(void);
void vga_putc(char c);
void vga_puts(const char* s);
void vga_setcolor(uint8_t fg, uint8_t bg);
