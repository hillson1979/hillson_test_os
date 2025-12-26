#include "types.h"
#include "memlayout.h"
#include "printf.h"
#include "mm.h"
#include "multiboot.h"

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
extern struct multiboot *multiboot_info;

// PMM 状态
static uint32_t pmm_start = 0;        // 物理内存管理起始地址
static uint32_t pmm_end = 0;          // 物理内存管理结束地址
static uint32_t pmm_next_free = 0;    // 下一个空闲页
static uint32_t pmm_total_pages = 0;  // 总页数
static uint32_t pmm_free_count = 0;   // 空闲页数（改名避免与函数冲突）
static uint32_t pmm_allocated_pages = 0; // 已分配页数

// 初始化 PMM
void pmm_init(void) {
    if (!multiboot_info) {
        printf("pmm_init: no multiboot info\n");
        return;
    }

    // 计算物理内存范围
    uint32_t kernel_end_phys = V2P((uint32_t)_kernel_end_virtual);
    pmm_start = (kernel_end_phys + 4095) & ~4095;  // 页对齐

    // 从 multiboot 获取物理内存大小
    uint32_t mem_upper_bytes = multiboot_info->mem_upper * 1024;
    pmm_end = mem_upper_bytes;  // 使用扩展内存作为上限

    pmm_total_pages = (pmm_end - pmm_start) / 4096;
    pmm_free_count = pmm_total_pages;
    pmm_next_free = pmm_start;
    pmm_allocated_pages = 0;

    printf("pmm_init: physical memory manager initialized\n");
    printf("  start: 0x%x, end: 0x%x\n", pmm_start, pmm_end);
    printf("  total pages: %u (%u MB)\n", pmm_total_pages,
           (pmm_total_pages * 4096) / (1024 * 1024));
}

// 分配一个物理页
uint32_t pmm_alloc_page(void) {
    if (pmm_next_free >= pmm_end) {
        printf("pmm_alloc_page: out of memory!\n");
        printf("  allocated: %u/%u pages\n", pmm_allocated_pages, pmm_total_pages);
        return 0;
    }

    uint32_t addr = pmm_next_free;
    pmm_next_free += 4096;
    pmm_free_count--;
    pmm_allocated_pages++;

    return addr;  // 返回物理地址
}

// 分配多个连续的物理页
uint32_t pmm_alloc_pages(uint32_t count) {
    if (count == 0) return 0;

    // 检查是否有足够的连续内存
    if (pmm_next_free + count * 4096 > pmm_end) {
        printf("pmm_alloc_pages: out of memory (need %u pages)!\n", count);
        printf("  allocated: %u/%u pages\n", pmm_allocated_pages, pmm_total_pages);
        return 0;
    }

    uint32_t addr = pmm_next_free;
    pmm_next_free += count * 4096;
    pmm_free_count -= count;
    pmm_allocated_pages += count;

    return addr;  // 返回物理地址
}

// 释放物理页（当前简单实现不支持释放）
void pmm_free_page(uint32_t phys_addr) {
    // TODO: 实现页释放功能
    printf("pmm_free_page: not implemented yet (addr=0x%x)\n", phys_addr);
}

// 释放多个物理页
void pmm_free_pages(uint32_t phys_addr, uint32_t count) {
    // TODO: 实现页释放功能
    printf("pmm_free_pages: not implemented yet (addr=0x%x, count=%u)\n",
           phys_addr, count);
}

// 打印 PMM 统计信息
void pmm_print_stats(void) {
    printf("=== Physical Memory Manager Statistics ===\n");
    printf("  Memory range: 0x%x - 0x%x\n", pmm_start, pmm_end);
    printf("  Total pages:  %u (%u MB)\n", pmm_total_pages,
           (pmm_total_pages * 4096) / (1024 * 1024));
    printf("  Free pages:   %u (%u MB)\n", pmm_free_count,
           (pmm_free_count * 4096) / (1024 * 1024));
    printf("  Allocated:    %u pages (%u MB)\n", pmm_allocated_pages,
           (pmm_allocated_pages * 4096) / (1024 * 1024));
    printf("  Next free:    0x%x\n", pmm_next_free);
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

    // 如果是物理页分配（page_count > 0），标记释放
    if (hdr->page_count > 0) {
        // 简单实现：暂不支持真正释放物理页
        // TODO: 实现真正的物理页释放
        printf("kfree: physical page release not yet implemented\n");
    }

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
