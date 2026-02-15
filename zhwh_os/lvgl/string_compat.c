/**
 * @file string_compat.c
 * @brief 字符串函数兼容实现
 */

#include "libuser_minimal.h"
#include <stdarg.h>

// strchr - 查找字符在字符串中首次出现的位置
char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) {
            return (char *)s;
        }
        s++;
    }
    return NULL;
}

// strcat 的非 inline 实现
char *strcat(char *dest, const char *src) {
    char *d = dest;
    while (*d) d++;
    while ((*d++ = *src++) != '\0');
    return dest;
}

/**
 * @brief 简单的 sprintf 实现
 * 只支持基本格式：%s, %d, %u, %x, %c, %%
 */
int sprintf(char *str, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char *dst = str;
    const char *p = fmt;

    while (*p) {
        if (*p == '%') {
            p++;
            switch (*p) {
                case 's': {  // 字符串
                    const char *s = va_arg(args, const char *);
                    while (*s) {
                        *dst++ = *s++;
                    }
                    break;
                }
                case 'd': {  // 有符号十进制整数
                    int val = va_arg(args, int);
                    if (val < 0) {
                        *dst++ = '-';
                        val = -val;
                    }
                    // 转换为字符串
                    char tmp[16];
                    int i = 0;
                    if (val == 0) {
                        tmp[i++] = '0';
                    } else {
                        while (val > 0) {
                            tmp[i++] = '0' + (val % 10);
                            val /= 10;
                        }
                    }
                    // 反转并复制
                    while (i > 0) {
                        *dst++ = tmp[--i];
                    }
                    break;
                }
                case 'u': {  // 无符号十进制整数
                    unsigned int val = va_arg(args, unsigned int);
                    char tmp[16];
                    int i = 0;
                    if (val == 0) {
                        tmp[i++] = '0';
                    } else {
                        while (val > 0) {
                            tmp[i++] = '0' + (val % 10);
                            val /= 10;
                        }
                    }
                    while (i > 0) {
                        *dst++ = tmp[--i];
                    }
                    break;
                }
                case 'x': {  // 十六进制（小写）
                    unsigned int val = va_arg(args, unsigned int);
                    char tmp[16];
                    int i = 0;
                    if (val == 0) {
                        tmp[i++] = '0';
                    } else {
                        while (val > 0) {
                            int digit = val % 16;
                            tmp[i++] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
                            val /= 16;
                        }
                    }
                    while (i > 0) {
                        *dst++ = tmp[--i];
                    }
                    break;
                }
                case 'c': {  // 字符
                    char c = (char)va_arg(args, int);
                    *dst++ = c;
                    break;
                }
                case '%': {  // %% 输出 %
                    *dst++ = '%';
                    break;
                }
                case '\0':  // % 在字符串末尾
                    *dst++ = '%';
                    goto done;
                default:
                    *dst++ = '%';
                    *dst++ = *p;
                    break;
            }
            p++;
        } else {
            *dst++ = *p++;
        }
    }

done:
    va_end(args);
    *dst = '\0';
    return dst - str;
}

/**
 * @brief 简单的 vsnprintf 实现
 */
int vsnprintf(char *str, size_t size, const char *fmt, va_list args) {
    if (size == 0) return 0;

    char *dst = str;
    const char *p = fmt;
    size_t remaining = size - 1;  // 保留空间给 null 终止符

    while (*p && remaining > 0) {
        if (*p == '%') {
            p++;
            switch (*p) {
                case 's': {
                    const char *s = va_arg(args, const char *);
                    while (*s && remaining > 0) {
                        *dst++ = *s++;
                        remaining--;
                    }
                    break;
                }
                case 'd': {
                    int val = va_arg(args, int);
                    if (val < 0 && remaining > 0) {
                        *dst++ = '-';
                        remaining--;
                        val = -val;
                    }
                    char tmp[16];
                    int i = 0;
                    if (val == 0) {
                        tmp[i++] = '0';
                    } else {
                        while (val > 0) {
                            tmp[i++] = '0' + (val % 10);
                            val /= 10;
                        }
                    }
                    while (i > 0 && remaining > 0) {
                        *dst++ = tmp[--i];
                        remaining--;
                    }
                    break;
                }
                case 'u': {
                    unsigned int val = va_arg(args, unsigned int);
                    char tmp[16];
                    int i = 0;
                    if (val == 0) {
                        tmp[i++] = '0';
                    } else {
                        while (val > 0) {
                            tmp[i++] = '0' + (val % 10);
                            val /= 10;
                        }
                    }
                    while (i > 0 && remaining > 0) {
                        *dst++ = tmp[--i];
                        remaining--;
                    }
                    break;
                }
                case 'x': {
                    unsigned int val = va_arg(args, unsigned int);
                    char tmp[16];
                    int i = 0;
                    if (val == 0) {
                        tmp[i++] = '0';
                    } else {
                        while (val > 0) {
                            int digit = val % 16;
                            tmp[i++] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
                            val /= 16;
                        }
                    }
                    while (i > 0 && remaining > 0) {
                        *dst++ = tmp[--i];
                        remaining--;
                    }
                    break;
                }
                case 'c': {
                    if (remaining > 0) {
                        char c = (char)va_arg(args, int);
                        *dst++ = c;
                        remaining--;
                    }
                    break;
                }
                case '%': {
                    if (remaining > 0) {
                        *dst++ = '%';
                        remaining--;
                    }
                    break;
                }
                case '\0':
                    if (remaining > 0) {
                        *dst++ = '%';
                        remaining--;
                    }
                    goto done_vsn;
                default:
                    if (remaining >= 2) {
                        *dst++ = '%';
                        *dst++ = *p;
                        remaining -= 2;
                    } else if (remaining == 1) {
                        *dst++ = '%';
                        remaining--;
                    }
                    break;
            }
            p++;
        } else {
            *dst++ = *p++;
            remaining--;
        }
    }

done_vsn:
    *dst = '\0';
    return size - remaining - 1;
}
