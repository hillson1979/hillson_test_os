// printf.c
#include "vga.h"
#include <stdarg.h>

// Define size_t for snprintf
typedef unsigned int size_t;

static void print_num(uint32_t num, uint32_t base, int width, char pad_char) {
    // 🔥 关键修复：防止除零异常
    // 如果 base < 2，会导致除零或无限循环
    if (base < 2 || base > 16) {
        // 输出错误标记而不是崩溃
        vga_puts("[EINVAL]");
        return;
    }

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
            char c = *fmt++;
            vga_putc(c);
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

/**
 * @brief snprintf - 格式化字符串到缓冲区（简化版）
 */
int snprintf(char *str, size_t size, const char *fmt, ...) {
    if (!str || size == 0) {
        return -1;
    }

    va_list ap;
    va_start(ap, fmt);

    int len = 0;
    const char *start = fmt;

    while (*fmt && len < size - 1) {
        if (*fmt != '%') {
            str[len++] = *fmt++;
            continue;
        }

        fmt++; // 跳过 '%'

        // 解析格式化选项
        int width = 0;
        char pad_char = ' ';
        int long_flag = 0;

        // 检查 '0' 填充
        if (*fmt == '0') {
            pad_char = '0';
            fmt++;
        }

        // 检查宽度
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        // 检查长度修饰符
        if (*fmt == 'l') {
            long_flag = 1;
            fmt++;
        }

        // 处理格式类型
        switch (*fmt++) {
            case 'd': {
                int num = va_arg(ap, int);
                char buf[32];
                int i = 0;
                int neg = 0;

                if (num == 0) {
                    buf[i++] = '0';
                } else {
                    neg = num < 0;
                    if (neg) num = -num;

                    while (num > 0) {
                        buf[i++] = '0' + (num % 10);
                        num /= 10;
                    }
                }

                // 填充
                while (i < width) buf[i++] = pad_char;

                // 负号
                if (neg && len < size - 1) {
                    str[len++] = '-';
                    len++;
                }

                // 数字
                while (i > 0 && len < size - 1) {
                    str[len++] = buf[--i];
                    len++;
                }
                break;
            }
            case 'u': {
                unsigned int num = va_arg(ap, unsigned int);
                char buf[32];
                int i = 0;

                if (num == 0) {
                    buf[i++] = '0';
                } else {
                    while (num > 0) {
                        buf[i++] = '0' + (num % 10);
                        num /= 10;
                    }
                }

                while (i < width) buf[i++] = pad_char;

                while (i > 0 && len < size - 1) {
                    str[len++] = buf[--i];
                    len++;
                }
                break;
            }
            case 'x':
            case 'X': {
                unsigned int num = va_arg(ap, unsigned int);
                char buf[16];
                int i = 0;

                if (num == 0) {
                    buf[i++] = '0';
                } else {
                    while (num > 0) {
                        buf[i++] = "0123456789ABCDEF"[num & 0xF];
                        num >>= 4;
                    }
                }

                while (i < width) buf[i++] = pad_char;

                while (i > 0 && len < size - 1) {
                    str[len++] = buf[--i];
                    len++;
                }
                break;
            }
            case 's': {
                char *s = va_arg(ap, char*);
                if (!s) s = "(null)";

                while (*s && len < size - 1) {
                    str[len++] = *s++;
                }

                // 填充剩余宽度
                while (len < width && len < size - 1) {
                    str[len++] = ' ';
                }
                break;
            }
            case 'c': {
                char c = (char)va_arg(ap, int);
                if (len < size - 1) {
                    str[len++] = c;
                }
                break;
            }
            case '%': {
                if (len < size - 1) {
                    str[len++] = '%';
                }
                break;
            }
            default:
                if (len < size - 1) {
                    str[len++] = '?';
                }
                break;
        }
    }

    str[len] = '\0';
    va_end(ap);

    return len;
}
