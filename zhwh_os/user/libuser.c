// libuser.c - 用户库实现
#include "libuser.h"

// 简单的 strlen 实现
static int strlen(const char *s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

// 简单的 memcpy 实现
static void *memcpy(void *dst, const void *src, int n) {
    char *d = (char*)dst;
    const char *s = (const char*)src;
    while (n--) *d++ = *s++;
    return dst;
}

// 简单的数字转字符串
static char *itoa(int num, char *str, int base) {
    int i = 0;
    int is_negative = 0;

    // 处理 0
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    // 处理负数
    if (num < 0 && base == 10) {
        is_negative = 1;
        num = -num;
    }

    // 转换数字
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }

    // 添加负号
    if (is_negative) {
        str[i++] = '-';
    }

    str[i] = '\0';

    // 反转字符串
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char tmp = str[start];
        str[start] = str[end];
        str[end] = tmp;
        start++;
        end--;
    }

    return str;
}

// va_list 支持
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

// printf 实现
int printf(const char *fmt, ...) {
    char buf[1024];
    int buf_idx = 0;

    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            if (*fmt == '\0') break;

            if (*fmt == 'd' || *fmt == 'u') {
                // 整数
                int num = va_arg(args, int);
                char num_str[32];
                itoa(num, num_str, 10);
                int len = strlen(num_str);
                memcpy(&buf[buf_idx], num_str, len);
                buf_idx += len;
            } else if (*fmt == 'x' || *fmt == 'X') {
                // 十六进制
                int num = va_arg(args, int);
                char num_str[32];
                itoa(num, num_str, 16);
                int len = strlen(num_str);
                memcpy(&buf[buf_idx], num_str, len);
                buf_idx += len;
            } else if (*fmt == 's') {
                // 字符串
                char *str = va_arg(args, char*);
                int len = strlen(str);
                memcpy(&buf[buf_idx], str, len);
                buf_idx += len;
            } else if (*fmt == 'c') {
                // 字符
                char c = (char)va_arg(args, int);
                buf[buf_idx++] = c;
            } else if (*fmt == '%') {
                // %% 输出 %
                buf[buf_idx++] = '%';
            }
            fmt++;
        } else {
            buf[buf_idx++] = *fmt++;
        }

        // 防止缓冲区溢出
        if (buf_idx >= 1010) break;
    }

    va_end(args);

    buf[buf_idx] = '\0';

    // 使用 write 系统调用输出
    write(1, buf, buf_idx);

    return buf_idx;
}

// ⚠️ 声明汇编包装函数，避免内联汇编的寄存器冲突问题
extern int syscall_write(int fd, const char *buf, int len);
extern int syscall_fork(void);
extern void syscall_exit(int code) __attribute__((noreturn));
extern void syscall_yield(void);

// write 系统调用
int write(int fd, const char *buf, int len) {
    return syscall_write(fd, buf, len);
}

// fork 系统调用
int fork(void) {
    return syscall_fork();
}

// exit 系统调用
void exit(int code) {
    syscall_exit(code);
    // 永远不会到达这里
    while (1) {
        __asm__ volatile("hlt");
    }
}

// yield - 让出CPU
void yield(void) {
    syscall_yield();
}
