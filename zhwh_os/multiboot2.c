#include "multiboot2.h"
#include "highmem_mapping.h"
#include "page.h"
#include "printf.h"

/* 全局变量: 保存 multiboot2 信息结构地址 */
uint32_t multiboot2_info_addr = 0;

/* 辅助函数: 获取基本内存信息 */
void get_multiboot2_memory_info(uint32_t *mem_lower, uint32_t *mem_upper) {
    *mem_lower = 0;
    *mem_upper = 0;

    if (!multiboot2_info_addr) {
        return;
    }

    // Multiboot 2 信息结构：[0-3]大小, [4-7]保留, [8+]标签数组
    uint32_t *mb_info_ptr = (uint32_t *)phys_to_virt(multiboot2_info_addr);
    multiboot_tag_t *tag = (multiboot_tag_t *)((uint8_t *)mb_info_ptr + 8);

    while (tag->type != MULTIBOOT_TAG_TYPE_END) {
        if (tag->type == MULTIBOOT_TAG_TYPE_BASIC_MEMINFO) {
            multiboot_tag_basic_meminfo_t *meminfo = (multiboot_tag_basic_meminfo_t *)tag;
            *mem_lower = meminfo->mem_lower;
            *mem_upper = meminfo->mem_upper;
            return;
        }
        tag = (multiboot_tag_t *)((uint8_t *)tag + ((tag->size + 7) & ~7));
    }
}
