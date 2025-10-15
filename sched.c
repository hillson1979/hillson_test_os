#include "time.h"
//#include "llist.h"
#include "task.h"
#include "sched.h"

#ifndef U64_MAX
#define U64_MAX 0xFFFFFFFFFFFFFFFFULL
#endif
extern void
llist_delete(struct llist_header* elem);
extern task_t* current_task;
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

static struct task_t *pick_next_task() 
{
    // 禁用抢占，保护调度过程
    no_preemption();

    // 如果当前任务仍然可运行，将其状态设置为就绪
    if (current_task->state == PS_RUNNING) {
        current_task->state = PS_READY;
    }

    // 检查是否有睡眠任务需要唤醒
    check_sleepers();

    // round-robin 调度算法
    struct task_t* current = current_task;
    struct task_t* to_check = current;
    bool found = false;
    
    // 遍历任务列表寻找可调度任务
    do {
        to_check = list_next(to_check->sched_node, llist_header_t, next);

        if (can_schedule(to_check)) {
            found = true;
            break;
        }

        // 如果遍历完所有任务都没有找到可调度的
        if (to_check == current) {
            break;
        }

    } while (1);

    // 如果没有找到可调度任务，返回空闲任务或当前任务
    if (!found) {
        // 可以选择返回空闲任务而不是当前任务
        // to_check = idle_task;
        printf("Warning: No runnable threads found!\n");
    }

    // 恢复抢占
    enable_preemption();
    
    return to_check;
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


static struct task_t *pick_next_task_cfs() 
{
    no_preemption();

    if (current_task->state == PS_RUNNING) {
        current_task->state = PS_READY;
        update_vruntime(current_task);
    }

    check_sleepers();

    struct task_t* next_task = NULL;
    struct task_t* best_task = NULL;
    uint64_t min_vruntime = U64_MAX;
    
    // 遍历所有任务寻找最优候选
    struct task_t* pos = get_first_task();
    if (!pos) {
        goto done;
    }
    
    struct task_t* start = pos;
    
    do {
        // 检查任务是否可运行
        if ((pos->state == PS_READY || pos->idle_flags == 1) && 
            can_schedule(pos)) {
            
            // 更新最小虚拟运行时间任务
            if (pos->vruntime < min_vruntime) {
                min_vruntime = pos->vruntime;
                best_task = pos;
            }
        }
        
        pos = list_next(pos->sched_node, struct llist_header, next);
    } while (pos && pos != start);

done:
    next_task = best_task ? best_task : current_task;
    
    if (!best_task) {
        printf("No better task found, keeping current\n");
    }

    enable_preemption();
    return next_task;
}

//schedule() 调用一次 pick_next_task
void schedule(void) {
    struct task_t *prev, *next;
    uint32_t flags; 
    
    /* 保护临界区 */
    __asm__ __volatile__("pushfl; popl %0; cli" : "=r"(flags));
    prev =current_task ;
    
    //调用一次决策函数，选出下一个任务 
    next = pick_next_task_cfs(); 
    task_setrun(next);
    
   
    if (prev == next) {
        // ... 处理一些边界情况，然后可能直接解锁返回...
        return;
    }
    /* 恢复中断并执行上下文切换 */
    __asm__ __volatile__("pushl %0; popfl" : : "r"(flags));
    switch_to(prev, next);// 执行实际的上下文切换
}

static inline void  switch_to(struct task_t *prev,struct task_t *next) {
    asm volatile("jmp switch_to\n");
    //unreachable;
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
