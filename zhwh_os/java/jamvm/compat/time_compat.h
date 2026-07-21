/**
 * compat/time_compat.h — time.h 替代
 *
 * 简化实现，使用 PIT 或时间戳计数器。
 */
#ifndef COMPAT_TIME_H
#define COMPAT_TIME_H

#include "compat.h"

/* 时间结构 */
struct timespec {
    long tv_sec;
    long tv_nsec;
};

struct timeval {
    long tv_sec;
    long tv_usec;
};

typedef long time_t;
typedef long clockid_t;

#define CLOCK_REALTIME           0
#define CLOCK_MONOTONIC          1
#define CLOCK_REALTIME_COARSE    5

/* 获取时间（简化实现 — 返回固定值用于启动） */
static inline int clock_gettime(clockid_t clk_id, struct timespec *tp) {
    (void)clk_id;
    /* TODO: 从内核获取实际时间 */
    if (tp) {
        tp->tv_sec = 0;
        tp->tv_nsec = 0;
    }
    return 0;
}

static inline int gettimeofday(struct timeval *tv, void *tz) {
    (void)tz;
    if (tv) {
        tv->tv_sec = 0;
        tv->tv_usec = 0;
    }
    return 0;
}

/* 休眠支持（简化忙等待） */
static inline int nanosleep(const struct timespec *req, struct timespec *rem) {
    (void)rem;
    if (req) {
        /* 忙等待 */
        volatile long count = req->tv_sec * 1000000 + req->tv_nsec / 1000;
        while (count-- > 0) __asm__ volatile("nop");
    }
    return 0;
}

#endif
