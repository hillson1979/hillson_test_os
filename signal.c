#include "printf.h"
#include "signal.h"
#include "task.h"
// 信号编号定义
#define SIGINT  2   // 中断信号 (Ctrl+C)
#define SIGTERM 15  // 终止信号
#define SIGUSR1 10  // 用户自定义信号1

// 全局当前进程指针（与汇编中的current对应）
extern struct task_t *current;

// 默认信号处理函数（终止进程）
static void default_signal_handler(int signum) {
    // 在实际系统中，这里会执行进程终止逻辑
    // 如释放资源、通知父进程等
    //while (1); // 简单起见，这里用死循环表示进程终止
    printf("enter a signal handler");
}

// 信号处理函数表（每个信号对应一个处理函数）
static sighandler_t signal_handlers[32] = {
    [SIGINT] = default_signal_handler,
    [SIGTERM] = default_signal_handler,
    [SIGUSR1] = default_signal_handler,
    // 其他信号默认使用默认处理函数
};

// 投递信号：将信号加入进程的待处理信号集
void send_signal(struct task_t *task, int signum) {
    if (signum < 1 || signum >= 32) return;
    
    // 将信号设置到待处理信号集中
    task->pending_signals |= (1 << (signum - 1));
    task->has_signal = 1; // 标记有信号待处理
}

// 处理待处理信号
void deliver_signal(void) {
    if (!current || current->pending_signals == 0) {
        return; // 没有当前进程或没有待处理信号
    }

    // 查找第一个待处理的信号
    int signum;
    for (signum = 1; signum < 32; signum++) {
        // 检查信号是否在待处理集合中，且未被屏蔽
        if ((current->pending_signals & (1 << (signum - 1))) &&
            !(current->signal_mask & (1 << (signum - 1)))) {
            
            // 清除该信号的待处理标志
            current->pending_signals &= ~(1 << (signum - 1));
            
            // 如果所有信号都处理完了，清除has_signal标志
            if (current->pending_signals == 0) {
                current->has_signal = 0;
            }
            
            // 获取该信号的处理函数
            sighandler_t handler = signal_handlers[signum];
            if (!handler) {
                handler = default_signal_handler;
            }
            
            // 设置当前进程的信号处理函数（供汇编代码使用）
            current->sig_handler = handler;
            
            // 保存用户态栈指针（供信号处理完成后恢复）
            current->user_stack = current->esp;
            
            // 可以在这里添加信号处理前的额外逻辑
            // 如保存寄存器状态、设置信号掩码等
            
            return;
        }
    }
}

// 注册信号处理函数（供用户态程序调用）
sighandler_t signal(int signum, sighandler_t handler) {
    if (signum < 1 || signum >= 32 || !handler) {
        return (sighandler_t)-1;
    }
    
    sighandler_t old_handler = signal_handlers[signum];
    signal_handlers[signum] = handler;
    return old_handler;
}
