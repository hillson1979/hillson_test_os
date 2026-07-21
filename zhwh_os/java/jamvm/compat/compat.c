/**
 * compat.c — HillsonOS 标准 C 库兼容实现
 *
 * 使用 HillsonOS syscall 和简单算法实现标准 C 函数。
 */
#include "compat.h"
#include "os_port.h"
#include "stdint.h"

/* ================================================================
 * 输出函数
 * ================================================================ */

static void print_uint(unsigned int n) {
    char buf[12];
    int i = 0;
    if (n == 0) {
        os_putchar('0');
        return;
    }
    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    while (i > 0) os_putchar(buf[--i]);
}

static void print_int(int n) {
    if (n < 0) { os_putchar('-'); n = -n; }
    print_uint((unsigned int)n);
}

static void print_hex(unsigned int n) {
    char buf[10];
    int i = 0;
    const char *hex = "0123456789abcdef";
    if (n == 0) { os_putchar('0'); return; }
    while (n > 0) {
        buf[i++] = hex[n & 0xf];
        n >>= 4;
    }
    while (i > 0) os_putchar(buf[--i]);
}

int jam_printf(const char *fmt, ...) {
    /* 简化版: 只处理 %s, %d, %x, %c, %p */
    va_list ap;
    __builtin_va_start(ap, fmt);

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
            case 's': { char *s = __builtin_va_arg(ap, char*);
                       if (s) jam_print(s); else jam_print("(null)"); break; }
            case 'd': print_int(__builtin_va_arg(ap, int)); break;
            case 'u': print_uint(__builtin_va_arg(ap, unsigned int)); break;
            case 'x': print_hex(__builtin_va_arg(ap, unsigned int)); break;
            case 'c': os_putchar((char)__builtin_va_arg(ap, int)); break;
            case 'p': os_print("0x");
                      print_hex((unsigned int)__builtin_va_arg(ap, void*));
                      break;
            case 'l':
                fmt++;
                if (*fmt == 'd') {
                    long long v = __builtin_va_arg(ap, long long);
                    if (v < 0) { os_putchar('-'); v = -v; }
                    print_uint((unsigned int)(v >> 32));
                    print_uint((unsigned int)v);
                } else if (*fmt == 'u') {
                    unsigned long long v = __builtin_va_arg(ap, unsigned long long);
                    print_uint((unsigned int)(v >> 32));
                    print_uint((unsigned int)v);
                } else if (*fmt == 'l') {
                    fmt++;
                    if (*fmt == 'd') {
                        long long v = __builtin_va_arg(ap, long long);
                        if (v < 0) { os_putchar('-'); v = -v; }
                        print_hex((unsigned int)v);
                    }
                }
                break;
            default:  os_putchar(*fmt); break;
            }
        } else {
            os_putchar(*fmt);
        }
        fmt++;
    }

    __builtin_va_end(ap);
    return 0;
}

int jam_fprintf(void *stream, const char *fmt, ...) {
    /* 忽略 stream，全部输出到控制台 */
    (void)stream;
    va_list ap;
    __builtin_va_start(ap, fmt);
    /* 简化：复用 printf */
    jam_printf("%s", ""); /* placeholder */
    __builtin_va_end(ap);
    return 0;
}

int jam_print(const char *str) {
    os_print(str);
    return 0;
}

int jam_sprintf(char *buf, const char *fmt, ...) {
    /* TODO: 实现到缓冲区的格式化输出 */
    (void)buf; (void)fmt;
    return 0;
}

int jam_snprintf(char *buf, unsigned int size, const char *fmt, ...) {
    /* TODO: 实现 */
    (void)buf; (void)size; (void)fmt;
    return 0;
}

/* 标准输出占位符 */
void *stdout = (void*)1;
void *stderr = (void*)2;

/* ================================================================
 * 内存函数
 * ================================================================ */

void *jam_malloc(unsigned int n) {
    return os_malloc(n);
}

void jam_free(void *ptr) {
    os_free(ptr);
}

void *jam_realloc(void *ptr, unsigned int n) {
    return os_realloc(ptr, n);
}

void *jam_calloc(unsigned int n, unsigned int size) {
    unsigned int total = n * size;
    void *ptr = os_malloc(total);
    if (ptr) {
        unsigned char *p = (unsigned char*)ptr;
        for (unsigned int i = 0; i < total; i++) p[i] = 0;
    }
    return ptr;
}

void *jam_memset(void *s, int c, unsigned int n) {
    unsigned char *p = (unsigned char*)s;
    for (unsigned int i = 0; i < n; i++) p[i] = (unsigned char)c;
    return s;
}

void *jam_memcpy(void *dest, const void *src, unsigned int n) {
    unsigned char *d = (unsigned char*)dest;
    const unsigned char *s = (const unsigned char*)src;
    for (unsigned int i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

int jam_memcmp(const void *s1, const void *s2, unsigned int n) {
    const unsigned char *a = (const unsigned char*)s1;
    const unsigned char *b = (const unsigned char*)s2;
    for (unsigned int i = 0; i < n; i++) {
        if (a[i] != b[i]) return (int)a[i] - (int)b[i];
    }
    return 0;
}

void *jam_memmove(void *dest, const void *src, unsigned int n) {
    /* 简化：分配临时缓冲区 */
    unsigned char *tmp = (unsigned char*)jam_malloc(n);
    if (tmp) {
        jam_memcpy(tmp, src, n);
        jam_memcpy(dest, tmp, n);
        jam_free(tmp);
    }
    return dest;
}

/* ================================================================
 * 字符串函数
 * ================================================================ */

unsigned int jam_strlen(const char *s) {
    unsigned int len = 0;
    while (*s++) len++;
    return len;
}

char *jam_strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++) != 0);
    return dest;
}

char *jam_strncpy(char *dest, const char *src, unsigned int n) {
    unsigned int i;
    for (i = 0; i < n && src[i]; i++) dest[i] = src[i];
    for (; i < n; i++) dest[i] = 0;
    return dest;
}

int jam_strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2 && *s1 == *s2) { s1++; s2++; }
    return (int)(unsigned char)*s1 - (int)(unsigned char)*s2;
}

int jam_strncmp(const char *s1, const char *s2, unsigned int n) {
    for (unsigned int i = 0; i < n; i++) {
        if (s1[i] != s2[i] || s1[i] == 0) {
            return (int)(unsigned char)s1[i] - (int)(unsigned char)s2[i];
        }
    }
    return 0;
}

char *jam_strcat(char *dest, const char *src) {
    char *d = dest;
    while (*d) d++;
    while ((*d++ = *src++) != 0);
    return dest;
}

char *jam_strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) return (char*)s;
        s++;
    }
    return NULL;
}

char *jam_strrchr(const char *s, int c) {
    char *last = NULL;
    while (*s) {
        if (*s == (char)c) last = (char*)s;
        s++;
    }
    return last;
}

char *jam_strdup(const char *s) {
    unsigned int len = jam_strlen(s) + 1;
    char *dup = (char*)jam_malloc(len);
    if (dup) jam_strcpy(dup, s);
    return dup;
}

char *jam_strstr(const char *haystack, const char *needle) {
    if (!*needle) return (char*)haystack;
    while (*haystack) {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && *h == *n) { h++; n++; }
        if (!*n) return (char*)haystack;
        haystack++;
    }
    return NULL;
}

char *jam_strtok_r(char *str, const char *delim, char **saveptr) {
    /* 简化实现 */
    if (str) *saveptr = str;
    if (!*saveptr) return NULL;

    /* 跳过分隔符 */
    while (**saveptr && jam_strchr(delim, **saveptr)) (*saveptr)++;

    if (!**saveptr) return NULL;

    char *token = *saveptr;
    /* 找到下一个分隔符 */
    while (**saveptr && !jam_strchr(delim, **saveptr)) (*saveptr)++;

    if (**saveptr) {
        **saveptr = 0;
        (*saveptr)++;
    }

    return token;
}

/* ================================================================
 * 标准库函数
 * ================================================================ */

int jam_atoi(const char *s) {
    int n = 0, sign = 1;
    while (*s == ' ') s++;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') { s++; }
    while (*s >= '0' && *s <= '9') { n = n * 10 + (*s - '0'); s++; }
    return n * sign;
}

long jam_atol(const char *s) {
    long n = 0, sign = 1;
    while (*s == ' ') s++;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') { s++; }
    while (*s >= '0' && *s <= '9') { n = n * 10 + (*s - '0'); s++; }
    return n * sign;
}

long long jam_strtoll(const char *nptr, char **endptr, int base) {
    long long n = 0, sign = 1;
    while (*nptr == ' ') nptr++;
    if (*nptr == '-') { sign = -1; nptr++; }
    else if (*nptr == '+') { nptr++; }
    if (base == 0) base = 10;
    while (*nptr) {
        int digit = -1;
        if (*nptr >= '0' && *nptr <= '9') digit = *nptr - '0';
        else if (*nptr >= 'a' && *nptr <= 'f') digit = *nptr - 'a' + 10;
        else if (*nptr >= 'A' && *nptr <= 'F') digit = *nptr - 'A' + 10;
        if (digit < 0 || digit >= base) break;
        n = n * base + digit;
        nptr++;
    }
    if (endptr) *endptr = (char*)nptr;
    return n * sign;
}
