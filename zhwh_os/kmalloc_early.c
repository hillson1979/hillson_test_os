#include "types.h"
#include "memlayout.h"
#include "printf.h"
#include "mm.h"
#include "mm/buddy.h"
#include "multiboot2.h"
#include "highmem_mapping.h"

extern uint32_t multiboot2_info_addr;

/* 早期内存池 - 用于内核初始化阶段 */
#define EARLY_MEM_POOL_SIZE  (1 * 1024 * 1024)  // 1MB 临时内存池
static uint8_t early_mem_pool[EARLY_MEM_POOL_SIZE];
static unsigned int early_mem_offset = 0;

/* 简单的早期内存分配函数（类似 malloc） */
void *kmalloc_early(unsigned int size) {
    /* 对齐到 8 字节边界（可根据架构调整） */
    size = (size + 7) & ~7;

    if (early_mem_offset + size > EARLY_MEM_POOL_SIZE) {
        printf("kmalloc_early: out of memory (need %u, have %u)\n",
               size, EARLY_MEM_POOL_SIZE - early_mem_offset);
        return 0;  // 内存不足
    }

    void *ptr = &early_mem_pool[early_mem_offset];
    early_mem_offset += size;
    return ptr;
}

/* ============ 内存块元数据 ============ */

#define MAX_ALLOCATIONS 1024  // 最大跟踪的分配数量

struct allocation_header {
    void *virt_addr;      // 虚拟地址
    uint32_t phys_addr;   // 物理地址
    uint32_t size;        // 实际大小
    uint32_t page_count;  // 占用页数
    bool in_use;          // 是否正在使用
};

static struct allocation_header alloc_table[MAX_ALLOCATIONS];
static uint32_t alloc_count = 0;

// 查找或创建分配记录
static struct allocation_header* find_or_create_allocation(void *virt_addr) {
    // 查找空闲槽位
    for (uint32_t i = 0; i < MAX_ALLOCATIONS; i++) {
        if (!alloc_table[i].in_use) {
            alloc_table[i].virt_addr = virt_addr;
            alloc_table[i].in_use = true;
            if (i >= alloc_count) {
                alloc_count = i + 1;
            }
            return &alloc_table[i];
        }
    }
    return NULL;  // 表满了
}

// 通过虚拟地址查找分配记录
static struct allocation_header* find_allocation(void *virt_addr) {
    for (uint32_t i = 0; i < alloc_count; i++) {
        if (alloc_table[i].in_use && alloc_table[i].virt_addr == virt_addr) {
            return &alloc_table[i];
        }
    }
    return NULL;
}

/* ============ 物理内存管理器 (PMM) ============ */

extern char _kernel_end_virtual[];   // 链接脚本里内核结束符号

// PMM 状态
static uint32_t pmm_start = 0;        // 物理内存管理起始地址
static uint32_t pmm_end = 0;          // 物理内存管理结束地址
static uint32_t pmm_total_pages = 0;  // 总页数
static bool pmm_buddy_enabled = false;  // Buddy System 是否已启用

// 初始化 PMM
void pmm_init(void) {
    if (!multiboot2_info_addr) {
        printf("pmm_init: no multiboot2 info\n");
        return;
    }

    // 计算物理内存范围
    uint32_t kernel_end_phys = V2P((uint32_t)_kernel_end_virtual);

    // 为 Buddy System 预留空间
    // Buddy System 数据放在 48MB，DMA 占用 40MB-48MB
    // 使用物理地址（Buddy System 管理的是物理内存）
    uint32_t buddy_data_phys = 0x03000000;  // 48MB 物理地址
    uint32_t buddy_data_reserved = 20 * 1024 * 1024;  // 20MB 预留空间
    pmm_start = buddy_data_phys + buddy_data_reserved;  // 68MB 物理地址 - Buddy 元数据之后

    printf("pmm_init: kernel_end_phys=0x%x\n", kernel_end_phys);
    printf("pmm_init: placing buddy system data at 48MB physical (0x3000000)\n");
    printf("pmm_init: buddy system virtual address: 0xC3000000\n");
    printf("pmm_init: buddy system data size: %u MB at phys=0x%x-0x%x\n",
           buddy_data_reserved / (1024 * 1024),
           buddy_data_phys, buddy_data_phys + buddy_data_reserved);

    // 获取内存信息
    uint32_t mem_lower_kb, mem_upper_kb;
    get_multiboot2_memory_info(&mem_lower_kb, &mem_upper_kb);

    // PMM 管理的内存范围 - 从4MB到实际物理内存顶部
    // 根据实际检测到的内存大小设置 pmm_end
    uint32_t total_phys_mem = (mem_upper_kb + 640) * 1024;  // 总物理内存（字节）
    pmm_end = total_phys_mem - 1;  // 物理内存顶部
    if (pmm_end > 0xFFFFFFFF) {
        pmm_end = 0xFFFFFFFF;  // 最大不超过 4GB
    }
    // 确保至少有一些内存可管理
    if (pmm_end < pmm_start) {
        pmm_end = pmm_start + 0x1000000;  // 至少 16MB
    }
    pmm_total_pages = (pmm_end - pmm_start + 1) / 4096;

    // 计算管理实际内存需要的 Buddy System 数据结构大小
    uint32_t max_order = 20;  // 支持 2^20 = 1,048,576 页 = 4GB
    uint32_t max_blocks = pmm_total_pages + max_order;

    uint32_t blocks_size = max_blocks * sizeof(buddy_block_t);
    uint32_t freelists_size = (max_order + 1) * sizeof(uint32_t);
    uint32_t nextfree_size = max_blocks * sizeof(uint32_t);
    uint32_t buddy_data_size = (blocks_size + freelists_size + nextfree_size + 4095) & ~4095;

    printf("pmm_init: buddy system data structures for %u MB:\n",
           (pmm_total_pages * 4096) / (1024 * 1024));
    printf("  max_blocks=%u, buddy_data_size=%u MB (%u bytes)\n",
           max_blocks, buddy_data_size / (1024 * 1024), buddy_data_size);

    // Buddy System 数组映射到虚拟地址 0xC3000000 (48MB + 0xC0000000)
    // 需要先映射物理内存才能访问
    uint32_t buddy_data_virt = 0xC3000000;

    printf("pmm_init: mapping buddy data area: phys=0x%x -> virt=0x%x\n",
           buddy_data_phys, buddy_data_virt);

    // 映射 Buddy System 数据区域（按 4KB 页映射）
    for (uint32_t off = 0; off < buddy_data_size; off += 4096) {
        map_4k_page(buddy_data_phys + off, buddy_data_virt + off, 0x3);  // Present + RW
    }

    printf("pmm_init: buddy data area mapped successfully\n");

    printf("pmm_init: physical memory manager initialized\n");
    printf("  start: 0x%x (%u MB), end: 0x%x (%u MB)\n",
           pmm_start, pmm_start / (1024 * 1024),
           pmm_end, pmm_end / (1024 * 1024));
    printf("  total pages: %u (%u MB)\n", pmm_total_pages,
           (pmm_total_pages * 4096) / (1024 * 1024));

    // 初始化 Buddy System - 管理从36MB到内存顶部的全部地址空间
    uint32_t base_page = pmm_start / 4096;
    uint32_t min_order = 0;  // 最小块：1 页

    // 计算合适的 max_order（不超过实际管理页数）
    while ((1 << max_order) > pmm_total_pages && max_order > 0) {
        max_order--;
    }

    printf("pmm_init: initializing buddy system...\n");
    printf("  base_page=%u, managed_pages=%u, max_order=%u\n",
           base_page, pmm_total_pages, max_order);

    // 为内核保留前 512MB 内存 (131072 页)
    uint32_t kernel_reserved_pages = 131072;  // 512MB
    printf("pmm_init: reserving %u MB for kernel (pages %u-%u)\n",
           (kernel_reserved_pages * 4096) / (1024 * 1024),
           base_page, base_page + kernel_reserved_pages - 1);

    if (buddy_init_with_memory(base_page, pmm_total_pages, min_order, max_order,
                               buddy_data_virt, kernel_reserved_pages) == 0) {
        pmm_buddy_enabled = true;
        printf("pmm_init: buddy system enabled successfully\n");
    } else {
        pmm_buddy_enabled = false;
        printf("pmm_init: WARNING - buddy system initialization failed\n");
    }

    // 🔥 初始化 DMA Coherent 内存区域（在 paging 完成后）
    extern void dma_map_region(void);
    printf("pmm_init: initializing DMA coherent region...\n");
    printf("pmm_init: dma_map_region function pointer = 0x%x\n", (uint32_t)dma_map_region);
    printf("pmm_init: about to call dma_map_region()...\n");
    dma_map_region();
    printf("pmm_init: returned from dma_map_region()\n");
}

// 分配一个物理页
uint32_t pmm_alloc_page(void) {
    return pmm_alloc_page_type(MEM_ALLOC_KERNEL);
}

// 按类型分配一个物理页
uint32_t pmm_alloc_page_type(uint8_t alloc_type) {
    if (pmm_buddy_enabled) {
        // 使用 Buddy System 分配
        uint32_t page = buddy_alloc_type(0, alloc_type);  // order 0 = 1 page
        if (page == 0) {
            printf("pmm_alloc_page_type: buddy system out of memory (type=%u)!\n", alloc_type);
            return 0;
        }
        return page * 4096;  // 转换为物理地址
    } else {
        // Buddy System 未启用，不应该到达这里
        printf("pmm_alloc_page_type: ERROR - buddy system not enabled!\n");
        return 0;
    }
}

// 分配多个连续的物理页
uint32_t pmm_alloc_pages(uint32_t count) {
    return pmm_alloc_pages_type(count, MEM_ALLOC_KERNEL);
}

// 按类型分配多个连续的物理页
uint32_t pmm_alloc_pages_type(uint32_t count, uint8_t alloc_type) {
    if (count == 0) return 0;

    if (pmm_buddy_enabled) {
        // 计算需要的 order
        uint32_t order = pages_to_order(count);

        // 从 Buddy System 分配
        uint32_t page = buddy_alloc_type(order, alloc_type);
        if (page == 0) {
            printf("pmm_alloc_pages_type: buddy system out of memory (need %u pages, type=%u)!\n",
                   count, alloc_type);
            return 0;
        }

        return page * 4096;  // 转换为物理地址
    } else {
        printf("pmm_alloc_pages_type: ERROR - buddy system not enabled!\n");
        return 0;
    }
}

// 释放物理页
void pmm_free_page(uint32_t phys_addr) {
    if (phys_addr == 0) {
        printf("pmm_free_page: warning - freeing null address\n");
        return;
    }

    if (!pmm_buddy_enabled) {
        printf("pmm_free_page: ERROR - buddy system not enabled!\n");
        return;
    }

    // 转换为页号
    uint32_t page = phys_addr / 4096;

    // 释放到 Buddy System (order 0 = 1 page)
    if (buddy_free(page, 0) != 0) {
        printf("pmm_free_page: failed to free page at 0x%x\n", phys_addr);
    }
}

// 释放多个物理页
void pmm_free_pages(uint32_t phys_addr, uint32_t count) {
    if (phys_addr == 0 || count == 0) {
        printf("pmm_free_pages: warning - invalid parameters (addr=0x%x, count=%u)\n",
               phys_addr, count);
        return;
    }

    if (!pmm_buddy_enabled) {
        printf("pmm_free_pages: ERROR - buddy system not enabled!\n");
        return;
    }

    // 计算页号和 order
    uint32_t page = phys_addr / 4096;
    uint32_t order = pages_to_order(count);

    // 释放到 Buddy System
    if (buddy_free(page, order) != 0) {
        printf("pmm_free_pages: failed to free pages at 0x%x (count=%u)\n",
               phys_addr, count);
    }
}

// 打印 PMM 统计信息
void pmm_print_stats(void) {
    printf("=== Physical Memory Manager Statistics ===\n");
    printf("  Memory range: 0x%x - 0x%x\n", pmm_start, pmm_end);
    printf("  Total pages:  %u (%u MB)\n", pmm_total_pages,
           (pmm_total_pages * 4096) / (1024 * 1024));
    printf("  Buddy System: %s\n", pmm_buddy_enabled ? "enabled" : "disabled");

    if (pmm_buddy_enabled) {
        uint32_t free_pages, used_pages, total_pages;
        buddy_stats(&free_pages, &used_pages, &total_pages);
        printf("  Free pages:   %u (%u MB)\n", free_pages,
               (free_pages * 4096) / (1024 * 1024));
        printf("  Used pages:   %u (%u MB)\n", used_pages,
               (used_pages * 4096) / (1024 * 1024));
    }

    printf("==========================================\n");
}

/* ============ 通用内核内存分配器 (kmalloc/kfree) ============ */

extern uint32_t high_virt_base;  // 来自 highmem_mapping.c

// 内核内存分配函数
void *kmalloc(unsigned int size) {
    if (size == 0) {
        return NULL;
    }

    // 小于页大小的分配，使用早期内存池
    if (size <= 2048) {  // 2KB 以下使用早期池
        void *ptr = kmalloc_early(size);
        if (ptr) {
            // 记录早期池分配（不需要物理页）
            struct allocation_header *hdr = find_or_create_allocation(ptr);
            if (hdr) {
                hdr->phys_addr = 0;  // 早期池没有物理地址
                hdr->size = size;
                hdr->page_count = 0;
            }
        }
        return ptr;
    }

    // 大于页大小的分配，使用 PMM
    // 计算需要的页数
    uint32_t page_count = (size + 4095) / 4096;

    // 从 PMM 分配物理页
    uint32_t phys_addr = pmm_alloc_pages(page_count);
    if (phys_addr == 0) {
        printf("kmalloc: failed to allocate %u pages\n", page_count);
        return NULL;
    }

    // 映射到虚拟地址（使用高内存映射）
    void *virt_addr = phys_to_virt(phys_addr);

    // 记录分配
    struct allocation_header *hdr = find_or_create_allocation(virt_addr);
    if (hdr) {
        hdr->phys_addr = phys_addr;
        hdr->size = size;
        hdr->page_count = page_count;
    } else {
        printf("kmalloc: allocation table full!\n");
        // 即使记录失败，内存已经分配，仍然返回地址
    }

    printf("kmalloc: allocated %u bytes (%u pages) at virt=0x%x, phys=0x%x\n",
           size, page_count, (uint32_t)virt_addr, phys_addr);

    return virt_addr;
}

// 释放内核内存
void kfree(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    // 查找分配记录
    struct allocation_header *hdr = find_allocation(ptr);
    if (hdr == NULL) {
        printf("kfree: warning - freeing untracked address 0x%x\n", (uint32_t)ptr);
        return;
    }

    if (!hdr->in_use) {
        printf("kfree: warning - double free at 0x%x\n", (uint32_t)ptr);
        return;
    }

    printf("kfree: freeing %u bytes (%u pages) at virt=0x%x, phys=0x%x\n",
           hdr->size, hdr->page_count, (uint32_t)hdr->virt_addr, hdr->phys_addr);

    // 如果是物理页分配（page_count > 0），真正释放物理页
    if (hdr->page_count > 0) {
        if (hdr->page_count == 1) {
            pmm_free_page(hdr->phys_addr);
        } else {
            pmm_free_pages(hdr->phys_addr, hdr->page_count);
        }
    }
    // 注意：早期内存池（page_count == 0）的分配不能释放

    // 清除分配记录
    hdr->in_use = false;
    hdr->virt_addr = NULL;
    hdr->phys_addr = 0;
    hdr->size = 0;
    hdr->page_count = 0;
}

// 分配并清零内存
void *kzalloc(unsigned int size) {
    void *ptr = kmalloc(size);
    if (ptr) {
        // 清零内存
        uint8_t *bytes = (uint8_t*)ptr;
        for (uint32_t i = 0; i < size; i++) {
            bytes[i] = 0;
        }
    }
    return ptr;
}

// 打印分配统计
void kmalloc_print_stats(void) {
    printf("=== Kernel Allocation Statistics ===\n");
    printf("  Total allocations tracked: %u/%u\n", alloc_count, MAX_ALLOCATIONS);

    uint32_t total_virt = 0;
    uint32_t total_phys_allocs = 0;
    uint32_t total_phys_pages = 0;

    for (uint32_t i = 0; i < alloc_count; i++) {
        if (alloc_table[i].in_use) {
            total_virt += alloc_table[i].size;
            if (alloc_table[i].page_count > 0) {
                total_phys_allocs++;
                total_phys_pages += alloc_table[i].page_count;
            }
        }
    }

    printf("  Virtual memory (early pool): %u bytes\n", total_virt);
    printf("  Physical allocations: %u\n", total_phys_allocs);
    printf("  Physical pages used: %u (%u MB)\n",
           total_phys_pages,
           (total_phys_pages * 4096) / (1024 * 1024));
    printf("=====================================\n");
}

/* ============ 用户空间内存分配 ============ */

// 分配用户空间物理页
uint32_t umem_alloc_pages(uint32_t count) {
    if (count == 0) {
        return 0;
    }

    // 使用 PMM 的用户空间类型分配
    uint32_t phys_addr = pmm_alloc_pages_type(count, MEM_ALLOC_USER);
    if (phys_addr == 0) {
        printf("umem_alloc_pages: failed to allocate %u pages for user space\n", count);
        return 0;
    }

    printf("umem_alloc_pages: allocated %u pages for user space at phys=0x%x\n",
           count, phys_addr);

    return phys_addr;
}

// 释放用户空间物理页
void umem_free_pages(uint32_t phys_addr, uint32_t count) {
    if (phys_addr == 0 || count == 0) {
        printf("umem_free_pages: warning - invalid parameters (addr=0x%x, count=%u)\n",
               phys_addr, count);
        return;
    }

    printf("umem_free_pages: freeing %u user pages at phys=0x%x\n", count, phys_addr);

    // 使用 PMM 释放
    pmm_free_pages(phys_addr, count);
}
