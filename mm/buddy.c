#include "mm/buddy.h"
#include "printf.h"
#include "string.h"
#include "spinlock.h"
#include "kmalloc_early.h"

static buddy_system_t buddy_sys;
static spinlock_t buddy_lock = SPINLOCK_INITIALIZER;

// 使用静态数组替代动态分配,避免 kmalloc_early 内存池不够
#define MAX_BUDDY_BLOCKS 150000

static buddy_block_t buddy_blocks_array[MAX_BUDDY_BLOCKS];
static uint32_t free_lists_array[MAX_ORDER + 1];
static uint32_t next_free_array[MAX_BUDDY_BLOCKS];

// 初始化 buddy system
int buddy_init(uint32_t base_page, uint32_t total_pages, uint32_t min_order, uint32_t max_order) {
    uint32_t i;

    // 验证参数
    if (max_order > MAX_ORDER || min_order > max_order || total_pages == 0) {
        printf("buddy_init: invalid parameters\n");
        return -1;
    }

    printf("buddy_init: base_page=%u, total_pages=%u, max_order=%u\n",
           base_page, total_pages, max_order);

    // 计算需要的块数量
    uint32_t max_blocks = total_pages + max_order;

    if (max_blocks > MAX_BUDDY_BLOCKS) {
        printf("buddy_init: ERROR - need %u blocks, but MAX_BUDDY_BLOCKS=%u\n",
               max_blocks, MAX_BUDDY_BLOCKS);
        printf("buddy_init: Please increase MAX_BUDDY_BLOCKS in buddy.c\n");
        return -1;
    }

    // 使用静态分配的数组
    buddy_sys.blocks = buddy_blocks_array;
    buddy_sys.free_lists = free_lists_array;
    buddy_sys.next_free = next_free_array;

    printf("buddy_init: using static arrays for %u blocks\n", max_blocks);

    // 初始化空闲链表
    memset(buddy_sys.free_lists, 0xFF, (max_order + 1) * sizeof(uint32_t));
    memset(buddy_sys.next_free, 0xFF, max_blocks * sizeof(uint32_t));

    printf("buddy_init: creating initial free blocks\n");

    // 计算实际可用的最大 order
    uint32_t actual_order = max_order;
    uint32_t actual_pages = 1 << actual_order;

    // 确保不超过实际可用页数
    while (actual_pages > total_pages && actual_order > 0) {
        actual_order--;
        actual_pages = 1 << actual_order;
    }

    if (actual_pages == 0) {
        printf("buddy_init: ERROR - no available pages\n");
        return -1;
    }

    // 创建初始大块
    buddy_sys.blocks[0].order = actual_order;
    buddy_sys.blocks[0].status = BLOCK_FREE;
    buddy_sys.blocks[0].start_page = base_page;
    buddy_sys.blocks[0].count = actual_pages;

    // 添加到空闲链表
    buddy_sys.next_free[0] = 0xFFFFFFFF;  // 无下一个块
    buddy_sys.free_lists[actual_order] = 0;

    // 设置 buddy system 信息
    buddy_sys.total_blocks = 1;
    buddy_sys.free_blocks = 1;
    buddy_sys.min_order = min_order;
    buddy_sys.max_order = actual_order;  // 使用实际的 order
    buddy_sys.base_page = base_page;
    buddy_sys.total_pages = total_pages;

    printf("buddy_init: initialized 1 block (order %u = %u pages)\n",
           actual_order, actual_pages);
    printf("buddy_init: SUCCESS - buddy system ready\n");

    return 0;
}

// 计算 buddy 块索引
static uint32_t buddy_index(uint32_t page, uint32_t order) {
    uint32_t buddy_page = page ^ (1 << order);
    return buddy_page;
}

// 分配内存块
uint32_t buddy_alloc(uint32_t order) {
    uint32_t i, j;
    uint32_t block_index, buddy_block_index;
    uint32_t page = 0;
    
    spinlock_acquire(&buddy_lock);
    
    // 验证 order
    if (order < buddy_sys.min_order || order > buddy_sys.max_order) {
        spinlock_release(&buddy_lock);
        return 0;
    }
    
    // 查找合适的空闲块
    for (i = order; i <= buddy_sys.max_order; i++) {
        if (buddy_sys.free_lists[i] != 0xFFFFFFFF) {
            // 找到空闲块
            block_index = buddy_sys.free_lists[i];
            buddy_sys.free_lists[i] = buddy_sys.next_free[block_index];
            
            buddy_sys.blocks[block_index].status = BLOCK_USED;
            buddy_sys.free_blocks--;
            
            // 如果块太大，分割成更小的块
            while (i > order) {
                i--;
                
                // 创建新的块
                buddy_block_index = buddy_sys.total_blocks++;
                buddy_sys.blocks[buddy_block_index].order = i;
                buddy_sys.blocks[buddy_block_index].status = BLOCK_FREE;
                buddy_sys.blocks[buddy_block_index].start_page = 
                    buddy_sys.blocks[block_index].start_page + (1 << i);
                buddy_sys.blocks[buddy_block_index].count = 1 << i;
                
                // 添加到空闲链表
                buddy_sys.next_free[buddy_block_index] = buddy_sys.free_lists[i];
                buddy_sys.free_lists[i] = buddy_block_index;
                
                buddy_sys.free_blocks++;
            }
            
            page = buddy_sys.blocks[block_index].start_page;
            break;
        }
    }
    
    spinlock_release(&buddy_lock);
    return page;
}

// 释放内存块
int buddy_free(uint32_t page, uint32_t order) {
    uint32_t i;
    uint32_t block_index = 0xFFFFFFFF;
    uint32_t buddy_page, buddy_block_index;
    
    spinlock_acquire(&buddy_lock);
    
    // 查找对应的块
    for (i = 0; i < buddy_sys.total_blocks; i++) {
        if (buddy_sys.blocks[i].start_page == page && 
            buddy_sys.blocks[i].order == order && 
            buddy_sys.blocks[i].status == BLOCK_USED) {
            block_index = i;
            break;
        }
    }
    
    if (block_index == 0xFFFFFFFF) {
        spinlock_release(&buddy_lock);
        return -1;
    }
    
    // 释放块
    buddy_sys.blocks[block_index].status = BLOCK_FREE;
    buddy_sys.free_blocks++;
    
    // 尝试合并 buddy 块
    while (order < buddy_sys.max_order) {
        buddy_page = buddy_index(page, order);
        
        // 查找 buddy 块
        buddy_block_index = 0xFFFFFFFF;
        for (i = 0; i < buddy_sys.total_blocks; i++) {
            if (buddy_sys.blocks[i].start_page == buddy_page && 
                buddy_sys.blocks[i].order == order && 
                buddy_sys.blocks[i].status == BLOCK_FREE) {
                buddy_block_index = i;
                break;
            }
        }
        
        if (buddy_block_index == 0xFFFFFFFF) {
            break; // 没有找到 buddy 块
        }
        
        // 从空闲链表中移除 buddy 块
        uint32_t* prev_ptr = &buddy_sys.free_lists[order];
        while (*prev_ptr != buddy_block_index) {
            prev_ptr = &buddy_sys.next_free[*prev_ptr];
        }
        *prev_ptr = buddy_sys.next_free[buddy_block_index];
        
        // 更新当前块
        if (page < buddy_page) {
            buddy_sys.blocks[block_index].order = order + 1;
            buddy_sys.blocks[block_index].count = 1 << (order + 1);
        } else {
            buddy_sys.blocks[buddy_block_index].order = order + 1;
            buddy_sys.blocks[buddy_block_index].count = 1 << (order + 1);
            block_index = buddy_block_index;
        }
        
        order++;
        page = buddy_sys.blocks[block_index].start_page;
        buddy_sys.free_blocks--;
    }
    
    // 将合并后的块添加到空闲链表
    buddy_sys.next_free[block_index] = buddy_sys.free_lists[order];
    buddy_sys.free_lists[order] = block_index;
    
    spinlock_release(&buddy_lock);
    return 0;
}

// 获取内存统计信息
void buddy_stats(uint32_t* free_pages, uint32_t* used_pages, uint32_t* total_pages) {
    uint32_t i;
    uint32_t free = 0;
    
    spinlock_acquire(&buddy_lock);
    
    for (i = 0; i < buddy_sys.total_blocks; i++) {
        if (buddy_sys.blocks[i].status == BLOCK_FREE) {
            free += buddy_sys.blocks[i].count;
        }
    }
    
    if (free_pages) {
        *free_pages = free;
    }
    if (used_pages) {
        *used_pages = buddy_sys.total_pages - free;
    }
    if (total_pages) {
        *total_pages = buddy_sys.total_pages;
    }
    
    spinlock_release(&buddy_lock);
}

// 计算页数对应的最小 order
uint32_t pages_to_order(uint32_t pages) {
    uint32_t order = 0;
    
    if (pages == 0) {
        return 0;
    }
    
    pages--;
    while (pages > 0) {
        pages >>= 1;
        order++;
    }
    
    return order;
}