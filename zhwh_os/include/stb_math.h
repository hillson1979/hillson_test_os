#ifndef STB_MATH_H
#define STB_MATH_H

// 禁用标准数学库
#ifndef STBI_NO_STDIO
#define STBI_NO_STDIO
#endif

// stb_image 需要的数学函数
#ifndef STB_NO_STDIO

// ldexp(x, exp) = x * 2^exp
static inline double stbi_ldexp(double x, int exp) {
    // 简化实现：使用乘法
    // 对于JPEG解码，通常exp的范围很小
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

#define ldexp stbi_ldexp

// pow(x, y) = x^y
static inline double stbi_pow(double x, double y) {
    // 简化实现：只处理整数幂
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

#define pow stbi_pow

// log(x) = 自然对数
static inline double stbi_log(double x) {
    // 简化实现：泰勒级数展开
    // log(1+x) ≈ x - x²/2 + x³/3 - ...
    // 对于0 < x < 2

    if (x <= 0.0) return -999999.0;  // 错误值

    // 使用 ln(x) = 2 * ln(sqrt(x))
    double result = 0.0;
    double term = (x - 1.0) / (x + 1.0);
    double term_squared = term * term;

    // 快速收敛级数
    result = 2.0 * (term + term*term_squared/3.0);

    return result;
}

#define log stbi_log

// fabs(x) = 绝对值
static inline double stbi_fabs(double x) {
    return (x < 0) ? -x : x;
}

#define fabs stbi_fabs

// floor(x) = 向下取整
static inline double stbi_floor(double x) {
    return (double)((int)x);
}

#define floor stbi_floor

// sqrt(x) = 平方根
static inline double stbi_sqrt(double x) {
    if (x < 0) return 0.0;
    if (x == 0) return 0.0;

    // 牛顿迭代法
    double guess = x;
    double prev = 0.0;

    while (guess != prev) {
        prev = guess;
        guess = (guess + x / guess) / 2.0;
    }

    return guess;
}

#define sqrt stbi_sqrt

#endif // STB_NO_STDIO

#endif // STB_MATH_H
