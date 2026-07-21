/**
 * compat/stdio.h — stdio.h 替代（映射到兼容层）
 */
#ifndef COMPAT_STDIO_H
#define COMPAT_STDIO_H

#include "compat.h"

/* FILE 类型 */
typedef void FILE;

extern void *stdout;
extern void *stderr;

/* 宏映射 */
#define printf(...)    jam_printf(__VA_ARGS__)
#define fprintf(...)   jam_fprintf(__VA_ARGS__)
#define sprintf(...)   jam_sprintf(__VA_ARGS__)
#define snprintf(...)  jam_snprintf(__VA_ARGS__)
#define puts(s)        jam_print(s)
#define fputs(s,f)     jam_print(s)
#define fflush(f)      /* 无操作 */
#define putchar(c)     os_putchar(c)

/* vsnprintf 使用 GCC 内置 */
#define vsnprintf(buf, size, fmt, ap) \
    __builtin_vsnprintf(buf, size, fmt, ap)

#endif
