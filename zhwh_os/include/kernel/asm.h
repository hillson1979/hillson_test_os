/**
 * @file asm.h
 */
#ifndef KERNEL_ASM_H
#define KERNEL_ASM_H

#include <string.h>
#include <kernel.h>

#define A_memset  memset
#define A_memcpy  memcpy

/* Byte extraction macros */
#define BYTE1(x)  ((uint8_t)((x) & 0xFF))
#define BYTE2(x)  ((uint8_t)(((x) >> 8) & 0xFF))
#define BYTE3(x)  ((uint8_t)(((x) >> 16) & 0xFF))
#define BYTE4(x)  ((uint8_t)(((x) >> 24) & 0xFF))

/* Static inline attribute */
#define STATIC_INLINE  static inline

/* Unused parameter suppression */
#define UNUSED(x)  (void)(x)

/* Min/max */
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

/* Debug print */
#define KDEBUG(fmt, ...)  /* disabled */

/* Pointer format string for printk */
#define _XPTR_  "0x%x"

/* Atomic compare-and-swap using GCC builtins */
#define __sync_bool_compare_and_swap(ptr, oldval, newval) \
    __sync_bool_compare_and_swap((ptr), (oldval), (newval))
#define __atomic_store_n(ptr, val, memorder) \
    __atomic_store_n((ptr), (val), (memorder))
#ifndef __ATOMIC_SEQ_CST
#define __ATOMIC_SEQ_CST  5
#endif

#endif /* KERNEL_ASM_H */
