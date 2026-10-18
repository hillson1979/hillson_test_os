#ifndef MM_BUDDY_H
#define MM_BUDDY_H

#include "types.h"

#define MAX_ORDER 20
#define PAGE_SIZE 4096
#define BLOCK_FREE 0
#define BLOCK_USED 1
#define MEM_ALLOC_KERNEL 0
#define MEM_ALLOC_USER 1
#define MEM_ALLOC_ANY 2
typedef struct { uint8_t order, status, alloc_type, reserved; uint32_t start_page, count; } buddy_block_t;
typedef struct { buddy_block_t *blocks; uint32_t *free_lists, *next_free; uint32_t total_blocks, free_blocks, min_order, max_order, base_page, total_pages, kernel_reserved_pages; } buddy_system_t;
int buddy_init_with_memory(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
uint32_t buddy_alloc_type(uint32_t, uint8_t);
int buddy_free(uint32_t, uint32_t);
void buddy_stats(uint32_t *, uint32_t *, uint32_t *);
uint32_t pages_to_order(uint32_t);

/* Physical page allocator boundary. All callers receive byte addresses. */
void pmm_init(void);
uint32_t pmm_alloc_page(void);
uint32_t pmm_alloc_page_type(uint8_t alloc_type);
uint32_t pmm_alloc_pages(uint32_t count);
uint32_t pmm_alloc_pages_type(uint32_t count, uint8_t alloc_type);
void pmm_free_page(uint32_t phys_addr);
void pmm_free_pages(uint32_t phys_addr, uint32_t count);
void pmm_print_stats(void);

#endif
