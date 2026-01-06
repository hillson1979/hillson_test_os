#ifndef MULTIBOOT_H
#define MULTIBOOT_H
/*
 * multiboot.h
 *
 * <+DESC+>
 */
#include "types.h"

#define MULTIBOOT_FLAG_MEM     0x001
#define MULTIBOOT_FLAG_DEVICE  0x002
#define MULTIBOOT_FLAG_CMDLINE 0x004
#define MULTIBOOT_FLAG_MODS    0x008
#define MULTIBOOT_FLAG_AOUT    0x010
#define MULTIBOOT_FLAG_ELF     0x020
#define MULTIBOOT_FLAG_MMAP    0x040
#define MULTIBOOT_FLAG_CONFIG  0x080
#define MULTIBOOT_FLAG_LOADER  0x100
#define MULTIBOOT_FLAG_APM     0x200
#define MULTIBOOT_FLAG_VBE     0x400
#define MULTIBOOT_FLAG_FRAMEBUFFER 0x800  // Framebuffer info available

struct multiboot {
   uint32_t flags;
   uint32_t mem_lower;
   uint32_t mem_upper;
   uint32_t boot_device;
   uint32_t cmdline;
   uint32_t mods_count;
   uint32_t mods_addr;
   uint32_t num;
   uint32_t size;
   uint32_t addr;
   uint32_t shndx;
   uint32_t mmap_length;
   uint32_t mmap_addr;
   uint32_t drives_length;
   uint32_t drives_addr;
   uint32_t config_table;
   uint32_t boot_loader_name;
   uint32_t apm_table;
   uint32_t vbe_control_info;
   uint32_t vbe_mode_info;
   uint32_t vbe_mode;
   uint32_t vbe_interface_seg;
   uint32_t vbe_interface_off;
   uint32_t vbe_interface_len;
   /* Framebuffer information (provided by GRUB2, also in some Multiboot 1 implementations) */
   uint64_t framebuffer_addr;
   uint32_t framebuffer_pitch;
   uint32_t framebuffer_width;
   uint32_t framebuffer_height;
   uint8_t  framebuffer_bpp;
   uint8_t  framebuffer_type;
   uint8_t  color_info[6];  /* For color field info */
} __attribute__((packed));

typedef struct multiboot_module {
    uint32_t mod_start;   /* 模块在内存中的起始物理地址 */
    uint32_t mod_end;     /* 结束物理地址（不含） */
    uint32_t string;      /* 指向模块名字符串（通常是物理地址或已被 GRUB 转换）*/
    uint32_t reserved;
} multiboot_module_t;

// 内存映射条目
typedef struct multiboot_mmap_entry {
    uint32_t size;        // 本条目大小(不包括size字段本身)
    uint32_t base_addr_low;   // 基地址低32位
    uint32_t base_addr_high;  // 基地址高32位
    uint32_t length_low;      // 长度低32位
    uint32_t length_high;     // 长度高32位
    uint32_t type;        // 类型: 1=RAM, 2=保留, 3=ACPI等
} __attribute__((packed)) multiboot_mmap_entry_t;

// 内存类型定义
#define MULTIBOOT_MEMORY_AVAILABLE         1
#define MULTIBOOT_MEMORY_RESERVED          2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE 3
#define MULTIBOOT_MEMORY_NVS               4
#define MULTIBOOT_MEMORY_UNUSABLE          5

extern struct multiboot *multiboot_info;
extern multiboot_module_t *mod;

#endif /* ndef MULTIBOOT_H */
