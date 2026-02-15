/**
 * @file libgcc_compat.c
 * @brief 64位除法和取模函数的兼容实现
 */

#include "stdint_compat.h"

// 64位无符号除法
uint64_t __udivdi3(uint64_t n, uint64_t d) {
    if (d == 0) {
        return 0;  // 除零错误
    }

    uint64_t quotient = 0;
    uint64_t remainder = 0;

    for (int i = 63; i >= 0; i--) {
        remainder = (remainder << 1) | ((n >> i) & 1);
        if (remainder >= d) {
            remainder -= d;
            quotient |= (1ULL << i);
        }
    }

    return quotient;
}

// 64位无符号取模
uint64_t __umoddi3(uint64_t n, uint64_t d) {
    if (d == 0) {
        return 0;  // 除零错误
    }

    uint64_t remainder = 0;

    for (int i = 63; i >= 0; i--) {
        remainder = (remainder << 1) | ((n >> i) & 1);
        if (remainder >= d) {
            remainder -= d;
        }
    }

    return remainder;
}
