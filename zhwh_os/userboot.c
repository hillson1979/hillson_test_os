#include "string.h"
#include "page.h"
#include "multiboot2.h"
#include "task.h"
#include "printf.h"
#include "interrupt.h"
#include "elf.h"
#include "highmem_mapping.h"
#include "mm/user_vm.h"

extern void interrupt_exit(void);
extern uint32_t multiboot2_info_addr;

#define SEG_UCODE 3
#define SEG_UDATA 4
#define DPL_USER 3
#define FL_IF    0x00000202  // IF=1, bit1=1 保留位

// 用户态段选择子
#define USER_CS  ((SEG_UCODE << 3) | DPL_USER)   /* 0x1B */
#define USER_DS  ((SEG_UDATA << 3) | DPL_USER)   /* 0x23 */

// 页表权限位
#define PTE_P         0x001
#define PTE_W         0x002
#define PTE_U         0x004
#define USER_PTE_FLAGS (PTE_P|PTE_W|PTE_U)

#define USER_STACK_PAGES 129
#define USER_STACK_SIZE (PAGE_SIZE * USER_STACK_PAGES)
#define VIRT_USER_STACK_TOP 0xBFFFF000
#define VIRT_USER_FRAMEBUFFER 0xF0000000u

int load_module_to_user(struct task_t *task, uint32_t *pd_user) {
    printf("[load_module_to_user] Starting...\n");

    if (!multiboot2_info_addr) {
        printf("[load_module_to_user] No multiboot2 info!\n");
        return -1;
    }

    // 遍历 multiboot2 标签查找模块
    uint32_t *mb_info_ptr = (uint32_t *)phys_to_virt(multiboot2_info_addr);
    multiboot_tag_t *tag = (multiboot_tag_t *)((uint8_t *)mb_info_ptr + 8);
    multiboot_tag_module_t *module_tag = NULL;

    while (tag->type != MULTIBOOT_TAG_TYPE_END) {
        if (tag->type == MULTIBOOT_TAG_TYPE_MODULE) {
            module_tag = (multiboot_tag_module_t *)tag;
            break;
        }
        tag = (multiboot_tag_t *)((uint8_t *)tag + ((tag->size + 7) & ~7));
    }

    if (!module_tag) {
        printf("[load_module_to_user] No modules found!\n");
        return -1;
    }

    uint32_t mod_start = module_tag->mod_start;
    uint32_t mod_end = module_tag->mod_end;
    uint32_t mod_size = mod_end - mod_start;

    printf("[load_module_to_user] Module: start=0x%x end=0x%x size=0x%x\n", mod_start, mod_end, mod_size);

    // 临时映射模块物理内存
    uint32_t mod_virt = phys_to_virt(mod_start);
    for (uint32_t off = 0; off < mod_size; off += PAGE_SIZE) {
        map_4k_page(mod_start + off, mod_virt + off, 0x3);
    }

    Elf32_Ehdr *eh = (Elf32_Ehdr *)mod_virt;
    if (eh->e_ident[0] != 0x7F || eh->e_ident[1] != 'E' || eh->e_ident[2] != 'L' || eh->e_ident[3] != 'F') {
        printf("[load_module_to_user] Not a valid ELF file!\n");
        return -2;
    }

    printf("[load_module_to_user] ELF entry=0x%x, phoff=%u, phnum=%u\n", eh->e_entry, eh->e_phoff, eh->e_phnum);

    // 遍历 Program Header
    uint32_t max_va_end = 0;
    Elf32_Phdr *ph = (Elf32_Phdr *)(mod_virt + eh->e_phoff);
    for (int i = 0; i < eh->e_phnum; i++, ph++) {
        if (ph->p_type != PT_LOAD) continue;

        uint32_t va = ph->p_vaddr;
        uint32_t file_pa = mod_start + ph->p_offset;
        uint32_t memsz = ph->p_memsz;
        uint32_t filesz = ph->p_filesz;
        if (va + memsz > max_va_end) max_va_end = va + memsz;

        for (uint32_t off = 0; off < memsz; off += PAGE_SIZE) {
            uint32_t dst_va = va + off;
            uint32_t dst_pa = pmm_alloc_page();
            if (!dst_pa) return -6;

            uint8_t *dst_virt = (dst_pa >= 0x800000) ? 
                                (uint8_t*)map_highmem_physical(dst_pa, PAGE_SIZE, 0x3) :
                                (uint8_t*)phys_to_virt(dst_pa);
            for (int j = 0; j < PAGE_SIZE; j++) dst_virt[j] = 0;

            if (off < filesz) {
                uint32_t copy_size = PAGE_SIZE;
                if (off + copy_size > filesz) copy_size = filesz - off;

                uint32_t src_pa = file_pa + off;
                uint8_t *src_virt = (src_pa >= 0x800000) ?
                                     (uint8_t*)map_highmem_physical(src_pa, PAGE_SIZE, 0x3) :
                                     (uint8_t*)phys_to_virt(src_pa);
                for (uint32_t j = 0; j < copy_size; j++) dst_virt[j] = src_virt[j];
            }

            user_vm_map_page(task, dst_va, dst_pa, USER_PTE_FLAGS);
        }
    }

    // 额外映射用户堆空间：在 ELF 段之后映射 8MB
    {
        uint32_t heap_start = (max_va_end + 0xFFF) & ~0xFFF;
        uint32_t heap_pages = 2048;  // 8MB = 2048 * 4KB
        printf("[load_module_to_user] Mapping extra %d MB heap at vaddr=0x%x\n",
               heap_pages / 256, heap_start);
        for (uint32_t i = 0; i < heap_pages; i++) {
            uint32_t hp_va = heap_start + i * PAGE_SIZE;
            uint32_t hp_pa = pmm_alloc_page();
            if (!hp_pa) break;
            user_vm_map_page(task, hp_va, hp_pa, USER_PTE_FLAGS);
        }
    }

    if (!task || !task->tf) return -5;
    struct trapframe *tf = task->tf;

    tf->eip = eh->e_entry;
    tf->cs = USER_CS;
    tf->ds = tf->es = tf->fs = tf->gs = tf->ss = USER_DS;
    tf->eflags = FL_IF;

    // 创建用户栈
    uint32_t last_stack_va = 0;
    for (int i = 0; i < USER_STACK_PAGES; i++) {
        uint32_t stack_pa = pmm_alloc_page();
        uint32_t stack_va = VIRT_USER_STACK_TOP - (i + 1) * PAGE_SIZE;
        last_stack_va = (i == 0) ? stack_va : last_stack_va;
        user_vm_map_page(task, stack_va, stack_pa, USER_PTE_FLAGS);
        if (stack_pa) {
            uint8_t *stack_mem = (uint8_t *)map_highmem_physical(stack_pa, PAGE_SIZE, 0x3);
            if (stack_mem) memset(stack_mem, 0, PAGE_SIZE);
        }
    }

    /* Map the physical VBE framebuffer into the user address space.  The
     * desktop and graph clients intentionally use the stable VA 0xF0000000.
     * This PDE is marked user-accessible by map_page for the framebuffer PT. */
    {
        multiboot_tag_framebuffer_common_t *fb = NULL;
        multiboot_tag_t *scan = (multiboot_tag_t *)((uint8_t *)mb_info_ptr + 8);
        while (scan->type != MULTIBOOT_TAG_TYPE_END) {
            if (scan->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
                fb = (multiboot_tag_framebuffer_common_t *)scan;
                break;
            }
            scan = (multiboot_tag_t *)((uint8_t *)scan + ((scan->size + 7) & ~7));
        }
        if (fb && fb->framebuffer_addr && fb->framebuffer_pitch && fb->framebuffer_height) {
            uint32_t bytes = fb->framebuffer_pitch * fb->framebuffer_height;
            uint32_t pages = (bytes + PAGE_SIZE - 1) / PAGE_SIZE;
            printf("[load_module_to_user] Mapping framebuffer: phys=0x%x pages=%u va=0x%x\n",
                   (uint32_t)fb->framebuffer_addr, pages, VIRT_USER_FRAMEBUFFER);
            for (uint32_t i = 0; i < pages; ++i)
                user_vm_map_page(task, VIRT_USER_FRAMEBUFFER + i * PAGE_SIZE,
                                 (uint32_t)fb->framebuffer_addr + i * PAGE_SIZE,
                                 USER_PTE_FLAGS);
        }
    }

    task->user_stack = VIRT_USER_STACK_TOP;

    // 填充栈顶NULL，防止argv/envp访问越界
    /* The target CR3 is not active while loading; stack pages are already
     * zeroed through their temporary kernel mappings. */

    // ESP 在栈顶下方 64 字节，避免 _start 访问边界外的内存
    tf->esp = VIRT_USER_STACK_TOP - 64;

    printf("[load_module_to_user] Trapframe: eip=0x%x, esp=0x%x, cs=0x%x, ss=0x%x, eflags=0x%x\n",
           tf->eip, tf->esp, tf->cs, tf->ss, tf->eflags);

    return 0;
}
