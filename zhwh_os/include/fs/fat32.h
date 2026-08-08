/**
 * @file fat32.h
 * @brief FAT32 filesystem driver for VFS integration
 *
 * Reads FAT32-formatted USB mass storage devices.
 * Provides VFS inode_ops and file_ops so that open()/read() work
 * through the standard VFS path.
 */
#ifndef FS_FAT32_H
#define FS_FAT32_H

#include <stdint.h>
#include "fs.h"

/* FAT32 cluster markers */
#define FAT32_CLUSTER_FREE  0x00000000
#define FAT32_CLUSTER_BAD   0x0FFFFFF7
#define FAT32_CLUSTER_EOC   0x0FFFFFF8  /* >= this is end-of-chain */

/* Directory entry attributes */
#define FAT32_ATTR_READONLY  0x01
#define FAT32_ATTR_HIDDEN    0x02
#define FAT32_ATTR_SYSTEM    0x04
#define FAT32_ATTR_VOLUMEID  0x08
#define FAT32_ATTR_DIRECTORY 0x10
#define FAT32_ATTR_ARCHIVE   0x20
#define FAT32_ATTR_LFN       0x0F  /* Long File Name entry */

/* Per-filesystem data */
typedef struct fat32_fs {
    void     *usb_dev;         /* pointer to struct ata_dev_s */
    uint32_t  part_lba;        /* partition start LBA */
    uint16_t  bytes_per_sec;
    uint8_t   sec_per_cluster;
    uint32_t  reserved_sec;
    uint8_t   num_fats;
    uint32_t  sectors_per_fat;
    uint32_t  root_cluster;    /* first cluster of root directory (typically 2) */
    uint32_t  fat_start_lba;   /* = reserved_sec */
    uint32_t  data_start_lba;  /* = reserved_sec + num_fats * sectors_per_fat */
    uint32_t  cluster_size;    /* = bytes_per_sec * sec_per_cluster */
    uint32_t  next_free_cluster;/* hint for sequential free-cluster search */
} fat32_fs_t;

/* Per-inode data (stored in inode_t->i_private) */
typedef struct fat32_inode {
    fat32_fs_t *fs;
    uint32_t    first_cluster;  /* 0 = root directory */
    uint32_t    file_size;
    uint8_t     is_dir;
    uint32_t    cur_cluster;    /* current cluster position for sequential reads */
    uint32_t    cur_cluster_offset; /* index of cur_cluster in the file chain */
    uint8_t    *cluster_buf;    /* cached cluster data */
} fat32_inode_t;

/* ---- Public API ---- */

/**
 * @brief Mount a FAT32 filesystem on a block device
 * @param usb_dev   Pointer to USB MSD device (struct ata_dev_s *)
 * @param part_lba  Partition start LBA
 * @param out_fs    Output: new filesystem handle
 * @return 0 on success, negative on error
 */
int fat32_mount(void *usb_dev, uint32_t part_lba, fat32_fs_t **out_fs);

/**
 * @brief Create a VFS inode for the FAT32 root directory
 *
 * The returned inode uses FAT32 inode_ops/file_ops so that VFS path
 * resolution and file I/O work through the standard interfaces.
 *
 * @param fs        Mounted FAT32 filesystem
 * @param out_inode Output: new VFS inode
 * @return 0 on success, negative on error
 */
int fat32_create_root_inode(fat32_fs_t *fs, inode_t **out_inode);

/**
 * @brief FAT32 directory lookup — VFS inode_ops.lookup callback
 */
int fat32_lookup(inode_t *dir, const char *name, struct dentry **result);

/**
 * @brief FAT32 file read — VFS file_ops.read callback
 */
int fat32_read(struct file *file, char *buffer, uint32_t size);

/**
 * @brief FAT32 file open — VFS file_ops.open callback
 */
int fat32_open(inode_t *inode, struct file *file);

/**
 * @brief FAT32 file close — VFS file_ops.close callback
 */
int fat32_close(struct file *file);

/* Overwrite an existing preallocated FAT32 file without changing its chain. */
int fat32_overwrite_file(const char *path, const char *data, uint32_t len);
int fat32_ensure_file(const char *path, uint32_t size);
int fat32_save_log(void);
void fat32_clear_io_error(void);

/* Operation tables shared with VFS */
extern inode_operations_t fat32_inode_ops;
extern file_operations_t fat32_file_ops;
extern file_operations_t fat32_dir_file_ops;

#endif /* FS_FAT32_H */
