// syscall.c
#include "vga.h"
#include "interrupt.h"
#include "printf.h"   // 需要一个 console_write(buf,len) 或 putchar
#include "task.h"      // do_exit(), current task, etc

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

/* sys_exit(code) */
static void sys_exit(int code) {
    // 简单实现：打印并标记任务退出。真实内核应释放资源并调度。
    printf("sys_exit(%d)\n", code);
    // 这里可以设置当前任务状态为 ZOMBIE 或直接 halt for demo:
    //do_exit(code); // 如果你已有实现
}

/*#define SYS_PRINTF 1
#define MAX_PRINTF_BUF 512

void syscall_dispatch_1(struct trapframe *tf) {
    uint32_t num = tf->ebx;         // syscall number来自用户态保存的tf
    switch (num) {
    case SYS_PRINTF: {
        const char *user_fmt = (const char *)tf->ecx; // 用户传来的指针
        char kbuf[MAX_PRINTF_BUF];
        // 为简单：拷贝最多 MAX_PRINTF_BUF-1 字节
        if (copy_from_user(kbuf, user_fmt, MAX_PRINTF_BUF - 1) != 0) {
            tf->eax = -1; // 返回错误
            break;
        }
        kbuf[MAX_PRINTF_BUF - 1] = '\0';
        // 现在用内核的 printf（或 vprintf）输出
        printf("%s", kbuf);
        tf->eax = 0; // 返回成功
        break;
    }
    default:
        printf("unknown syscall %u\n", num);
        tf->eax = (uint32_t)-1;
        break;
    }

    // syscall_dispatch 结束后通常会 return 到通用中断退栈路径，
    // 中断返回代码会用 tf 的值恢复用户态寄存器并 iret。
}*/

enum {
    SYS_PRINTF = 1,
    SYS_EXIT,
    SYS_YIELD,
    SYS_GET_MEM_STATS,
    SYS_READ_MEM,
    SYS_GETCHAR,      // 新增：读取单个字符
    SYS_KBHIT,        // 新增：检查是否有按键
    SYS_PUTCHAR,      // 新增：输出单个字符(用于显示提示符)
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

            // 输出字符串
            printf("%s", kbuf);
            tf->eax = 0;
            break;
        }
        case SYS_EXIT:
            printf("[user] exit code=%d\n", arg1);
            tf->eax = 0;
            break;
        case SYS_YIELD:
            // 让出CPU,调度其他任务
            tf->eax = 0;
            break;
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
        default:
            printf("[syscall] unknown num=%d\n", num);
            tf->eax = -1;
            break;
    }

    // 不要在这里切换CR3!
    // CR3应该只在任务切换时切换,不应该在每次系统调用时切换
    // Linux 0.11也是在系统调用返回时不切换CR3的
}

/* syscall dispatcher: eax = syscall_num, args in ebx/ecx/edx */
void syscall_dispatch_(struct trapframe *tf) {
    uint32_t nr = tf->eax;
    uint32_t a1 = tf->ebx;
    uint32_t a2 = tf->ecx;
    uint32_t a3 = tf->edx;

    switch (nr) {
    case 1: { // write(fd, buf, len)
        // 为安全，先把用户缓冲区拷到内核缓冲（此处为示例，直接读取用户地址也可）
        // 注意：a2 是用户虚拟地址
        char tmp[256];
        uint32_t tocopy = (a3 > sizeof(tmp)) ? sizeof(tmp) : a3;
        copy_from_user(tmp, (const char *)a2, tocopy);
        int wrote = sys_write(a1, tmp, tocopy);
        tf->eax = (uint32_t)wrote;
        break;
    }
    case 2: { // exit(code)
        sys_exit((int)a1);
        // not return
        tf->eax = 0;
        break;
    }
    default:
        tf->eax = (uint32_t)-1;
        break;
    }
}

