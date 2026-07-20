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

//Framebuffer 信息结构
typedef struct {
    void *fb_addr;      // 帧缓冲区地址
    uint32_t width;     // 屏幕宽度
    uint32_t height;    // 屏幕高度
    uint32_t pitch;     // 每行字节数
    uint32_t bpp;       // 每像素位数
} fb_info_t;

// 系统调用号
#define SYS_GUI_FB_INFO  70
#define SYS_EXIT          2  // 修复：应该与 libuser.h 一致（SYS_PRINTF=1, SYS_EXIT=2）
#define SYS_WRITE         11
#define SYS_KMALLOC       74
#define SYS_KFREE         75
#define SYS_OPEN          20
#define SYS_CLOSE         21
#define SYS_READ          22
#define SYS_LSEEK         23

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

// 文件操作函数
static inline int write(int fd, const char *buf, int len) {
    int ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_WRITE), "b"(fd), "c"(buf), "d"(len)
        : "memory", "cc"
    );
    return ret;
}

static inline int open(const char *pathname, int flags) {
    int ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_OPEN), "b"(pathname), "c"(flags)
        : "memory", "cc"
    );
    return ret;
}

static inline int close(int fd) {
    int ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_CLOSE), "b"(fd)
        : "memory", "cc"
    );
    return ret;
}

static inline int lseek(int fd, int offset, int whence) {
    int ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_LSEEK), "b"(fd), "c"(offset), "d"(whence)
        : "memory", "cc"
    );
    return ret;
}

// ⚠️ 所有字符串函数都使用 libuser.a 静态库中的实现
//    这样可以确保所有库函数都在 _start 之后
extern void *memcpy(void *dest, const void *src, size_t n);
extern void *memset(void *s, int c, size_t n);
extern size_t strlen(const char *s);
extern int strcmp(const char *s1, const char *s2);
extern int strncmp(const char *s1, const char *s2, size_t n);
extern int memcmp(const void *s1, const void *s2, size_t n);
extern char *strcpy(char *dest, const char *src);

// strcat 在 string_compat.c 中实现
char *strcat(char *dest, const char *src);

// sprintf 在 string_compat.c 中实现
// 注意：vsnprintf 不在这里声明，因为需要 va_list 类型（在 stdarg.h 中定义）
int sprintf(char *str, const char *fmt, ...);

// 🔥 用户态动态内存分配（通过系统调用）
static inline void *video_kmalloc(unsigned int size) {
    void *ptr;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ptr)
        : "a"(SYS_KMALLOC), "b"(size)
        : "memory", "cc"
    );
    return ptr;
}

static inline void video_kfree(void *ptr) {
    int ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_KFREE), "b"(ptr)
        : "memory", "cc"
    );
    (void)ret;
}

#endif /* LIBUSER_MINIMAL_H */
