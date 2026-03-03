#ifndef STB_IMAGE_OS_H
#define STB_IMAGE_OS_H

// ====== 禁用所有不需要的stb_image功能 ======
#define STB_NO_STDIO
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR

// ====== 包含OS头文件（使用现有定义） ======
#include "kmalloc.h"
#include <string.h>

// ====== 禁用assert ======
#ifndef assert
#define assert(x) ((void)0)
#endif

// ====== 基本类型定义（仅当不存在时） ======
#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

#ifndef INT_MAX
#define INT_MAX 2147483647
#endif

// ====== 数学函数实现 ======
static inline double stbi_ldexp(double x, int exp) {
    double result = x;
    if (exp > 0) {
        while (exp > 0) {
            result *= 2.0;
            exp--;
        }
    } else if (exp < 0) {
        while (exp < 0) {
            result /= 2.0;
            exp++;
        }
    }
    return result;
}

static inline double stbi_pow(double x, double y) {
    double result = 1.0;
    int int_y = (int)y;

    if (y < 0) {
        return 1.0 / stbi_pow(x, -y);
    }

    while (int_y > 0) {
        if (int_y & 1) {
            result *= x;
        }
        x *= x;
        int_y >>= 1;
    }

    return result;
}

static inline double stbi_log(double x) {
    if (x <= 0.0) return -999999.0;
    double term = (x - 1.0) / (x + 1.0);
    double term_squared = term * term;
    return 2.0 * (term + term*term_squared/3.0);
}

static inline double stbi_fabs(double x) {
    return (x < 0) ? -x : x;
}

static inline double stbi_floor(double x) {
    return (double)((int)x);
}

static inline double stbi_sqrt(double x) {
    if (x < 0) return 0.0;
    if (x == 0) return 0.0;
    double guess = x;
    double prev = 0.0;
    while (guess != prev) {
        prev = guess;
        guess = (guess + x / guess) / 2.0;
    }
    return guess;
}

static inline int stbi_abs(int x) {
    return (x < 0) ? -x : x;
}

// ====== 映射到stb需要的函数名 ======
#define ldexp stbi_ldexp
#define pow stbi_pow
#define log stbi_log
#define fabs stbi_fabs
#define floor stbi_floor
#define sqrt stbi_sqrt
#define abs stbi_abs

// ====== stb_image内存分配宏（使用kmalloc） ======
#ifndef STBI_MALLOC
#define STBI_MALLOC(s)    kmalloc(s)
#endif

#ifndef STBI_FREE
#define STBI_FREE(p)      kfree(p)
#endif

#ifndef STBI_REALLOC
// 简单的realloc实现
static inline void* stbi__realloc(void* ptr, unsigned int size) {
    if (size == 0) {
        kfree(ptr);
        return NULL;
    }
    void* new_ptr = kmalloc(size);
    if (new_ptr && ptr) {
        memcpy(new_ptr, ptr, size);
        kfree(ptr);
    }
    return new_ptr;
}
#define STBI_REALLOC(p,s) stbi__realloc(p,s)
#endif

#endif // STB_IMAGE_OS_H
