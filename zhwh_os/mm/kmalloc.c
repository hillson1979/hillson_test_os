
/* Kernel heap implementation. User pages are owned by mm/user_vm.c. */
#include "types.h"
#include "kmalloc.h"
#include "mm.h"
#include "page.h"
#include "task.h"
#include "highmem_mapping.h"
#include "printf.h"
#include "string.h"
#include "mm/buddy.h"

#define MAX_ALLOCATIONS 1024
struct allocation_header {
    void *virt_addr;
    uint32_t phys_addr;
    uint32_t size;
    uint32_t page_count;
    bool in_use;
};
static struct allocation_header alloc_table[MAX_ALLOCATIONS];
static uint32_t alloc_count;

static struct allocation_header *find_or_create_allocation(void *virt_addr)
{
    for (uint32_t i = 0; i < MAX_ALLOCATIONS; ++i) {
        if (!alloc_table[i].in_use) {
            alloc_table[i].virt_addr = virt_addr;
            alloc_table[i].in_use = true;
            if (i >= alloc_count) alloc_count = i + 1;
            return &alloc_table[i];
        }
    }
    return NULL;
}

static struct allocation_header *find_allocation(void *virt_addr)
{
    for (uint32_t i = 0; i < alloc_count; ++i)
        if (alloc_table[i].in_use && alloc_table[i].virt_addr == virt_addr)
            return &alloc_table[i];
    return NULL;
}

void *kernel_heap_alloc(unsigned int size)
{
    if (!size) return NULL;
    if (size <= 2048) {
        void *p = kmalloc_early(size);
        if (p) {
            struct allocation_header *h = find_or_create_allocation(p);
            if (h) { h->phys_addr = 0; h->size = size; h->page_count = 0; }
        }
        return p;
    }
    uint32_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    uint32_t phys = pmm_alloc_pages(pages);
    if (!phys) return NULL;
    void *virt = phys_to_virt(phys);
    extern uint32_t kernel_page_directory_phys;
    for (uint32_t i = 0; i < pages; ++i)
        map_page(kernel_page_directory_phys, (uint32_t)virt + i * PAGE_SIZE,
                 phys + i * PAGE_SIZE, PAGE_PRESENT | PAGE_WRITABLE);
    extern task_t *current_task[];
    extern int logical_cpu_id(void);
    task_t *cur = current_task[logical_cpu_id()];
    if (cur && cur->cr3 && (uint32_t)cur->cr3 != kernel_page_directory_phys) {
        uint32_t *src = (uint32_t *)phys_to_virt(kernel_page_directory_phys);
        uint32_t *dst = (uint32_t *)map_highmem_physical((uint32_t)cur->cr3, PAGE_SIZE, 0x3);
        if (dst) {
            uint32_t first = (uint32_t)virt >> 22;
            uint32_t last = ((uint32_t)virt + pages * PAGE_SIZE - 1) >> 22;
            for (uint32_t i = first; i <= last; ++i) dst[i] = src[i] & ~PAGE_USER;
        }
    }
    struct allocation_header *h = find_or_create_allocation(virt);
    if (h) { h->phys_addr = phys; h->size = size; h->page_count = pages; }
    printf("kmalloc: allocated %u bytes (%u pages) at virt=0x%x, phys=0x%x\n", size, pages, (uint32_t)virt, phys);
    return virt;
}

void *kmalloc(unsigned int size) { return kernel_heap_alloc(size); }

void kernel_heap_free(void *ptr)
{
    if (!ptr) return;
    struct allocation_header *h = find_allocation(ptr);
    if (!h) return;
    if (h->page_count) pmm_free_pages(h->phys_addr, h->page_count);
    h->in_use = false; h->virt_addr = NULL; h->phys_addr = 0; h->size = 0; h->page_count = 0;
}

void kfree(void *ptr) { kernel_heap_free(ptr); }

void *kzalloc(unsigned int size)
{
    uint8_t *p = (uint8_t *)kmalloc(size);
    if (p) memset(p, 0, size);
    return p;
}

void kmalloc_print_stats(void)
{
    uint32_t bytes = 0, pages = 0;
    for (uint32_t i = 0; i < alloc_count; ++i)
        if (alloc_table[i].in_use) { bytes += alloc_table[i].size; pages += alloc_table[i].page_count; }
    printf("kmalloc: active=%u bytes, %u pages\n", bytes, pages);
}


