/**
 * @file stdint_compat.h
 * @brief stdint.h 扩展 - 确保 uintptr_t 可用
 */

#ifndef _STDINT_COMPAT_H
#define _STDINT_COMPAT_H

#include <stdint.h>

// 确保 uintptr_t 和 intptr_t 存在
#ifndef uintptr_t
typedef uint32_t uintptr_t;
#endif

#ifndef intptr_t
typedef int32_t intptr_t;
#endif

// 确保 intmax_t 和 uintmax_t 存在
#ifndef intmax_t
typedef int64_t intmax_t;
#endif

#ifndef uintmax_t
typedef uint64_t uintmax_t;
#endif

// Fast integer types
typedef uint32_t uint_fast8_t;
typedef uint32_t uint_fast16_t;
typedef uint32_t uint_fast32_t;
typedef uint64_t uint_fast64_t;

typedef int32_t int_fast8_t;
typedef int32_t int_fast16_t;
typedef int32_t int_fast32_t;
typedef int64_t int_fast64_t;

// Least integer types
typedef uint8_t uint_least8_t;
typedef uint16_t uint_least16_t;
typedef uint32_t uint_least32_t;
typedef uint64_t uint_least64_t;

typedef int8_t int_least8_t;
typedef int16_t int_least16_t;
typedef int32_t int_least32_t;
typedef int64_t int_least64_t;

// 定义 INT16_MIN 和 INT16_MAX
#ifndef INT16_MIN
#define INT16_MIN (-32768)
#endif

#ifndef INT16_MAX
#define INT16_MAX 32767
#endif

#ifndef UINT16_MAX
#define UINT16_MAX 65535U
#endif

// 定义 INT32_MIN 和 INT32_MAX
#ifndef INT32_MIN
#define INT32_MIN (-2147483647 - 1)
#endif

#ifndef INT32_MAX
#define INT32_MAX 2147483647
#endif

#ifndef INT64_MIN
#define INT64_MIN (-9223372036854775807LL - 1)
#endif

#ifndef INT64_MAX
#define INT64_MAX 9223372036854775807LL
#endif

// 定义 UINT32_MAX 和 UINT64_MAX
#ifndef UINT32_MAX
#define UINT32_MAX 4294967295U
#endif

#ifndef UINT64_MAX
#define UINT64_MAX 18446744073709551615ULL
#endif

// 定义 SIZE_MAX
#ifndef SIZE_MAX
#define SIZE_MAX 4294967295U
#endif

#endif /* _STDINT_COMPAT_H */
