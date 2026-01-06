#include "printf.h"
#include "page.h"
#include "highmem_mapping.h"
#include "memlayout.h"
#include "x86/mmu.h"

uint32_t high_page_directory[1024] __attribute__((aligned(4096)));
// 1. 内存常量定义（需根据实际硬件内存布局调整）
//#define PAGE_SIZE     0x1000          // 4KB页大小

// 2. 空闲页链表节点：每个节点代表1个空闲物理页，记录下一个空闲页地址
typedef struct free_page_node {
    struct free_page_node *next; // 指向下一个空闲页的节点（物理地址）
} free_page_node_t;

// 3. 全局内存管理变量（需在系统初始化时初始化）
static free_page_node_t *free_page_list = NULL; // 空闲页链表头
static uint8_t page_used[TOTAL_PAGES] = {0};    // 页使用标记：0=空闲，1=已分配（辅助校验）

// 页表数组（指向所有页表的指针数组）
pde_t *page_directory=pd;
pte_t *page_tables=pt;
extern pt3[1024],pt4[1024];
extern map_flag;


/**
 * 使用 4KB 页恒等映射 8MB 空间
 *
 * @param pd      : 页目录表基地址（物理地址，1024 项）
 * @param addr    : 起始虚拟地址（建议 4KB 对齐）
 * @param pt1     : 第一个页表基地址（物理地址，映射前 4MB）
 * @param pt2     : 第二个页表基地址（物理地址，映射后 4MB）
 *
 * 注意：
 * - pt1 和 pt2 必须是 4KB 对齐的物理地址
 * - 调用者负责分配这两个页表（如 memset(pt1, 0, 4096)）
 */
void identity_map_8m_4k( uint32_t addr)
{
    uint32_t i;
    uint32_t *pd=page_directory;
    
    uint32_t start = addr & ~0xFFF;  // 4KB 对齐起始地址
    uint32_t end   = start + 0x800000; // 8MB 结束地址

    // 页目录索引范围
    uint32_t pde_idx1 = (start >> 22) & 0x3FF; // 第一个 PDE
    uint32_t pde_idx2 = (end - 1 >> 22) & 0x3FF; // 最后一个 PDE（应为 pde_idx1 或 +1）

    // 清空两个页表（可选，确保无残留）
    for (i = 0; i < 1024; i++) {
        pt3[i] = 0;
        pt4[i] = 0;
    }

    // 填写第一个页表（映射前 4MB）
    for (i = 0; i < 1024; i++) {
        uint32_t phys_addr = start + (i << 12); // 每项 4KB
        pt3[i] = phys_addr | 0x03; // P=1, R/W=1
    }

    // 填写第二个页表（映射后 4MB）
    for (i = 0; i < 1024; i++) {
        uint32_t phys_addr = start + 0x400000 + (i << 12);
        pt4[i] = phys_addr | 0x03; // P=1, R/W=1
    }

    // 构造页目录项：指向页表，P=1, R/W=1, US=0
    uint32_t pde1 = ((uint32_t)pt3) | 0x03; // 指向第一个页表
    uint32_t pde2 = ((uint32_t)pt4) | 0x03; // 指向第二个页表

    // 使用内联汇编写入页目录
    __asm__ volatile (
        "movl %0, %%eax\n\t"
        "movl %%eax, %1\n\t"         // pd[pde_idx1] = pde1
        "movl %2, %%eax\n\t"
        "movl %%eax, %3\n\t"         // pd[pde_idx2] = pde2
        :
        : "r"(pde1), "m"(pd[pde_idx1]),
          "r"(pde2), "m"(pd[pde_idx2])
        : "eax", "memory"
    );

    // 注意：TLB 不会自动更新
    // 建议在启用分页后使用 invlpg 或 reload cr3
    // 刷新TLB
    __asm__ volatile ("invlpg (%0)" : : "r" (addr) : "memory");
}
// 常量定义（x86 32位）
//#define PAGE_SIZE        0x1000       // 4KB页大小
#define PAGE_MASK        ~0xFFF       // 页对齐掩码
#define PTRS_PER_PTE     1024         // 每个页表的PTE数量
#define USER_VA_END      0xC0000000   // 用户态虚拟地址上限（内核空间起始）


// 权限位定义
#define PTE_PRESENT      0x001        // 存在位
#define PTE_RW           0x002        // 读写位
#define PTE_US           0x004        // 用户态位（0=内核态，1=用户态）
#define PTE_KERNEL       (PTE_PRESENT | PTE_RW)  // 内核页表项权限
#define PTE_USER         (PTE_PRESENT | PTE_RW | PTE_US)  // 用户页表项权限



#define PAGE_SIZE 0x1000
#define PDE_COUNT 1024
#define PTE_COUNT 1024

#define PAGE_PRESENT 0x1
#define PAGE_WRITE   0x2
#define PAGE_USER    0x4

extern uint32_t pmm_alloc_page();
//extern void* phys_to_virt(uintptr_t phys);

// 前置声明
static uint32_t alloc_early_page_table(void);

// 在页目录 pde_phys 里，把 vaddr -> paddr 建立映射
void map_page(uint32_t pde_phys, uint32_t vaddr, uint32_t paddr, uint32_t flags) {
    uint32_t *pd_user = (uint32_t*)phys_to_virt(pde_phys);

    // 页目录索引和页表索引
    uint32_t pd_index = vaddr >> 22;
    uint32_t pt_index = (vaddr >> 12) & 0x3FF;

    // 页表地址
    if (!(pd_user[pd_index] & PAGE_PRESENT)) {
        // 页表不存在，分配一个物理页
        uint32_t pt_phys = pmm_alloc_page();
        printf("[map_page] vaddr=0x%x new PT at phys=0x%x\n", vaddr, pt_phys);

        // 确保这个物理页在内核页目录中有映射（按需映射）
        uint32_t pt_virt_addr = (uint32_t)phys_to_virt(pt_phys);
        uint32_t kernel_pd_index = pt_virt_addr >> 22;
        uint32_t kernel_pt_index = (pt_virt_addr >> 12) & 0x3FF;

        // 检查内核页目录是否有对应的页表
        if (!(pd[kernel_pd_index] & PAGE_PRESENT)) {
            // 内核页目录也没有页表，使用早期页表分配器创建一个
            uint32_t kernel_pt_phys = alloc_early_page_table();
            if (kernel_pt_phys == 0) {
                printf("[map_page] ERROR: Failed to allocate kernel page table!\n");
                return;
            }

            // 填写内核页目录
            pd[kernel_pd_index] = kernel_pt_phys | 0x3;
        }

        // 检查内核页表中是否有这个具体的页映射
        uint32_t *kernel_pt = (uint32_t*)phys_to_virt(pd[kernel_pd_index] & ~0xFFF);

        if (!(kernel_pt[kernel_pt_index] & PAGE_PRESENT)) {
            // 为内核创建这个物理页的映射
            kernel_pt[kernel_pt_index] = pt_phys | 0x3;
            // 刷新 TLB
            __asm__ volatile ("invlpg (%0)" : : "r" (pt_virt_addr) : "memory");
        }

        // 清零页表
        uint32_t *pt_virt = (uint32_t*)pt_virt_addr;
        memset(pt_virt, 0, PAGE_SIZE);

        // 填写用户页目录的 PDE
        pd_user[pd_index] = (pt_phys & ~0xFFF) | (flags & 0xFFF) | PAGE_PRESENT;
        printf("[map_page] Set pd_user[%u]=0x%x\n", pd_index, pd_user[pd_index]);
    }

    // 得到页表虚拟地址
    uint32_t *pt = (uint32_t*)phys_to_virt(pd_user[pd_index] & ~0xFFF);

    // 填写 PTE
    pt[pt_index] =  (paddr & ~0xFFF) | (flags & 0xFFF) | PAGE_PRESENT;
    printf("[map_page] Set pt[%u]=0x%x (vaddr=0x%x -> paddr=0x%x)\n", pt_index, pt[pt_index], vaddr, paddr);
}


// 早期页表分配器 - 使用已映射的物理内存
// 实际映射情况 (boot.s):
//   pd[0]   = pt   -> 物理地址 0-4MB 映射到虚拟地址 0x00000000-0x003FFFFF
//   pd[0x300] = pt  -> 物理地址 0-4MB 映射到虚拟地址 0xC0000000-0xC03FFFFF
//   pd[0x301] = pt  -> 物理地址 0-4MB 映射到虚拟地址 0xC0400000-0xC07FFFFF (重复!)
//
// 实际上只有前4MB物理内存被映射!
// - 内核代码/数据占用约1MB (0x100000-0x200000)
// - 可用空间: 2MB-4MB (0x200000-0x400000)
static uint32_t early_pt_alloc_addr = 0x200000;  // 从2MB开始
#define EARLY_PT_ALLOC_END 0x400000  // 到4MB结束(可分配512个页表)

static uint32_t alloc_early_page_table(void) {
    if (early_pt_alloc_addr >= EARLY_PT_ALLOC_END) {
        printf("alloc_early_page_table: ERROR - Out of space!\n");
        return 0;
    }
    uint32_t pt_phys = early_pt_alloc_addr;
    early_pt_alloc_addr += 0x1000;  // 分配4KB

    // 清零新页表(使用已映射的虚拟地址)
    uint32_t* pt_virt = (uint32_t*)phys_to_virt(pt_phys);
    for (int i = 0; i < 1024; i++) {
        pt_virt[i] = 0;
    }

    printf("alloc_early_page_table: allocated phys=0x%x, virt=0x%x\n", pt_phys, pt_virt);
    return pt_phys;
}

// 分配页表（在恒等映射区域内）
void* alloc_page_table(uint32_t virt_addr, uint32_t phys_addr,uint32_t flags) {
    //#define printf(...)
    uint32_t addr = virt_addr & ~0xFFF;  // 4KB 对齐起始地址
    uint32_t pd_index = (addr >> 22) & 0x3FF;

    printf("alloc_page_table: virt=0x%x, phys=0x%x, pd_idx=%u\n", virt_addr, phys_addr, pd_index);
    printf("  pd[pd_idx]=0x%x\n", pd[pd_index]);

    // 检查页表是否已存在
    if (!(pd[pd_index] & 0x1)) {
        // 页表不存在,需要分配
        printf("  Page table not present, allocating...\n");
        uint32_t pt_phys = alloc_early_page_table();
        printf("  Allocated PT at phys=0x%x\n", pt_phys);

        if (pt_phys == 0) {
            printf("  ERROR: Failed to allocate page table!\n");
            return NULL;
        }

        // 填写页目录项(使用物理地址!)
        uint32_t pde_value = pt_phys | 0x03;  // P=1, RW=1
        printf("  Writing PDE: pd[%u] = 0x%x\n", pd_index, pde_value);
        __asm__ volatile (
            "movl %0, %%eax\n\t"
            "movl %%eax, %1\n\t"
            :
            : "r"(pde_value), "m"(pd[pd_index])
            : "eax", "memory"
        );
        printf("  pd[%u] now = 0x%x\n", pd_index, pd[pd_index]);
    }

    // 获取页表虚拟地址
    uint32_t* pt_ = (uint32_t*)phys_to_virt(pd[pd_index] & ~0xFFF);
    printf("  pt_virt = 0x%x (from pd[%u]=0x%x)\n", pt_, pd_index, pd[pd_index]);

    if ((uint32_t)pt_ < 0xC0000000) {
        printf("  ERROR: pt_ not in kernel space!\n");
        return NULL;
    }

    // 清空页表(如果已存在)
    printf("  Clearing page table...\n");
    for (uint32_t i = 0; i < 1024; i++) {
        pt_[i] = 0;
    }

    // 填写页表项
    printf("  Filling PTEs: phys=0x%x -> virt=0x%x\n", phys_addr, virt_addr);
    for (uint32_t i = 0; i < 1024; i++) {
        pt_[i] = (phys_addr + (i << 12)) | flags;
    }

    // 刷新TLB
    printf("  Flushing TLB for virt=0x%x\n", virt_addr);
    __asm__ volatile ("invlpg (%0)" : : "r" (virt_addr) : "memory");

    printf("  Done!\n");
    return (void*)virt_addr;
}

// 映射4KB页（针对高位内核布局）
void map_4k_page(uint32_t phys_addr, uint32_t virt_addr, uint32_t flags) {
    uint32_t pd_index = (virt_addr >> 22) & 0x3FF;
    uint32_t pt_index = (virt_addr >> 12) & 0x3FF;
    uint32_t* high_page_directory = (uint32_t*)pd;
    
    // 确保页目录项存在
    if (!(high_page_directory[pd_index] & 0x1)) {
        // 分配新页表（在above 8MB）
        printf("Allocating new page table for directory entry %u\n", pd_index);
        
        uint32_t* new_pt = alloc_page_table(virt_addr, phys_addr,flags);
        printf("Allocated new page table for directory entry 0x%x\n", new_pt);
        if (!new_pt) {
            printf("ERROR: Failed to allocate page table!\n");
            return;
        }
        //new_pt[pt_index] = (phys_addr & ~0xFFF)| flags;//phys_addr| 0x3;  // 现在可以通过虚拟地址访问原数据
        //high_page_directory[pd_index] = (uint32_t)new_pt | 0x3;
        // 刷新TLB

        printf("Allocated new page table for directory entry 0x%x\n", new_pt);
        printf("phys_addr is 0x%x\n", phys_addr);        
        printf("virt_addr is 0x%x\n", virt_addr);       
        printf("pte is 0x%x\n", new_pt[pt_index]);         
        __asm__ volatile ("invlpg (%0)" : : "r" (virt_addr) : "memory");
        return;
    }
    
    // 获取页表
    uint32_t* page_table =(uint32_t*)(high_page_directory[pd_index] & ~0xFFF);//high_page_directory[pd_index];//
    
    // 设置页表项
    page_table[pt_index] = (phys_addr & ~0xFFF)| flags;//phys_addr | flags;
    
    // 刷新TLB
    __asm__ volatile ("invlpg (%0)" : : "r" (virt_addr) : "memory");

    //printf("virt_addr is %x\n",virt_addr);
}


// 支持多页映射的扩展函数（基于原mappages）
pde_user_t * mappages_multi( uint32_t va, uint32_t pa, int len, uint32_t perm) {
    // 计算需要映射的总页数（向上取整，如5115字节→1页，5000字节→2页）
    pde_user_t *pg_dir;
    uint32_t* new_pt;
    int total_pages = (len + PAGE_SIZE - 1) / PAGE_SIZE;
    for (int i = 0; i < total_pages; i++) {
        // 循环调用单页mappages，依次映射每个连续页
        if (new_pt=alloc_page_table( va + i*PAGE_SIZE, pa + i*PAGE_SIZE, perm)) {
           if (i==1) {
                   pg_dir=(pde_user_t *)new_pt;
           }
        }else {return -1;} // 任一页面映射失败则整体返回失败
    }
    return pg_dir;
}

/**
 * @brief 分配1个4KB对齐的空闲物理页
 * @return 成功：返回空闲页的物理地址（void*类型，需强转为对应指针）；失败：返回NULL
 */
void *get_free_phys_page(void) {
    // 1. 检查空闲页链表是否为空
    if (free_page_list == NULL) {
        return NULL; // 无空闲物理页，分配失败
    }

    // 2. 从链表头取出一个空闲页（链表头指针后移）
    free_page_node_t *allocated_page = free_page_list;
    free_page_list = free_page_list->next;

    // 3. 标记该页为已分配（避免重复分配）
    uint32_t page_idx = ((uint32_t)allocated_page - PHYS_MEM_BASE_USER) / PAGE_SIZE;
    page_used[page_idx] = 1;

    // 4. 清空页内容（可选但推荐，避免旧数据泄露）
    //memset(allocated_page, 0, PAGE_SIZE);

    // 5. 返回分配的物理页地址（强转为void*，适配不同指针类型需求）
    return (void *)allocated_page;
}

/**
 * @brief 释放已分配的物理页（配套函数，避免内存泄漏）
 * @param page_phys 待释放的物理页地址（必须是get_free_phys_page分配的4KB对齐地址）
 */
void free_phys_page(void *page_phys) {
    // 1. 校验地址合法性（必须是4KB对齐 + 在可用物理内存范围内）
    uint32_t page_addr = (uint32_t)page_phys;
    if ((page_addr & (PAGE_SIZE - 1)) != 0 ||  // 地址未4KB对齐
        page_addr < PHYS_MEM_BASE_USER ||           // 地址低于物理内存起始点
        page_addr >= PHYS_MEM_BASE_USER + PHYS_MEM_SIZE) { // 地址超出可用内存范围
        return; // 非法地址，不执行释放
    }

    // 2. 校验该页是否已分配（避免重复释放）
    uint32_t page_idx = (page_addr - PHYS_MEM_BASE_USER) / PAGE_SIZE;
    if (page_used[page_idx] != 1) {
        return; // 页未分配或已释放，不执行操作
    }

    // 3. 将释放的页加入空闲页链表头部（链表头插法，高效）
    free_page_node_t *freed_node = (free_page_node_t *)page_phys;
    freed_node->next = free_page_list;
    free_page_list = freed_node;

    // 4. 标记该页为空闲
    page_used[page_idx] = 0;
}

