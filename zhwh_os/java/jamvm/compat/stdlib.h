/**
 * compat/stdlib.h — stdlib.h 替代（映射到兼容层）
 */
#ifndef COMPAT_STDLIB_H
#define COMPAT_STDLIB_H

#include "compat.h"

#define atoi(s)        jam_atoi(s)
#define atol(s)        jam_atol(s)
#define strtoll(n,e,b) jam_strtoll(n,e,b)

#define malloc(n)      jam_malloc(n)
#define free(p)        jam_free(p)
#define realloc(p,n)   jam_realloc(p,n)
#define calloc(n,s)    jam_calloc(n,s)

#define exit(n)        jam_exit(n)
#define abort()        jam_exit(1)

#define abs(n)         ((n) < 0 ? -(n) : (n))

#endif
