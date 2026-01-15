// syscall.c
#include "vga.h"
#include "interrupt.h"
#include "printf.h"   // 需要一个 console_write(buf,len) 或 putchar
#include "task.h"      // do_exit(), current task, etc
#include "multiboot2.h" // multiboot2_info_addr
#include "highmem_mapping.h"
#include "page.h"
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

    // 标记任务为终止状态
    task->state = PS_TERMNAT;

    // 触发重新调度，让其他任务运行
    need_resched = 1;

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
    SYS_WRITE,        // 占位符，使 SYS_FORK = 11
    SYS_FORK,         // fork 系统调用 (11)
};

void syscall_dispatch(struct trapframe *tf) {
    // 调试:打印trapframe字段地址
    // printf("[syscall] tf=%p\n", tf);
    // printf("  &tf->eax=%p, &tf->ebx=%p, &tf->ecx=%p\n", &tf->eax, &tf->ebx, &tf->ecx);
    // printf("  tf->eax=%d, tf->ebx=%x, tf->ecx=%x, tf->edx=%x\n",
    //        tf->eax, tf->ebx, tf->ecx, tf->edx);
    // printf("  tf->trapno=%d, tf->eip=%x\n", tf->trapno, tf->eip);

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
            char c = (char)arg1;
            extern void vga_putc(char);
            vga_putc(c);
            tf->eax = 0;
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
