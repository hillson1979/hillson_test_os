/**
 * compat/inttypes.h — inttypes.h 替代
 *
 * 为 i386 提供标准整数类型定义。
 */
#ifndef COMPAT_INTTYPES_H
#define COMPAT_INTTYPES_H

/* 使用 GCC 内置类型 */
#ifndef __INT8_TYPE__
#define __INT8_TYPE__    signed char
#define __INT16_TYPE__   short
#define __INT32_TYPE__   int
#define __INT64_TYPE__   long long
#endif

/* 包含 stdint.h 获取基本类型 */
/* 注意: stdint.h 在 zhwh_os/include/ 中已提供 */

/* intmax_t / uintmax_t */
typedef long long           intmax_t;
typedef unsigned long long  uintmax_t;

/* uintptr_t (如果在 stdint.h 中未定义) */
#ifndef __UINTPTR_TYPE__
typedef unsigned int        uintptr_t;
#else
/* 使用 GCC 内置 */
#endif

#define PRIx64   "llx"
#define PRId64   "lld"

#endif
