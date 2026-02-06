#include "types.h"

// 类型定义
#ifndef size_t
typedef uint32_t size_t;
#endif


#define PAGE_SIZE       4096
#define PAGE_PRESENT    (1 << 0)
#define PAGE_WRITABLE   (1 << 1)
#define PAGE_USER       (1 << 2)

#define VMS_MASK        ( -1UL )
// 页表标志位
#define PDE_PRESENT     (1 << 0)
#define PDE_WRITABLE    (1 << 1)
#define PDE_USER        (1 << 2)
#define PDE_WRITETHROUGH (1 << 3)
#define PDE_CACHE_DISABLE (1 << 4)
#define PDE_ACCESSED    (1 << 5)
#define PDE_PAGE_SIZE   (1 << 7)  // PS bit for 4MB pages

#define PTE_PRESENT     (1 << 0)
#define PTE_WRITABLE    (1 << 1)
#define PTE_USER        (1 << 2)
#define PTE_WRITETHROUGH (1 << 3)
#define PTE_CACHE_DISABLE (1 << 4)
#define PTE_ACCESSED    (1 << 5)
#define PTE_DIRTY       (1 << 6)
#define PTE_GLOBAL      (1 << 8)
#define KERNEL_VA_OFFSET 0xC0000000   // 内核虚拟地址偏移
// 地址转换宏（内核直接映射）
#define phys_to_virt(pa) ((void*)((uint32_t)(pa) + KERNEL_VA_OFFSET))
#define virt_to_phys(va) ((uint32_t)(va) - KERNEL_VA_OFFSET)
// 页目录项和页表项结构（x86架构）
typedef uint32_t pde_t;    // 页目录项
typedef uint32_t pte_t;    // 页表项

// 外部声明汇编中定义的页表和页目录（使用正确的变量名）
extern pde_t pd[1024];     // 对应汇编中的 pd
extern pte_t pt[1024];     // 对应汇编中的 pt
extern pte_t pt2[1024];    // 对应汇编中的 pt2

typedef struct page_entry_t
{
    uint8_t present : 1;  // 在内存中
    uint8_t write : 1;    // 0 只读 1 可读可写
    uint8_t user : 1;     // 1 所有人 0 超级用户 DPL < 3
    uint8_t pwt : 1;      // page write through 1 直写模式，0 回写模式
    uint8_t pcd : 1;      // page cache disable 禁止该页缓冲
    uint8_t accessed : 1; // 被访问过，用于统计使用频率
    uint8_t dirty : 1;    // 脏页，表示该页缓冲被写过
    uint8_t pat : 1;      // page attribute table 页大小 4K/4M
    uint8_t global : 1;   // 全局，所有进程都用到了，该页不刷新缓冲
    uint8_t shared : 1;   // 共享内存页，与 CPU 无关
    uint8_t privat : 1;   // 私有内存页，与 CPU 无关
    uint8_t readonly : 1; // 只读内存页，与 CPU 无关
    uint32_t index : 20;  // 页索引
} page_entry_t;

#define VIRT_USER_STACK_TOP 0xBFFFF000

#define VIRT_MEM_BASE_USER 0x08000000 //0x6000000
#define PHYS_MEM_BASE_USER V2P(2 * 0x400000+KERNLINK) // 物理内存起始地址（避开1MB以下的BIOS/设备区域）
#define PHYS_MEM_SIZE 0x400000//0x07F00000      // 可用物理内存大小（127MB，示例值）
#define TOTAL_PAGES   (PHYS_MEM_SIZE / PAGE_SIZE) // 总可用物理页数
// X86 32位关键定义：页大小4KB，页目录/页表项占4字节，共1024项
//#define PAGE_SIZE     0x1000  // 4096字节
#define PDE_NUM       1024    // 页目录项数量
#define PTE_NUM       1024    // 页表项数量
#define PDE_SHIFT     22      // 页目录索引移位（2^22 = 4MB，对应页目录粒度）
#define PTE_SHIFT     12      // 页表索引移位（2^12 = 4KB，对应页表粒度）

// 页表项/页目录项权限位（关键标志）
//#define PTE_P         0x001   // 存在位（1=物理页存在）
//#define PTE_W         0x002   // 可写位（1=允许写操作）
//#define PTE_U         0x004   // 用户位（1=用户态可访问）

// 页目录结构（每个页目录指向1个页表）
typedef uint32_t pde_user_t;
// 页表结构（每个页表项指向1个物理页）
typedef uint32_t pte_user_t;

void early_remap_page(uint32_t phys_addr, uint32_t virt_addr, uint32_t flags);

void map_4k_page(uint32_t phys_addr, uint32_t virt_addr, uint32_t flags);

void identity_map_8m_4k( uint32_t addr);

void* alloc_page_table(uint32_t virt_addr, uint32_t phys_addr,uint32_t flags);
void* alloc_user_page_table(uint32_t *pd_virt,uint32_t virt_addr, uint32_t phys_addr, uint32_t flags) ;

void * get_page_fast(void * addr,uint32_t count);

void phys_user_mem_init(void);

//int mappages_user(pde_user_t *pg_dir, uint32_t va, uint32_t pa, uint32_t perm);
void *get_free_phys_page(void);
//int mappages_multi(pde_user_t *pg_dir, uint32_t va, uint32_t pa, int len, uint32_t perm);
//void* alloc_user_page_table(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags, bool map_kernel_space);

pde_user_t * mappages_multi( uint32_t va, uint32_t pa, int len, uint32_t perm) ;

// ==================== DMA Coherent 内存管理（Linux 风格）===================
// 预映射的 DMA 区域：在 boot 阶段建立，运行时只分配，不改页表

void dma_map_region(void);  // 在 paging 初始化时调用，建立 DMA 区域映射
void *dma_alloc_coherent(size_t size, uint32_t *dma_handle);  // 分配 DMA 内存
void dma_free_coherent(void *cpu_addr, size_t size);  // 释放（bump allocator 不支持）


