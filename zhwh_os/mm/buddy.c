#include "mm/buddy.h"
#include "printf.h"
#include "string.h"
#include "kmalloc_early.h"

static buddy_system_t buddy_sys;

// 大幅减少静态数组大小,只用于初始化阶段
#define MAX_BUDDY_BLOCKS 2000  // 最小块数,只用于启动

// 使用指针,后续动态分配到高内存
static buddy_block_t* buddy_blocks_array;
static uint32_t* free_lists_array;
static uint32_t* next_free_array;
static uint32_t allocated_blocks = 0;  // 实际分配的块数

// 使用预分配的内存初始化 buddy system (带内核内存保留)
int buddy_init_with_memory(uint32_t base_page, uint32_t total_pages,
                          uint32_t min_order, uint32_t max_order,
                          uint32_t memory_start,
                          uint32_t kernel_reserved_pages) {
    uint32_t i;

    // 验证参数
    if (max_order > MAX_ORDER || min_order > max_order || total_pages == 0) {
        printf("buddy_init: invalid parameters\n");
        return -1;
    }

    printf("buddy_init: base_page=%u, total_pages=%u, max_order=%u\n",
           base_page, total_pages, max_order);
    printf("buddy_init: kernel_reserved_pages=%u (%u MB)\n",
           kernel_reserved_pages, (kernel_reserved_pages * 4096) / (1024 * 1024));

    // 计算需要的块数量
    uint32_t max_blocks = total_pages + max_order;

    printf("buddy_init: using pre-allocated memory at 0x%x\n", memory_start);

    // 使用预分配的内存
    uint8_t* mem_ptr = (uint8_t*)memory_start;
    uint32_t blocks_size = max_blocks * sizeof(buddy_block_t);
    uint32_t freelists_size = (max_order + 1) * sizeof(uint32_t);
    uint32_t nextfree_size = max_blocks * sizeof(uint32_t);

    printf("buddy_init: max_blocks=%u, sizeof(buddy_block_t)=%u\n",
           max_blocks, sizeof(buddy_block_t));
    printf("buddy_init: blocks_size=%u, freelists_size=%u, nextfree_size=%u\n",
           blocks_size, freelists_size, nextfree_size);

    buddy_blocks_array = (buddy_block_t*)mem_ptr;
    mem_ptr += blocks_size;
    free_lists_array = (uint32_t*)mem_ptr;
    mem_ptr += freelists_size;
    next_free_array = (uint32_t*)mem_ptr;

    allocated_blocks = max_blocks;

    // 使用预分配的内存
    buddy_sys.blocks = buddy_blocks_array;
    buddy_sys.free_lists = free_lists_array;
    buddy_sys.next_free = next_free_array;

    printf("buddy_init: buddy_blocks_array=%p\n", buddy_blocks_array);
    printf("buddy_init: free_lists_array=%p\n", free_lists_array);
    printf("buddy_init: next_free_array=%p\n", next_free_array);

    printf("buddy_init: skipping write test, proceeding to memset...\n");

    // 初始化空闲链表
    printf("buddy_init: free_lists=%p, size=%u bytes\n",
           buddy_sys.free_lists, (max_order + 1) * sizeof(uint32_t));

    memset(buddy_sys.free_lists, 0xFF, (max_order + 1) * sizeof(uint32_t));
    printf("buddy_init: memset free_lists done\n");

    memset(buddy_sys.next_free, 0xFF, max_blocks * sizeof(uint32_t));
    printf("buddy_init: memset next_free done\n");

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
    buddy_sys.kernel_reserved_pages = kernel_reserved_pages;

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
    return buddy_alloc_type(order, MEM_ALLOC_ANY);
}

// 按类型分配内存块
uint32_t buddy_alloc_type(uint32_t order, uint8_t alloc_type) {
    uint32_t i, j;
    uint32_t block_index, buddy_block_index;
    uint32_t page = 0;

    // 验证 order
    if (order < buddy_sys.min_order || order > buddy_sys.max_order) {
        return 0;
    }

    // 查找合适的空闲块
    for (i = order; i <= buddy_sys.max_order; i++) {
        if (buddy_sys.free_lists[i] != 0xFFFFFFFF) {
            // 找到空闲块,检查是否满足类型要求
            block_index = buddy_sys.free_lists[i];
            uint32_t start_page = buddy_sys.blocks[block_index].start_page;
            uint32_t block_pages = 1 << i;

            // 检查内存类型限制
            bool is_kernel_mem = false;
            if (buddy_sys.kernel_reserved_pages > 0) {
                uint32_t reserved_end = buddy_sys.base_page + buddy_sys.kernel_reserved_pages;
                // 块在内核保留区域内
                if (start_page < reserved_end) {
                    is_kernel_mem = true;
                }
            }

            // 如果请求内核内存,但块不在保留区域内,跳过
            if (alloc_type == MEM_ALLOC_KERNEL && !is_kernel_mem) {
                continue;
            }

            // 如果请求用户内存,但块在内核保留区域内,跳过
            if (alloc_type == MEM_ALLOC_USER && is_kernel_mem) {
                continue;
            }

            // 找到合适的块,进行分配
            buddy_sys.free_lists[i] = buddy_sys.next_free[block_index];

            buddy_sys.blocks[block_index].status = BLOCK_USED;
            buddy_sys.blocks[block_index].alloc_type = alloc_type;
            buddy_sys.free_blocks--;

            // 如果块太大,分割成更小的块
            while (i > order) {
                i--;

                // 创建新的块
                buddy_block_index = buddy_sys.total_blocks++;
                buddy_sys.blocks[buddy_block_index].order = i;
                buddy_sys.blocks[buddy_block_index].status = BLOCK_FREE;
                buddy_sys.blocks[buddy_block_index].alloc_type = MEM_ALLOC_ANY;
                buddy_sys.blocks[buddy_block_index].start_page =
                    buddy_sys.blocks[block_index].start_page + (1 << i);
                buddy_sys.blocks[buddy_block_index].count = 1 << i;

                // 添加到空闲链表
                buddy_sys.next_free[buddy_block_index] = buddy_sys.free_lists[i];
                buddy_sys.free_lists[i] = buddy_block_index;

                buddy_sys.free_blocks++;
            }

            page = buddy_sys.blocks[block_index].start_page;
            buddy_sys.blocks[block_index].alloc_type = alloc_type;
            break;
        }
    }

    return page;
}

// 释放内存块
int buddy_free(uint32_t page, uint32_t order) {
    uint32_t i;
    uint32_t block_index = 0xFFFFFFFF;
    uint32_t buddy_page, buddy_block_index;

    // 查找对应的块（不需要精确匹配 order，只要找到该页的已使用块即可）
    for (i = 0; i < buddy_sys.total_blocks; i++) {
        if (buddy_sys.blocks[i].start_page == page &&
            buddy_sys.blocks[i].status == BLOCK_USED) {
            block_index = i;
            // 使用实际块的 order，而不是传入的 order
            order = buddy_sys.blocks[i].order;
            break;
        }
    }

    if (block_index == 0xFFFFFFFF) {
        printf("buddy_free: failed to find block at page %u (order=%u)\n", page, order);
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

    return 0;
}

// 获取内存统计信息
void buddy_stats(uint32_t* free_pages, uint32_t* used_pages, uint32_t* total_pages) {
    uint32_t i;
    uint32_t free = 0;

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

// 获取空闲页数
uint32_t buddy_get_free_pages(void) {
    uint32_t free_pages = 0;

    for (uint32_t i = 0; i < buddy_sys.total_blocks; i++) {
        if (buddy_sys.blocks[i].status == BLOCK_FREE) {
            free_pages += (1 << buddy_sys.blocks[i].order);
        }
    }

    return free_pages;
}

// 获取已用页数
uint32_t buddy_get_used_pages(void) {
    uint32_t used_pages = 0;

    for (uint32_t i = 0; i < buddy_sys.total_blocks; i++) {
        if (buddy_sys.blocks[i].status == BLOCK_USED) {
            used_pages += (1 << buddy_sys.blocks[i].order);
        }
    }

    return used_pages;
}uint32_t buddy_get_total_pages(void) {
    return buddy_sys.total_pages;
}
