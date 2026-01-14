// sched_test5.c - 测试5个并发进程的调度
// 使用内联汇编直接调用系统调用

#include <stdint.h>

// 系统调用号定义
#define SYS_PUTCHAR 8
#define SYS_YIELD 3
#define SYS_EXIT 2
#define SYS_FORK 11

// 系统调用包装函数(使用内联汇编)
static inline void sys_putchar(char c) {
    // ⚠️ 关键修复：使用正确的寄存器约束
    // "a" 约束强制 GCC 使用 EAX 寄存器存储系统调用号
    // "b" 约束强制 GCC 使用 EBX 寄存器存储参数
    __asm__ volatile("int $0x80" : : "a"(SYS_PUTCHAR), "b"(c));
}

static inline void sys_yield(void) {
    __asm__ volatile("int $0x80" : : "a"(SYS_YIELD));
}

static inline void sys_exit(int code) {
    __asm__ volatile("int $0x80" : : "a"(SYS_EXIT), "b"(code));
}

static inline int sys_fork(void) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_FORK));
    return ret;
}

void print_num(int n) {
    if (n == 0) {
        sys_putchar('0');
        return;
    }

    char buf[16];
    int i = 0;

    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }

    while (i > 0) {
        sys_putchar(buf[--i]);
    }
}

// 每个进程的执行函数
void process_func(int id) {
    for (int round = 0; round < 5; round++) {
        // 输出进程ID和轮次
        sys_putchar('P');
        print_num(id);
        sys_putchar('-');
        print_num(round);
        sys_putchar(' ');

        // 让出CPU，让其他进程运行
        sys_yield();
    }

    sys_putchar('\n');
    sys_exit(0);
}

void _start() {
    // ⚠️ 移除寄存器初始化，避免干扰系统调用的寄存器设置
    // task_to_user_mode_with_task 已经正确设置了段寄存器

    sys_putchar('S');
    sys_putchar('t');
    sys_putchar('a');
    sys_putchar('r');
    sys_putchar('t');
    sys_putchar('\n');

    // 创建第一个子进程（进程1）
    int pid1 = sys_fork();
    if (pid1 == 0) {
        process_func(1);
        sys_exit(0);
    }

    // 父进程继续创建第二个子进程（进程2）
    int pid2 = sys_fork();
    if (pid2 == 0) {
        process_func(2);
        sys_exit(0);
    }

    // 父进程继续创建第三个子进程（进程3）
    int pid3 = sys_fork();
    if (pid3 == 0) {
        process_func(3);
        sys_exit(0);
    }

    // 父进程继续创建第四个子进程（进程4）
    int pid4 = sys_fork();
    if (pid4 == 0) {
        process_func(4);
        sys_exit(0);
    }

    // 父进程本身就是进程0
    process_func(0);

    // 等待所有子进程结束（简化版本，直接退出）
    sys_exit(0);
}
