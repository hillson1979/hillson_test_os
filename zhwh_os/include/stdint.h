// stdint.h - 标准整数类型 + 常用 stddef 定义
#ifndef STDINT_H
#define STDINT_H

/* ---- 固定宽度整数类型 ---- */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;

/* 指针大小整数 */
typedef unsigned int uintptr_t;
typedef int intptr_t;

/* ---- stddef.h 常用定义 (nostdinc 下不可用) ---- */
#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef __SIZE_TYPE__
#define __SIZE_TYPE__ unsigned int
#endif
typedef __SIZE_TYPE__ size_t;
typedef int ssize_t;

typedef int ptrdiff_t;

typedef unsigned short wchar_t;

#endif // STDINT_H
