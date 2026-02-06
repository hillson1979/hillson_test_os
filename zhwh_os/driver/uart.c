/**
 * @file uart.c
 * @brief 串口驱动 - 基于标准 UART 16550
 *
 * 功能：
 * - 串口初始化
 * - 字符发送/接收
 * - 字符串发送
 * - 调试输出接口
 */

#include "types.h"
#include "uart.h"
#include "io.h"

// 🔥 UART 寄存器定义（基于标准 UART 16550）
#define UART_BASE 0x3F8  // COM1 基地址

// 寄存器偏移
#define UART_RBR  0   // Receive Buffer Register (读)
#define UART_THR  0   // Transmit Holding Register (写)
#define UART_IER  1   // Interrupt Enable Register
#define UART_DLL  0   // Divisor Latch Low (当 DLAB=1)
#define UART_DLM  1   // Divisor Latch High (当 DLAB=1)
#define UART_IIR  2   // Interrupt Identification (读)
#define UART_FCR  2   // FIFO Control Register (写)
#define UART_LCR  3   // Line Control Register
#define UART_MCR  4   // Modem Control Register
#define UART_LSR  5   // Line Status Register
#define UART_MSR  6   // Modem Status Register
#define UART_SCR  7   // Scratch Register

// 🔥 LSR (Line Status Register) 位定义
#define UART_LSR_DR     0x01  // Data Ready
#define UART_LSR_OE     0x02  // Overrun Error
#define UART_LSR_PE     0x04  // Parity Error
#define UART_LSR_FE     0x08  // Framing Error
#define UART_LSR_BI     0x10  // Break Interrupt
#define UART_LSR_THRE   0x20  // Transmitter Holding Register Empty
#define UART_LSR_TEMT   0x40  // Transmitter Empty

// 🔥 串口配置
#define UART_BAUD_DIVISOR 115200  // 默认波特率
#define UART_DATA_BITS 8          // 8位数据位
#define UART_STOP_BITS 1          // 1位停止位
#define UART_PARITY      0          // 无奇偶校验

// 🔥 端口 I/O 操作
static inline uint8_t uart_read_reg(uint16_t offset) {
    return inb(UART_BASE + offset);
}

static inline void uart_write_reg(uint16_t offset, uint8_t value) {
    outb(value, UART_BASE + offset);
}

/**
 * @brief 读取 LSR (Line Status Register)
 */
static inline uint8_t uart_read_lsr(void) {
    return uart_read_reg(UART_LSR);
}

/**
 * @brief 等待发送器就绪
 */
static void uart_wait_transmit_ready(void) {
    // 等待 THRE (Transmitter Holding Register Empty) 位置位
    while (!(uart_read_lsr() & UART_LSR_THRE)) {
        __asm__ volatile("nop");
    }
}

/**
 * @brief 等待发送器完全空闲
 */
static void uart_wait_transmit_empty(void) {
    // 等待 TEMT (Transmitter Empty) 位置位
    while (!(uart_read_lsr() & UART_LSR_TEMT)) {
        __asm__ volatile("nop");
    }
}

/**
 * @brief 初始化串口
 */
int uart_init(void) {
    // 🔥 禁用中断
    uart_write_reg(UART_IER, 0x00);

    // 🔥 设置波特率 (DLAB=1)
    uart_write_reg(UART_LCR, 0x80);  // 设置 DLAB 位

    // 设置波特率除数器
    // 对于 115200 波特率，除数 = 115200 / 9600 = 12
    uint16_t divisor = 115200 / UART_BAUD_DIVISOR;
    uint8_t divisor_low = divisor & 0xFF;
    uint8_t divisor_high = (divisor >> 8) & 0xFF;

    uart_write_reg(UART_DLL, divisor_low);   // DLL
    uart_write_reg(UART_DLM, divisor_high);  // DLM

    // 🔥 设置数据格式：8位数据，1位停止位，无校验 (DLAB=0)
    uart_write_reg(UART_LCR, 0x03);  // 8N1

    // 🔥 启用 FIFO，清空 FIFO
    uart_write_reg(UART_FCR, 0x07);  // 清空 FIFO，启用 FIFO

    // 🔥 设置调制解调器控制：禁用所有流控
    uart_write_reg(UART_MCR, 0x00);

    // 🔥 测试串口是否可用（发送一个字节）
    uart_write_reg(UART_THR, 0xAE);  // 发送测试字节
    uart_wait_transmit_empty();

    return 0;
}

/**
 * @brief 发送一个字符
 */
void uart_putc(char c) {
    uart_wait_transmit_ready();
    uart_write_reg(UART_THR, (uint8_t)c);
}

/**
 * @brief 接收一个字符（阻塞）
 */
char uart_getc(void) {
    // 等待数据就绪
    while (!(uart_read_lsr() & UART_LSR_DR)) {
        __asm__ volatile("nop");
    }
    return (char)uart_read_reg(UART_RBR);
}

/**
 * @brief 检查是否有数据可读（非阻塞）
 */
int uart_data_available(void) {
    return (uart_read_lsr() & UART_LSR_DR) ? 1 : 0;
}

/**
 * @brief 发送字符串（最简化版本）
 */
void uart_puts(const char *str) {
    // 🔥 最简化版本：只发送字符串，不做任何其他操作
    while (*str) {
        uart_putc(*str++);
    }
}

/**
 * @brief 发送十六进制数
 */
void uart_puthex(uint32_t value) {
    const char *hex_chars = "0123456789ABCDEF";
    uart_puts("0x");

    for (int i = 28; i >= 0; i -= 4) {
        uint8_t digit = (value >> i) & 0x0F;
        uart_putc(hex_chars[digit]);
    }
}

/**
 * @brief 发送十进制数
 */
void uart_putdec(uint32_t value) {
    if (value == 0) {
        uart_putc('0');
        return;
    }

    char buffer[16];
    int pos = 0;

    // 转换为字符串（逆序）
    while (value > 0) {
        buffer[pos++] = '0' + (value % 10);
        value /= 10;
    }

    // 反向输出
    while (pos > 0) {
        uart_putc(buffer[--pos]);
    }
}

/**
 * @brief 打印调试信息（简化版 printf）
 */
void uart_printf(const char *format, ...) {
    const char *p = format;

    // 简单实现：只支持 %s, %c, %d, %x, %p
    // 注意：这里没有实现 va_list，因为内核可能没有

    while (*p) {
        if (*p == '%' && *(p + 1)) {
            p++;  // 跳过 '%'

            switch (*p) {
                case 's': {  // 字符串
                    // const char *str = *(const char **)&format;  // 未使用，注释掉
                    uart_puts("(string)");
                    break;
                }
                case 'c': {  // 字符
                    char c = *(char *)&format;
                    uart_putc(c);
                    break;
                }
                case 'd': {  // 十进制整数
                    uint32_t val = *(uint32_t *)&format;
                    uart_putdec(val);
                    break;
                }
                case 'x': {  // 十六进制整数
                    uint32_t val = *(uint32_t *)&format;
                    uart_puthex(val);
                    break;
                }
                case 'p': {  // 指针
                    uint32_t val = *(uint32_t *)&format;
                    uart_puts("0x");
                    uart_puthex(val);
                    break;
                }
                case '%':  // %% 转义
                    uart_putc('%');
                    break;
                default:
                    uart_putc('%');
                    uart_putc(*p);
                    break;
            }
            p++;
        } else {
            uart_putc(*p++);
        }
    }
}

/**
 * @brief 串口调试输出接口（类似 printf）
 * 🔥 禁用：uart_init() 未初始化，访问串口会导致 GP Fault 和重启
 */
void uart_debug(const char *fmt, ...) {
    // 🔥 完全禁用，避免访问未初始化的串口硬件
    return;
    // 原代码（已禁用）:
    // uart_puts("[UART DEBUG] ");
    // uart_puts(fmt);
    // uart_puts("\r\n");
}

/**
 * @brief 串口 panic 输出
 */
void uart_panic(const char *msg) {
    uart_puts("\r\n\n╔══════════════════════════════════════════════════╗\r\n");
    uart_puts("║              🔴 KERNEL PANIC 🔴                         ║\r\n");
    uart_puts("╠══════════════════════════════════════════════════╣\r\n");
    uart_puts("║  ");
    uart_puts(msg);
    uart_puts("\r\n");
    uart_puts("╚══════════════════════════════════════════════════╝\r\n\n");

    // 停机
    while (1) {
        __asm__ volatile("hlt");
    }
}

/**
 * @brief 十六进制 dump 内存
 */
void uart_dump(void *ptr, uint32_t len) {
    uint8_t *data = (uint8_t *)ptr;
    const char *hex_chars = "0123456789ABCDEF";

    for (uint32_t i = 0; i < len; i += 16) {
        // 地址
        uart_puthex((uint32_t)data + i);
        uart_puts(": ");

        // 数据
        for (uint32_t j = 0; j < 16; j++) {
            if (i + j < len) {
                uint8_t b = data[i + j];
                uart_putc(hex_chars[(b >> 4) & 0x0F]);
                uart_putc(hex_chars[b & 0x0F]);
                uart_putc(' ');
            } else {
                uart_puts("   ");
            }
        }

        // ASCII（可选）
        uart_puts(" | ");
        for (uint32_t j = 0; j < 16 && (i + j < len); j++) {
            uint8_t b = data[i + j];
            uart_putc((b >= 32 && b < 127) ? b : '.');
        }

        uart_puts("\r\n");
    }
}
