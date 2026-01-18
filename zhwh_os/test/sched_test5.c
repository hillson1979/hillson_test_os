// sched_test5.c - 测试5个并发进程的调度
// 使用汇编包装函数调用系统调用，避免内联汇编的寄存器冲突问题

#include <stdint.h>

// ⚠️ 使用汇编包装函数，避免内联汇编的寄存器冲突问题
extern void syscall_putchar(char c);
extern void syscall_yield(void);
extern void syscall_exit(int code) __attribute__((noreturn));
extern int syscall_fork(void);

void print_num(int n) {
    if (n == 0) {
        syscall_putchar('0');
        return;
    }

    char buf[16];
    int i = 0;

    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }

    while (i > 0) {
        syscall_putchar(buf[--i]);
    }
}

// 每个进程的执行函数
void process_func(int id) {
    for (int round = 0; round < 5; round++) {
        // 输出进程ID和轮次
        syscall_putchar('P');
        print_num(id);
        syscall_putchar('-');
        print_num(round);
        syscall_putchar(' ');

        // 让出CPU，让其他进程运行
        syscall_yield();
    }

    syscall_putchar('\n');
    syscall_exit(0);
}

void _start() {
    // ⚠️ 移除寄存器初始化，避免干扰系统调用的寄存器设置
    // task_to_user_mode_with_task 已经正确设置了段寄存器

    syscall_putchar('S');
    syscall_putchar('t');
    syscall_putchar('a');
    syscall_putchar('r');
    syscall_putchar('t');
    syscall_putchar('\n');

    // 创建第一个子进程（进程1）
    int pid1 = syscall_fork();
    if (pid1 == 0) {
        process_func(1);
        syscall_exit(0);
    }

    // 父进程继续创建第二个子进程（进程2）
    int pid2 = syscall_fork();
    if (pid2 == 0) {
        process_func(2);
        syscall_exit(0);
    }

    // 父进程继续创建第三个子进程（进程3）
    int pid3 = syscall_fork();
    if (pid3 == 0) {
        process_func(3);
        syscall_exit(0);
    }

    // 父进程继续创建第四个子进程（进程4）
    int pid4 = syscall_fork();
    if (pid4 == 0) {
        process_func(4);
        syscall_exit(0);
    }
    
    // 父进程本身就是进程0
    //process_func(0);
    syscall_putchar('@');
    syscall_putchar('@');
    syscall_putchar('@');
    syscall_putchar('@');
    syscall_putchar('@');
    syscall_putchar('@');
    // 等待所有子进程结束（简化版本，直接退出）
    //syscall_exit(0);

}
