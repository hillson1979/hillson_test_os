/**
 * @file string.h
 * @brief string.h 兼容层 - 内联实现避免冲突
 */

#ifndef _STRING_H
#define _STRING_H

#include "stddef.h"

// 函数声明 (实现在 libuser_minimal.h 中)
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
size_t strlen(const char *s);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);

#endif /* _STRING_H */
