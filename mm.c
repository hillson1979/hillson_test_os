#include "types.h"
#include "x86/mmu.h"
#include "memlayout.h"
//#include "page.h"
#include "mm.h"
#include "highmem_mapping.h"
#include "multiboot.h"
#include "mm/buddy.h"
#include "printf.h"
#define NELEM(x) (sizeof(x)/sizeof((x)[0]))

#define pde_t uint32_t
extern void* _data_start;  // defined by kernel.ld
extern char text[];
static struct kernel_mem_mapping mappings[64];

#define PDE_MASK 0xFFC00000
#define IDX(addr) ((uint32_t)addr >> 12)            // 获取 addr 的页索引
extern uint32_t task_total_count;

// This table defines the kernel's mappings, which are present in
// every process's page table.
static struct kmap {
  void *virt;
  uint32_t phys_start;
  uint32_t phys_end;
  int perm;
} kmap[] = {
// { (void*)KERNBASE, 0,             2*EXTMEM,    PTE_W}, // I/O space
 { (void*)(KERNLINK), V2P(KERNLINK), V2P(0x400000+KERNLINK), PTE_P},     // kern text+rodata
 { (void*)(0x400000+KERNLINK),     V2P(0x400000+KERNLINK), V2P(2 * 0x400000+KERNLINK) ,   PTE_W}, // kern data+memory
 //{ (void*)(KERNBASE-PHYS_MEM_SIZE), PHYS_MEM_BASE_USER,      PHYS_MEM_BASE_USER+PHYS_MEM_SIZE,  PTE_P|PTE_W|PTE_U}, // more devices
};


// 设置 cr3 寄存器，参数是页目录的地址
void set_cr3(uint32_t pde)
{
    //ASSERT_PAGE(pde);
    asm volatile("movl %%eax, %%cr3\n" ::"a"(pde));
}


// 获取已映射的地址
void* get_kmapped_address(uint32_t phys_addr) {
    
   //if (IS_IDENTITY_MAPPED(phys_addr)) {
     //   return PHYS_TO_VIRT(phys_addr);
    //}
    
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

// 核心映射函数 mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm)

void* mappages(void *virt_addr,uint32_t phys_addr, uint32_t size, uint32_t flags) {
    
    //#define printf(...)
    
    printf("in mappages function---------\n");
    // 检查是否已经映射
    void* existing = get_kmapped_address(phys_addr);
    if (existing) {
        return existing;
    }
    
    // 查找空闲映射槽
    int free_slot = -1;
    for (int i = 0; ; i++) {
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
     /*
    if (next_virt_addr + total_size > DYNAMIC_MAP_WINDOW_BASE + DYNAMIC_MAP_WINDOW_SIZE) {
        printf("ERROR: Dynamic mapping window full!\n");
        return NULL;
    }*/
    
    uint32_t virt_base = virt_addr;
    
    // 创建页表映射（使用4KB页）
    uint32_t page_flags = flags | PTE_P;//0x3; // Present + RW
    //if (flags & 0x10) page_flags |= 0x10; // PCD=1 (Uncached)
   // if (flags & 0x8) page_flags |= 0x8;   // PWT=1 (Write-Through)
    
    int i=0;
    for (uint32_t addr = aligned_phys; addr < aligned_phys + total_size; addr += 4096) {
        uint32_t current_virt = virt_base + (addr - aligned_phys);
        //printf("before addr is 0x%x!\n",addr);
        alloc_page_table(current_virt, addr,flags);
        //map_4k_page(addr, current_virt, page_flags);
        //printf("after addr is 0x%x!\n",addr);
        i++;
        //printf("i is %d!\n",i);  
     }
    printf("i is %d!\n",i);  
    // 记录映射
    mappings[free_slot].phys_addr = phys_addr;
    mappings[free_slot].virt_addr = virt_base + offset;
    mappings[free_slot].size = size;
    mappings[free_slot].flags = flags;
    mappings[free_slot].in_use = true;
    mappings[free_slot].description = "kernel mem mapping";
    
    printf("Mapped kernel mem: phys 0x%x -> virt 0x%x (size: %u)\n",
           phys_addr, virt_base + offset, size);
    
    //next_virt_addr += total_size;
    return (void*)(virt_base + offset);
}

// Set up kernel part of a page table.
int km_init(void)
{
  //pde_t *pgdir;
  struct kmap *k;
  int test=0;

/*  if((pgdir = (pde_t*)kalloc()) == 0)
    return 0;
  memset(pgdir, 0, PGSIZE);
*/
  if (PHYSTOP < V2P(KERNLINK)){
    printf("PHYSTOP too low");
    return -1;}


  if (test==1){
        map_from_addr=KERNLINK;
        if(mappages(KERNLINK, V2P(KERNLINK),0x100000,0) < 0) {
          printf("mappages1 error!");
          return -1;
         }
         map_from_addr=KERNLINK+0x100000;
         if(mappages(KERNLINK+0x100000, V2P(KERNLINK+0x100000),0x100000,0) < 0) {
          printf("mappages2 error!");
          return -1;
         }
       return 0;
  }

  for(k = kmap; k < &kmap[NELEM(kmap)]; k++){
    map_from_addr=(uint32_t)k->phys_start;
    if(mappages(k->virt, (uint32_t)k->phys_start,k->phys_end - k->phys_start, k->perm) < 0) {
      //freevm(pgdir);
      printf("mappages error!");
      return -1;
     }
  }
  return 0;
}

// ============ 内存管理初始化和辅助函数 ============

struct multiboot *multiboot_info;
extern void* _kernel_end_virtual;

// 内存管理初始化函数
int mm_init(void) {
    printf("mm_init: starting memory management initialization\n");

    if (!multiboot_info) {
        printf("mm_init: no multiboot info\n");
        return -1;
    }

    // 计算物理内存大小
    uint32_t mem_upper_kb = multiboot_info->mem_upper;
    uint32_t mem_upper_bytes = mem_upper_kb * 1024;
    uint32_t total_memory_mb = mem_upper_kb / 1024;

    printf("mm_init: detected %u MB physical memory (mem_upper=%u KB)\n",
           total_memory_mb, mem_upper_kb);

    // 初始化物理内存管理器 (PMM)
    printf("mm_init: initializing physical memory manager...\n");
    pmm_init();

    printf("mm_init: basic memory detection complete (buddy system disabled)\n");
    printf("mm_init: memory management initialization complete\n");
    return 0;
}

// 打印内存检测结果
void print_memory_detection_result(void) {
    if (!multiboot_info) {
        printf("=== Memory Detection ===\nNo multiboot info\n");
        return;
    }

    uint32_t mem_mb = multiboot_info->mem_upper / 1024;
    printf("=== Memory Detection Result ===\n");
    printf("Physical Memory: %u MB\n", mem_mb);
    printf("================================\n");
}

// 设置内核页目录(占位符)
pde_t *setupkvm(void) {
    // 简单返回0,表示暂未实现
    printf("setupkvm: not implemented\n");
    return 0;
}

// 初始化用户虚拟内存(占位符)
int inituvm(pde_t *pgdir, char *init, uint32_t sz) {
    return 0;
}

// 释放虚拟内存(占位符)
void freevm(pde_t *pgdir) {
    // 什么都不做
}
