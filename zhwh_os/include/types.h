// types.h
#pragma once

// 类型定义 - 仅当未定义时才定义
#ifndef int8_t
typedef signed char int8_t;
#endif

#ifndef uint8_t
typedef unsigned char  uint8_t;
#endif

#ifndef uint16_t
typedef unsigned short uint16_t;
#endif

#ifndef uint32_t
typedef unsigned int   uint32_t;
#endif

#ifndef uint64_t
typedef unsigned long long uint64_t;
#endif

#ifndef int32_t
typedef int            int32_t;
#endif

#ifndef int64_t
typedef long long      int64_t;
#endif

#ifndef uintptr_t
typedef uint32_t       uintptr_t;
#endif

// bool类型 - 与stdbool.h兼容
#if !defined(bool) && !defined(__bool_true_false_are_defined)
typedef enum { false, true } bool;
#endif

#ifndef NULL
#define NULL 0
#endif

// 计算数组元素个数的宏
#define NELEM(x) (sizeof(x)/sizeof((x)[0]))
