#include "page.h"
#include "task.h"
struct kernel_mem_mapping {
    uint32_t phys_addr;
    uint32_t virt_addr;
    uint32_t size;
    uint32_t flags;
    bool in_use;
    const char* description;
};
int km_init(void);
page_entry_t *copy_pde_(task_t *task);

uint32_t *copy_pde(task_t *task);
