// types.h
#pragma once
typedef signed char int8_t;

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef unsigned long  uint64_t;
typedef int            int32_t;
typedef uint32_t       uintptr_t;
typedef enum { false, true } bool;

#undef NULL
#define NULL 0

// 计算数组元素个数的宏
#define NELEM(x) (sizeof(x)/sizeof((x)[0]))

