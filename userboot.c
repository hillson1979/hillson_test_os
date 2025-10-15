#include "string.h"
#include "page.h"
#include "multiboot.h"
#include "task.h"
#include "printf.h"
#include "interrupt.h"
#include "elf.h"
extern void interrupt_exit(void);

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
    if (!(multiboot_info->flags & (1 << 3)))
        return -1;

    multiboot_module_t *mods = (multiboot_module_t *)phys_to_virt(multiboot_info->mods_addr);
    uint32_t mod_start = mods[0].mod_start;
    uint32_t mod_end   = mods[0].mod_end;

    Elf32_Ehdr *eh = (Elf32_Ehdr *)phys_to_virt(mod_start);

    // 检查 ELF 魔数
    if (eh->e_ident[0] != 0x7F || eh->e_ident[1] != 'E' || eh->e_ident[2] != 'L' || eh->e_ident[3] != 'F') {
        printf("[load_module_to_user] Not a valid ELF file!\n");
        return -2;
    }

    // 遍历 Program Header
    Elf32_Phdr *ph = (Elf32_Phdr *)phys_to_virt(mod_start + eh->e_phoff);
    for (int i = 0; i < eh->e_phnum; i++, ph++) {
        if (ph->p_type != PT_LOAD)
            continue;

        uint32_t va = ph->p_vaddr;           // 用户虚拟地址 用户空间目标地址
        uint32_t pa = mod_start + ph->p_offset; // ELF 数据在物理内存中的位置 ELF中的段数据
        uint32_t memsz = ph->p_memsz;
        uint32_t filesz = ph->p_filesz;

        printf("[load_module_to_user] PT_LOAD: va=0x%x, pa=0x%x, memsz=0x%x, filesz=0x%x\n", va, pa, memsz, filesz);

        uint32_t va_add=0,pa_add=0;
        for (uint32_t off = 0; off < memsz; off += PAGE_SIZE) {
            uint32_t dst_va = va + off;
            uint32_t dst_pa = pa+off;
            va_add=dst_va;
            pa_add=dst_pa;
            printf("[load_module_to_user] [map_page] [for] dst_va=0x%x dst_pa=0x%x\n",dst_va, dst_pa);
            map_page(pd_user, dst_va, dst_pa, USER_PTE_FLAGS);
        }
        //map_page(pd_user, va_add+8192, pa_add+8192, USER_PTE_FLAGS);
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

    // 创建用户栈
    uint32_t stack_pa =pmm_alloc_page();// th_u->user_stack;//
    map_page(pd_user, VIRT_USER_STACK_TOP - PAGE_SIZE, stack_pa, USER_PTE_FLAGS);
    tf->esp = VIRT_USER_STACK_TOP;

    printf("[load_module_to_user] [map_page] entry=0x%x, stack=0x%x\n",tf->eip, tf->esp);
    return 0;
}

