/**
 * @file ata.h
 * @brief ATA disk subsystem compatibility stubs
 */
#ifndef KERNEL_ATA_H
#define KERNEL_ATA_H

#include <stdint.h>
#include <kernel/dev.h>

struct parttab_s;

#define IDE_UNKNOWN  0

struct ata_dev_s {
    uint8_t  type;
    uint16_t base;
    uint8_t  irq;
    void    *priv;
    uint32_t bytes_per_sector;
    uint64_t size;
};

#define MAX_AHCI_DEVICES  16
#define AHCI_DEV_MAJ      8

extern struct ata_dev_s *ahci_disk_dev[MAX_AHCI_DEVICES];
extern struct parttab_s  *ahci_disk_part[MAX_AHCI_DEVICES];

/* Stub for disk registration */
static inline int ahci_register_dev(void *dev, struct parttab_s *part, int n) {
    (void)dev; (void)part; (void)n;
    return 0;
}

static inline void __ahci_remove_dev(dev_t dev_id, int remove_parent, int force) {
    (void)dev_id; (void)remove_parent; (void)force;
}

#endif /* KERNEL_ATA_H */
