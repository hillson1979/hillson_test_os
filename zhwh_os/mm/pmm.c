/* Physical memory manager and buddy-backed page allocation. */
#include "types.h"
#include "memlayout.h"
#include "printf.h"
#include "mm.h"
#include "mm/buddy.h"
#include "multiboot2.h"
#include "highmem_mapping.h"
#include "page.h"
#include "mm/buddy.h"

/* ============ 物理内存管理器 (PMM) ============ */

extern char _kernel_end_virtual[];   // 链接脚本里内核结束符号

// PMM 状态
static uint32_t pmm_start = 0;        // 物理内存管理起始地址
static uint32_t pmm_end = 0;          // 物理内存管理结束地址
static uint32_t pmm_total_pages = 0;  // 总页数
static bool pmm_buddy_enabled = false;  // Buddy System 是否已启用

bool pmm_is_ready(void) { return pmm_buddy_enabled; }

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

