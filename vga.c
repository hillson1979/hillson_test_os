// vga.c
#include "vga.h"
#include "string.h"
#include "x86/io.h"

// 串口端口定义
#define SERIAL_COM1 0x3F8

// 串口初始化
static void serial_init() {
    outb(SERIAL_COM1 + 1, 0x00);    // 禁用中断
    outb(SERIAL_COM1 + 3, 0x80);    // 启用DLAB (设置波特率除数)
    outb(SERIAL_COM1 + 0, 0x03);    // 设置除数为3 (38400波特率)
    outb(SERIAL_COM1 + 1, 0x00);    //
    outb(SERIAL_COM1 + 3, 0x03);    // 8位,无 parity, 1停止位
    outb(SERIAL_COM1 + 2, 0xC7);    // 启用FIFO, 清空, 14字节阈值
    outb(SERIAL_COM1 + 4, 0x0B);    // 启用IRQ, 设置RTS/DSR
}

// 串口输出字符
static void serial_putchar(char c) {
    while ((inb(SERIAL_COM1 + 5) & 0x20) == 0); // 等待发送缓冲区空
    outb(SERIAL_COM1, c);
}

// 串口输出字符串
static void serial_puts(const char *str) {
    while (*str) {
        if (*str == '\n') {
            serial_putchar('\r');
        }
        serial_putchar(*str);
        str++;
    }
}

static uint16_t* const VGA_BUFFER = (uint16_t*)0xC00B8000; // 映射到高端的VGA内存
static uint8_t vga_color = 0x0F; // 白字黑底
static uint32_t vga_row = 0;
static uint32_t vga_col = 0;

#define C_BLACK           0
#define C_BLUE            1
#define C_GREEN           2
#define C_CYAN            3
#define C_RED             4
#define C_MAGENTA         5
#define C_BROWN           6
#define C_LIGHTGRAY       7
#define C_DARKGRAY        8
#define C_LIGHTBLUE       9
#define C_LIGHTGREEN     10
#define C_LIGHTCYAN      11
#define C_LIGHTRED       12
#define C_LIGHTMAGENTA   13
#define C_LIGHTBROWN     14
#define C_WHITE          15

#define MAKE_CHAR(c, fore, back) (c | (back<<12) | (fore<<8))

static int c_back = C_BLACK;
static int c_fore = C_LIGHTGRAY;

static void scroll() {
  if (vga_row >= 25) {
    memcpy((uint8_t*)VGA_BUFFER,
           (uint8_t*)&VGA_BUFFER[80], 2 * 80 * 24);
    memsetw(&VGA_BUFFER[80 * 24],
            MAKE_CHAR(' ', c_back, c_back), 80);
    vga_row = 24;
  }
}

static void update_cursor() {
  uint16_t loc =vga_row * 80 + vga_col;

  outb(0x3D4, 14);
  outb(0x3D5, loc >> 8);
  outb(0x3D4, 15);
  outb(0x3D5, loc & 0xFF);
}

void vga_init(void) {
    // 初始化串口
    serial_init();
    serial_puts("=== Serial initialized ===\r\n");

    for (uint32_t y = 0; y < VGA_HEIGHT; y++) {
        for (uint32_t x = 0; x < VGA_WIDTH; x++) {
            VGA_BUFFER[y * VGA_WIDTH + x] = (vga_color << 8) | ' ';
        }
    }
}

void disable_cursor(){
  outb(0x3D4,0x0A);
  outb(0x3D5,0x20);
}

void vga_setcolor(uint8_t fg, uint8_t bg) {
    vga_color = (bg << 4) | (fg & 0x0F);
}

void vga_putc(char c) {
    // 输出到串口
    serial_putchar(c);
    if (c == '\n') {
        serial_putchar('\r');  // 串口需要\r\n
    }

    // 输出到VGA
    if (c == '\n') {
        vga_col = 0;
        ++vga_row;
       /*if (++vga_row >= VGA_HEIGHT) {
            vga_row = VGA_HEIGHT - 1;
        }
        return;*/
    }else{
       VGA_BUFFER[vga_row * VGA_WIDTH + vga_col] = (vga_color << 8) | c;
       ++vga_col;
    }

    /*VGA_BUFFER[vga_row * VGA_WIDTH + vga_col] = (vga_color << 8) | c;
    if (++vga_col >= VGA_WIDTH) {
        vga_col = 0;
        if (++vga_row >= VGA_HEIGHT) {
            vga_row = VGA_HEIGHT - 1;
        }
    }*/


    if (vga_col >= 80) {
        vga_col -= 80;
        ++vga_row;
    }

    scroll();
    update_cursor();
}

void vga_puts(const char* s) {
    while (*s) {
       
       vga_putc(*s++);
       if (*s =='\0')return;
     }
}
