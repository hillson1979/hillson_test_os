// printf.c
#include "vga.h"

static void print_num(uint32_t num, uint32_t base) {
    const char* digits = "0123456789ABCDEF";
    char buf[64];
    uint32_t i = 0;

    do {
        buf[i++] = digits[num % base];
        num /= base;
    } while (num > 0);

    while (i--) {
        vga_putc(buf[i]);
    }
}

void printf(const char* fmt, ...) {
    uint32_t* ap = (uint32_t*)&fmt + 1; // 手动实现可变参数

    while (*fmt) {
        if (*fmt != '%') {
            vga_putc(*fmt++);
            continue;
        }

        fmt++; // 跳过 '%'
        switch (*fmt++) {
            case 'd': {
                int32_t num = *(int32_t*)ap++;
                if (num < 0) {
                    vga_putc('-');
                    num = -num;
                }
                print_num(num, 10);
                break;
            }
            case 'u':
                print_num(*(uint32_t*)ap++, 10);
                break;
            case 'x':
                print_num(*(uint32_t*)ap++, 16);
                break;
            case 'c':
                vga_putc((char)*(uint32_t*)ap++);
                break;
            case 's':
                vga_puts(*(char**)ap++);
                break;
            default:
                vga_putc('?');
                break;
        }
    }
}

// 与printf相同实现的cprintf函数
void cprintf(const char* fmt, ...) {
    uint32_t* ap = (uint32_t*)&fmt + 1; // 手动实现可变参数

    while (*fmt) {
        if (*fmt != '%') {
            vga_putc(*fmt++);
            continue;
        }

        fmt++; // 跳过 '%'
        switch (*fmt++) {
            case 'd': {
                int32_t num = *(int32_t*)ap++;
                if (num < 0) {
                    vga_putc('-');
                    num = -num;
                }
                print_num(num, 10);
                break;
            }
            case 'u':
                print_num(*(uint32_t*)ap++, 10);
                break;
            case 'x':
                print_num(*(uint32_t*)ap++, 16);
                break;
            case 'c':
                vga_putc((char)*(uint32_t*)ap++);
                break;
            case 's':
                vga_puts(*(char**)ap++);
                break;
            default:
                vga_putc('?');
                break;
        }
    }
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
