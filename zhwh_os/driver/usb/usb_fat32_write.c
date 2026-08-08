/**
 * @file usb_fat32_write.c
 * @brief FAT32 filesystem auto-mount for USB Mass Storage devices
 *
 * After a USB mass storage device is detected and initialized
 * (via init_msd in usb_msd.c), this module attempts to mount
 * a FAT32 filesystem and register it with the VFS at /.
 */
#include <string.h>
#include <kernel/hillsonos.h>
#include <kernel/usb.h>
#include <kernel/ata.h>
#include <mm/kheap.h>

#include "fat32.h"
#include "fs.h"

extern int snprintf(char *str, unsigned int size, const char *fmt, ...);

/* usb_msd_read 鈥?block-level read from USB MSD device */
extern long usb_msd_read(struct ata_dev_s *dev, size_t lba,
                         int sectors, uintptr_t buf);
extern void kernel_usb_status_line(const char *stage, const char *detail);
extern void kernel_usb_msc_status_line(const char *stage, const char *detail);
extern void kernel_usb_error_line(const char *stage, const char *detail);
extern void fat32_clear_io_error(void);


int fat32_save_log(void);

/**
 * Parse MBR partition table and extract the first partition's LBA.
 * Returns partition start LBA, or 0 if no valid partition found.
 */
static uint32_t mbr_get_first_part_lba(uint8_t *mbr)
{
    if (mbr[510] != 0x55 || mbr[511] != 0xAA)
        return 0;

    /* Partition table starts at offset 446, each entry is 16 bytes */
    uint8_t *ent = mbr + 446;

    for (int i = 0; i < 4; i++, ent += 16) {
        uint8_t type = ent[4];
        if (type == 0x00 || type == 0x05 || type == 0x0F)
            continue;  /* empty or extended partition */

        uint32_t lba = ent[8]  | ((uint32_t)ent[9]  << 8) |
                       ((uint32_t)ent[10] << 16) | ((uint32_t)ent[11] << 24);
        uint32_t sz  = ent[12] | ((uint32_t)ent[13] << 8) |
                       ((uint32_t)ent[14] << 16) | ((uint32_t)ent[15] << 24);

        if (lba > 0 && sz > 0) {
            usb_printk("fat32: MBR partition %d type=0x%02x lba=%u size=%u\n",
                       i + 1, type, lba, sz);
            return lba;
        }
    }
    return 0;
}

/**
 * Try to mount FAT32 on a given USB device at a specific LBA.
 * On success, registers the mount at /usb in the VFS.
 */
static int try_fat32_mount_at(void *dev, uint32_t part_lba)
{
    fat32_fs_t *fs = NULL;

    kernel_usb_msc_status_line("fat32", "try mount LBA");
    if (fat32_mount(dev, part_lba, &fs) < 0) {
        kernel_usb_error_line("fat32", "fat32_mount failed");
        return -1;
    }

    inode_t *root = NULL;
    if (fat32_create_root_inode(fs, &root) < 0) {
        kfree(fs);
        return -1;
    }

    if (vfs_mount("/usb", root) < 0) {
        kernel_usb_error_line("fat32", "vfs_mount /usb failed");
        kfree(root->i_private);
        kfree(root);
        kfree(fs);
        return -1;
    }

    usb_printk("fat32: mounted /usb from USB disk at part_lba=%u\n", part_lba);
    kernel_usb_msc_status_line("fat32", "mounted /usb");
    return 0;
}

/**
 * Try to mount a specific USB MSD device as FAT32.
 *
 * The AHCI registration helpers in this tree are compatibility stubs, so USB
 * disks are not guaranteed to appear in ahci_disk_dev[].  MSD init has the
 * freshly-created ata_dev_s already; mount that device directly.
 */
int fat32_try_mount_dev(struct ata_dev_s *dev)
{
    if (!dev || !dev->priv || dev->bytes_per_sector == 0) {
        usb_printk("fat32: invalid USB MSD device for mount\n");
        kernel_usb_error_line("fat32", "invalid USB MSD device");
        return -1;
    }

    kernel_usb_msc_status_line("fat32", "probing USB disk");
    usb_printk("fat32: trying USB MSD device directly (size=%u MB, bps=%u)\n",
               (uint32_t)(dev->size / (1024 * 1024)),
               dev->bytes_per_sector);

    /* Prefer a real MBR partition.  Probing LBA0 first on partitioned USB
     * sticks produces an expected "bad sector size" from the MBR bytes and
     * pollutes the sticky LAST USB ERROR line. */
    {
        uint8_t mbr[512];
        long ret = usb_msd_read(dev, 0, 1, (uintptr_t)mbr);
        if (ret >= 0) {
            uint32_t part_lba = mbr_get_first_part_lba(mbr);
            if (part_lba > 0) {
                kernel_usb_msc_status_line("fat32", "MBR partition found");
                if (try_fat32_mount_at(dev, part_lba) == 0)
                    return 0;
                kernel_usb_error_line("fat32", "partition mount failed");
                return -1;
            }
            usb_printk("fat32: no usable MBR partition found; trying LBA0\n");
            kernel_usb_msc_status_line("fat32", "no MBR, trying LBA0");
        } else {
            usb_printk("fat32: failed to read MBR for partition probe\n");
            kernel_usb_error_line("fat32", "MBR read failed");
            return -1;
        }
    }

    /* Superfloppy fallback: FAT32 BPB directly at LBA0. */
    if (try_fat32_mount_at(dev, 0) == 0)
        return 0;

    usb_printk("fat32: no FAT32 filesystem found on this USB device\n");
    kernel_usb_error_line("fat32", "no FAT32 filesystem");
    return -1;
}
/**
 * Called after USB mass storage initialization succeeds (from init_msd).
 *
 * Iterates over registered AHCI disk devices and tries to mount FAT32:
 *   1. Try LBA 0 (superfloppy / no partition table)
 *   2. Read MBR, find first partition, try that LBA
 */
void fat32_try_mount_all(void)
{
    for (int i = 0; i < MAX_AHCI_DEVICES; i++) {
        if (ahci_disk_dev[i] == NULL ||
            ahci_disk_dev[i]->priv == NULL ||
            ahci_disk_dev[i]->bytes_per_sector == 0)
            continue;

        struct ata_dev_s *dev = ahci_disk_dev[i];

        usb_printk("fat32: trying device %d (size=%u MB, bps=%u)\n",
                   i, (uint32_t)(dev->size / (1024 * 1024)),
                   dev->bytes_per_sector);

        /* 1. Try LBA 0 (superfloppy / no partition table) */
        if (try_fat32_mount_at(dev, 0) == 0) return;

        /* 2. Read MBR, find first partition */
        uint8_t mbr[512];
        long ret = usb_msd_read(dev, 0, 1, (uintptr_t)mbr);
        if (ret >= 0) {
            uint32_t part_lba = mbr_get_first_part_lba(mbr);
            if (part_lba > 0) {
                if (try_fat32_mount_at(dev, part_lba) == 0) return;
            }
        }
    }

    usb_printk("fat32: no FAT32 filesystem found on any USB device\n");
}

/* Save USB log to file on FAT32 USB disk. */
int fat32_save_log(void)
{
    extern void usb_log_snapshot(void);
    extern void *usb_log_get_buf(void);
    extern int usb_log_get_size(void);
    char *snapshot = NULL;
    fat32_clear_io_error();

    /*
     * Freeze the current log first, then copy it to private storage.  The
     * USB write path may take a long time and must not read directly from
     * the live log ring while other code is still appending messages.
     */
    usb_log_snapshot();
    char *buf = (char *)usb_log_get_buf();
    int len = usb_log_get_size();
    if (!buf || len < 0) {
        kernel_usb_error_line("fat32 save", "log buffer invalid");
        return -1;
    }
    if (len == 0) {
        usb_printk("fat32_save_log: empty buffer, nothing to write\n");
        kernel_usb_msc_status_line("fat32 save", "empty log");
        return 0;
    }

    snapshot = (char *)kmalloc((size_t)len + 1);
    if (!snapshot) {
        kernel_usb_error_line("fat32 save", "snapshot alloc failed");
        kernel_usb_msc_status_line("fat32 save", "failed");
        return -1;
    }
    memcpy(snapshot, buf, (size_t)len);
    snapshot[len] = '\0';

    {
        char msg[64];
        snprintf(msg, sizeof(msg), "len=%d", len);
        kernel_usb_msc_status_line("fat32 save", msg);

        /* Avoid repeated failed WRITE attempts: create/extend once, then
         * perform exactly one overwrite. */
        int ret = fat32_ensure_file("/usb/USB_AI.LOG", (uint32_t)len);
        if (ret < 0) {
            kernel_usb_error_line("fat32 save", "ensure failed");
            usb_printk("fat32_save_log: ensure failed (%d bytes)\n", len);
            kernel_usb_msc_status_line("fat32 save", "failed");
            kfree(snapshot);
            return ret;
        }

        kernel_usb_msc_status_line("fat32 save", "ensure ok write");
        ret = fat32_overwrite_file("/usb/USB_AI.LOG", (const char *)snapshot,
                                   (uint32_t)len);
        if (ret < 0)
            kernel_usb_error_line("fat32 save", "overwrite failed");

        usb_printk("fat32_save_log: %s (%d bytes)\n",
                   ret == 0 ? "saved" : "failed", len);
        kernel_usb_msc_status_line("fat32 save", ret == 0 ? "saved" : "failed");
        kfree(snapshot);
        return ret;
    }
}





