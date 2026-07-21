/**
 * compat.h — HillsonOS 标准 C 库兼容层
 *
 * 为 JamVM 提供缺失的标准 C 库函数。
 * 映射到 HillsonOS libuser 的 syscall 接口。
 */
#ifndef JAMVM_COMPAT_H
#define JAMVM_COMPAT_H

#include "stdint.h"

/* ================================================================
 * 输出函数
 * ================================================================ */

/** 简化版 printf（仅支持 %s, %d, %x, %c） */
int jam_printf(const char *fmt, ...);
int jam_fprintf(void *stream, const char *fmt, ...);
int jam_print(const char *str);
int jam_sprintf(char *buf, const char *fmt, ...);
int jam_snprintf(char *buf, unsigned int size, const char *fmt, ...);

/* ================================================================
 * 内存函数
 * ================================================================ */

void *jam_malloc(unsigned int n);
void jam_free(void *ptr);
void *jam_realloc(void *ptr, unsigned int n);
void *jam_calloc(unsigned int n, unsigned int size);

void *jam_memset(void *s, int c, unsigned int n);
void *jam_memcpy(void *dest, const void *src, unsigned int n);
int   jam_memcmp(const void *s1, const void *s2, unsigned int n);
void *jam_memmove(void *dest, const void *src, unsigned int n);

/* ================================================================
 * 字符串函数
 * ================================================================ */

unsigned int jam_strlen(const char *s);
char *jam_strcpy(char *dest, const char *src);
char *jam_strncpy(char *dest, const char *src, unsigned int n);
int jam_strcmp(const char *s1, const char *s2);
int jam_strncmp(const char *s1, const char *s2, unsigned int n);
char *jam_strcat(char *dest, const char *src);
char *jam_strchr(const char *s, int c);
char *jam_strrchr(const char *s, int c);
char *jam_strdup(const char *s);
char *jam_strstr(const char *haystack, const char *needle);
char *jam_strtok_r(char *str, const char *delim, char **saveptr);

/* ================================================================
 * 标准库函数
 * ================================================================ */

int jam_atoi(const char *s);
long jam_atol(const char *s);
long long jam_strtoll(const char *nptr, char **endptr, int base);

/* ================================================================
 * 宏映射 — 将标准 C 函数映射到兼容层
 * ================================================================ */

#define memset(p,c,n)    jam_memset(p,c,n)
#define memcpy(d,s,n)    jam_memcpy(d,s,n)
#define memcmp(a,b,n)    jam_memcmp(a,b,n)
#define memmove(d,s,n)   jam_memmove(d,s,n)
#define strlen(s)        jam_strlen(s)
#define strcpy(d,s)      jam_strcpy(d,s)
#define strncpy(d,s,n)   jam_strncpy(d,s,n)
#define strcmp(a,b)      jam_strcmp(a,b)
#define strncmp(a,b,n)   jam_strncmp(a,b,n)
#define strcat(d,s)      jam_strcat(d,s)
#define strchr(s,c)      jam_strchr(s,c)
#define strrchr(s,c)     jam_strrchr(s,c)
#define strdup(s)        jam_strdup(s)
#define strstr(h,n)      jam_strstr(h,n)
#define strtok_r(s,d,p)  jam_strtok_r(s,d,p)
#define atoi(s)          jam_atoi(s)
#define atol(s)          jam_atol(s)
#define strtoll(n,e,b)   jam_strtoll(n,e,b)

#endif /* JAMVM_COMPAT_H */
