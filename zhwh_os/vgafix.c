/*
 * vgafix.c - VGA 图形模式支持
 *
 * 支持 VGA 模式 0x13 (320x200, 256色)
 */

#include "types.h"
#include "x86/io.h"

// VGA 寄存器端口
#define VGA_MISC_WRITE   0x3C2
#define VGA_INDEX        0x3C4
#define VGA_DATA         0x3C5

// VGA 图形模式 0x13 的显存地址
#define VGA_FRAMEBUFFER  0xA0000
#define VGA_WIDTH        320
#define VGA_HEIGHT       200

static uint8_t *vga_mem = (uint8_t *)VGA_FRAMEBUFFER;
static int vga_mode = 0; // 0=文本, 1=图形

// 导出给 syscall.c 使用的变量
uint8_t *vga_graph_mem = (uint8_t *)VGA_FRAMEBUFFER;
int vga_graphics_mode = 0;

/* VGA 256色调色板 */
static const uint32_t vga_palette[256] = {
    0x000000, 0x0000AA, 0x00AA00, 0x00AAAA, 0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
    0x555555, 0x5555FF, 0x55FF55, 0x55FFFF, 0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF,
    // ... 其余颜色
};

/**
 * 设置 VGA 模式 0x13 (320x200, 256色)
 */
void vga_set_mode_13h(void) {
    // 使用 BIOS 中断设置视频模式
    asm volatile (
        "movw $0x13, %%ax\n\t"
        "int $0x10\n\t"
        : : : "ax"
    );

    vga_mem = (uint8_t *)VGA_FRAMEBUFFER;
    vga_mode = 1;
    vga_graphics_mode = 1;
    vga_graph_mem = (uint8_t *)VGA_FRAMEBUFFER;
}

/**
 * 返回文本模式 (0x03)
 */
void vga_set_text_mode(void) {
    asm volatile (
        "movw $0x03, %%ax\n\t"
        "int $0x10\n\t"
        : : : "ax"
    );

    vga_mode = 0;
}

/**
 * 绘制像素
 */
void vga_put_pixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        vga_mem[y * VGA_WIDTH + x] = color;
    }
}

/**
 * 获取像素颜色
 */
uint8_t vga_get_pixel(int x, int y) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        return vga_mem[y * VGA_WIDTH + x];
    }
    return 0;
}

/**
 * 填充矩形
 */
void vga_fill_rect(int x, int y, int w, int h, uint8_t color) {
    for (int j = y; j < y + h && j < VGA_HEIGHT; j++) {
        for (int i = x; i < x + w && i < VGA_WIDTH; i++) {
            vga_mem[j * VGA_WIDTH + i] = color;
        }
    }
}

/**
 * 绘制水平线
 */
void vga_draw_hline(int x, int y, int w, uint8_t color) {
    for (int i = x; i < x + w && i < VGA_WIDTH; i++) {
        vga_mem[y * VGA_WIDTH + i] = color;
    }
}

/**
 * 绘制垂直线
 */
void vga_draw_vline(int x, int y, int h, uint8_t color) {
    for (int j = y; j < y + h && j < VGA_HEIGHT; j++) {
        vga_mem[j * VGA_WIDTH + x] = color;
    }
}

/**
 * 清屏（用颜色填充）
 */
void vga_clear(uint8_t color) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_mem[i] = color;
    }
}

/**
 * 清屏（给 syscall.c 使用的版本）
 */
void vga_clear_graph(uint8_t color) {
    vga_clear(color);
}

/* 颜色定义 */
#define COLOR_BLACK     0
#define COLOR_BLUE      1
#define COLOR_GREEN     2
#define COLOR_CYAN      3
#define COLOR_RED       4
#define COLOR_MAGENTA   5
#define COLOR_BROWN     6
#define COLOR_LIGHTGRAY 7
#define COLOR_DARKGRAY  8
#define COLOR_LIGHTBLUE 9
#define COLOR_LIGHTGREEN 10
#define COLOR_LIGHTCYAN 11
#define COLOR_LIGHTRED  12
#define COLOR_LIGHTMAGENTA 13
#define COLOR_YELLOW    14
#define COLOR_WHITE     15

/**
 * 图形测试 - 绘制彩色图案
 */
void vga_graphics_demo(void) {
    vga_set_mode_13h();

    // 清屏为黑色
    vga_clear(COLOR_BLACK);

    // 绘制彩虹条纹
    for (int i = 0; i < 16; i++) {
        vga_fill_rect(i * 20, 0, 20, 200, i);
    }

    // 绘制矩形
    vga_fill_rect(50, 50, 100, 50, COLOR_RED);
    vga_fill_rect(170, 50, 100, 50, COLOR_GREEN);
    vga_fill_rect(110, 120, 100, 50, COLOR_BLUE);

    // 绘制边框
    for (int i = 0; i < 320; i++) {
        vga_put_pixel(i, 0, COLOR_WHITE);
        vga_put_pixel(i, 199, COLOR_WHITE);
    }
    for (int i = 0; i < 200; i++) {
        vga_put_pixel(0, i, COLOR_WHITE);
        vga_put_pixel(319, i, COLOR_WHITE);
    }

    // 等待按键
    // 在实际使用中应该等待键盘输入
}

/**
 * 获取当前 VGA 模式
 */
int vga_get_mode(void) {
    return vga_mode;
}
