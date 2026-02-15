/**
 * @file types_minimal.h
 * @brief 最小化类型定义 - 完全独立于内核
 */

#ifndef TYPES_MINIMAL_H
#define TYPES_MINIMAL_H

// 基本类型
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

typedef signed char        int8_t;
typedef signed short       int16_t;
typedef signed int         int32_t;
typedef signed long long   int64_t;

// 指针类型
typedef uint32_t uintptr_t;
typedef int32_t  intptr_t;

// size_t
typedef uint32_t size_t;
typedef int32_t  ptrdiff_t;

// NULL 定义
#define NULL ((void *)0)

// bool 类型 (从 stdbool.h)
typedef enum {
    false = 0,
    true  = 1
} bool;

#endif /* TYPES_MINIMAL_H */
