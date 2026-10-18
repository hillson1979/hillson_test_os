#include "printf.h"
#include "page.h"
#include "highmem_mapping.h"
#include "memlayout.h"
#include "x86/mmu.h"
#include "string.h"

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
extern bool pmm_is_ready(void);
//extern void* phys_to_virt(uintptr_t phys);

// 前置声明
static uint32_t alloc_early_page_table(void);

/* Runtime page-table pages come from the buddy allocator.  The dynamic
 * highmem window cannot be used to access its own new PDE, so use the last
 * bootstrap direct-map PTE as a serialized scratch mapping. */
static void *kernel_scratch_phys(uint32_t phys)
{
    extern uint32_t kernel_page_directory_phys;
    const uint32_t va = KERNEL_VIRT_BASE + IDENTITY_MAP_SIZE - PAGE_SIZE;
    uint32_t *pd = (uint32_t *)phys_to_virt(kernel_page_directory_phys);
    uint32_t pdi = va >> 22;
    uint32_t pti = (va >> 12) & 0x3ff;
    if (!(pd[pdi] & PAGE_PRESENT)) return NULL;
    uint32_t *pt = (uint32_t *)phys_to_virt(pd[pdi] & ~0xfff);
    pt[pti] = (phys & ~0xfff) | PAGE_PRESENT | PAGE_WRITE;
    __asm__ volatile ("invlpg (%0)" : : "r" (va) : "memory");
    return (void *)va;
}

void *map_physical_scratch(uint32_t phys)
{
    return kernel_scratch_phys(phys);
}

// 在页目录 pde_phys 里，把 vaddr -> paddr 建立映射
static void map_page_impl(uint32_t pde_phys, uint32_t vaddr, uint32_t paddr, uint32_t flags) {
    /* pde_phys is always a physical page-directory address.  User mappings
     * must be installed in that address space, never in the kernel CR3. */
    uint32_t *target_pd = (uint32_t *)map_highmem_physical(pde_phys, PAGE_SIZE, 0x3);
    if (!target_pd) {
        printf("[map_page] cannot access PD phys=0x%x\n", pde_phys);
        return;
    }

    uint32_t pd_index = vaddr >> 22;
    uint32_t pt_index = (vaddr >> 12) & 0x3ff;
    uint32_t user = (flags & PAGE_USER) ? PAGE_USER : 0;
    uint32_t pt_phys = target_pd[pd_index] & ~0xfff;
    uint32_t *pt;

    if (!(target_pd[pd_index] & PAGE_PRESENT)) {
        pt_phys = pmm_alloc_page();
        if (!pt_phys) {
            printf("[map_page] no memory for PT (vaddr=0x%x)\n", vaddr);
            return;
        }
        pt = (uint32_t *)map_highmem_physical(pt_phys, PAGE_SIZE, 0x3);
        if (!pt) {
            printf("[map_page] cannot access new PT phys=0x%x\n", pt_phys);
            return;
        }
        memset(pt, 0, PAGE_SIZE);
        target_pd[pd_index] = pt_phys | PAGE_PRESENT | PAGE_WRITE | user;
    } else {
        /* x86 requires U/S=1 in both PDE and PTE for user access. */
        if (user && !(target_pd[pd_index] & PAGE_USER))
            target_pd[pd_index] |= PAGE_USER;
        pt = (uint32_t *)map_highmem_physical(pt_phys, PAGE_SIZE, 0x3);
        if (!pt) {
            printf("[map_page] cannot access PT phys=0x%x\n", pt_phys);
            return;
        }
    }

    pt[pt_index] = (paddr & ~0xfff) | (flags & 0xfff) | PAGE_PRESENT;
    /* The target CR3 may not be active; its next load will flush non-global
     * translations.  invlpg handles the active address space case. */
    __asm__ volatile ("invlpg (%0)" : : "r" (vaddr) : "memory");
    return;
    /* Legacy implementation retained temporarily for reference; its scope
     * prevents declarations from colliding with the active implementation. */
    {
    uint32_t pd_virt_addr = (uint32_t)phys_to_virt(pde_phys);
    uint32_t kernel_pd_index = pd_virt_addr >> 22;
    uint32_t kernel_pt_index = (pd_virt_addr >> 12) & 0x3FF;

    // ⚠️⚠️⚠️ 关键修复：使用全局变量 kernel_page_directory_phys，不读取当前 CR3
    // 原因：当从系统调用（如 do_fork）调用 map_page 时，当前 CR3 是用户进程的 CR3
    // 必须使用内核初始化时保存的内核页目录物理地址
    extern uint32_t kernel_page_directory_phys;
    uint32_t *kernel_pd = (uint32_t*)phys_to_virt(kernel_page_directory_phys);

    // 检查内核页目录是否有对应的页表
    if (!(kernel_pd[kernel_pd_index] & PAGE_PRESENT)) {
        // 先试早期分配器, 再用 buddy
        uint32_t kernel_pt_phys = pmm_alloc_page();
        if (kernel_pt_phys == 0) { printf("[map_page] ERROR: No mem for kernel PT\n"); return; }
        void *tmp = kernel_scratch_phys(kernel_pt_phys);
        if (!tmp) return;
        memset(tmp, 0, PAGE_SIZE);
        // 填写内核页目录
        kernel_pd[kernel_pd_index] = kernel_pt_phys | 0x3;
    }

    // 检查内核页表中是否有 pde_phys 的映射
    uint32_t kpt_p = kernel_pd[kernel_pd_index] & ~0xFFF;
    uint32_t *kernel_pt;
    if (kpt_p >= 0x800000)
        kernel_pt = (uint32_t*)map_highmem_physical(kpt_p, 4096, 0x3);
    else
        kernel_pt = (uint32_t*)phys_to_virt(kpt_p);
    if (!kernel_pt) { printf("[map_page] Cannot access kernel PT\n"); return; }

    if (!(kernel_pt[kernel_pt_index] & PAGE_PRESENT)) {
        // 为内核创建 pde_phys 的映射
        kernel_pt[kernel_pt_index] = pde_phys | 0x3;
        // 刷新 TLB
        __asm__ volatile ("invlpg (%0)" : : "r" (pd_virt_addr) : "memory");
    }

    // 现在可以安全地访问 pd_user 了
    uint32_t *pd_user = (uint32_t*)map_highmem_physical(pde_phys, PAGE_SIZE, 0x3);
    if (!pd_user) {
        printf("[map_page] Cannot access target page directory phys=0x%x\n", pde_phys);
        return;
    }

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
        uint32_t pt_kernel_pd_index = pt_virt_addr >> 22;
        uint32_t pt_kernel_pt_index = (pt_virt_addr >> 12) & 0x3FF;

        // 检查内核页目录是否有对应的页表（使用前面获取的 kernel_pd）
        if (!(kernel_pd[pt_kernel_pd_index] & PAGE_PRESENT)) {
            // 内核页目录也没有页表，使用早期页表分配器创建一个
            uint32_t kernel_pt_phys = pmm_alloc_page();
            if (kernel_pt_phys == 0) {
                printf("[map_page] ERROR: Failed to allocate kernel page table!\n");
                return;
            }

            void *tmp = kernel_scratch_phys(kernel_pt_phys);
            if (!tmp) return;
            memset(tmp, 0, PAGE_SIZE);
            // 填写内核页目录
            kernel_pd[pt_kernel_pd_index] = kernel_pt_phys | 0x3;
        }

        // 检查内核页表中是否有这个具体的页映射
        uint32_t pt_kernel_pt_phys = kernel_pd[pt_kernel_pd_index] & ~0xFFF;
        uint32_t *pt_kernel_pt;
        if (pt_kernel_pt_phys >= 0x800000)
            pt_kernel_pt = (uint32_t*)map_highmem_physical(pt_kernel_pt_phys, 4096, 0x3);
        else
            pt_kernel_pt = (uint32_t*)phys_to_virt(pt_kernel_pt_phys);
        if (!pt_kernel_pt) {
            printf("[map_page] Cannot access PT kernel mapping page\n");
            return;
        }

        if (!(pt_kernel_pt[pt_kernel_pt_index] & PAGE_PRESENT)) {
            // 为内核创建这个物理页的映射
            pt_kernel_pt[pt_kernel_pt_index] = pt_phys | 0x3;
            // 刷新 TLB
            __asm__ volatile ("invlpg (%0)" : : "r" (pt_virt_addr) : "memory");
        }

        // 清零页表
        uint32_t *pt_virt;
        if (pt_phys >= 0x800000)
            pt_virt = (uint32_t*)map_highmem_physical(pt_phys, PAGE_SIZE, 0x3);
        else
            pt_virt = (uint32_t*)pt_virt_addr;
        if (!pt_virt) {
            printf("[map_page] Cannot zero new PT phys=0x%x\n", pt_phys);
            return;
        }
        memset(pt_virt, 0, PAGE_SIZE);

        // 填写用户页目录的 PDE（只设置一次，后续不会覆盖）
        pd_user[pd_index] = (pt_phys & ~0xFFF) | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
        printf("[map_page] Set pd_user[%u]=0x%x\n", pd_index, pd_user[pd_index]);
    } else {
        // 页表已存在，复用
        //printf("[map_page] vaddr=0x%x reusing existing pd_user[%u]=0x%x\n", vaddr, pd_index, pd_user[pd_index]);
    }

    // 得到页表虚拟地址
    uint32_t mapped_pt_phys = pd_user[pd_index] & ~0xFFF;
    uint32_t *pt;
    if (mapped_pt_phys >= 0x800000)
        pt = (uint32_t*)map_highmem_physical(mapped_pt_phys, 4096, 0x3);
    else
        pt = (uint32_t*)phys_to_virt(mapped_pt_phys);
    if (!pt) {
        printf("[map_page] Cannot access target PT phys=0x%x\n", mapped_pt_phys);
        return;
    }

    // 填写 PTE
    pt[pt_index] =  (paddr & ~0xFFF) | (flags & 0xFFF) | PAGE_PRESENT;
    //printf("[map_page] Set pt[%u]=0x%x (vaddr=0x%x -> paddr=0x%x, flags=0x%x)\n", pt_index, pt[pt_index], vaddr, paddr, flags);
    
    // 🔥 关键修复：刷新 TLB，确保新映射生效
    __asm__ volatile ("invlpg (%0)" : : "r" (vaddr) : "memory");
    }
}

/* Page-table edits may allocate a new kernel highmem-window PDE.  A user task
 * runs with a private CR3, so perform the edit while the kernel CR3 is active
 * and restore the caller's address space afterwards. */
void map_page(uint32_t pde_phys, uint32_t vaddr, uint32_t paddr, uint32_t flags)
{
    extern uint32_t kernel_page_directory_phys;
    uint32_t old_cr3;
    __asm__ volatile ("movl %%cr3, %0" : "=r"(old_cr3));
    if (pde_phys != kernel_page_directory_phys && old_cr3 != kernel_page_directory_phys) {
        __asm__ volatile ("movl %0, %%cr3" :: "r"(kernel_page_directory_phys) : "memory");
        map_page_impl(pde_phys, vaddr, paddr, flags);
        __asm__ volatile ("movl %0, %%cr3" :: "r"(old_cr3) : "memory");
        return;
    }
    map_page_impl(pde_phys, vaddr, paddr, flags);
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
#define EARLY_PT_ALLOC_END 0x600000  // 到6MB结束(可分配1024个页表)

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
        uint32_t pt_phys = pmm_is_ready() ? pmm_alloc_page() : alloc_early_page_table();
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
    /* User tasks share the kernel CR3.  Do not use the legacy `pd` symbol
     * here: it may refer to the bootstrap/linked address rather than the
     * physical page directory currently loaded in CR3. */
    extern uint32_t kernel_page_directory_phys;
    uint32_t* high_page_directory =
        (uint32_t*)phys_to_virt(kernel_page_directory_phys);

    // 确保页目录项存在
    if (!(high_page_directory[pd_index] & 0x1)) {
        // 分配新页表（在above 8MB）
        printf("Allocating new page table for directory entry %u\n", pd_index);

        uint32_t pt_phys = pmm_is_ready() ? pmm_alloc_page() : alloc_early_page_table();
        if (pt_phys == 0) {
            printf("ERROR: Failed to allocate page table!\n");
            return;
        }

        // 清零新页表
        uint32_t* new_pt = (uint32_t*)kernel_scratch_phys(pt_phys);
        if (!new_pt) return;
        memset(new_pt, 0, PAGE_SIZE);

        // 设置单个页表项（只映射当前页，不是整个4MB）
        new_pt[pt_index] = (phys_addr & ~0xFFF) | flags;

        // 填写页目录项
        /* Propagate the caller's user permission to the PDE.  Kernel mappings
         * pass 0x3 and remain supervisor-only; sbrk() passes 0x7 and needs a
         * user-accessible PDE as well as user PTEs. */
        high_page_directory[pd_index] = pt_phys | 0x3 | (flags & 0x4);

        printf("Allocated new page table for pd_idx=%u: pt_phys=0x%x, mapped virt=0x%x -> phys=0x%x\n",
               pd_index, pt_phys, virt_addr, phys_addr);
        printf("  PTE[%u] = 0x%x\n", pt_index, new_pt[pt_index]);

        // 刷新TLB
        __asm__ volatile ("invlpg (%0)" : : "r" (virt_addr) : "memory");
        return;
    }

    // 获取已存在的页表（需要转换为虚拟地址）
    uint32_t pt_phys = high_page_directory[pd_index] & ~0xFFF;
    uint32_t* page_table = (uint32_t*)kernel_scratch_phys(pt_phys);
    if (!page_table) return;

    /* A page table may have been created by a kernel mapping first and later
     * reused for a user mapping.  Upgrade only the PDE user bit when the
     * caller explicitly requests it; never downgrade an existing mapping. */
    if (flags & 0x4)
        high_page_directory[pd_index] |= 0x4;

    // 设置页表项
    page_table[pt_index] = (phys_addr & ~0xFFF) | flags;

    // 刷新TLB
    __asm__ volatile ("invlpg (%0)" : : "r" (virt_addr) : "memory");
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

// ==================== DMA Coherent 内存管理（Linux 风格）====================
// 预映射的 DMA 区域：在 boot 阶段建立，运行时只分配，不改页表
//
// 设计原则：
// 1. 预留专用物理内存区域作为 DMA 池
// 2. 在 paging 初始化时一次性映射为 uncached
// 3. 运行时只做简单的 bump allocator 分配
// 4. 绝不在运行时修改页表（避免 triple fault）

#define DMA_PHYS_BASE   0x02800000   // 40MB 物理地址开始
#define DMA_SIZE        (8 * 1024 * 1024)  // 8MB DMA 区域 (40-48MB)
#define DMA_VIRT_BASE   (0xC2800000) // 映射到内核虚拟地址 0xC1000000

static uint32_t dma_alloc_ptr = 0;  // Bump allocator 指针

/**
 * @brief 在分页初始化时建立 DMA 区域的 uncached 映射
 * @note 必须在 paging_init() 后、驱动初始化前调用一次
 */
void dma_map_region(void) {
    uint32_t phys = DMA_PHYS_BASE;
    uint32_t virt = DMA_VIRT_BASE;

    printf("[dma] Mapping DMA region: phys=0x%x -> virt=0x%x (size=%d MB)\n",
           phys, virt, DMA_SIZE / (1024 * 1024));

    // 按 4KB 页映射整个 DMA 区域
    for (uint32_t off = 0; off < DMA_SIZE; off += 4096) {
        // PTE_P | PTE_RW | PTE_PCD | PTE_PWT = Present + RW + Uncached
        uint32_t flags = 0x3 | 0x10;  // 0x3 = Present+RW, 0x10 = PCD (cache disable)
        map_4k_page(phys + off, virt + off, flags);
    }

    printf("[dma] DMA region mapped as uncached\n");
}

/**
 * @brief 从 DMA 区域分配 coherent 内存（Linux dma_alloc_coherent 风格）
 * @param size 需要分配的大小
 * @param dma_handle 输出参数：返回物理地址供设备使用
 * @return CPU 可访问的虚拟地址，失败返回 NULL
 *
 * @note 这是 bump allocator，不支持释放
 * @note 内存自动 64 字节对齐（cache line 对齐）
 * @note 分配的内存是 uncached 的，CPU 和设备都能看到最新数据
 */
void *dma_alloc_coherent(size_t size, uint32_t *dma_handle) {
    // 64 字节对齐（cache line 对齐）
    size = (size + 63) & ~63;

    // 检查是否超出 DMA 区域
    if (dma_alloc_ptr + size > DMA_SIZE) {
        printf("[dma] ERROR: Out of DMA memory (requested=%u, used=%u, total=%u)\n",
               (uint32_t)size, dma_alloc_ptr, DMA_SIZE);
        return NULL;
    }

    // 计算虚拟和物理地址
    void *vaddr = (void *)(DMA_VIRT_BASE + dma_alloc_ptr);
    uint32_t paddr = DMA_PHYS_BASE + dma_alloc_ptr;

#if DMA_ALLOC_VERBOSE
    printf("[dma] alloc: size=%u, virt=0x%x, phys=0x%x\n",
           (uint32_t)size, (uint32_t)vaddr, paddr);
#endif

    // 更新分配指针
    dma_alloc_ptr += size;

    // 内存屏障（确保 DMA 操作可见）
    asm volatile("mfence" ::: "memory");

    // 返回物理地址
    if (dma_handle) {
        *dma_handle = paddr;
    }

    return vaddr;
}

/**
 * @brief 释放 DMA coherent 内存（bump allocator 不支持释放）
 * @note 这是 stub 实现，仅打印警告
 */
void dma_free_coherent(void *cpu_addr, size_t size) {
    printf("[dma] WARNING: dma_free_coherent not supported (bump allocator)\n");
}

