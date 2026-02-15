/**
 * @file stddef.h
 * @brief stddef.h 兼容层
 */

#ifndef _STDDEF_H
#define _STDDEF_H

#include "stdint_compat.h"

// 定义 size_t
#ifndef _SIZE_T
#define _SIZE_T
typedef uint32_t size_t;
#endif

// 定义 ptrdiff_t
#ifndef _PTRDIFF_T
#define _PTRDIFF_T
typedef int32_t ptrdiff_t;
#endif

// 定义 NULL
#ifndef NULL
#define NULL ((void *)0)
#endif

// 定义 offsetof
#define offsetof(type, member) ((size_t)&(((type *)0)->member))

#endif /* _STDDEF_H */
