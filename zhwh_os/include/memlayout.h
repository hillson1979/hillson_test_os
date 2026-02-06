// Memory layout

#define EXTMEM  0x100000            // Start of extended memory
#define PHYSTOP 0xE000000           // Top physical memory
#define DEVSPACE 0xCE000000         // Other devices are at high addresses

// 虚拟地址空间布局 (x86 32-bit)
// 用户空间：0x00000000 - 0xBFFFFFFF (0-3GB)
// 内核空间：0xC0000000 - 0xFFFFFFFF (3-4GB)
#define KERNBASE     0xC0000000     // 内核虚拟地址起始 (3GB)
#define USER_SPACE_END 0xBFFFFFFF   // 用户空间结束地址 (3GB - 1)

// Key addresses for address space layout (see kmap in vm.c for layout)
#define KERNLINK (KERNBASE + 16 * EXTMEM)  // Address where kernel is linked

// 用户空间虚拟地址范围
#define USER_BASE    0x00000000     // 用户空间起始地址
#define USER_LIMIT   0xC0000000     // 用户空间限制（3GB）

#define V2P(a) (((uint32_t) (a)) - KERNBASE)
#define P2V(a) ((void *)(((char *) (a)) + KERNBASE))

#define V2P_WO(x) ((x) - KERNBASE)    // same as V2P, but without casts
#define P2V_WO(x) ((x) + KERNBASE)    // same as P2V, but without casts
static uint32_t map_from_addr=0;

// ==================== 物理内存布局 (Physical Memory Layout) ====================
//
// 0x00000000 - 0x000FFFFF : Low 1MB (BIOS/IVT/VGA)
// 0x00100000 - 0x003E6000 : Kernel code/data (~3.9MB)
// 0x00200000 - 0x003FFFFF : Early page table allocator (2MB-4MB)
// 0x00800000 - 0x027FFFFF : Reserved (给将来使用, 8MB-40MB)
// 0x02800000 - 0x02FFFFFF : DMA Coherent region (8MB) ← 40MB-48MB
// 0x03000000 - 0x043FFFFF : Buddy System metadata (20MB) ← 48MB-68MB (物理)
// 0x04400000+          : PMM managed physical memory (68MB+)
//
// DMA Region:    40MB-48MB (物理) → 0xC2800000 (虚拟, uncached)
// Buddy System:  48MB-68MB (物理) → 0xC3000000 (虚拟)

#define PHYS_KERNEL_END    0x003E6000   // 内核结束地址
#define PHYS_EARLY_PT_START 0x00200000   // 2MB - 早期页表起始
#define PHYS_EARLY_PT_END   0x00400000   // 4MB - 早期页表结束
#define PHYS_DMA_BASE       0x02800000   // 40MB - DMA 物理基地址
#define PHYS_DMA_SIZE       (8 * 1024 * 1024)   // 8MB
#define PHYS_DMA_END        (PHYS_DMA_BASE + PHYS_DMA_SIZE)  // 48MB
#define PHYS_BUDDY_BASE    0x03000000   // 48MB - Buddy System 元数据起始 (物理地址)
#define PHYS_BUDDY_SIZE   (20 * 1024 * 1024)  // 20MB
#define PHYS_BUDDY_END    (PHYS_BUDDY_BASE + PHYS_BUDDY_SIZE)  // 68MB

#define PHYS_PMM_START    0x04400000   // 68MB - PMM 管理内存起始
#define PHYS_PMM_END      0x07F80000   // 127MB - PMM 管理内存结束

// ==================== 虚拟内存布局 (Virtual Memory Layout) ====================
//
// 0x00000000 - 0xBFFFFFFF : User space (3GB)
// 0xC0000000 - 0xFFFFFFFF : Kernel space (1GB)
//
// 内核空间细分:
//   0xC0000000 - 0xC007FFFF : Identity mapping (0-8MB, 包含内核+早期页表)
//   0xC0080000 - 0xC27FFFFF : Reserved/Other mappings
//   0xC2800000 - 0xC2FFFFFF : DMA Coherent region (8MB, uncached) ← 40MB-48MB
//   0xC3000000 - 0xC43FFFFF : Buddy System metadata (20MB, 映射到物理48MB-68MB)
//   0xC4400000+          : Direct mapping (remaining physical memory)

#define VIRT_IDENTITY_START   0xC0000000   // 恒等映射起始
#define VIRT_IDENTITY_END     0xC07FFFFF   // 恒等映射结束 (8MB)
#define VIRT_DMA_BASE         0xC2800000   // DMA 虚拟地址 (40MB, uncached)
#define VIRT_BUDDY_BASE       0xC3000000   // Buddy 元数据虚拟地址 (映射到物理48MB)
#define VIRT_DYNAMIC_BASE     0xC0800000   // 动态映射窗口
#define VIRT_DYNAMIC_SIZE     (8 * 1024 * 1024)  // 8MB
#define VIRT_PMM_BASE         0xC4400000   // PMM 直接映射起始 (68MB+)

// ==================== Identity Mapping ====================
#define IDENTITY_MAP_SIZE     0x800000     // 8MB
#define IS_IDENTITY_MAPPED(phys) ((phys) < IDENTITY_MAP_SIZE)

// ==================== DMA Mapping Flags ====================
// DMA 内存必须使用 uncached 映射，确保硬件写入能被 CPU 立即看到
#define DMA_PTE_FLAGS  (PAGE_PRESENT | PAGE_WRITABLE | 0x010 | 0x008)
//                                                         ↑      ↑
//                                                      PCD    PWT
//                                                   Cache  Write
//                                                   Disable Through

