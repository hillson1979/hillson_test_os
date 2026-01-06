#include "page.h"
#include "task.h"
#include "mm/buddy.h"
#include "mm/slab.h"
struct kernel_mem_mapping {
    uint32_t phys_addr;
    uint32_t virt_addr;
    uint32_t size;
    uint32_t flags;
    bool in_use;
    const char* description;
};
int km_init(void);
int mm_init(void);
void print_memory_detection_result(void);
page_entry_t *copy_pde_(task_t *task);

uint32_t *copy_pde(task_t *task);

// Physical Memory Manager
void pmm_init(void);
uint32_t pmm_alloc_page(void);
uint32_t pmm_alloc_page_type(uint8_t alloc_type);
void pmm_free_page(uint32_t phys_addr);
uint32_t pmm_alloc_pages(uint32_t count);
uint32_t pmm_alloc_pages_type(uint32_t count, uint8_t alloc_type);
void pmm_free_pages(uint32_t phys_addr, uint32_t count);
void pmm_print_stats(void);

// User Space Memory Allocation
uint32_t umem_alloc_pages(uint32_t count);  // 分配用户空间物理页
void umem_free_pages(uint32_t phys_addr, uint32_t count);  // 释放用户空间物理页

// Memory Testing Functions
void test_memory_management(void);
void test_kernel_user_memory_isolation(void);

