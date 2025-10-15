// highmem_mapping.c
#include "highmem_mapping.h"
#include "page.h"

#include "string.h"

static struct highmem_mapping mappings[64];
static uint32_t next_virt_addr = DYNAMIC_MAP_WINDOW_BASE;

static inline void x86_refresh_tlb(void)
{
    __asm__ volatile (
        "mov %%cr3, %%eax\n\t"
        "mov %%eax, %%cr3"
        : : : "eax", "memory"
    );
}

void init_highmem_mapping(void) {
    memset(mappings, 0, sizeof(mappings));
    next_virt_addr = DYNAMIC_MAP_WINDOW_BASE;
    
    printf("Highmem mapping initialized\n");

    printf("Identity mapped: 0x%x-0x%x\n", 
           KERNEL_VIRT_BASE, KERNEL_VIRT_BASE + IDENTITY_MAP_SIZE - 1);

    identity_map_8m_4k(DYNAMIC_MAP_WINDOW_BASE);
    
}

// 核心映射函数
void* map_highmem_physical(uint32_t phys_addr, uint32_t size, uint32_t flags) {
    // 如果地址在恒等映射区域内，直接转换
    /*if (IS_IDENTITY_MAPPED(phys_addr)) {
        return PHYS_TO_VIRT(phys_addr);
    }*/
    //#define printf(...)
    
    printf("no Identity mapped---------\n");
    // 检查是否已经映射
    void* existing = get_mapped_address(phys_addr);
    if (existing) {
        return existing;
    }
    
    // 查找空闲映射槽
    int free_slot = -1;
    for (int i = 0; i < 64; i++) {
        if (!mappings[i].in_use) {
            free_slot = i;
            break;
        }
    }
    
    if (free_slot == -1) {
        printf("ERROR: No free mapping slots!\n");
        return NULL;
    }
    
    // 计算对齐
    uint32_t aligned_phys = phys_addr & ~0xFFF;
    uint32_t offset = phys_addr & 0xFFF;
    uint32_t total_size = ((size + offset + 4095) / 4096) * 4096;
    
    // 检查窗口边界
    if (next_virt_addr + total_size > DYNAMIC_MAP_WINDOW_BASE + DYNAMIC_MAP_WINDOW_SIZE) {
        printf("ERROR: Dynamic mapping window full!\n");
        return NULL;
    }
    
    uint32_t virt_base = next_virt_addr;
    
    // 创建页表映射（使用4KB页）
    uint32_t page_flags = 0x3; // Present + RW
    if (flags & 0x10) page_flags |= 0x10; // PCD=1 (Uncached)
    if (flags & 0x8) page_flags |= 0x8;   // PWT=1 (Write-Through)
    
    int i=0;
    for (uint32_t addr = aligned_phys; addr < aligned_phys + total_size; addr += 4096) {
        uint32_t current_virt = virt_base + (addr - aligned_phys);
        //printf("before i is %d!\n",i++);
        map_4k_page(addr, current_virt, page_flags);

        //printf("after i is %d!\n",i);
    }
    
    // 记录映射
    mappings[free_slot].phys_addr = phys_addr;
    mappings[free_slot].virt_addr = virt_base + offset;
    mappings[free_slot].size = size;
    mappings[free_slot].flags = flags;
    mappings[free_slot].in_use = true;
    mappings[free_slot].description = "Highmem mapping";
    
    printf("Mapped highmem: phys 0x%x -> virt 0x%x (size: %u)\n",
           phys_addr, virt_base + offset, size);
    
    next_virt_addr += total_size;
    return (void*)(virt_base + offset);
}

// 获取已映射的地址
void* get_mapped_address(uint32_t phys_addr) {
    if (IS_IDENTITY_MAPPED(phys_addr)) {
        return PHYS_TO_VIRT(phys_addr);
    }
    
    for (int i = 0; i < 64; i++) {
        if (mappings[i].in_use && 
            phys_addr >= mappings[i].phys_addr &&
            phys_addr < mappings[i].phys_addr + mappings[i].size) {
            uint32_t offset = phys_addr - mappings[i].phys_addr;
            return (void*)(mappings[i].virt_addr + offset);
        }
    }
    return NULL;
}

