/**
 * compat/sys_mman.h — sys/mman.h 替代
 *
 * HillsonOS 无 mmap。使用 malloc 替代。
 */
#ifndef COMPAT_SYS_MMAN_H
#define COMPAT_SYS_MMAN_H

#include "compat.h"

/* mmap 标志 */
#define PROT_READ    1
#define PROT_WRITE   2
#define PROT_EXEC    4

#define MAP_PRIVATE     2
#define MAP_ANONYMOUS   32
#define MAP_FAILED      ((void*)-1)

/* 使用 malloc 模拟 mmap */
static inline void *mmap(void *addr, unsigned int length, int prot,
                         int flags, int fd, int offset) {
    (void)addr; (void)prot; (void)fd; (void)offset;
    if (flags & MAP_ANONYMOUS) {
        return malloc(length);
    }
    return MAP_FAILED;
}

static inline int munmap(void *addr, unsigned int length) {
    (void)length;
    free(addr);
    return 0;
}

static inline int mprotect(void *addr, unsigned int length, int prot) {
    (void)addr; (void)length; (void)prot;
    return 0; /* HillsonOS: 无内存保护 */
}

#endif
