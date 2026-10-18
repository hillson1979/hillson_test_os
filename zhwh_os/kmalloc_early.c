#include "types.h"
#include "memlayout.h"
#include "printf.h"
#include "mm.h"
#include "mm/buddy.h"
#include "multiboot2.h"
#include "highmem_mapping.h"
#include "page.h"
#include "task.h"

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


