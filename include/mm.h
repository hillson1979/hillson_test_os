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
void pmm_free_page(uint32_t phys_addr);
uint32_t pmm_alloc_pages(uint32_t count);
void pmm_free_pages(uint32_t phys_addr, uint32_t count);
void pmm_print_stats(void);

