/**
 * @file hdreg.h
 * @brief Disk request type compatibility
 */
#ifndef SYS_HDREG_H
#define SYS_HDREG_H

#include <stdint.h>

struct disk_req_t {
    uint32_t dev;
    void    *data;
    uint32_t datasz;
    uint32_t blockno;
    int      write;
    uint32_t fs_blocksz;
};

struct parttab_s {
    uint32_t lba;
    uint32_t size;
};

typedef int (*read_sector_fn_t)(void *dev, uintptr_t phys_buf, uintptr_t virt_buf, uint32_t lba);
typedef void (*register_part_fn_t)(void *dev, struct parttab_s *part, int n);

static inline int read_disk_mbr(const char *name, void *dev, uint32_t blksz,
                                 read_sector_fn_t read_fn, register_part_fn_t reg_fn) {
    (void)name; (void)dev; (void)blksz; (void)read_fn; (void)reg_fn;
    return -1;
}

#endif /* SYS_HDREG_H */
