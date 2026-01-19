// syscall.c
#include "vga.h"
#include "interrupt.h"
#include "printf.h"   // 需要一个 console_write(buf,len) 或 putchar
#include "task.h"      // do_exit(), current task, etc (task.h 已经包含了 mm.h)
#include "multiboot2.h" // multiboot2_info_addr
#include "highmem_mapping.h"
#include "page.h"      // phys_to_virt 宏
#include "lapic.h"     // logical_cpu_id()

// 声明当前任务数组
extern task_t *current_task[];

/*
typedef struct trapframe {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    uint32_t vec;
    uint32_t err;

    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;   // user esp
    uint32_t ss;
} trapframe_t;
*/

/* 简单的 copy_from_user（不做边界检查，适合测试） */
int copy_from_user(char *dst, const char *src, uint32_t n) {
    // 假设内核/页表设置允许直接从用户空间读取
    for (uint32_t i = 0; i < n; ++i) dst[i] = src[i];
    return 0;
}

/* sys_write(fd, buf, len) */
static int sys_write(uint32_t fd, const char *buf, uint32_t len) {
    if (fd != 1) return -1;
    // 为了简单，分配一个小内核缓冲（或逐字输出）
    // 这里逐字打印到 console（假设 console_putc 可用）
    for (uint32_t i = 0; i < len; ++i) {
        char c = buf[i];      // 直接读用户地址（因为现在内核映射在位）
        vga_putc(c);
    }
    return (int)len;
}

/* 进程退出实现 */
void do_exit(int code) {
    extern task_t *current_task[];
    extern int need_resched;
    task_t *task = current_task[logical_cpu_id()];

    if (!task) {
        printf("[do_exit] No current task!\n");
        return;
    }

    printf("[do_exit] Task %d exiting with code %d\n", task->pid, code);

    // 1. 标记任务为终止状态
    task->state = PS_TERMNAT;

    // 2. 回收用户栈内存（如果有）
    // 注意：task->user_stack 是虚拟地址，需要转换为物理地址
    if (task->user_stack != 0) {
        printf("[do_exit] Freeing user stack at 0x%x (virt)\n", (uint32_t)task->user_stack);
        // 假设 user_stack 存储的是物理地址（根据 do_fork 的实现）
        pmm_free_page((uint32_t)task->user_stack);
        task->user_stack = 0;
    }

    // 3. 回收页表和用户空间内存（除了内核映射）
    // 注意：task->pde 也是虚拟地址，需要转换为物理地址
    if (task->pde != 0 && task->pde != (uint32_t*)0x101000) {
        printf("[do_exit] Freeing user page directory at 0x%x (virt)\n", (uint32_t)task->pde);
        // TODO: 遍历并释放用户空间的页表和页面
        // 这里暂时只释放页目录本身
        // pmm_free_page 需要物理地址，但我们这里只有虚拟地址
        // 暂时跳过，需要实现 virt_to_phys 转换
        printf("[do_exit] TODO: Need virt_to_phys conversion for PDE\n");
        task->pde = 0;
    }

    // 4. 回收 trapframe（如果在内核栈中）
    if (task->tf != 0) {
        printf("[do_exit] Trapframe was at 0x%x (will be freed with kstack)\n", (uint32_t)task->tf);
        task->tf = 0;
    }

    // 5. 注意：内核栈 (kstack) 暂时不释放
    // 原因：我们还在内核栈上运行！
    // 这个内存会在进程被完全清理时回收
    printf("[do_exit] Kernel stack at 0x%x (keeping for now)\n", task->kstack);

    // 6. 从调度链表中移除任务
    // TODO: 实现双向链表的删除操作
    printf("[do_exit] Task %d marked as terminated\n", task->pid);

    // 7. 触发重新调度，让其他任务运行
    need_resched = 1;

    // 8. 如果这是唯一的活动任务，进入空闲循环
    // 检查是否还有其他可运行的任务
    task_t *next = task->next;
    if (next == task || next->state == PS_TERMNAT || next->state == PS_DESTROY) {
        printf("[do_exit] No more runnable tasks, halting...\n");
        __asm__ volatile("cli");
        while (1) {
            __asm__ volatile("hlt");
        }
    }

    // 注意：这里不应该直接返回到用户空间
    // 调度器会选择下一个任务运行
}

/* sys_exit(code) */
static void sys_exit(int code) {
    do_exit(code);

    // 永远不会到达这里，因为 do_exit 会触发调度
    // 但为了安全起见，进入死循环
    while (1) {
        __asm__ volatile("hlt");
    }
}

enum {
    SYS_PRINTF = 1,
    SYS_EXIT,
    SYS_YIELD,
    SYS_GET_MEM_STATS,
    SYS_READ_MEM,
    SYS_GETCHAR,      // 新增：读取单个字符
    SYS_KBHIT,        // 新增：检查是否有按键
    SYS_PUTCHAR,      // 新增：输出单个字符(用于显示提示符)
    SYS_GET_FRAMEBUFFER,  // 新增：获取 framebuffer 信息
    SYS_GETCWD,       // 新增：获取当前工作目录
    SYS_WRITE,        // 占位符，使 SYS_FORK = 11
    SYS_FORK,         // fork 系统调用 (11)
    SYS_OPEN = 20,    // open 系统调用
    SYS_CLOSE,        // close 系统调用
    SYS_READ,         // read 系统调用
    SYS_LSEEK,        // lseek 系统调用
};

void syscall_dispatch(struct trapframe *tf) {
    // ⚠️ 临时禁用调试输出，减少串口输出量
    // printf("[syscall_dispatch] ENTER: tf=%p\n", tf);
    // printf("  eax=%d (syscall num), ebx=0x%x (arg1), ecx=0x%x (arg2), edx=0x%x (arg3)\n",
    //        tf->eax, tf->ebx, tf->ecx, tf->edx);
    // printf("  trapno=%d, eip=0x%x, cs=0x%x, ds=0x%x\n", tf->trapno, tf->eip, tf->cs, tf->ds);

    uint32_t num = tf->eax;
    uint32_t arg1 = tf->ebx;
    uint32_t arg2 = tf->ecx;
    uint32_t arg3 = tf->edx;

    switch (num) {
        case SYS_PRINTF: {
            // 兼容旧CPU的方法:使用pushf/popf设置EFLAGS.AC位
            const char *user_fmt = (const char*)arg1;
            char kbuf[512];
            int i = 0;

            // 直接用汇编拷贝,避免编译器优化和SMAP问题
            for (i = 0; i < 511; i++) {
                char c;
                // 使用pushf/popf临时设置EFLAGS.AC位
                __asm__ volatile (
                    "pushfl\n"                    // 保存EFLAGS
                    "orl $0x40000, (%%esp)\n"    // 设置AC位(bit 18)
                    "popfl\n"                     // 恢复EFLAGS(现在AC=1)

                    "movb (%1), %0\n"             // 读取用户空间字符

                    "pushfl\n"
                    "andl $~0x40000, (%%esp)\n"  // 清除AC位
                    "popfl\n"                     // 恢复EFLAGS

                    : "=&r"(c)
                    : "r"(user_fmt + i)
                    : "memory", "cc"
                );
                if (c == '\0') break;
                kbuf[i] = c;
            }
            kbuf[i] = '\0';

            // ⚠️⚠️⚠️ 启用输出：直接使用 vga_putc 输出每个字符
            for (int j = 0; j < i; j++) {
                vga_putc(kbuf[j]);
            }
            tf->eax = i;  // 返回输出的字符数
            break;
        }
        case SYS_EXIT:
            // ⚠️ 暂时禁用 printf，避免破坏 ES 寄存器
            // printf("[user] exit code=%d\n", arg1);
            do_exit(arg1);
            // do_exit() 会触发调度，不会返回到这里
            // 但为了安全，添加死循环
            tf->eax = 0;
            break;
        case SYS_YIELD: {
            // 让出CPU,调度其他任务
            extern int need_resched;
            need_resched = 1;
            tf->eax = 0;
            break;
        }
        case SYS_GET_MEM_STATS: {
            // 获取内存统计
            extern uint32_t buddy_get_total_pages(void);
            extern uint32_t buddy_get_free_pages(void);
            extern uint32_t buddy_get_used_pages(void);

            struct mem_stats {
                uint32_t total_pages;
                uint32_t free_pages;
                uint32_t used_pages;
            } *stats = (struct mem_stats*)arg1;

            if (stats) {
                stats->total_pages = buddy_get_total_pages();
                stats->free_pages = buddy_get_free_pages();
                stats->used_pages = buddy_get_used_pages();
                tf->eax = 0;
            } else {
                tf->eax = -1;
            }
            break;
        }
        case SYS_READ_MEM: {
            // 读取内存地址
            uint32_t addr = arg1;
            uint32_t *value = (uint32_t*)arg2;

            if (value && addr >= 0xC0000000) {
                *value = *(uint32_t*)addr;
                tf->eax = 0;
            } else {
                tf->eax = -1;
            }
            break;
        }
        case SYS_GETCHAR: {
            // 从键盘读取一个字符
            extern int keyboard_getchar(void);
            int c = keyboard_getchar();
            tf->eax = c;
            break;
        }
        case SYS_KBHIT: {
            // 检查是否有按键可用
            extern int keyboard_kbhit(void);
            int hit = keyboard_kbhit();
            tf->eax = hit;
            break;
        }
        case SYS_PUTCHAR: {
            // 输出单个字符(字符在EBX中)
            printf("[syscall] SYS_PUTCHAR: char=0x%x ('%c')\n", arg1, (char)arg1);
            char c = (char)arg1;
            extern void vga_putc(char);
            vga_putc(c);
            tf->eax = 0;
            printf("[syscall] SYS_PUTCHAR: done, eax=0\n");
            break;
        }
        case SYS_GET_FRAMEBUFFER: {
            // 获取 framebuffer 信息
            struct framebuffer_info {
                uint32_t addr;
                uint32_t width;
                uint32_t height;
                uint32_t pitch;
                uint8_t bpp;
            } *fb = (struct framebuffer_info*)arg1;

            if (fb && multiboot2_info_addr) {
                // 遍历 multiboot2 标签查找 framebuffer 信息
                // Multiboot 2 info: [0-3] size, [4-7] reserved, [8+] tags
                uint32_t *mb_info_ptr = (uint32_t *)phys_to_virt(multiboot2_info_addr);
                multiboot_tag_t *tag = (multiboot_tag_t *)((uint8_t *)mb_info_ptr + 8);
                while (tag->type != MULTIBOOT_TAG_TYPE_END) {
                    if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
                        multiboot_tag_framebuffer_t *fb_info = (multiboot_tag_framebuffer_t *)tag;
                        fb->addr = (uint32_t)(fb_info->framebuffer_addr & 0xFFFFFFFF);
                        fb->width = fb_info->framebuffer_width;
                        fb->height = fb_info->framebuffer_height;
                        fb->pitch = fb_info->framebuffer_pitch;
                        fb->bpp = fb_info->framebuffer_bpp;
                        tf->eax = 0;
                        break;
                    }
                    tag = (multiboot_tag_t *)((uint8_t *)tag + ((tag->size + 7) & ~7));
                }
                if (tag->type == MULTIBOOT_TAG_TYPE_END) {
                    tf->eax = -1;
                }
            } else {
                tf->eax = -1;
            }
            break;
        }
        case SYS_GETCWD: {
            // getcwd(buf, size) - 获取当前工作目录
            char *buf = (char*)arg1;
            uint32_t size = arg2;

            if (buf && size >= 2) {
                // 简化版：所有进程的当前工作目录都是根目录
                buf[0] = '/';
                buf[1] = '\0';
                tf->eax = 1;  // 返回长度（不包括null）
            } else {
                tf->eax = -1;  // 错误：缓冲区太小或为NULL
            }
            break;
        }
        case SYS_WRITE: {
            // write(fd, buf, len) - arg1=fd, arg2=buf, arg3=len
            int fd = (int)arg1;
            const char *user_buf = (const char*)arg2;
            uint32_t len = arg3;

            if (fd == 1 && len < 512) {  // stdout，限制长度
                // 先从用户空间拷贝到内核缓冲区
                char kbuf[512];
                int copied = 0;

                // 使用汇编拷贝，避免SMAP问题
                for (uint32_t i = 0; i < len; i++) {
                    char c;
                    __asm__ volatile (
                        "pushfl\n"                    // 保存EFLAGS
                        "orl $0x40000, (%%esp)\n"    // 设置AC位
                        "popfl\n"

                        "movb (%1), %0\n"             // 读取用户空间字符

                        "pushfl\n"
                        "andl $~0x40000, (%%esp)\n"  // 清除AC位
                        "popfl\n"

                        : "=&r"(c)
                        : "r"(user_buf + i)
                        : "memory", "cc"
                    );
                    kbuf[i] = c;
                    copied++;
                }

                // 输出到 VGA
                for (int i = 0; i < copied; i++) {
                    vga_putc(kbuf[i]);
                }
                tf->eax = copied;
            } else {
                tf->eax = -1;
            }
            break;
        }
        case SYS_FORK: {
            // fork() 系统调用 - 创建子进程
            // 返回值：父进程返回子进程PID，子进程返回0
            extern task_t* do_fork(void);
            task_t *child = do_fork();
            if (child) {
                // 父进程：返回子进程的 PID
                tf->eax = child->pid;
                // ⚠️ 暂时禁用 printf，避免破坏 ES 寄存器
                // printf("[fork] Parent PID=%d, Child PID=%d\n", current_task[logical_cpu_id()]->pid, child->pid);
            } else {
                // 子进程或失败：返回 0
                tf->eax = 0;
            }
            break;
        }
        case SYS_OPEN: {
            // open(pathname, flags)
            const char *pathname = (const char*)arg1;
            int flags = (int)arg2;

            printf("[syscall] SYS_OPEN: pathname=0x%x, flags=%d\n", (uint32_t)pathname, flags);

            // 从用户空间拷贝路径名
            // 使用页表遍历获取用户地址的物理地址
            char kpath[256];
            int i = 0;

            printf("[syscall] Reading from user address 0x%x\n", (uint32_t)pathname);

            // 手动实现页表遍历，避免调用 get_physical_address 导致的页错误
            extern uint32_t kernel_page_directory_phys;
            uint32_t *pd_virt = (uint32_t*)phys_to_virt(kernel_page_directory_phys);

            // 检查是否在用户空间范围
            printf("[syscall] DEBUG: pathname=0x%x, 0xC0000000=%d\n",
                   (uint32_t)pathname, ((uint32_t)pathname < 0xC0000000));
            if ((uint32_t)pathname < 0xC0000000) {
                printf("[syscall] User space address, attempting page table walk...\n");

                // 手动遍历页表获取物理地址
                uint32_t str_virt = (uint32_t)pathname;
                uint32_t pd_idx = (str_virt >> 22) & 0x3FF;
                uint32_t pt_idx = (str_virt >> 12) & 0x3FF;
                uint32_t page_offset = str_virt & 0xFFF;

                printf("[syscall] pd_idx=%d, pt_idx=%d, offset=0x%x\n", pd_idx, pt_idx, page_offset);

                // 检查 PDE
                uint32_t pde_entry = pd_virt[pd_idx];
                printf("[syscall] pde_entry=0x%x\n", pde_entry);

                if (!(pde_entry & 0x1)) {
                    printf("[syscall] ERROR: PDE not present!\n");
                    kpath[0] = '\0';
                } else {
                    // 获取页表物理地址
                    uint32_t pt_phys = pde_entry & ~0xFFF;

                    printf("[syscall] Page table at phys=0x%x\n", pt_phys);

                    // 使用 map_highmem_physical 映射页表
                    extern void* map_highmem_physical(uint32_t phys_addr, uint32_t size, uint32_t flags);
                    uint32_t *pt_virt = (uint32_t*)map_highmem_physical(pt_phys, 4096, 0);

                    if (pt_virt != NULL) {
                        uint32_t pte = pt_virt[pt_idx];
                        printf("[syscall] pte=0x%x\n", pte);

                        if (pte & 0x1) {
                            uint32_t phys_page = pte & ~0xFFF;
                            printf("[syscall] phys_page=0x%x\n", phys_page);

                            // 使用 map_highmem_physical 动态映射用户物理页
                            uint8_t *user_page_virt = (uint8_t*)map_highmem_physical(phys_page, 4096, 0);

                            printf("[syscall] user_page_virt=0x%x\n", (uint32_t)user_page_virt);

                            if (user_page_virt != NULL) {
                                // 验证映射：尝试直接读取映射地址的字符串位置（page_offset）
                                printf("[syscall] Test read: user_page_virt[%d]=0x%x\n", page_offset, user_page_virt[page_offset]);
                                printf("[syscall] Test read: user_page_virt[%d]=0x%x\n", page_offset+1, user_page_virt[page_offset+1]);
                                printf("[syscall] Test read: user_page_virt[%d]=0x%x\n", page_offset+2, user_page_virt[page_offset+2]);

                                // 打印前16字节
                                printf("[syscall] Raw data: ");
                                for (int j = 0; j < 16; j++) {
                                    printf("%02x ", user_page_virt[page_offset + j]);
                                }
                                printf("\n");

                                // 复制字符串
                                int i;
                                for (i = 0; i < 255; i++) {
                                    kpath[i] = user_page_virt[page_offset + i];
                                    if (kpath[i] == '\0') break;
                                }
                                kpath[i] = '\0';

                                printf("[syscall] Copied path: '%s' (len=%d)\n", kpath, i);
                                printf("[syscall] kpath[0]=0x%x ('%c'), kpath[1]=0x%x\n",
                                       (unsigned char)kpath[0], kpath[0] ? kpath[0] : '?',
                                       (unsigned char)kpath[1]);
                            } else {
                                printf("[syscall] ERROR: Failed to map user page!\n");
                                kpath[0] = '\0';
                            }
                        } else {
                            printf("[syscall] ERROR: PTE not present!\n");
                            kpath[0] = '\0';
                        }
                    } else {
                        printf("[syscall] ERROR: Failed to map page table!\n");
                        kpath[0] = '\0';
                    }
                }
            } else {
                printf("[syscall] Kernel space address, copying directly\n");
                // 内核空间地址，直接复制
                for (i = 0; i < 255; i++) {
                    kpath[i] = pathname[i];
                    if (kpath[i] == '\0') break;
                }
                kpath[i] = '\0';
                printf("[syscall] Copied path: '%s' (len=%d)\n", kpath, i);
            }

            // 调用 VFS 层
            extern struct file *filp_open(const char *, int);
            struct file *file = filp_open(kpath, flags);
            if (file) {
                // ⚠️ 简化版：使用文件指针作为 fd
                // 后续需要实现 fd 表
                tf->eax = (int)file;
            } else {
                tf->eax = -1;
            }
            break;
        }
        case SYS_CLOSE: {
            // close(fd)
            int fd = (int)arg1;
            struct file *file = (struct file*)fd;

            // 调用 VFS 层
            extern int filp_close(struct file *);
            int ret = filp_close(file);
            tf->eax = ret;
            break;
        }
        case SYS_READ: {
            // read(fd, buf, len)
            int fd = (int)arg1;
            char *user_buf = (char*)arg2;
            uint32_t len = arg3;
            struct file *file = (struct file*)fd;

            // 调用 VFS 层（先读入内核缓冲区）
            extern int filp_read(struct file *, char *, uint32_t);
            char kbuf[512];
            uint32_t to_read = (len < 512) ? len : 512;
            int ret = filp_read(file, kbuf, to_read);

            if (ret > 0) {
                // 拷贝到用户空间
                for (int i = 0; i < ret; i++) {
                    char c = kbuf[i];
                    __asm__ volatile (
                        "pushfl\n"
                        "orl $0x40000, (%%esp)\n"
                        "popfl\n"
                        "movb %0, (%1)\n"
                        "pushfl\n"
                        "andl $~0x40000, (%%esp)\n"
                        "popfl\n"
                        :
                        : "r"(c), "r"(user_buf + i)
                        : "memory", "cc"
                    );
                }
            }
            tf->eax = ret;
            break;
        }
        case SYS_LSEEK: {
            // lseek(fd, offset, whence)
            int fd = (int)arg1;
            int offset = (int)arg2;
            int whence = (int)arg3;
            struct file *file = (struct file*)fd;

            // 调用 VFS 层
            extern int filp_lseek(struct file *, int64_t, int);
            int ret = filp_lseek(file, (int64_t)offset, whence);
            tf->eax = ret;
            break;
        }
        default:
            // ⚠️ 暂时禁用 printf，避免破坏 ES 寄存器
            // printf("[syscall] unknown num=%d\n", num);
            tf->eax = -1;
            break;
    }

    // 不要在这里切换CR3!
    // CR3应该只在任务切换时切换,不应该在每次系统调用时切换
    // Linux 0.11也是在系统调用返回时不切换CR3的
}
