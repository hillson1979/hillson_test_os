#include "types.h"
#include "memlayout.h"
#include "printf.h"
/* 假设我们有一块连续的物理内存区域 */
#define EARLY_MEM_POOL_SIZE  (1 * 1024 * 1024)  // 1MB 临时内存池
static uint8_t early_mem_pool[EARLY_MEM_POOL_SIZE];
static unsigned int early_mem_offset = 0;

/* 简单的早期内存分配函数（类似 malloc） */
void *kmalloc_early(unsigned int size) {
    /* 对齐到 8 字节边界（可根据架构调整） */
    size = (size + 7) & ~7;

    if (early_mem_offset + size > EARLY_MEM_POOL_SIZE) {
        return 0;  // 内存不足
    }

    void *ptr = &early_mem_pool[early_mem_offset];
    early_mem_offset += size;
    return ptr;
}

#define KERNEL_VA_OFFSET 0xC0000000   // 内核虚拟地址偏移
// 地址转换宏（内核直接映射）
#define phys_to_virt(pa) ((void*)((uint32_t)(pa) + KERNEL_VA_OFFSET))
#define virt_to_phys(va) ((uint32_t)(va) - KERNEL_VA_OFFSET)

extern char _kernel_end_virtual[];   // 链接脚本里内核结束符号
#define PMM_START  V2P((uint32_t)_kernel_end_virtual)
#define PMM_END   0x08000000   // 128MB，随便写个上限
static uint32_t addr;
static uint32_t pmm_next_free = PMM_START;
uint32_t pmm_alloc_page() {
    if (pmm_next_free >= PMM_END) {
        // 内存不足，panic 或者返回 0
        printf("oom------\n");
        return 0;
    }

    //printf("pmm_next_free is 0x%x\n",pmm_next_free);
    addr = pmm_next_free;
    pmm_next_free += 4096;
    return addr;  // 物理地址
}
