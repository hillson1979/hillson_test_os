#ifndef BUDDY_H
#define BUDDY_H

#include "types.h"

// Buddy System 配置
#define MAX_ORDER 20  // 最大块大小：2^20 = 1048576 页 = 4GB
#define PAGE_SIZE 4096

// 块状态
#define BLOCK_FREE 0
#define BLOCK_USED 1

// 内存分配类型
#define MEM_ALLOC_KERNEL  0  // 内核专用内存
#define MEM_ALLOC_USER    1  // 用户空间内存
#define MEM_ALLOC_ANY     2  // 任意类型内存

// Buddy System 块结构
typedef struct {
    uint8_t order;        // 块大小级别 (2^order 页)
    uint8_t status;       // 块状态 (BLOCK_FREE 或 BLOCK_USED)
    uint8_t alloc_type;   // 分配类型 (MEM_ALLOC_KERNEL/USER/ANY)
    uint8_t reserved;     // 保留字段
    uint32_t start_page;  // 块起始页号
    uint32_t count;       // 块中的页数
} buddy_block_t;

// Buddy System 控制结构
typedef struct {
    buddy_block_t* blocks;      // 块数组
    uint32_t* free_lists;       // 空闲链表头指针数组
    uint32_t* next_free;        // 空闲链表下一个指针数组
    uint32_t total_blocks;      // 总块数
    uint32_t free_blocks;       // 空闲块数
    uint32_t min_order;         // 最小块大小级别
    uint32_t max_order;         // 最大块大小级别
    uint32_t base_page;         // 起始页号
    uint32_t total_pages;       // 总页数
    uint32_t kernel_reserved_pages;  // 为内核保留的页数
} buddy_system_t;

// 初始化 buddy system
int buddy_init(uint32_t base_page, uint32_t total_pages, uint32_t min_order, uint32_t max_order);

// 使用预分配的内存初始化 buddy system (带内核内存保留)
int buddy_init_with_memory(uint32_t base_page, uint32_t total_pages,
                           uint32_t min_order, uint32_t max_order,
                           uint32_t memory_start,
                           uint32_t kernel_reserved_pages);

// 分配内存块（返回页号）
uint32_t buddy_alloc(uint32_t order);

// 按类型分配内存块（返回页号）
uint32_t buddy_alloc_type(uint32_t order, uint8_t alloc_type);

// 释放内存块
int buddy_free(uint32_t page, uint32_t order);

// 获取内存统计信息
void buddy_stats(uint32_t* free_pages, uint32_t* used_pages, uint32_t* total_pages);

// 计算 order 对应的页数
static inline uint32_t order_to_pages(uint32_t order) {
    return 1 << order;
}

// 计算页数对应的最小 order
uint32_t pages_to_order(uint32_t pages);

#endif /* BUDDY_H */