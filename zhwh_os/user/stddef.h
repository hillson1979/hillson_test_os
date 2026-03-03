/**
 * @file stddef.h
 * @brief 标准类型定义
 */

#ifndef _STDDEF_H
#define _STDDEF_H

// 定义 size_t
#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int size_t;
#endif

// 定义 ptrdiff_t
#ifndef _PTRDIFF_T
#define _PTRDIFF_T
typedef int ptrdiff_t;
#endif

// 定义 NULL
#ifndef NULL
#define NULL ((void *)0)
#endif

// 定义 offsetof
#define offsetof(type, member) ((size_t)&(((type *)0)->member))

#endif /* _STDDEF_H */
