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
// 用户态段选择子定义
#define USER_CS  ((SEG_UCODE << 3) | DPL_USER)   /* 0x1B */
#define USER_DS  ((SEG_UDATA << 3) | DPL_USER)   /* 0x23 */
// 页表项/页目录项权限位（关键标志）
#define PTE_P         0x001   // 存在位（1=物理页存在）
#define PTE_W         0x002   // 可写位（1=允许写操作）
#define PTE_U         0x004   // 用户位（1=用户态可访问）
#define USER_PTE_FLAGS (PTE_P|PTE_W|PTE_U)

//#define USER_STACK_TOP  0xBFFFF000   // 用户栈顶（示例）
#define USER_STACK_SIZE PAGE_SIZE * 2

int load_module_to_user(struct task_t *task, uint32_t *pd_user) {
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

    // 🔥 临时映射用户模块所在的物理内存（68MB+ 超出恒等映射）
    uint32_t mod_size = mod_end - mod_start;
    uint32_t mod_virt = phys_to_virt(mod_start);

    printf("[load_module_to_user] Mapping module: phys=0x%x -> virt=0x%x (size=%u)\n",
           mod_start, mod_virt, mod_size);

    // 按 4KB 页映射整个模块
    for (uint32_t off = 0; off < mod_size; off += 4096) {
        map_4k_page(mod_start + off, mod_virt + off, 0x3);  // Present + RW
    }
    printf("[load_module_to_user] Module mapped successfully\n");

    // 先直接读取物理内存，看看原始数据
    uint8_t *raw_phys = (uint8_t *)mod_virt;
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

            //printf("[load_module_to_user] Loop: off=%u, dst_va=0x%x\n", off, dst_va);

            // ========== 关键修复：分配新的物理页，而不是直接使用ELF文件所在的物理内存 ==========
            // 原因：multiboot模块的物理内存是内核临时使用的，可能被覆盖
            // 必须复制到新分配的物理页中

            // 1. 分配新的物理页
            dst_pa = pmm_alloc_page();
            if (!dst_pa) {
                printf("[load_module_to_user] FATAL: failed to allocate physical page!\n");
                return -6;
            }

            // 2. 🔥 动态映射物理页到内核空间（如果 > 8MB）
            uint8_t *dst_virt;
            if (dst_pa >= 0x800000) {
                dst_virt = (uint8_t*)map_highmem_physical(dst_pa, PAGE_SIZE, 0x3);
                if (!dst_virt) {
                    printf("[load_module_to_user] FATAL: failed to map physical page 0x%x!\n", dst_pa);
                    return -7;
                }
            } else {
                dst_virt = (uint8_t*)phys_to_virt(dst_pa);
            }

            // 3. 清零整个页
            for (int j = 0; j < PAGE_SIZE; j++) {
                dst_virt[j] = 0;
            }

            // 4. 如果这是文件数据部分，从ELF复制
            if (off < filesz) {
                uint32_t copy_size = PAGE_SIZE;
                if (off + copy_size > filesz) {
                    copy_size = filesz - off;
                }

                // 🔥 动态映射源物理页（ELF文件）
                uint8_t *src_virt;
                uint32_t src_pa = file_pa + off;
                if (src_pa >= 0x800000) {
                    src_virt = (uint8_t*)map_highmem_physical(src_pa, PAGE_SIZE, 0x3);
                    if (!src_virt) {
                        printf("[load_module_to_user] FATAL: failed to map source page 0x%x!\n", src_pa);
                        return -7;
                    }
                } else {
                    src_virt = (uint8_t*)phys_to_virt(src_pa);
                }

                for (uint32_t j = 0; j < copy_size; j++) {
                    dst_virt[j] = src_virt[j];
                }
                //printf("[load_module_to_user] Copied 0x%x bytes from ELF to new page\n", copy_size);
            } else {
                //printf("[load_module_to_user] BSS: allocated and zeroed new page\n");
            }

            printf("[load_module_to_user] Calling map_page: dst_va=0x%x dst_pa=0x%x flags=0x%x\n", dst_va, dst_pa, USER_PTE_FLAGS);
            // 🔥 使用内核页目录物理地址（共享 CR3）
            extern uint32_t kernel_page_directory_phys;
            map_page(kernel_page_directory_phys, dst_va, dst_pa, USER_PTE_FLAGS);

            // ⚠️⚠️⚠️ 关键修复：同时映射到内核页目录！
            // 原因：现在使用共享 CR3（kernel CR3），所有进程都使用同一个内核页目录
            //       用户代码段必须在内核页目录中也能访问
            //extern uint32_t pd[];
            //map_page(pd, dst_va, dst_pa, USER_PTE_FLAGS);
           // printf("[load_module_to_user] Also mapped to kernel page table\n");
        }
        printf("[load_module_to_user] Page mapping loop done.\n");
    }

    // 确保任务和 trapframe 已初始化
    if (!task || !task->tf) {
        printf("[load_module_to_user] task or task->tf not ready\n");
        return -5;
    }

    struct trapframe *tf = task->tf;

    // ⚠️ 不要清零整个trapframe!
    // 原因:这会把所有寄存器(eax,ebx,ecx,edx等)清零
    // 用户程序可能期望某些寄存器有非零值
    // 只显式设置我们需要的字段

    // 设置入口地址
    tf->eip = eh->e_entry;
    printf("[load_module_to_user] Set tf->eip = 0x%x (from ELF entry)\n", tf->eip);

    // 设置段寄存器为用户态选择子
    tf->cs = USER_CS;  // 0x1B = 用户代码段
    tf->ds = USER_DS;  // 0x23 = 用户数据段
    tf->es = USER_DS;
    tf->fs = USER_DS;
    tf->gs = USER_DS;
    tf->ss = USER_DS;  // 0x23 = 用户栈段
    printf("[load_module_to_user] Set segment registers: CS=0x%x, DS/ES/FS/GS/SS=0x%x\n", tf->cs, tf->ds);

    // 设置EFLAGS - 开启中断
    tf->eflags = FL_IF;
    printf("[load_module_to_user] Set tf->eflags = 0x%x\n", tf->eflags);

    // 创建用户栈 - 多页(16KB)，确保栈有足够空间
    // 栈从高地址向低地址增长
    #define USER_STACK_PAGES 4  // 4页 = 16KB

    printf("[load_module_to_user] Mapping user stack (%u pages)...\n", USER_STACK_PAGES);

    uint32_t last_stack_va = 0;  // 🔥 保存最后一页(最高地址)的**虚拟地址**
    for (int i = 0; i < USER_STACK_PAGES; i++) {
        uint32_t stack_pa = pmm_alloc_page();
        printf("[load_module_to_user] Allocated stack page %u: phys=0x%x\n", i, stack_pa);

        // 🔥 修复：栈页映射应该从 VIRT_USER_STACK_TOP - (i+1)*PAGE_SIZE 开始
        // 这样第一页(i=0)映射到 VIRT_USER_STACK_TOP - PAGE_SIZE
        // 栈顶(VIRT_USER_STACK_TOP)在第一页的页尾
        uint32_t stack_va = VIRT_USER_STACK_TOP - (i + 1) * PAGE_SIZE;
        printf("[load_module_to_user] Mapping stack page: va=0x%x -> pa=0x%x\n", stack_va, stack_pa);

        // 🔥 使用内核页目录物理地址（共享 CR3）
        extern uint32_t kernel_page_directory_phys;
        map_page(kernel_page_directory_phys, stack_va, stack_pa, USER_PTE_FLAGS);

        // 保存最后一页的虚拟地址(最高地址页)
        if (i == 0) {
            last_stack_va = stack_va;  // 🔥 保存虚拟地址！
        }
    }
    printf("[load_module_to_user] User stack mapping complete.\n");

    // 🔥🔥🔥 关键修复：保存用户栈的**虚拟地址**到 task->user_stack
    // 用户栈虚拟地址范围：0xBFFFC000 - 0xBFFFF000
    // 栈顶（最高地址）：VIRT_USER_STACK_TOP = 0xBFFFF000
    task->user_stack = VIRT_USER_STACK_TOP;
    printf("[load_module_to_user] Set task->user_stack = 0x%x (virtual address of stack top)\n", task->user_stack);

    // ⚠️ 删除 BRUTE FORCE 映射代码！
    // 原因：它会覆盖 ELF 加载时正确设置的映射，导致用户代码无法执行
    // ELF 的 PT_LOAD 段已经正确映射了所有需要的页面

    // ========== 设置用户栈ABI布局 ==========
    // Linux ELF ABI标准的用户栈布局（从高地址到低地址）:
    //
    //   [高地址]
    //   NULL                (auxv数组结束标记，如果有的话)
    //   auxv entries        (辅助向量，可选)
    //   NULL                (envp数组结束标记)
    //   envp[]              (环境变量字符串指针数组)
    //   NULL                (argv数组结束标记)
    //   argv[0]             (程序名，如果没有则为NULL)
    //   argv[1...n]         (其他参数)
    //   argc                (参数个数，ESP指向这里)
    //   [低地址]
    //
    // ⚠️⚠️⚠️ 关键修复：必须确保栈上有足够的数据，防止C运行时代码访问越界
    // 原因：fault_addr=0xFFFFFFE8说明代码在访问NULL-0x18，很可能是
    //       C运行时代码试图访问argv/envp指针数组时越界

    // 🔥 使用虚拟地址直接访问用户栈（通过用户页表映射）
    // last_stack_va 是第一页的虚拟地址（0xBFFFE000）
    uint32_t *stack_top_virt = (uint32_t *)last_stack_va;

    // ⚠️⚠️⚠️ 关键修复：在栈顶填充大量NULL，防止越界访问
    // 原因：C运行时代码可能会访问argv[argc]、envp[0]等
    // 如果这些位置是未初始化的垃圾数据，会导致崩溃
    for (int i = 1; i <= 32; i++) {
        stack_top_virt[PAGE_SIZE/4 - i] = 0;  // 🔥 从页尾向前填充32个NULL
    }

    // ESP指向argc（栈顶往下4字节，在第一页的页尾）
    // 布局：[ESP]=argc, [ESP+4]=argv[0], [ESP+8]=argv[1], ...
    tf->esp = VIRT_USER_STACK_TOP - 1 * sizeof(uint32_t);

    printf("[load_module_to_user] Set up user stack ABI (Linux standard):\n");
    printf("  argc=0 at [ESP]=0x%x\n", tf->esp);
    printf("  argv[0]=NULL at [ESP+4]=0x%x\n", tf->esp + 4);
    printf("  Stack protected with 32 NULL dwords (128 bytes) below stack_top\n");

    // 调试输出:验证trapframe设置
    printf("[load_module_to_user] Trapframe setup:\n");
    printf("  eip=0x%x, esp=0x%x\n", tf->eip, tf->esp);
    printf("  cs=0x%x, ss=0x%x, ds=0x%x, eflags=0x%x\n", tf->cs, tf->ss, tf->ds, tf->eflags);

    return 0;
}

