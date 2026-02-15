// highmem_mapping.c
#include "highmem_mapping.h"
#include "page.h"

#include "string.h"

// 外部函数声明
extern void identity_map_8m_4k(uint32_t addr);

static struct highmem_mapping mappings[512];  // Increased from 64 to 512 for large ELF files
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

    // boot.s 中设置的映射：
    // pd[0]   = pt (物理 0-4MB → 虚拟 0x00000000-0x003FFFFF)
    // pd[0x300] = pt (物理 0-4MB → 虚拟 0xC0000000-0xC03FFFFF)
    // pd[0x301] = pt (物理 0-4MB → 虚拟 0xC0400000-0xC07FFFFF，重复映射！)
    //
    // 实际上只有前 4MB 物理内存被映射到内核虚拟地址空间

    printf("Identity mapped: 0x%x-0x%x\n",
           KERNEL_VIRT_BASE, KERNEL_VIRT_BASE + 0x400000 - 1);

    // 🔥 不再预映射 Buddy System 区域！使用直接映射 (PHYS_TO_VIRT)
    // 预映射会破坏内存布局，导致系统崩溃
    //
    // 旧的代码（已禁用）：
    // // 为 Buddy System 数据区域建立页表映射（16MB-48MB）
    // printf("Mapping buddy system metadata area (phys 16MB-48MB, 32MB total)...\n");
    // for (uint32_t phys = 0x1000000; phys < 0x3000000; phys += 0x1000) {
    //     uint32_t virt = 0xC1000000 + (phys - 0x1000000);
    //     map_4k_page(phys, virt, 0x3);
    // }

    // 不再预映射整个 4GB 地址空间（这需要约 1GB 页表内存，会导致实体机闪退）
    // 改为按需映射：Buddy System 只维护物理页的链表，不需要预先映射
    // 当真正需要访问某个物理页时，再通过 PHYS_TO_VIRT 宏直接计算虚拟地址
    //
    // 虚拟地址空间布局：
    //   用户空间：0x00000000 - 0xBFFFFFFF (0-3GB)
    //   内核空间：0xC0000000 - 0xFFFFFFFF (3-4GB)
    //     - 恒等映射：0xC0000000 - 0xC07FFFFF (前 8MB 物理内存)
    //     - 直接映射：0xC0000000 + 物理地址（剩余物理内存通过计算访问）
    //
    // 注意：物理地址超过 8MB 的内存，通过 KERNEL_VIRT_BASE + phys_addr 计算
    // 这种方式不需要建立页表，因为页目录会自动处理

    printf("Buddy System will use direct mapping (PHYS_TO_VIRT) instead of pre-mapping\n");
    printf("Virtual address layout:\n");
    printf("  User space:   0x00000000 - 0xBFFFFFFF (0-3GB)\n");
    printf("  Kernel space: 0xC0000000 - 0xFFFFFFFF (3-4GB)\n");
    printf("    Identity mapped: 0xC0000000 - 0xC07FFFFF (first 8MB)\n");
    printf("    Direct mapping:  0xC0800000+ (computed as KERNEL_VIRT_BASE + phys)\n");

}

// 核心映射函数
void* map_highmem_physical(uint32_t phys_addr, uint32_t size, uint32_t flags) {
    // 如果地址在恒等映射区域内，直接转换
    /*if (IS_IDENTITY_MAPPED(phys_addr)) {
        return PHYS_TO_VIRT(phys_addr);
    }*/
    //#define printf(...)
    
    //printf("no Identity mapped---------\n");
    // 检查是否已经映射
    void* existing = get_mapped_address(phys_addr);
    if (existing) {
        return existing;
    }
    
    // 查找空闲映射槽
    int free_slot = -1;
    for (int i = 0; i < 512; i++) {
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

    // ⚠️ 关键修复：刷新 TLB，确保新的页表映射生效
    x86_refresh_tlb();

    // 记录映射
    mappings[free_slot].phys_addr = phys_addr;
    mappings[free_slot].virt_addr = virt_base + offset;
    mappings[free_slot].size = size;
    mappings[free_slot].flags = flags;
    mappings[free_slot].in_use = true;
    mappings[free_slot].description = "Highmem mapping";

   // printf("Mapped highmem: phys 0x%x -> virt 0x%x (size: %u)\n",
        //   phys_addr, virt_base + offset, size);

    next_virt_addr += total_size;
    return (void*)(virt_base + offset);
}

// 获取已映射的地址
void* get_mapped_address(uint32_t phys_addr) {
    if (IS_IDENTITY_MAPPED(phys_addr)) {
        return PHYS_TO_VIRT(phys_addr);
    }

    for (int i = 0; i < 512; i++) {
        if (mappings[i].in_use &&
            phys_addr >= mappings[i].phys_addr &&
            phys_addr < mappings[i].phys_addr + mappings[i].size) {
            uint32_t offset = phys_addr - mappings[i].phys_addr;
            return (void*)(mappings[i].virt_addr + offset);
        }
    }
    return NULL;
}

