#include "mm/user_vm.h"
#include "page.h"
#include "mm/buddy.h"
#include "highmem_mapping.h"
#include "string.h"

extern uint32_t pmm_alloc_page(void);

int user_vm_map_page(task_t *task, uint32_t va, uint32_t pa, uint32_t flags)
{
    if (!task || !task->cr3 || !pa) return -1;
    map_page((uint32_t)task->cr3, va, pa, flags | PAGE_USER);
    return 0;
}

uint32_t user_vm_alloc_page(task_t *task, uint32_t va, uint32_t flags)
{
    uint32_t pa = pmm_alloc_page();
    if (!pa || user_vm_map_page(task, va, pa, flags) != 0) return 0;
    void *mapped = map_physical_scratch(pa);
    if (mapped) memset(mapped, 0, PAGE_SIZE);
    return pa;
}

/* Legacy physical-page helpers used by the kernel memory isolation test. */
uint32_t umem_alloc_pages(uint32_t count)
{
    return pmm_alloc_pages_type(count, MEM_ALLOC_USER);
}

void umem_free_pages(uint32_t phys_addr, uint32_t count)
{
    pmm_free_pages(phys_addr, count);
}
