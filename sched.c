#include "time.h"
//#include "llist.h"
#include "task.h"
#include "sched.h"
#include "segment.h"  // 添加 segment.h 以获取 TSS 和段定义
#include "x86/mmu.h"  // 添加段定义
#include "lapic.h"    // 添加 logical_cpu_id

#ifndef U64_MAX
#define U64_MAX 0xFFFFFFFFFFFFFFFFULL
#endif
extern void
llist_delete(struct llist_header* elem);
extern task_t* current_task[];  // 修正为数组类型
// 类似Linux的权重计算：nice值每变化1，权重变化约10%
 int prio_to_weight[40] = {
        /* -20 */ 88761, 71755, 56483, 46273, 36291,
        /* -15 */ 29154, 23254, 18705, 14949, 11916,
        /* -10 */ 9548, 7620, 6100, 4904, 3906,
        /* -5 */ 3121, 2501, 1991, 1586, 1277,
        /* 0 */ 1024, 820, 655, 526, 423,
        /* 5 */ 335, 272, 215, 172, 137,
        /* 10 */ 110, 87, 70, 56, 45,
        /* 15 */ 36, 29, 23, 18, 15,
 };

// 权重计算函数实现
int calculate_weight(int nice)
{
    if (nice < -20) nice = -20;
    if (nice > 19) nice = 19;
    return prio_to_weight[nice + 20];
}

// 在你的代码中添加 64 位无符号除法实现
unsigned long long __udivdi3(unsigned long long num, unsigned long long den)
{
    unsigned long long quot = 0;
    unsigned long long rem = 0;
    
    for (int i = 63; i >= 0; i--) {
        rem = (rem << 1) | ((num >> i) & 1);
        if (rem >= den) {
            rem -= den;
            quot |= (1ULL << i);
        }
    }
    
    return quot;
}

//static uint64_t calculate_inverse_weight(int weight);
#ifndef UINT64_MAX
#define UINT64_MAX ((uint64_t)-1)
#endif

// 如果 weight 不是太大，可以使用 32 位运算
static uint64_t calculate_inverse_weight(int weight)
{
    if (weight == 0) return UINT64_MAX; // 防止除零
    if (weight == 1) return (1ULL << 32);
    
    // 使用移位避免除法
    if (weight <= (1 << 16)) {
        return (1ULL << 32) / weight;
    } else {
        // 对于大权重，使用近似计算
        return (1ULL << 16) / (weight >> 16);
    }
}


void
task_setrun(struct task_t* thread)
{
    thread->state = PS_RUNNING;
    //thread->process->state = PS_RUNNING;
    //thread->process->th_active = thread;
}


bool
can_schedule(struct task_t* thread)
{
    if (!thread) {
        return 0;
    }

    if (task_terminated(thread)) {
        return false;
    }

    //if (preempt_check_stalled(thread)) {
       // thread_flags_set(thread, TH_STALLED);
       // return true;
    //}

    if (thread->user_stack==0) {
        // a kernel process is always runnable
        return thread->state == PS_READY;
    }

    /*struct sigctx* sh = &thread->sigctx;

    if ((thread->state & PS_PAUSED)) {
        return !!(sh->sig_pending & ~1);
    }

    if ((thread->state & PS_BLOCKED)) {
        return sigset_test(sh->sig_pending, _SIGINT);
    }

    if (sigset_test(sh->sig_pending, _SIGSTOP)) {
        // If one thread is experiencing SIGSTOP, then we know
        // all other threads are also SIGSTOP (as per POSIX-2008.1)
        // In which case, the entire process is stopped.
        thread->state = PS_STOPPED;
        return false;
    }
    
    if (sigset_test(sh->sig_pending, _SIGCONT)) {
        thread->state = PS_READY;
    }*/

    return (thread->state == PS_READY) \
            && task_runnable(thread);
}


void
check_sleepers()
{
    struct task_t *pos, *n;
    time_t now = (uint32_t)2500/1000;//clock_systime() / 1000;

    llist_for_each(pos, n, sched_sleep, sleep.sleepers)
    {
        if (task_terminated(pos)) {
            goto del;
        }

        time_t wtime = pos->sleep.wakeup_time;
        time_t atime = pos->sleep.alarm_time;

        if (wtime && now >= wtime) {
            pos->sleep.wakeup_time = 0;
            pos->state = PS_READY;
        }

        if (atime && now >= atime) {
            pos->sleep.alarm_time = 0;
            //thread_setsignal(pos, _SIGALRM);//signal.c
        }

        if (!wtime && !atime) {
        del:
            llist_delete(&pos->sleep.sleepers);
        }
    }

}

// 简单的时间片分配，基于nice值
unsigned int get_time_slice(struct task_t *task)
{
    int nice = task->nice;
    unsigned int base_time_slice = 100; // 基准时间片（ms）
    
    // nice值范围通常为-20到19，值越小优先级越高
    if (nice < 0) {
        // 高优先级任务获得更长的时间片
        return base_time_slice * (20 - nice) / 20;
    } else {
        // 低优先级任务获得较短的时间片
        return base_time_slice * (20 - nice) / 20;
    }
}

static void update_vruntime(struct task_t *task)
{
    uint64_t delta_exec = get_time_slice(task) - task->time_slice;
    
    if (delta_exec > 0) {
        // 计算权重倒数，用于虚拟时间更新
        uint64_t inv_weight = calculate_inverse_weight(task->load_weight);
        
        // 虚拟运行时间 = 实际运行时间 * (NICE_0_LOAD / 当前任务权重)
        task->vruntime += delta_exec * inv_weight >> 32;
        
        task->time_slice = get_time_slice(task);
    }
} 
// kernel/sched.c
uint32_t preempt_count = 0;

void enable_preemption(void)
{
    preempt_count--;
}

void no_preemption(void)
{
    preempt_count++;
}


// 获取链表中的第一个任务
static struct task_t* get_first_task(void)
{
    if (sched_root->next == &sched_root) {  // 空链表
        return NULL;
    }
    return list_entry(sched_root->next, task_t, sched_node);
}


// 简化的轮转调度器 - 选择下一个就绪任务
static struct task_t *pick_next_task_cfs()
{
    uint8_t cpu_id = logical_cpu_id();
    struct task_t *current = current_task[cpu_id];
    struct task_t *next = NULL;

    if (!current) {
        return NULL;
    }

    // 如果当前任务正在运行，标记为就绪
    if (current->state == PS_RUNNING) {
        current->state = PS_READY;
    }

    // 简单的轮转：从任务链表中找下一个就绪任务
    next = current->next;
    while (next != NULL && next != current) {
        if (next->state == PS_READY && can_schedule(next)) {
            break;
        }
        next = next->next;
    }

    // 如果没找到其他就绪任务，保持当前任务
    if (next == current || next == NULL) {
        next = current;
    }

    return next;
}

//schedule() 调用一次 pick_next_task
void schedule(void) {
    struct task_t *prev, *next;
    uint32_t flags;
    uint8_t cpu_id = logical_cpu_id();

    /* 保护临界区 */
    __asm__ __volatile__("pushfl; popl %0; cli" : "=r"(flags));

    prev = current_task[cpu_id];
    if (!prev) {
        printf("[schedule] No current task!\n");
        __asm__ __volatile__("pushl %0; popfl" : : "r"(flags));
        return;
    }

    //调用一次决策函数，选出下一个任务
    next = pick_next_task_cfs();
    if (!next) {
        printf("[schedule] No next task available!\n");
        __asm__ __volatile__("pushl %0; popfl" : : "r"(flags));
        return;
    }

    task_setrun(next);

    if (prev == next) {
        __asm__ __volatile__("pushl %0; popfl" : : "r"(flags));
        return;
    }

    printf("[schedule] CPU%d: switch from task_%d to task_%d\n",
           cpu_id, prev->pid, next->pid);

    /* 恢复中断并执行上下文切换 */
    __asm__ __volatile__("pushl %0; popfl" : : "r"(flags));
    switch_to(prev, next);// 执行实际的上下文切换
}

// 外部汇编函数 - 实际的上下文切换
extern void context_switch(struct task_t *prev, struct task_t *next);

static inline void switch_to(struct task_t *prev, struct task_t *next) {
    if (prev == next) {
        return;  // 不需要切换
    }

    // 保存当前任务的内核栈指针
    uint32_t saved_esp;
    asm volatile("movl %%esp, %0" : "=r"(saved_esp));
    prev->esp = saved_esp;

    // 切换页表（如果需要）
    if (prev->pde != next->pde) {
        asm volatile("movl %0, %%cr3" : : "r"(next->pde) : "memory");
    }

    // 恢复下一个任务的内核栈指针
    asm volatile("movl %0, %%esp" : : "r"(next->esp));

    // 更新全局当前任务指针
    current_task[next->cpu] = next;

    // 更新 TSS.esp0
    extern struct tss_t tss;
    tss.ss0 = SEG_KDATA << 3;
    tss.esp0 = (uint32_t)next->kstack;

    // 注意：实际的寄存器恢复在 interrupt_exit 中完成
}

/**
 * 推荐的高效抢占式调度器
 */
void efficient_scheduler_loop() {
    uint8_t cpu = logical_cpu_id();
    
    for (;;) {
        /* 主调度循环 */
        schedule();
        
        /* 处理空闲状态 */
        handle_idle_state(cpu);
    }
}
