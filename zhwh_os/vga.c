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

// Kernel log ring buffer
#define KLOG_SZ 16384
static char klog[KLOG_SZ];
static int klog_w = 0;
void klog_read(char *buf, int max) {
    int len = klog_w; if (len > max-1) len = max-1;
    for (int i = 0; i < len; i++) buf[i] = klog[i];
    buf[len] = 0;
}
void klog_clear(void) { klog_w = 0; klog[0] = 0; }
void *klog_get_buf(void) { return klog; }
int klog_get_len(void) { return klog_w; }
/* 用户控制台输出 buffer (环形缓冲，保留最新数据) */
#define CONSOLE_SZ 65536
static char console_buf[CONSOLE_SZ];
static int console_w = 0; /* total written, never reset */
void *console_get_buf(void) { return console_buf; }
int  console_get_len(void) {
    if (console_w < CONSOLE_SZ) return console_w;
    return CONSOLE_SZ - 1; /* wrapped */
}
void console_write_char(char c) {
    if (console_w < CONSOLE_SZ - 1) {
        console_buf[console_w++] = c;
        console_buf[console_w] = 0;
    } else {
        /* Ring: shift left by 4KB to make room */
        for (int i = 0; i < CONSOLE_SZ - 4096; i++)
            console_buf[i] = console_buf[i + 4096];
        console_w = CONSOLE_SZ - 4096;
        console_buf[console_w++] = c;
        console_buf[console_w] = 0;
    }
}

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
  // 边界保护：防止光标写到非法位置导致消失
  if (vga_row >= 25) vga_row = 24;
  if (vga_col >= 80) vga_col = 79;
  uint16_t loc = vga_row * 80 + vga_col;

  outb(0x3D4, 14);
  outb(0x3D5, loc >> 8);
  outb(0x3D4, 15);
  outb(0x3D5, loc & 0xFF);
}

static void panic_puts_at(int row, int col, const char *s, uint8_t attr)
{
    while (*s && row < VGA_HEIGHT && col < VGA_WIDTH) {
        VGA_BUFFER[row * VGA_WIDTH + col] = ((uint16_t)attr << 8) | (uint8_t)*s;
        col++;
        s++;
    }
}

void vga_write_text(int row, int col, const char *s, uint8_t attr)
{
    if (!s)
        return;
    if (row < 0 || row >= (int)VGA_HEIGHT || col < 0 || col >= (int)VGA_WIDTH)
        return;
    panic_puts_at(row, col, s, attr);
    update_cursor();
}

void vga_message_screen(const char *title,
                        const char *line1,
                        const char *line2,
                        const char *line3,
                        uint8_t attr)
{
    char logbuf[4096];
    const char *klog = "";
    int klen = 0;

    for (uint32_t y = 0; y < VGA_HEIGHT; y++) {
        for (uint32_t x = 0; x < VGA_WIDTH; x++) {
            VGA_BUFFER[y * VGA_WIDTH + x] = ((uint16_t)attr << 8) | ' ';
        }
    }

    if (title)
        panic_puts_at(1, 2, title, attr);
    if (line1)
        panic_puts_at(3, 2, line1, attr);
    if (line2)
        panic_puts_at(4, 2, line2, attr);
    if (line3)
        panic_puts_at(5, 2, line3, attr);

    {
        extern void *klog_get_buf(void);
        extern int klog_get_len(void);
        klog = (const char *)klog_get_buf();
        klen = klog_get_len();
    }

    if (klog && klen > 0) {
        int start = 0;
        if (klen > (int)sizeof(logbuf) - 1)
            start = klen - ((int)sizeof(logbuf) - 1);
        int copy = klen - start;
        if (copy > (int)sizeof(logbuf) - 1)
            copy = (int)sizeof(logbuf) - 1;
        for (int i = 0; i < copy; i++)
            logbuf[i] = klog[start + i];
        logbuf[copy] = 0;

        int row = 7;
        const char *p = logbuf;
        while (*p && row < VGA_HEIGHT) {
            const char *e = p;
            while (*e && *e != '\n' && *e != '\r')
                e++;
            int len = (int)(e - p);
            if (len > VGA_WIDTH - 4)
                len = VGA_WIDTH - 4;
            for (int i = 0; i < len; i++)
                VGA_BUFFER[row * VGA_WIDTH + 2 + i] = ((uint16_t)attr << 8) | (uint8_t)p[i];
            row++;
            p = e;
            while (*p == '\n' || *p == '\r')
                p++;
        }
    }

    update_cursor();
}

void vga_panic_screen(const char *title,
                      const char *line1,
                      const char *line2,
                      const char *line3)
{
    vga_message_screen(title, line1, line2, line3, 0x1F);
}

void vga_init(void) {
    // 初始化串口
    serial_init();
    serial_puts("=== Serial initialized ===\r\n");

    vga_row = 0;
    vga_col = 0;

    for (uint32_t y = 0; y < VGA_HEIGHT; y++) {
        for (uint32_t x = 0; x < VGA_WIDTH; x++) {
            VGA_BUFFER[y * VGA_WIDTH + x] = (vga_color << 8) | ' ';
        }
    }
    update_cursor();  // 光标复位到(0,0)
}

void disable_cursor(){
  outb(0x3D4,0x0A);
  outb(0x3D5,0x20);
}

void vga_setcolor(uint8_t fg, uint8_t bg) {
    vga_color = (bg << 4) | (fg & 0x0F);
}

void vga_putc(char c) {
    // Capture to kernel log ring buffer
    if (klog_w < KLOG_SZ - 1) { klog[klog_w++] = c; klog[klog_w] = 0; }
    else { for(int i=0;i<KLOG_SZ/2;i++)klog[i]=klog[i+KLOG_SZ/2]; klog_w=KLOG_SZ/2; }
    // Also write to console buffer (for syscall 82 / netlog)
    console_write_char(c);
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
    // 检查指针是否有效
    //  NULL 指针和低地址检查
    if ((uint32_t)s < 0x1000) {
        return;
    }

    // ⚠️ 修复：内核空间指针 (>= 0xC0000000) 是安全的，可以直接访问
    // 只有用户空间指针需要额外验证，但这里我们假设调用者已经验证过了
    // 所以直接打印字符串

    while (*s) {
       vga_putc(*s++);
       if (*s =='\0')return;
     }
}
