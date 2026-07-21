/**
 * compat/unistd.h — unistd.h 替代
 */
#ifndef COMPAT_UNISTD_H
#define COMPAT_UNISTD_H

#include "compat.h"

/* sysconf 常量 */
#define _SC_PHYS_PAGES  1
#define _SC_PAGESIZE     2

static inline long sysconf(int name) {
    switch (name) {
    case 1: return 32768; /* 128MB / 4096 pages */
    case 2: return 4096;  /* 4KB pages */
    default: return -1;
    }
}

static inline int getpid(void) { return 1; }
static inline int sched_yield(void) { return 0; }
static inline unsigned int sleep(unsigned int sec) {
    /* 忙等待（简化） */
    volatile unsigned int i;
    for (unsigned int s = 0; s < sec; s++)
        for (i = 0; i < 10000000; i++) __asm__ volatile("nop");
    return 0;
}

#endif
