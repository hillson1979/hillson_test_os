/**
 * @file dev.h
 * @brief Device node compatibility stubs
 */
#ifndef KERNEL_DEV_H
#define KERNEL_DEV_H

#include <stdint.h>

typedef uint32_t dev_t;

#define TO_DEVID(major, minor)  (((major) << 16) | (minor))
#define MINOR(dev)              ((dev) & 0xFFFF)

#endif /* KERNEL_DEV_H */
