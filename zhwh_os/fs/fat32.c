/**
 * @file fat32.c
 * @brief FAT32 filesystem driver 鈥?VFS integration
 *
 * Reads files from FAT32-formatted USB mass storage devices.
 * Implements VFS inode_ops (lookup) and file_ops (open/close/read).
 *
 * This driver is read-only. Write support is a stub.
 *
 * References:
 *   Microsoft FAT Specification (FAT32)
 *   https://www.win.tue.nl/~aeb/linux/fs/fat/fat-1.html
 */
#include "fat32.h"
#include "fs.h"
#include "mm.h"
#include "kmalloc.h"
#include "printf.h"
#include "string.h"
#include <kernel/ata.h>    /* struct ata_dev_s */

/* Block-read callback: implemented in driver/usb/usb_msd.c */
extern long usb_msd_read(struct ata_dev_s *dev, size_t lba, int sectors,
                         uintptr_t buf);
extern long usb_msd_write(struct ata_dev_s *dev, size_t lba, int sectors,
                          uintptr_t buf);
extern int usb_printk(const char *fmt, ...);
extern void kernel_usb_msc_status_line(const char *stage, const char *detail);
extern void kernel_usb_error_line(const char *stage, const char *detail);
static fat32_fs_t *g_fat32_fs;
static int g_fat32_write_failed;
static int g_fat32_read_failed;

void fat32_clear_write_error(void)
{
    g_fat32_write_failed = 0;
}

void fat32_clear_io_error(void)
{
    g_fat32_write_failed = 0;
    g_fat32_read_failed = 0;
}

/* ================================================================
 * Internal helpers
 * ================================================================ */

static const char *fat32_basename(const char *path);

/** Read a single sector from the FAT32 block device */
static int fat32_read_sector(fat32_fs_t *fs, uint32_t lba, uint8_t *buf)
{
    long ret = usb_msd_read((struct ata_dev_s *)fs->usb_dev,
                            fs->part_lba + lba, 1, (uintptr_t)buf);
    if (ret < 0) {
        usb_printk("[fat32] sector read failed at lba=%u\n", lba);
        g_fat32_read_failed = 1;
        return -1;
    }
    return 0;
}
static int fat32_write_sector(fat32_fs_t *fs, uint32_t lba, const uint8_t *buf)
{
    long ret = usb_msd_write((struct ata_dev_s *)fs->usb_dev,
                             fs->part_lba + lba, 1, (uintptr_t)buf);
    if (ret < 0) {
        char msg[64];
        usb_printk("[fat32] sector write failed at lba=%u\n", lba);
        snprintf(msg, sizeof(msg), "lba=%u", lba);
        kernel_usb_error_line("fat32 write fail", msg);
        g_fat32_write_failed = 1;
        return -1;
    }
    return 0;
}

static int fat32_write_cluster(fat32_fs_t *fs, uint32_t first_sector,
                               const uint8_t *buf)
{
    for (uint32_t i = 0; i < fs->sec_per_cluster; i++) {
        if (fat32_write_sector(fs, first_sector + i,
                               buf + i * fs->bytes_per_sec) < 0)
            return -1;
    }
    return 0;
}

/** Read an entire cluster into a buffer (caller must free) */
static uint8_t *fat32_read_cluster(fat32_fs_t *fs, uint32_t cluster)
{
    uint32_t sz = fs->cluster_size;
    uint8_t *buf = (uint8_t *)kmalloc(sz);
    if (!buf) return NULL;

    uint32_t first_sector = fs->data_start_lba +
                            (cluster - 2) * fs->sec_per_cluster;

    /* Hardware xHCI/MSD path is more reliable with one sector per transfer.
     * A cluster may be 4K/8K/32K; reading it as one large transfer can fail
     * on some real USB sticks while single-sector BPB reads still succeed. */
    for (uint32_t i = 0; i < fs->sec_per_cluster; i++) {
        if (fat32_read_sector(fs, first_sector + i,
                              buf + i * fs->bytes_per_sec) < 0) {
            kfree(buf);
            return NULL;
        }
    }
    return buf;
}

static int fat32_read_fat_entry(fat32_fs_t *fs, uint32_t cluster,
                                uint32_t *out_value)
{
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fs->fat_start_lba +
                          fat_offset / fs->bytes_per_sec;
    uint32_t ent_offset = fat_offset % fs->bytes_per_sec;
    uint8_t sec_buf[512];

    if (fat32_read_sector(fs, fat_sector, sec_buf) < 0)
        return -1;

    if (ent_offset > fs->bytes_per_sec - 4)
        return -1;

    *out_value = sec_buf[ent_offset] |
                 ((uint32_t)sec_buf[ent_offset + 1] << 8) |
                 ((uint32_t)sec_buf[ent_offset + 2] << 16) |
                 ((uint32_t)sec_buf[ent_offset + 3] << 24);
    return 0;
}

/** Get next cluster number from the FAT table */
static uint32_t fat32_next_cluster(fat32_fs_t *fs, uint32_t cluster)
{
    uint32_t entry = 0;
    if (fat32_read_fat_entry(fs, cluster, &entry) < 0)
        return FAT32_CLUSTER_EOC;
    return entry & 0x0FFFFFFF;
}

static int fat32_set_cluster(fat32_fs_t *fs, uint32_t cluster, uint32_t value)
{
    uint32_t off = cluster * 4;
    uint32_t sec = fs->fat_start_lba + off / fs->bytes_per_sec;
    uint32_t pos = off % fs->bytes_per_sec;
    uint8_t b[512];
    uint32_t old_entry = 0;
    uint32_t entry;
    uint8_t e[4];

    if (fat32_read_fat_entry(fs, cluster, &old_entry) < 0)
        return -1;
    entry = (old_entry & 0xF0000000) | (value & 0x0FFFFFFF);
    e[0] = (uint8_t)entry;
    e[1] = (uint8_t)(entry >> 8);
    e[2] = (uint8_t)(entry >> 16);
    e[3] = (uint8_t)(entry >> 24);

    if (fat32_read_sector(fs, sec, b) < 0) return -1;
    if (pos > fs->bytes_per_sec - 4)
        return -1;
    for (uint32_t i = 0; i < 4; i++)
        b[pos + i] = e[i];

    for (uint32_t n = 0; n < fs->num_fats; n++) {
        uint32_t lba = fs->reserved_sec + n * fs->sectors_per_fat +
                       off / fs->bytes_per_sec;
        if (usb_msd_write((struct ata_dev_s *)fs->usb_dev,
                          fs->part_lba + lba, 1, (uintptr_t)b) < 0)
            return -1;
    }
    return 0;
}

static int fat32_find_free_cluster(fat32_fs_t *fs, uint32_t *out)
{
    uint32_t max = fs->sectors_per_fat * fs->bytes_per_sec / 4;
    uint32_t start = fs->next_free_cluster;
    if (start < 2 || start >= max) start = 2;

    /* Read one FAT sector per iteration, not one USB transaction per entry. */
    uint8_t sec_buf[512];
    uint32_t sectors = fs->sectors_per_fat;
    for (uint32_t pass = 0; pass < 2; pass++) {
        uint32_t first = pass ? 2 : start;
        uint32_t last = pass ? start : max;
        uint32_t first_sec = (first * 4) / fs->bytes_per_sec;
        uint32_t last_sec = ((last * 4) + fs->bytes_per_sec - 1) /
                            fs->bytes_per_sec;
        if (last_sec > sectors) last_sec = sectors;
        for (uint32_t s = first_sec; s < last_sec; s++) {
            if (fat32_read_sector(fs, fs->fat_start_lba + s, sec_buf) < 0)
                return -1;
            uint32_t begin = (s * fs->bytes_per_sec) / 4;
            uint32_t end = begin + fs->bytes_per_sec / 4;
            if (begin < first) begin = first;
            if (end > last) end = last;
            for (uint32_t c = begin; c < end && c < max; c++) {
                uint32_t pos = (c * 4) % fs->bytes_per_sec;
                uint32_t entry = sec_buf[pos] |
                    ((uint32_t)sec_buf[pos + 1] << 8) |
                    ((uint32_t)sec_buf[pos + 2] << 16) |
                    ((uint32_t)sec_buf[pos + 3] << 24);
                if ((entry & 0x0FFFFFFF) == FAT32_CLUSTER_FREE) {
                    *out = c;
                    fs->next_free_cluster = (c + 1 < max) ? c + 1 : 2;
                    return 0;
                }
            }
        }
    }
    return -1;
}

static int fat32_create_root_file(fat32_fs_t *fs, const char *name,
                                  uint32_t size, uint32_t *first_out)
{
    kernel_usb_msc_status_line("fat32 create", "start");
    const char *base = fat32_basename(name);
    uint32_t need = (size + fs->cluster_size - 1) / fs->cluster_size;
    if (!need) need = 1;
    uint32_t first = 0, prev = 0;
    for (uint32_t i = 0; i < need; i++) {
        uint32_t c;
        if (fat32_find_free_cluster(fs, &c) < 0) return -1;
        if (!first) first = c;
        if (prev && fat32_set_cluster(fs, prev, c) < 0) return -1;
        prev = c;
        if (fat32_set_cluster(fs, c, 0x0FFFFFFF) < 0) return -1;
    }

    uint8_t *cl = fat32_read_cluster(fs, fs->root_cluster);
    if (!cl) return -1;
    int slot = -1;
    int nent = fs->cluster_size / 32;
    for (int i = 0; i < nent; i++) {
        if (cl[i * 32] == 0x00 || cl[i * 32] == 0xE5) { slot = i; break; }
    }
    if (slot < 0) { kfree(cl); return -1; }
    uint8_t *e = cl + slot * 32;
    memset(e, 0, 32);
    char shortname[11];
    memset(shortname, ' ', sizeof(shortname));
    const char *dot = NULL;
    for (const char *p = base; p && *p; p++) if (*p == '.') dot = p;
    int nb = dot ? (int)(dot - base) : (int)strlen(base);
    int ne = dot ? (int)strlen(dot + 1) : 0;
    if (nb > 8) nb = 8; if (ne > 3) ne = 3;
    for (int i = 0; i < nb; i++) shortname[i] = (base[i] >= 'a' && base[i] <= 'z') ? base[i] - 32 : base[i];
    for (int i = 0; i < ne; i++) { char c = dot[1+i]; shortname[8+i] = (c >= 'a' && c <= 'z') ? c - 32 : c; }
    memcpy(e, shortname, 11);
    e[11] = FAT32_ATTR_ARCHIVE;
    e[26] = (uint8_t)first; e[27] = (uint8_t)(first >> 8);
    e[20] = (uint8_t)(first >> 16); e[21] = (uint8_t)(first >> 24);
    e[28] = (uint8_t)size; e[29] = (uint8_t)(size >> 8);
    e[30] = (uint8_t)(size >> 16); e[31] = (uint8_t)(size >> 24);
    uint32_t first_sec = fs->data_start_lba +
                         (fs->root_cluster - 2) * fs->sec_per_cluster;
    int ret = fat32_write_cluster(fs, first_sec, cl);
    kfree(cl);
    if (ret < 0) {
        kernel_usb_error_line("fat32 create", "root entry write failed");
        g_fat32_write_failed = 1;
        return -1;
    }
    kernel_usb_msc_status_line("fat32 create", "ok");
    *first_out = first;
    return 0;
}

static int fat32_update_root_size(fat32_fs_t *fs, const char *name, uint32_t size)
{
    uint8_t *cl = fat32_read_cluster(fs, fs->root_cluster); if (!cl) return -1;
    const char *base = fat32_basename(name);
    char target[11]; memset(target, ' ', 11); const char *dot = NULL;
    for (const char *p=base; p && *p; p++) if (*p=='.') dot=p;
    int nb=dot?(int)(dot-base):(int)strlen(base), ne=dot?(int)strlen(dot+1):0; if(nb>8)nb=8;if(ne>3)ne=3;
    for(int i=0;i<nb;i++){char c=base[i];target[i]=(c>='a'&&c<='z')?c-32:c;}
    for(int i=0;i<ne;i++){char c=dot[1+i];target[8+i]=(c>='a'&&c<='z')?c-32:c;}
    int found=-1; for(int i=0;i<(int)(fs->cluster_size/32);i++) if(memcmp(cl+i*32,target,11)==0){found=i;break;}
    if(found<0){kfree(cl);return -1;} uint8_t *e=cl+found*32; e[28]=size;e[29]=size>>8;e[30]=size>>16;e[31]=size>>24;
    uint32_t lba=fs->data_start_lba+(fs->root_cluster-2)*fs->sec_per_cluster;
    int r = fat32_write_cluster(fs, lba, cl); kfree(cl); return r < 0 ? -1 : 0;
}

static int fat32_update_root_entry(fat32_fs_t *fs, const char *name,
                                   uint32_t first_cluster, uint32_t size)
{
    uint8_t *cl = fat32_read_cluster(fs, fs->root_cluster);
    if (!cl) return -1;

    const char *base = fat32_basename(name);
    char target[11];
    const char *dot = NULL;
    memset(target, ' ', 11);
    for (const char *p = base; p && *p; p++) if (*p == '.') dot = p;
    int nb = dot ? (int)(dot - base) : (int)strlen(base);
    int ne = dot ? (int)strlen(dot + 1) : 0;
    if (nb > 8) nb = 8;
    if (ne > 3) ne = 3;
    for (int i = 0; i < nb; i++) {
        char c = base[i];
        target[i] = (c >= 'a' && c <= 'z') ? c - 32 : c;
    }
    for (int i = 0; i < ne; i++) {
        char c = dot[1 + i];
        target[8 + i] = (c >= 'a' && c <= 'z') ? c - 32 : c;
    }

    int found = -1;
    for (int i = 0; i < (int)(fs->cluster_size / 32); i++) {
        if (memcmp(cl + i * 32, target, 11) == 0) {
            found = i;
            break;
        }
    }
    if (found < 0) {
        kfree(cl);
        return -1;
    }

    uint8_t *e = cl + found * 32;
    e[26] = (uint8_t)first_cluster;
    e[27] = (uint8_t)(first_cluster >> 8);
    e[20] = (uint8_t)(first_cluster >> 16);
    e[21] = (uint8_t)(first_cluster >> 24);
    e[28] = (uint8_t)size;
    e[29] = (uint8_t)(size >> 8);
    e[30] = (uint8_t)(size >> 16);
    e[31] = (uint8_t)(size >> 24);

    uint32_t lba = fs->data_start_lba +
                   (fs->root_cluster - 2) * fs->sec_per_cluster;
    int r = fat32_write_cluster(fs, lba, cl);
    kfree(cl);
    return r < 0 ? -1 : 0;
}

static const char *fat32_basename(const char *path)
{
    const char *name = path;
    for (const char *p = path; p && *p; p++) {
        if (*p == '/')
            name = p + 1;
    }
    return name;
}

static int fat32_chain_tail(fat32_fs_t *fs, uint32_t start_cluster,
                            uint32_t *out_tail, uint32_t *out_count)
{
    uint32_t max = fs->sectors_per_fat * fs->bytes_per_sec / 4;
    uint32_t cluster = start_cluster;
    uint32_t tail = 0;
    uint32_t count = 0;

    while (cluster >= 2 && cluster < FAT32_CLUSTER_EOC && count < max) {
        tail = cluster;
        count++;
        cluster = fat32_next_cluster(fs, cluster);
    }

    if (count == 0 || count >= max)
        return -1;

    if (out_tail) *out_tail = tail;
    if (out_count) *out_count = count;
    return 0;
}

static int fat32_extend_root_file(fat32_fs_t *fs, const char *name,
                                  uint32_t new_size)
{
    file_t *file = filp_open(name, 0);
    if (!file || !file->f_inode)
        return -1;

    fat32_inode_t *fi = (fat32_inode_t *)file->f_inode->i_private;
    if (!fi || !fi->fs || fi->is_dir) {
        filp_close(file);
        return -1;
    }

    if (new_size <= fi->file_size) {
        filp_close(file);
        return 0;
    }

    uint32_t need_clusters = (new_size + fs->cluster_size - 1) /
                             fs->cluster_size;
    if (need_clusters < 1)
        need_clusters = 1;

    if (fi->first_cluster < 2) {
        uint32_t first_cluster = 0;
        if (fat32_find_free_cluster(fs, &first_cluster) < 0) {
            filp_close(file);
            return -1;
        }
        if (fat32_set_cluster(fs, first_cluster, 0x0FFFFFFF) < 0) {
            filp_close(file);
            return -1;
        }
        fi->first_cluster = first_cluster;
        if (fat32_update_root_entry(fs, name, first_cluster, new_size) < 0) {
            filp_close(file);
            return -1;
        }
        fi->file_size = new_size;
        file->f_inode->i_size = new_size;
        filp_close(file);
        return 0;
    }

    uint32_t current_tail = 0;
    uint32_t current_clusters = 0;
    if (fat32_chain_tail(fs, fi->first_cluster, &current_tail,
                         &current_clusters) < 0) {
        filp_close(file);
        return -1;
    }

    uint32_t prev = current_tail;
    for (uint32_t i = current_clusters; i < need_clusters; i++) {
        uint32_t c;
        if (fat32_find_free_cluster(fs, &c) < 0) {
            filp_close(file);
            return -1;
        }
        if (fat32_set_cluster(fs, prev, c) < 0 ||
            fat32_set_cluster(fs, c, 0x0FFFFFFF) < 0) {
            filp_close(file);
            return -1;
        }
        prev = c;
    }

    fi->file_size = new_size;
    file->f_inode->i_size = new_size;
    if (fat32_update_root_size(fs, name, new_size) < 0) {
        filp_close(file);
        return -1;
    }

    filp_close(file);
    return 0;
}

/**
 * Read the nth cluster in a cluster chain (0-indexed).
 * Returns cluster number, or 0 on error / end of chain.
 */
static uint32_t fat32_cluster_nth(fat32_fs_t *fs, uint32_t start_cluster,
                                  uint32_t n)
{
    uint32_t cl = start_cluster;
    for (uint32_t i = 0; i < n; i++) {
        if (cl < 2 || cl >= FAT32_CLUSTER_EOC)
            return 0;
        cl = fat32_next_cluster(fs, cl);
    }
    if (cl < 2 || cl >= FAT32_CLUSTER_EOC)
        return 0;
    return cl;
}

/** Trim trailing spaces from an 8.3 name field (in-place) */
static void trim_spaces(char *s, int len)
{
    while (len > 0 && s[len - 1] == ' ') {
        s[len - 1] = '\0';
        len--;
    }
}

/** Convert a UCS-2 (UTF-16LE) character to ASCII (best-effort) */
static char ucs2_to_ascii(uint16_t wch)
{
    if (wch < 0x80) return (char)wch;
    if (wch == 0xF00F) return 0;  /* padding marker */
    return '?';
}

/**
 * Read an 8.3 entry's short name into a buffer.
 * Returns the number of characters (not including null terminator).
 */
static int read_short_name(const uint8_t *entry, char *out, int max)
{
    int p = 0;
    /* 8-char name */
    for (int i = 0; i < 8 && p < max - 1; i++) {
        char c = entry[i];
        if (c == ' ') break;
        if (c >= 0x20 && c < 0x7F) out[p++] = c;
        else if (c == 0x05) out[p++] = 0xE5;  /* KANJI lead byte 鈫?0xE5 */
    }

    /* Extension */
    if (entry[8] != ' ') {
        if (p < max - 1) out[p++] = '.';
        for (int i = 8; i < 11 && p < max - 1; i++) {
            char c = entry[i];
            if (c == ' ') break;
            if (c >= 0x20 && c < 0x7F) out[p++] = c;
        }
    }
    out[p] = '\0';
    return p;
}

/* ================================================================
 * Directory entry search
 * ================================================================ */

static char fat32_upper(char c)
{
    if (c >= 'a' && c <= 'z')
        return c - ('a' - 'A');
    return c;
}

static int fat32_name_eq(const char *a, const char *b)
{
    while (*a && *b) {
        if (fat32_upper(*a) != fat32_upper(*b))
            return 0;
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

/**
 * Search a directory (starting at dir_cluster) for a file named `name`.
 * Supports both 8.3 short names and Long File Names (LFN).
 *
 * On success, fills in:
 *   *out_cluster 鈥?first cluster of the found entry
 *   *out_size    鈥?file size in bytes
 *   *out_is_dir  鈥?1 if directory, 0 if file
 *
 * Returns 0 on success, -1 if not found, -2 on I/O error.
 */
static int fat32_find_entry(fat32_fs_t *fs, uint32_t dir_cluster,
                            const char *name,
                            uint32_t *out_cluster, uint32_t *out_size,
                            uint8_t *out_is_dir)
{
    char lfn_buf[256];
    int  lfn_len = 0;
    uint32_t cluster = (dir_cluster == 0) ? fs->root_cluster : dir_cluster;

    while (cluster >= 2 && cluster < FAT32_CLUSTER_EOC) {
        uint8_t *cl_buf = fat32_read_cluster(fs, cluster);
        if (!cl_buf) return -2;

        int entries_per_cluster = fs->cluster_size / 32;

        for (int i = 0; i < entries_per_cluster; i++) {
            uint8_t *ent = cl_buf + i * 32;

            /* Free entry: stop searching */
            if (ent[0] == 0x00) {
                kfree(cl_buf);
                return -1;
            }
            /* Deleted entry: skip */
            if (ent[0] == 0xE5) continue;

            uint8_t attr = ent[11];

            /* LFN entry */
            if (attr == FAT32_ATTR_LFN) {
                uint8_t seq = ent[0];
                uint8_t slot = (seq & 0x3F) - 1;  /* 0-indexed */

                if (seq & 0x40) {
                    /* First LFN entry (last in sequence): reset buffer */
                    lfn_len = 0;
                    lfn_buf[0] = '\0';
                }

                /* Collect 13 UCS-2 chars from this LFN entry */
                char lfn_part[14];
                int pp = 0;
                /* chars at offsets 1,3,5,7,9 (5 chars, 2 bytes each) */
                for (int j = 0; j < 5; j++) {
                    uint16_t wch = ent[1 + j*2] | ((uint16_t)ent[1 + j*2 + 1] << 8);
                    char ascii = ucs2_to_ascii(wch);
                    if (ascii) lfn_part[pp++] = ascii;
                }
                /* chars at offsets 14,16,18,20,22,24 (6 chars) */
                for (int j = 0; j < 6; j++) {
                    uint16_t wch = ent[14 + j*2] | ((uint16_t)ent[14 + j*2 + 1] << 8);
                    char ascii = ucs2_to_ascii(wch);
                    if (ascii) lfn_part[pp++] = ascii;
                }
                /* chars at offsets 28,30 (2 chars) */
                for (int j = 0; j < 2; j++) {
                    uint16_t wch = ent[28 + j*2] | ((uint16_t)ent[28 + j*2 + 1] << 8);
                    char ascii = ucs2_to_ascii(wch);
                    if (ascii) lfn_part[pp++] = ascii;
                }
                lfn_part[pp] = '\0';

                /* Prepend (LFN slots come in reverse order) */
                if (strlen(lfn_buf) + pp < 250) {
                    char tmp[256];
                    strcpy(tmp, lfn_buf);
                    strcpy(lfn_buf, lfn_part);
                    strcat(lfn_buf, tmp);
                }
                lfn_len = strlen(lfn_buf);
                continue;
            }

            /* Regular 8.3 entry */
            if (attr & FAT32_ATTR_VOLUMEID) {
                lfn_len = 0;
                continue;
            }

            /* Compare name */
            int match = 0;

            /* Try LFN first */
            if (lfn_len > 0) {
                if (fat32_name_eq(lfn_buf, name))
                    match = 1;
            }

            /* Try 8.3 short name */
            if (!match) {
                char sname[13];
                read_short_name(ent, sname, sizeof(sname));
                if (fat32_name_eq(sname, name))
                    match = 1;
            }

            if (match) {
                uint32_t cluster_hi = ent[20] | ((uint32_t)ent[21] << 8);
                uint32_t cluster_lo = ent[26] | ((uint32_t)ent[27] << 8);
                *out_cluster = (cluster_hi << 16) | cluster_lo;
                *out_size    = ent[28] | ((uint32_t)ent[29] << 8) |
                               ((uint32_t)ent[30] << 16) |
                               ((uint32_t)ent[31] << 24);
                *out_is_dir  = (attr & FAT32_ATTR_DIRECTORY) ? 1 : 0;
                kfree(cl_buf);
                return 0;
            }

            lfn_len = 0;
        }

        kfree(cl_buf);
        cluster = fat32_next_cluster(fs, cluster);
    }

    return -1;  /* not found */
}

/* ================================================================
 * FAT32 inode operations (VFS callbacks)
 * ================================================================ */

int fat32_lookup(inode_t *dir, const char *name, struct dentry **result)
{
    fat32_inode_t *fi = (fat32_inode_t *)dir->i_private;
    if (!fi || !fi->fs) {
        usb_printk("[fat32] lookup: not a FAT32 inode\n");
        return -1;
    }

    usb_printk("[fat32] lookup: '%s' in cluster %u\n", name, fi->first_cluster);

    uint32_t first_cluster, file_size;
    uint8_t is_dir;

    int ret = fat32_find_entry(fi->fs, fi->first_cluster, name,
                               &first_cluster, &file_size, &is_dir);
    if (ret < 0)
        return -1;

    /* Allocate a new VFS inode for this entry */
    inode_t *inode = (inode_t *)kmalloc(sizeof(inode_t));
    if (!inode) return -1;
    memset(inode, 0, sizeof(inode_t));

    inode->i_ino = 0;  /* FAT32 doesn't have inode numbers */
    inode->i_mode = is_dir ? (S_IFDIR | 0555) : (S_IFREG | 0444);
    inode->i_size = file_size;
    inode->i_nlink = 1;
    inode->i_sb = dir->i_sb;
    inode->i_parent = dir;
    inode->i_op = &fat32_inode_ops;
    inode->i_fop = is_dir ? &fat32_dir_file_ops : &fat32_file_ops;

    /* Allocate FAT32 private data */
    fat32_inode_t *priv = (fat32_inode_t *)kmalloc(sizeof(fat32_inode_t));
    if (!priv) { kfree(inode); return -1; }
    priv->fs = fi->fs;
    priv->first_cluster = first_cluster;
    priv->file_size = file_size;
    priv->is_dir = is_dir;
    priv->cur_cluster = 0;
    priv->cur_cluster_offset = 0;
    priv->cluster_buf = NULL;
    inode->i_private = priv;

    /* Allocate dentry */
    dentry_t *dentry = (dentry_t *)kmalloc(sizeof(dentry_t));
    if (!dentry) { kfree(priv); kfree(inode); return -1; }
    memset(dentry, 0, sizeof(dentry_t));
    dentry->d_inode = inode;
    dentry->d_parent = NULL;
    dentry->d_name_len = strlen(name);
    dentry->d_name = (char *)kmalloc(dentry->d_name_len + 1);
    if (!dentry->d_name) { kfree(dentry); kfree(priv); kfree(inode); return -1; }
    strcpy(dentry->d_name, name);
    dentry->d_sb = dir->i_sb;
    llist_init_head(&dentry->d_hash);
    llist_init_head(&dentry->d_list);
    llist_init_head(&dentry->d_lru);

    *result = dentry;
    usb_printk("[fat32] lookup: found '%s' cluster=%u size=%u is_dir=%d\n",
           name, first_cluster, file_size, is_dir);
    return 0;
}

/* Stub: FAT32 create (read-only) */
static int fat32_create_stub(inode_t *dir, const char *name, int mode,
                              struct dentry **result)
{
    (void)dir; (void)name; (void)mode; (void)result;
    usb_printk("[fat32] create: read-only filesystem\n");
    return -1;
}

/* Stub: FAT32 mkdir (read-only) */
static int fat32_mkdir_stub(inode_t *dir, const char *name, int mode)
{
    (void)dir; (void)name; (void)mode;
    usb_printk("[fat32] mkdir: read-only filesystem\n");
    return -1;
}

/**
 * fat32_listdir 鈥?enumerate all entries in a FAT32 directory.
 *
 * Reads all clusters in the directory chain and outputs one line per entry.
 * Format: "name  size\\n" or "name/\\n" for subdirectories.
 */
static int fat32_listdir(inode_t *dir, char *buf, int max)
{
    fat32_inode_t *fi = (fat32_inode_t *)dir->i_private;
    if (!fi || !fi->fs) return -1;

    fat32_fs_t *fs = fi->fs;
    uint32_t cluster = fi->first_cluster;
    if (cluster == 0) cluster = fs->root_cluster;

    int pos = 0;
    char lfn_buf[256];
    int  lfn_len = 0;

    while (cluster >= 2 && cluster < FAT32_CLUSTER_EOC) {
        uint8_t *cl_buf = fat32_read_cluster(fs, cluster);
        if (!cl_buf) break;

        int entries_per_cluster = fs->cluster_size / 32;

        for (int i = 0; i < entries_per_cluster; i++) {
            uint8_t *ent = cl_buf + i * 32;

            if (ent[0] == 0x00) {
                kfree(cl_buf);
                goto done;
            }
            if (ent[0] == 0xE5) continue;

            uint8_t attr = ent[11];

            /* LFN entry */
            if (attr == FAT32_ATTR_LFN) {
                uint8_t seq = ent[0];
                if (seq & 0x40) { lfn_len = 0; lfn_buf[0] = '\0'; }

                char part[14]; int pp = 0;
                for (int j = 0; j < 5; j++) {
                    uint16_t w = ent[1+j*2] | ((uint16_t)ent[1+j*2+1] << 8);
                    char c = ucs2_to_ascii(w); if (c) part[pp++] = c;
                }
                for (int j = 0; j < 6; j++) {
                    uint16_t w = ent[14+j*2] | ((uint16_t)ent[14+j*2+1] << 8);
                    char c = ucs2_to_ascii(w); if (c) part[pp++] = c;
                }
                for (int j = 0; j < 2; j++) {
                    uint16_t w = ent[28+j*2] | ((uint16_t)ent[28+j*2+1] << 8);
                    char c = ucs2_to_ascii(w); if (c) part[pp++] = c;
                }
                part[pp] = '\0';

                if (strlen(lfn_buf) + pp < 250) {
                    char tmp[256];
                    strcpy(tmp, lfn_buf);
                    strcpy(lfn_buf, part);
                    strcat(lfn_buf, tmp);
                }
                lfn_len = strlen(lfn_buf);
                continue;
            }

            /* Skip volume label */
            if (attr & FAT32_ATTR_VOLUMEID) { lfn_len = 0; continue; }

            /* Get name: prefer LFN, fall back to 8.3 */
            char name[64];
            if (lfn_len > 0) {
                int j = 0;
                while (lfn_buf[j] && j < 62) { name[j] = lfn_buf[j]; j++; }
                name[j] = '\0';
            } else {
                read_short_name(ent, name, sizeof(name));
            }

            /* Skip "." and ".." */
            if (name[0] == '.' && (name[1] == '\0' ||
                (name[1] == '.' && name[2] == '\0'))) {
                lfn_len = 0;
                continue;
            }

            int is_dir = (attr & FAT32_ATTR_DIRECTORY) ? 1 : 0;
            uint32_t fsize = ent[28] | ((uint32_t)ent[29] << 8) |
                             ((uint32_t)ent[30] << 16) |
                             ((uint32_t)ent[31] << 24);

            /* Write entry */
            int nlen = strlen(name);
            if (pos + nlen + 25 >= max) {
                /* No room 鈥?stop but don't free buffer yet */
                kfree(cl_buf);
                goto done;
            }

            for (int j = 0; j < nlen; j++) buf[pos++] = name[j];

            if (is_dir) {
                const char *tag = "/\n";
                for (int j = 0; tag[j]; j++) buf[pos++] = tag[j];
            } else {
                buf[pos++] = ' ';
                buf[pos++] = ' ';
                /* Convert size to string */
                if (fsize == 0) {
                    buf[pos++] = '0';
                } else {
                    char tmp[12]; int tp = 0;
                    uint32_t sz = fsize;
                    while (sz > 0) { tmp[tp++] = '0' + (sz % 10); sz /= 10; }
                    while (tp > 0 && pos < max - 1) buf[pos++] = tmp[--tp];
                }
                buf[pos++] = '\n';
            }

            lfn_len = 0;
        }

        kfree(cl_buf);
        cluster = fat32_next_cluster(fs, cluster);
    }

done:
    if (pos >= max) pos = max - 1;
    buf[pos] = '\0';
    return pos;
}

/* ================================================================
 * FAT32 file operations (VFS callbacks)
 * ================================================================ */

int fat32_open(inode_t *inode, struct file *file)
{
    fat32_inode_t *fi = (fat32_inode_t *)inode->i_private;
    if (!fi) return -1;

    usb_printk("[fat32] open: cluster=%u size=%u\n",
           fi->first_cluster, fi->file_size);

    file->f_pos = 0;
    fi->cur_cluster = fi->first_cluster;
    fi->cur_cluster_offset = 0;

    /* Allocate a cluster-sized buffer for sequential read caching */
    if (fi->cluster_buf) kfree(fi->cluster_buf);
    fi->cluster_buf = NULL;

    return 0;
}

int fat32_close(struct file *file)
{
    fat32_inode_t *fi = (fat32_inode_t *)file->f_inode->i_private;
    if (fi && fi->cluster_buf) {
        kfree(fi->cluster_buf);
        fi->cluster_buf = NULL;
    }
    usb_printk("[fat32] close\n");
    return 0;
}

int fat32_overwrite_file(const char *path, const char *data, uint32_t len)
{
    if (g_fat32_write_failed) {
        kernel_usb_error_line("fat32 overwrite", "write disabled");
        return -1;
    }
    kernel_usb_msc_status_line("fat32 overwrite", "start");
    file_t *file = filp_open(path, 0);
    if (!file && g_fat32_fs && path && data) {
        uint32_t first;
        if (fat32_create_root_file(g_fat32_fs, path, len, &first) == 0)
            file = filp_open(path, 0);
    }
    if (!file || !file->f_inode || !data) return -1;
    fat32_inode_t *fi = (fat32_inode_t *)file->f_inode->i_private;
    if (!fi || !fi->fs || fi->is_dir) {
        filp_close(file);
        return -1;
    }

    if (len > fi->file_size) {
        filp_close(file);
        if (fat32_extend_root_file(g_fat32_fs, path, len) < 0)
            return -1;
        file = filp_open(path, 0);
        if (!file || !file->f_inode)
            return -1;
        fi = (fat32_inode_t *)file->f_inode->i_private;
        if (!fi || !fi->fs || fi->is_dir || len > fi->file_size) {
            filp_close(file);
            return -1;
        }
    }

    fat32_fs_t *fs = fi->fs;
    uint8_t *buf = (uint8_t *)kmalloc(fs->cluster_size);
    if (!buf) { filp_close(file); return -1; }

    uint32_t cluster = fi->first_cluster;
    uint32_t written = 0;
    uint32_t max_clusters = (fi->file_size + fs->cluster_size - 1) /
                            fs->cluster_size;
    if (!max_clusters) max_clusters = 1;
    uint32_t need_clusters = (len + fs->cluster_size - 1) /
                             fs->cluster_size;
    uint32_t visited = 0;
    if (!need_clusters) need_clusters = 1;

    while (cluster >= 2 && cluster < FAT32_CLUSTER_EOC &&
           written < len && visited < need_clusters &&
           visited < max_clusters) {
        uint32_t chunk = len - written;
        if (chunk > fs->cluster_size) chunk = fs->cluster_size;
        memset(buf, 0, fs->cluster_size);
        memcpy(buf, data + written, chunk);

        uint32_t first_sec = fs->data_start_lba +
                             (cluster - 2) * fs->sec_per_cluster;
        if (fat32_write_cluster(fs, first_sec, buf) < 0) {
            kernel_usb_error_line("fat32 overwrite", "data write failed");
            g_fat32_write_failed = 1;
            kfree(buf);
            filp_close(file);
            return -1;
        }
        written += chunk;
        visited++;
        if (written < len)
            cluster = fat32_next_cluster(fs, cluster);
    }

    kfree(buf);
    if (written < len) {
        filp_close(file);
        return -1;
    }
    if (fat32_update_root_size(fs, path, len) < 0) {
        kernel_usb_error_line("fat32 overwrite", "size update failed");
        g_fat32_write_failed = 1;
        filp_close(file);
        return -1;
    }
    filp_close(file);
    kernel_usb_msc_status_line("fat32 overwrite", "ok");
    return 0;
}

int fat32_ensure_file(const char *path, uint32_t size)
{
    if (g_fat32_write_failed) {
        kernel_usb_error_line("fat32 ensure", "write disabled");
        return -1;
    }
    kernel_usb_msc_status_line("fat32 ensure", "start");
    if (!g_fat32_fs || !path) {
        kernel_usb_error_line("fat32 ensure", "fs/path invalid");
        return -1;
    }
    file_t *f = filp_open(path, 0);
    if (f) {
        fat32_inode_t *fi=(fat32_inode_t*)f->f_inode->i_private;
        if (!fi || fi->is_dir) { filp_close(f); return -1; }
        if (size > fi->file_size) {
            filp_close(f);
            return fat32_extend_root_file(g_fat32_fs, path, size);
        }
        filp_close(f);
        return 0;
    }
    /*
     * filp_open() also fails when directory I/O fails. Do not mistake a
     * failed Bulk IN/CSW read for "file does not exist", otherwise the
     * caller immediately starts a second create/write request.
     */
    if (g_fat32_read_failed) {
        kernel_usb_error_line("fat32 ensure", "directory read failed");
        return -1;
    }
    uint32_t first;
    return fat32_create_root_file(g_fat32_fs, path, size, &first);
}

/**
 * fat32_read 鈥?read file data by following the FAT cluster chain.
 *
 * For each read call:
 *   1. Locate the cluster containing f_pos
 *   2. Read that cluster (and subsequent ones) from disk
 *   3. Copy the requested bytes to the user buffer
 *
 * This is not highly optimized (no large read-ahead), but it works
 * for sequential reads of model files.
 */
int fat32_read(struct file *file, char *buffer, uint32_t size)
{
    fat32_inode_t *fi = (fat32_inode_t *)file->f_inode->i_private;
    if (!fi || !fi->fs) return -1;

    fat32_fs_t *fs = fi->fs;
    uint32_t cluster_size = fs->cluster_size;

    /* Clamp to file size */
    uint64_t remaining = fi->file_size - file->f_pos;
    uint32_t to_read = (size < remaining) ? size : (uint32_t)remaining;

    if (to_read == 0)
        return 0;  /* EOF */

    uint32_t read_total = 0;
    uint8_t *cluster_buf = (uint8_t *)kmalloc(cluster_size);
    if (!cluster_buf) return -1;

    uint32_t wanted_idx = (uint32_t)file->f_pos / cluster_size;
    if (fi->cur_cluster < 2 || wanted_idx < fi->cur_cluster_offset) {
        fi->cur_cluster = fi->first_cluster;
        fi->cur_cluster_offset = 0;
    }
    while (fi->cur_cluster_offset < wanted_idx &&
           fi->cur_cluster >= 2 && fi->cur_cluster < FAT32_CLUSTER_EOC) {
        fi->cur_cluster = fat32_next_cluster(fs, fi->cur_cluster);
        fi->cur_cluster_offset++;
    }

    while (read_total < to_read) {
        uint32_t file_offset = (uint32_t)file->f_pos + read_total;
        uint32_t cluster_idx = file_offset / cluster_size;
        uint32_t offset_in_cluster = file_offset % cluster_size;
        uint32_t chunk = to_read - read_total;
        if (chunk > cluster_size - offset_in_cluster)
            chunk = cluster_size - offset_in_cluster;

        while (fi->cur_cluster_offset < cluster_idx &&
               fi->cur_cluster >= 2 && fi->cur_cluster < FAT32_CLUSTER_EOC) {
            fi->cur_cluster = fat32_next_cluster(fs, fi->cur_cluster);
            fi->cur_cluster_offset++;
        }
        uint32_t cl = fi->cur_cluster;
        if (cl < 2 || cl >= FAT32_CLUSTER_EOC) {
            usb_printk("[fat32] read: cluster chain ended at idx=%u\n", cluster_idx);
            break;
        }

        /* Read the cluster from disk */
        uint32_t first_sec = fs->data_start_lba +
                             (cl - 2) * fs->sec_per_cluster;
        if (usb_msd_read((struct ata_dev_s *)fs->usb_dev,
                         fs->part_lba + first_sec,
                         fs->sec_per_cluster,
                         (uintptr_t)cluster_buf) < 0) {
            kfree(cluster_buf);
            return read_total > 0 ? (int)read_total : -1;
        }

        /* Copy to user buffer */
        memcpy(buffer + read_total, cluster_buf + offset_in_cluster, chunk);
        read_total += chunk;
        if (offset_in_cluster + chunk == cluster_size) {
            fi->cur_cluster = fat32_next_cluster(fs, cl);
            fi->cur_cluster_offset = cluster_idx + 1;
        }
    }

    kfree(cluster_buf);
    file->f_pos += read_total;
    return (int)read_total;
}

/* Stub: FAT32 write (read-only) */
static int fat32_write_stub(struct file *file, const char *buffer, uint32_t size)
{
    (void)file; (void)buffer; (void)size;
    usb_printk("[fat32] write: read-only filesystem\n");
    return -1;
}

/* Stub: FAT32 lseek */
static int fat32_lseek_stub(struct file *file, int64_t offset, int whence)
{
    fat32_inode_t *fi = (fat32_inode_t *)file->f_inode->i_private;
    if (!fi) return -1;

    uint64_t new_pos;
    switch (whence) {
        case 0: /* SEEK_SET */
            new_pos = (uint64_t)offset;
            break;
        case 1: /* SEEK_CUR */
            new_pos = file->f_pos + offset;
            break;
        case 2: /* SEEK_END */
            new_pos = fi->file_size + offset;
            break;
        default:
            return -1;
    }

    if (new_pos > fi->file_size)
        new_pos = fi->file_size;

    file->f_pos = new_pos;
    return (int)new_pos;
}

/* ================================================================
 * Operation tables
 * ================================================================ */

inode_operations_t fat32_inode_ops = {
    .lookup  = fat32_lookup,
    .create  = fat32_create_stub,
    .mkdir   = fat32_mkdir_stub,
    .rmdir   = NULL,
    .unlink  = NULL,
    .rename  = NULL,
    .listdir = fat32_listdir,
};

file_operations_t fat32_file_ops = {
    .open  = fat32_open,
    .close = fat32_close,
    .read  = fat32_read,
    .write = fat32_write_stub,
    .lseek = fat32_lseek_stub,
    .ioctl = NULL,
};

/* Directory file_ops 鈥?reading a directory returns empty */
static int fat32_dir_read_stub(struct file *file, char *buffer, uint32_t size)
{
    (void)file; (void)buffer; (void)size;
    return 0;  /* directory listing not supported */
}

file_operations_t fat32_dir_file_ops = {
    .open  = fat32_open,
    .close = fat32_close,
    .read  = fat32_dir_read_stub,
    .write = fat32_write_stub,
    .lseek = fat32_lseek_stub,
    .ioctl = NULL,
};

/* ================================================================
 * Mount / unmount
 * ================================================================ */

/**
 * fat32_mount 鈥?parse BPB and initialize a FAT32 filesystem.
 *
 * @param usb_dev   Pointer to struct ata_dev_s (MSD device)
 * @param part_lba  Partition start LBA
 * @param out_fs    Output: allocated fat32_fs_t
 * @return 0 on success, negative on error
 */
int fat32_mount(void *usb_dev, uint32_t part_lba, fat32_fs_t **out_fs)
{
    uint8_t bpb[512];
    char screen_msg[80];

    snprintf(screen_msg, sizeof(screen_msg), "part_lba=%u", part_lba);
    kernel_usb_msc_status_line("fat32 mount", screen_msg);

    /* Read BPB (sector 0 of the partition) */
    long ret = usb_msd_read((struct ata_dev_s *)usb_dev,
                            part_lba, 1, (uintptr_t)bpb);
    if (ret < 0) {
        usb_printk("[fat32] mount: failed to read BPB at lba=%u\n", part_lba);
        return -1;
    }

    usb_printk("[fat32] BPB lba=%u sig=%02x %02x jmp=%02x %02x %02x oem=%c%c%c%c%c%c%c%c\n",
           part_lba, bpb[510], bpb[511],
           bpb[0], bpb[1], bpb[2],
           bpb[3], bpb[4], bpb[5], bpb[6],
           bpb[7], bpb[8], bpb[9], bpb[10]);

    /* Validate boot signature */
    if (bpb[510] != 0x55 || bpb[511] != 0xAA) {
        usb_printk("[fat32] mount: invalid boot signature at lba=%u\n", part_lba);
        return -1;
    }

    /* Parse BPB fields */
    uint16_t bytes_per_sec  = bpb[11] | ((uint16_t)bpb[12] << 8);
    uint8_t  sec_per_cluster = bpb[13];
    uint16_t reserved_sec   = bpb[14] | ((uint16_t)bpb[15] << 8);
    uint8_t  num_fats       = bpb[16];
    uint32_t sectors_per_fat = bpb[36] | ((uint32_t)bpb[37] << 8) |
                               ((uint32_t)bpb[38] << 16) |
                               ((uint32_t)bpb[39] << 24);
    uint32_t root_cluster    = bpb[44] | ((uint32_t)bpb[45] << 8) |
                               ((uint32_t)bpb[46] << 16) |
                               ((uint32_t)bpb[47] << 24);
    uint16_t root_ent_cnt    = bpb[17] | ((uint16_t)bpb[18] << 8);
    uint16_t total_sec16     = bpb[19] | ((uint16_t)bpb[20] << 8);
    uint16_t fat_sz16        = bpb[22] | ((uint16_t)bpb[23] << 8);
    uint32_t total_sec32     = bpb[32] | ((uint32_t)bpb[33] << 8) |
                               ((uint32_t)bpb[34] << 16) |
                               ((uint32_t)bpb[35] << 24);

    uint32_t total_sectors = total_sec32 ? total_sec32 : total_sec16;

    usb_printk("[fat32] BPB fields: bps=%u spc=%u rsv=%u fats=%u spf=%u root=%u tot16=%u tot32=%u rootents=%u fat16=%u\n",
           bytes_per_sec, sec_per_cluster, reserved_sec, num_fats,
           sectors_per_fat, root_cluster, total_sec16, total_sec32,
           root_ent_cnt, fat_sz16);
    snprintf(screen_msg, sizeof(screen_msg), "bps=%u spc=%u rsv=%u fats=%u",
             bytes_per_sec, sec_per_cluster, reserved_sec, num_fats);
    kernel_usb_msc_status_line("fat32 BPB", screen_msg);
    snprintf(screen_msg, sizeof(screen_msg), "spf=%u root=%u total=%u",
             sectors_per_fat, root_cluster, total_sectors);
    kernel_usb_msc_status_line("fat32 BPB", screen_msg);

    /* Basic validation: reject impossible BPBs, but tolerate valid FAT32 variants. */
    if (bytes_per_sec != 512) {
        usb_printk("[fat32] mount: unsupported sector size %u\n", bytes_per_sec);
        kernel_usb_error_line("fat32 BPB", "bad sector size");
        return -1;
    }
    if (sec_per_cluster == 0 || sec_per_cluster > 128) {
        usb_printk("[fat32] mount: invalid sec_per_cluster %u\n", sec_per_cluster);
        kernel_usb_error_line("fat32 BPB", "bad cluster size");
        return -1;
    }
    if ((sec_per_cluster & (sec_per_cluster - 1)) != 0) {
        usb_printk("[fat32] mount: non-power-of-two sec_per_cluster %u\n",
               sec_per_cluster);
        kernel_usb_error_line("fat32 BPB", "cluster not power2");
        return -1;
    }
    if (reserved_sec == 0) {
        usb_printk("[fat32] mount: invalid reserved sectors %u\n", reserved_sec);
        kernel_usb_error_line("fat32 BPB", "bad reserved sectors");
        return -1;
    }
    if (num_fats == 0 || num_fats > 2) {
        usb_printk("[fat32] mount: invalid FAT count %u\n", num_fats);
        kernel_usb_error_line("fat32 BPB", "bad FAT count");
        return -1;
    }
    if (sectors_per_fat == 0) {
        usb_printk("[fat32] mount: FAT32 sectors_per_fat is zero fat16=%u\n", fat_sz16);
        kernel_usb_error_line("fat32 BPB", "bad FAT32 size");
        return -1;
    }
    if (root_cluster < 2) {
        usb_printk("[fat32] mount: invalid root cluster %u\n", root_cluster);
        kernel_usb_error_line("fat32 BPB", "bad root cluster");
        return -1;
    }
    if (total_sectors == 0) {
        usb_printk("[fat32] mount: total sectors is zero tot16=%u tot32=%u\n",
               total_sec16, total_sec32);
        kernel_usb_error_line("fat32 BPB", "bad total sectors");
        return -1;
    }
    if (root_ent_cnt != 0 || fat_sz16 != 0) {
        usb_printk("[fat32] mount: FAT32 legacy fields nonzero rootents=%u fat16=%u, tolerate\n",
               root_ent_cnt, fat_sz16);
        kernel_usb_msc_status_line("fat32 BPB", "legacy fields tolerated");
    }

    /* Allocate and fill filesystem struct */
    fat32_fs_t *fs = (fat32_fs_t *)kmalloc(sizeof(fat32_fs_t));
    if (!fs) return -1;
    memset(fs, 0, sizeof(fat32_fs_t));

    fs->usb_dev          = usb_dev;
    fs->part_lba         = part_lba;
    fs->bytes_per_sec    = bytes_per_sec;
    fs->sec_per_cluster  = sec_per_cluster;
    fs->reserved_sec     = reserved_sec;
    fs->num_fats         = num_fats;
    fs->sectors_per_fat  = sectors_per_fat;
    fs->root_cluster     = root_cluster;
    fs->fat_start_lba    = reserved_sec;
    fs->data_start_lba   = reserved_sec + num_fats * sectors_per_fat;
    fs->cluster_size     = bytes_per_sec * sec_per_cluster;

    usb_printk("[fat32] mount: OK 鈥?%u sectors, cluster=%u bytes, root_cl=%u\n",
           sectors_per_fat, fs->cluster_size, root_cluster);

    /* Quick validation: try to read root directory */
    uint8_t *root_buf = fat32_read_cluster(fs, root_cluster);
    if (!root_buf) {
        usb_printk("[fat32] mount: failed to read root directory\n");
        snprintf(screen_msg, sizeof(screen_msg), "root=%u data=%u clsz=%u",
                 root_cluster, fs->data_start_lba, fs->cluster_size);
        kernel_usb_error_line("fat32 root read", screen_msg);
        kfree(fs);
        return -1;
    }
    kfree(root_buf);

    *out_fs = fs;
    g_fat32_fs = fs;
    return 0;
}

/**
 * fat32_create_root_inode 鈥?create a VFS inode for the FAT32 root.
 *
 * This inode gets FAT32's inode_ops so that path_lookup() can walk
 * into the FAT32 filesystem starting from this directory.
 */
int fat32_create_root_inode(fat32_fs_t *fs, inode_t **out_inode)
{
    inode_t *inode = (inode_t *)kmalloc(sizeof(inode_t));
    if (!inode) return -1;
    memset(inode, 0, sizeof(inode_t));

    inode->i_ino = 0;
    inode->i_mode = S_IFDIR | 0555;
    inode->i_size = 0;
    inode->i_nlink = 1;
    inode->i_op = &fat32_inode_ops;
    inode->i_fop = &fat32_dir_file_ops;
    /* i_sb and i_parent are set by the caller (mount-point setup) */

    fat32_inode_t *priv = (fat32_inode_t *)kmalloc(sizeof(fat32_inode_t));
    if (!priv) { kfree(inode); return -1; }
    memset(priv, 0, sizeof(fat32_inode_t));

    priv->fs = fs;
    priv->first_cluster = fs->root_cluster;
    priv->file_size = 0;
    priv->is_dir = 1;
    priv->cluster_buf = NULL;
    inode->i_private = priv;

    *out_inode = inode;
    return 0;
}
