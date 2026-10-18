#include "types.h"
#include "mm.h"
#include "multiboot2.h"
#include "printf.h"

extern uint32_t multiboot2_info_addr;
extern void pmm_init(void);

int mm_init(void)
{
    printf("mm_init: starting memory management initialization\n");
    if (!multiboot2_info_addr) {
        printf("mm_init: no multiboot2 info\n");
        return -1;
    }

    uint32_t mem_lower_kb, mem_upper_kb;
    get_multiboot2_memory_info(&mem_lower_kb, &mem_upper_kb);
    printf("mm_init: detected %u MB physical memory (mem_upper=%u KB)\n",
           mem_upper_kb / 1024, mem_upper_kb);

    printf("mm_init: initializing physical memory manager...\n");
    pmm_init();
    printf("mm_init: memory management initialization complete\n");
    return 0;
}

void print_memory_detection_result(void)
{
    if (!multiboot2_info_addr) {
        printf("=== Memory Detection ===\nNo multiboot2 info\n");
        return;
    }

    uint32_t mem_lower_kb, mem_upper_kb;
    get_multiboot2_memory_info(&mem_lower_kb, &mem_upper_kb);
    printf("=== Memory Detection Result ===\n");
    printf("Physical Memory: %u MB\n", mem_upper_kb / 1024);
    printf("================================\n");
}
