/**
 * @file libuser_minimal.h
 * @brief libuser 最小化接口 - 只包含必要的函数声明
 */

#ifndef LIBUSER_MINIMAL_H
#define LIBUSER_MINIMAL_H

#include "stdint_compat.h"
#include "stddef.h"

// 定义 NULL
#ifndef NULL
#define NULL ((void *)0)
#endif

// Framebuffer 信息结构
typedef struct {
    void *fb_addr;      // 帧缓冲区地址
    uint32_t width;     // 屏幕宽度
    uint32_t height;    // 屏幕高度
    uint32_t pitch;     // 每行字节数
    uint32_t bpp;       // 每像素位数
} fb_info_t;

// 系统调用号
#define SYS_GUI_FB_INFO  70
#define SYS_WRITE         4
#define SYS_EXIT          1

// 基本函数
int printf(const char *format, ...);
void exit(int code);

// 系统调用包装
static inline int gui_get_fb_info(fb_info_t *info) {
    int ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_GUI_FB_INFO), "b"(info)
        : "memory", "cc"
    );
    return ret;
}

// 标准库函数实现 (inline 以避免链接冲突)
static inline void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dest;
}

static inline void *memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

static inline size_t strlen(const char *s) {
    const char *p = s;
    while (*p) p++;
    return p - s;
}

static inline int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

static inline int strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

static inline int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

static inline char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++) != '\0');
    return dest;
}

// strcat 在 string_compat.c 中实现
char *strcat(char *dest, const char *src);

// sprintf 在 string_compat.c 中实现
// 注意：vsnprintf 不在这里声明，因为需要 va_list 类型（在 stdarg.h 中定义）
int sprintf(char *str, const char *fmt, ...);

#endif /* LIBUSER_MINIMAL_H */
