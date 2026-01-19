// printf.c
#include "vga.h"
#include <stdarg.h>

static void print_num(uint32_t num, uint32_t base, int width, char pad_char) {
    const char* digits = "0123456789ABCDEF";
    char buf[64];
    uint32_t i = 0;

    do {
        buf[i++] = digits[num % base];
        num /= base;
    } while (num > 0);

    // 填充到指定宽度
    while (i < width) {
        buf[i++] = pad_char;
    }

    while (i--) {
        vga_putc(buf[i]);
    }
}

void printf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    while (*fmt) {
        if (*fmt != '%') {
            vga_putc(*fmt++);
            continue;
        }

        fmt++; // 跳过 '%'

        // 解析格式化选项
        int width = 0;
        char pad_char = ' ';  // 默认空格填充

        // 检查是否有 '0' 填充标志
        if (*fmt == '0') {
            pad_char = '0';
            fmt++;
        }

        // 解析宽度
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        switch (*fmt++) {
            case 'd': {
                int32_t num = va_arg(ap, int32_t);
                if (num < 0) {
                    vga_putc('-');
                    num = -num;
                }
                print_num(num, 10, width, pad_char);
                break;
            }
            case 'u':
                print_num(va_arg(ap, uint32_t), 10, width, pad_char);
                break;
            case 'x':
                print_num(va_arg(ap, uint32_t), 16, width, pad_char);
                break;
            case 'c':
                vga_putc((char)va_arg(ap, int));
                break;
            case 's':
                vga_puts(va_arg(ap, char*));
                break;
            default:
                vga_putc('?');
                break;
        }
    }

    va_end(ap);
}

// 与printf相同实现的cprintf函数
void cprintf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    while (*fmt) {
        if (*fmt != '%') {
            vga_putc(*fmt++);
            continue;
        }

        fmt++; // 跳过 '%'

        // 解析格式化选项
        int width = 0;
        char pad_char = ' ';  // 默认空格填充

        // 检查是否有 '0' 填充标志
        if (*fmt == '0') {
            pad_char = '0';
            fmt++;
        }

        // 解析宽度
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        switch (*fmt++) {
            case 'd': {
                int32_t num = va_arg(ap, int32_t);
                if (num < 0) {
                    vga_putc('-');
                    num = -num;
                }
                print_num(num, 10, width, pad_char);
                break;
            }
            case 'u':
                print_num(va_arg(ap, uint32_t), 10, width, pad_char);
                break;
            case 'x':
                print_num(va_arg(ap, uint32_t), 16, width, pad_char);
                break;
            case 'c':
                vga_putc((char)va_arg(ap, int));
                break;
            case 's':
                vga_puts(va_arg(ap, char*));
                break;
            default:
                vga_putc('?');
                break;
        }
    }

    va_end(ap);
}


char * decimal_to_hex(int decimal) {
    char hex_digits[] = "0123456789ABCDEF";
    char *hex_str;
    char temp[20];
    int i = 0, j = 0;
    
    // 处理0的情况
    if (decimal == 0) {
        hex_str[0] = '0';
        hex_str[1] = '\0';
        return;
    }
    
    // 转换为十六进制（逆序）
    while (decimal > 0) {
        temp[i++] = hex_digits[decimal % 16];
        decimal /= 16;
    }
    
    // 反转字符串得到正确顺序
    hex_str[0] = '0';
    hex_str[1] = 'x';
    for (j = 0; j < i; j++) {
        hex_str[j + 2] = temp[i - j - 1];
    }
    hex_str[i + 2] = '\0';
    return *hex_str;
}
