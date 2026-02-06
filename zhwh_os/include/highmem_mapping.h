// highmem_mapping.h
#ifndef HIGHMEM_MAPPING_H
#define HIGHMEM_MAPPING_H

#include "types.h"

// 内核虚拟地址基址
#define KERNEL_VIRT_BASE   0xC0000000
#define IDENTITY_MAP_SIZE  0x0800000  // 8MB恒等映射

// 动态映射窗口（在恒等映射区域之后）
#define DYNAMIC_MAP_WINDOW_BASE (KERNEL_VIRT_BASE + IDENTITY_MAP_SIZE)  // 0xC0800000
#define DYNAMIC_MAP_WINDOW_SIZE 0x08000000  // 128MB窗口 (足够用于用户进程加载)

// 硬件物理地址
#define APIC_PHYS_BASE     0xFEE00000
#define PCI_CONFIG_PHYS    0xFEC00000
#define PCI_MEM_PHYS_BASE  0xE0000000

// 物理到虚拟地址转换
#define PHYS_TO_VIRT(phys) ((void*)((phys) + KERNEL_VIRT_BASE))
#define VIRT_TO_PHYS(virt) ((uint32_t)(virt) - KERNEL_VIRT_BASE)

// 检查地址是否在恒等映射区域内
#define IS_IDENTITY_MAPPED(phys) ((phys) < IDENTITY_MAP_SIZE)

struct highmem_mapping {
    uint32_t phys_addr;
    uint32_t virt_addr;
    uint32_t size;
    uint32_t flags;
    bool in_use;
    const char* description;
};

void init_highmem_mapping(void);
void* map_highmem_physical(uint32_t phys_addr, uint32_t size, uint32_t flags);
void unmap_highmem_physical(void* virt_addr);
void* get_mapped_address(uint32_t phys_addr);

#endif // HIGHMEM_MAPPING_H
