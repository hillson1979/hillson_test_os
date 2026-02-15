/**
 * @file stdarg.h
 * @brief stdarg.h 兼容层 - 变参函数支持
 */

#ifndef _STDARG_H
#define _STDARG_H

// 使用 GCC 内置的 va_list 支持
typedef __builtin_va_list va_list;

#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type)   __builtin_va_arg(ap, type)
#define va_end(ap)         __builtin_va_end(ap)
#define va_copy(dest, src) __builtin_va_copy(dest, src)

#endif /* _STDARG_H */
