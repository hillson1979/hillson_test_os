#include "string.h"
#include "page.h"
#include "multiboot2.h"
#include "task.h"
#include "printf.h"
#include "interrupt.h"
#include "elf.h"
#include "highmem_mapping.h"
extern void interrupt_exit(void);

extern uint32_t multiboot2_info_addr;

#define SEG_UCODE 3
#define SEG_UDATA 4
#define DPL_USER 3
#define FL_IF    0x00000200
// 页表项/页目录项权限位（关键标志）
#define PTE_P         0x001   // 存在位（1=物理页存在）
#define PTE_W         0x002   // 可写位（1=允许写操作）
#define PTE_U         0x004   // 用户位（1=用户态可访问）
#define USER_PTE_FLAGS (PTE_P|PTE_W|PTE_U)

//#define USER_STACK_TOP  0xBFFFF000   // 用户栈顶（示例）
#define USER_STACK_SIZE PAGE_SIZE

int load_module_to_user(uint32_t *pd_user) {
    printf("[load_module_to_user] Starting...\n");

    if (!multiboot2_info_addr) {
        printf("[load_module_to_user] No multiboot2 info!\n");
        return -1;
    }

    // 遍历 multiboot2 标签查找模块信息
    // Multiboot 2 info: [0-3] size, [4-7] reserved, [8+] tags
    uint32_t *mb_info_ptr = (uint32_t *)phys_to_virt(multiboot2_info_addr);
    multiboot_tag_t *tag = (multiboot_tag_t *)((uint8_t *)mb_info_ptr + 8);
    multiboot_tag_module_t *module_tag = NULL;
    int found_modules = 0;

    while (tag->type != MULTIBOOT_TAG_TYPE_END) {
        if (tag->type == MULTIBOOT_TAG_TYPE_MODULE) {
            module_tag = (multiboot_tag_module_t *)tag;
            found_modules = 1;
            break;
        }
        tag = (multiboot_tag_t *)((uint8_t *)tag + ((tag->size + 7) & ~7));
    }

    if (!found_modules || !module_tag) {
        printf("[load_module_to_user] No modules found in multiboot2!\n");
        return -1;
    }

    uint32_t mod_start = module_tag->mod_start;
    uint32_t mod_end = module_tag->mod_end;
    const char *cmdline = module_tag->cmdline;

    printf("[load_module_to_user] Module: start=0x%x end=0x%x size=0x%x cmdline=%s\n",
           mod_start, mod_end, mod_end - mod_start, cmdline);

    // 先直接读取物理内存，看看原始数据
    uint8_t *raw_phys = (uint8_t *)mod_start;
    printf("[load_module_to_user] Raw physical bytes at 0x%x: %02x %02x %02x %02x\n",
           mod_start, raw_phys[0], raw_phys[1], raw_phys[2], raw_phys[3]);

    Elf32_Ehdr *eh = (Elf32_Ehdr *)phys_to_virt(mod_start);

    printf("[load_module_to_user] ELF header at virtual 0x%x\n", eh);
    printf("[load_module_to_user] First 4 bytes: 0x%x 0x%x 0x%x 0x%x\n",
           eh->e_ident[0], eh->e_ident[1], eh->e_ident[2], eh->e_ident[3]);

    // 检查 ELF 魔数
    if (eh->e_ident[0] != 0x7F || eh->e_ident[1] != 'E' || eh->e_ident[2] != 'L' || eh->e_ident[3] != 'F') {
        printf("[load_module_to_user] Not a valid ELF file!\n");
        printf("[load_module_to_user] Expected: 0x7F 'E' 'L' 'F'\n");
        return -2;
    }

    printf("[load_module_to_user] ELF file validated!\n");
    printf("[load_module_to_user] e_entry=0x%x, e_phoff=%u, e_phnum=%u\n", eh->e_entry, eh->e_phoff, eh->e_phnum);

    // 遍历 Program Header
    Elf32_Phdr *ph = (Elf32_Phdr *)phys_to_virt(mod_start + eh->e_phoff);
    for (int i = 0; i < eh->e_phnum; i++, ph++) {
        printf("[load_module_to_user] Program Header %u: p_type=0x%x\n", i, ph->p_type);
        if (ph->p_type != PT_LOAD)
            continue;

        uint32_t va = ph->p_vaddr;           // 用户虚拟地址 用户空间目标地址
        uint32_t file_pa = mod_start + ph->p_offset; // ELF 数据在物理内存中的位置
        uint32_t memsz = ph->p_memsz;
        uint32_t filesz = ph->p_filesz;

        printf("[load_module_to_user] PT_LOAD: va=0x%x, file_pa=0x%x, memsz=0x%x, filesz=0x%x\n", va, file_pa, memsz, filesz);

        // 按 4KB 页映射整个段（包括 .bss）
        printf("[load_module_to_user] Starting page mapping loop...\n");
        for (uint32_t off = 0; off < memsz; off += PAGE_SIZE) {
            uint32_t dst_va = va + off;
            uint32_t dst_pa;

            printf("[load_module_to_user] Loop: off=%u, dst_va=0x%x\n", off, dst_va);

            if (off < filesz) {
                // 文件中的数据：使用 ELF 中的物理页
                dst_pa = file_pa + off;
                printf("[load_module_to_user] Using file data: dst_pa=0x%x\n", dst_pa);
            } else {
                // .bss 部分：分配新的零页
                dst_pa = pmm_alloc_page();
                printf("[load_module_to_user] BSS: allocated phys=0x%x\n", dst_pa);
                // 清零
                uint8_t *zero_page = (uint8_t*)phys_to_virt(dst_pa);
                for (int j = 0; j < PAGE_SIZE; j++) {
                    zero_page[j] = 0;
                }
            }

            printf("[load_module_to_user] Calling map_page: dst_va=0x%x dst_pa=0x%x\n", dst_va, dst_pa);
            map_page(pd_user, dst_va, dst_pa, USER_PTE_FLAGS);

            // 同时映射到内核页表，这样用户进程可以在内核页表下运行
            extern uint32_t pd[];
            map_page(pd, dst_va, dst_pa, USER_PTE_FLAGS);
        }
        printf("[load_module_to_user] Page mapping loop done.\n");
    }

    // 确保 th_u 和 th_u->tf 已初始化后再写
    if (!th_u || !th_u->tf) {
        printf("[load_module_to_user] th_u or th_u->tf not ready\n");
        return -5;
    }

    task_t *task = th_u;

    struct trapframe *tf = task->tf;


    // 设置入口地址
    tf->eip = eh->e_entry;

    // 创建用户栈 - 1页(4KB)
    uint32_t stack_pa = pmm_alloc_page();
    map_page(pd_user, VIRT_USER_STACK_TOP - PAGE_SIZE, stack_pa, USER_PTE_FLAGS);

    // 同时映射到内核页表
    extern uint32_t pd[];
    map_page(pd, VIRT_USER_STACK_TOP - PAGE_SIZE, stack_pa, USER_PTE_FLAGS);

    tf->esp = VIRT_USER_STACK_TOP;

    // 调试输出暂时禁用，可能导致 Page Fault
    printf("[load_module_to_user] [map_page] entry=0x%x, stack=0x%x\n",tf->eip, tf->esp);

    return 0;
}

