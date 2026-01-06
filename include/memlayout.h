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
