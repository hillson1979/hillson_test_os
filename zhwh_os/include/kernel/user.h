/**
 * @file user.h
 * @brief User/kernel copy compatibility stubs
 */
#ifndef KERNEL_USER_H
#define KERNEL_USER_H

#include <stdint.h>
#include <string.h>

/* In kernel mode, user pointers are just pointers */
#define COPY_TO_USER(dst, src, sz)    (memcpy((dst), (src), (sz)), 0)
#define COPY_FROM_USER(dst, src, sz)  (memcpy((dst), (src), (sz)), 0)

static inline int copy_to_user(void *dst, const void *src, size_t sz) {
    memcpy(dst, src, sz);
    return 0;
}

#endif /* KERNEL_USER_H */
