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

    // 关键修复：支持 PS_READY 和 PS_CREATED 状态
    // PS_READY: 已经初始化，等待运行的任务
    // PS_CREATED: 首次运行的用户任务（刚完成初始化，需要跳转到用户态）
    return ((thread->state == PS_READY || thread->state == PS_CREATED) \
            && task_runnable(thread));
}


void
check_sleepers()
{
    struct llist_header *pos, *next;
    time_t now = (uint32_t)2500/1000;//clock_systime() / 1000;

    llist_for_each_safe(pos, next, sched_sleep)
    {
        // 从链表节点获取 task_t 结构体
        // pos 指向 task->sleep.sleepers
        // task = container_of(pos, struct task_t, sleep.sleepers)
        struct task_t *task = (struct task_t*)((char*)pos - __builtin_offsetof(struct task_t, sleep.sleepers));

        if (task_terminated(task)) {
            goto del;
        }

        time_t wtime = task->sleep.wakeup_time;
        time_t atime = task->sleep.alarm_time;

        if (wtime && now >= wtime) {
            task->sleep.wakeup_time = 0;
            task->state = PS_READY;
        }

        if (atime && now >= atime) {
            task->sleep.alarm_time = 0;
            //thread_setsignal(task, _SIGALRM);//signal.c
        }

        if (!wtime && !atime) {
        del:
            llist_del(pos);
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

// ⚠️⚠️⚠️ 全局变量：保存 schedule() 中 switch_to 返回后的地址
// 供 fork() 创建的子进程设置其 switch_to 帧的返回地址
uint32_t schedule_switch_to_return_addr = 0;

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

    printf("[pick_next_task_cfs] current: pid=%d, state=%d, user_stack=0x%x\n",
           current->pid, current->state, current->user_stack);

    // 如果当前任务正在运行，标记为就绪
    if (current->state == PS_RUNNING) {
        current->state = PS_READY;
    }

    // ⚠️⚠️⚠️ 关键修复：遍历整个循环链表，而不是只从 current->next 开始
    // 原因：fork() 创建的子进程被插入到链表头部（combined_task_list），
    //      在父进程之前（current->prev），所以必须从 current->next 开始遍历一圈

    // 从 current->next 开始查找
    next = current->next;
    printf("[pick_next_task_cfs] current->next=0x%x\n", (uint32_t)next);

    int loop_count = 0;
    int found = 0;

    // 遍历整个链表（包括 current->prev 方向的任务）
    while (next != NULL && next != current) {
        printf("[pick_next_task_cfs] [%d] checking next: pid=%d, state=%d, user_stack=0x%x, can_schedule=%d\n",
               loop_count++, next->pid, next->state, next->user_stack, can_schedule(next));

        // ⚠️⚠️⚠️ 关键修复：跳过内核任务（user_stack == 0）！
        // 原因：内核任务没有合法的返回路径（trapframe 或 task_to_user_mode_with_task）
        //      如果调度到内核任务，switch_to 会尝试 ret 到非法地址，导致 triple fault
        if (next->user_stack == 0) {
            printf("[pick_next_task_cfs] skipping kernel task pid=%d (no user_stack)\n", next->pid);
            next = next->next;
            continue;
        }

        // 支持 PS_READY 和 PS_CREATED 状态
        // PS_CREATED: 首次运行的用户任务
        // PS_READY: 已经初始化，等待运行的任务
        if ((next->state == PS_READY || next->state == PS_CREATED) && can_schedule(next)) {
            printf("[pick_next_task_cfs] selected next: pid=%d\n", next->pid);
            found = 1;
            break;
        }
        next = next->next;
    }

    // ⚠️⚠️⚠️ 如果 next 为 NULL，说明链表是单向的
    // 需要从 combined_task_list 开始遍历
    if (!found && next == NULL) {
        printf("[pick_next_task_cfs] reached end of list, checking from combined_task_list\n");

        extern struct task_t *combined_task_list;
        next = combined_task_list;

        while (next != NULL && next != current) {
            printf("[pick_next_task_cfs] [%d] checking from head: pid=%d, state=%d, user_stack=0x%x, can_schedule=%d\n",
                   loop_count++, next->pid, next->state, next->user_stack, can_schedule(next));

            // ⚠️⚠️⚠️ 关键修复：跳过内核任务（user_stack == 0）！
            if (next->user_stack == 0) {
                printf("[pick_next_task_cfs] skipping kernel task pid=%d (no user_stack)\n", next->pid);
                next = next->next;
                continue;
            }

            if ((next->state == PS_READY || next->state == PS_CREATED) && can_schedule(next)) {
                printf("[pick_next_task_cfs] selected next from head: pid=%d\n", next->pid);
                found = 1;
                break;
            }
            next = next->next;
        }
    }

    // 如果没找到其他就绪任务，保持当前任务
    if (!found) {
        next = current;
        printf("[pick_next_task_cfs] no other task, keeping current: pid=%d\n", current->pid);
    }

    return next;
}

//schedule() 调用一次 pick_next_task
void schedule(void) {
    printf("[schedule] ENTRY - schedule() called!\n");

    // ⚠️⚠️⚠️ 初始化返回地址（只执行一次）
    extern uint32_t schedule_switch_to_return_addr;
    if (schedule_switch_to_return_addr == 0) {
        __asm__ __volatile__("call 1f; 1: popl %0; addl $(after_switch_to-1b), %0"
                             : "=m" (schedule_switch_to_return_addr)
                             : : "eax", "memory");
        printf("[schedule] Initialized schedule_switch_to_return_addr=0x%x\n",
               schedule_switch_to_return_addr);
    }

    struct task_t *prev, *next;
    uint32_t flags;
    uint8_t cpu_id = logical_cpu_id();

    printf("[schedule] cpu_id=%d\n", cpu_id);
    printf("[schedule] Before inline asm\n");

    /* 保护临界区 */
    __asm__ __volatile__("pushfl; popl %0; cli" : "=r"(flags));

    printf("[schedule] After inline asm, flags=0x%x\n", flags);

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

    // ⚠️⚠️⚠️ 关键修复：区分三种情况
    // 1. 第一次运行用户任务 (state == PS_CREATED)
    //    → 使用 task_to_user_mode_with_task,不返回
    //    包括:
    //      - user_task_main() 初始化的任务
    //      - fork() 创建的子进程 (第一次被调度时)
    //
    // 2. 切换到用户任务 (user_stack != 0, state == PS_READY 或 PS_RUNNING)
    //    → 使用 switch_to,然后通过 interrupt_exit 恢复 trapframe
    //    这些任务之前运行过,被中断打断,内核栈上有合法的 trapframe
    //
    // 3. 切换到内核任务 (user_stack == 0)
    //    → 使用 switch_to,正常返回

    // ⚠️⚠️⚠️ 铁律1：NEVER call task_to_user_mode_with_task for EXITED tasks!
    // 原因：已退出任务的trapframe/esp0/cr3已失效，调用会导致内核崩溃
    if (next->state == PS_TERMNAT || next->state == PS_DESTROY) {
        printf("[schedule] ERROR: next task pid=%d is EXITED (state=%d), skipping!\n",
               next->pid, next->state);
        __asm__ __volatile__("pushl %0; popfl" : : "r"(flags));
        return;
    }

    // ⚠️⚠️⚠️ 铁律2：first_time_user只对PS_CREATED有意义
    // 判断标准：state == PS_CREATED（不是has_run_user == 0）
    int first_time_user = (next->state == PS_CREATED && next->user_stack != 0);
    int switch_to_user = (next->user_stack != 0 && next->has_run_user == 1);

    printf("[schedule] first_time_user=%d, switch_to_user=%d (user_stack=0x%x, state=%d)\n",
           first_time_user, switch_to_user, next->user_stack, next->state);

    task_setrun(next);

    // ⚠️⚠️⚠️ 关键修复：PS_CREATED任务必须在prev==next检查之前处理！
    // 原因：首次进入用户态的任务即使是prev==next，也必须调用task_to_user_mode_with_task
    //      否则会陷入无限循环（state=PS_CREATED → prev==next返回 → 永远无法进入用户态）
    if (first_time_user) {
        printf("[schedule] First time entering user mode for pid=%d\n", next->pid);

        next->state = PS_RUNNING;

        // ⚠️⚠️⚠️ 关键修复：在切换前更新 current_task 和全局 current
        // 这样中断处理程序能读取到正确的当前任务
        current_task[cpu_id] = next;
        current = next;  // 同步更新全局 current（汇编代码需要）

        // ⚠️ 关键：确保中断保持禁用！
        // 前面的 cli 已经禁用了中断，不要恢复

        printf("[schedule] About to call task_to_user_mode_with_task, next=0x%x\n", (uint32_t)next);

        // ⚠️⚠️⚠️ 调用 task_to_user_mode_with_task（汇编实现）
        // 这个函数会恢复 trapframe 并 iret 到用户态，不会返回！
        extern void task_to_user_mode_with_task_wrapper(struct task_t *task);
        task_to_user_mode_with_task_wrapper(next);

        // ⚠️⚠️⚠️ 永远不会执行到这里（如果执行到这里说明出错了）
        // 使用 __builtin_unreachable() 告诉编译器这是不可达路径
        __builtin_unreachable();
    }

    // ⚠️⚠️⚠️ 铁律3：prev == next时，绝不能调用task_to_user_mode_with_task
    // 原因：已经在当前任务的内核上下文中，没有发生切换
    //      如果调用task_to_user_mode_with_task，会使用无效的trapframe导致崩溃
    //
    // 注意：这个检查必须在first_time_user检查之后！
    //       因为PS_CREATED任务需要特殊处理（即使prev==next）
    if (prev == next) {
        printf("[schedule] prev==next (pid=%d), no switch needed\n", next->pid);
        __asm__ __volatile__("pushl %0; popfl" : : "r"(flags));
        return;
    }

    printf("[schedule] CPU%d: switch from task_%d to task_%d\n",
           cpu_id, prev->pid, next->pid);

    // ================================
    // 情况 2: 切换到用户任务 (非首次)
    // ================================
    if (switch_to_user) {
        // ⚠️⚠️⚠️ Linux 模型关键修复:
        // 切换到用户任务时,switch_to 会:
        //   1. 保存 prev 的内核上下文
        //   2. 切换到 next 的内核栈(上面有 trapframe)
        //   3. 恢复 next 的内核上下文
        //   4. 返回到这里(schedule)
        //
        // 然后 schedule() 返回到 interrupt_exit
        // interrupt_exit 会恢复 next 的 trapframe 并 iret
        //
        // 这是唯一的中断返回路径!

        printf("[schedule] Switching to user task %d\n", next->pid);
        printf("[schedule] next->esp=0x%x, next->esp0=0x%x\n", next->esp, next->esp0);

        // ⚠️⚠️⚠️ 关键修复：在切换前更新 current_task 和全局 current
        // 这样中断处理程序能读取到正确的当前任务
        current_task[cpu_id] = next;
        current = next;  // 同步更新全局 current（汇编代码需要）

        /* 恢复中断并执行上下文切换 */
        __asm__ __volatile__("pushl %0; popfl" : : "r"(flags));

        // ⚠️⚠️⚠️ 调试：打印传递给 switch_to 的参数
        printf("[schedule] Calling switch_to(prev=0x%x, next=0x%x)\n",
               (uint32_t)prev, (uint32_t)next);

        switch_to(prev, next);

        // ⇪️⚠️⚠️ switch_to 返回后的标签
        // fork() 创建的子进程的 switch_to 帧[4] 保存这个地址
        // 标签: after_switch_to
        __asm__ __volatile__("after_switch_to:");

        // ⚠️⚠️⚠️ switch_to 返回后:
        //   - 当前栈 = next 的内核栈
        //   - 栈上有 next 的 trapframe
        //   - 返回到 interrupt_exit,恢复 trapframe 并 iret
        printf("[schedule] switch_to returned, continuing to interrupt_exit\n");
        return;
    }

    // ================================
    // 情况 3: 切换到内核任务
    // ================================
    // ⚠️⚠️⚠️ 关键修复：在切换前更新 current_task 和全局 current
    current_task[cpu_id] = next;
    current = next;  // 同步更新全局 current（汇编代码需要）

    /* 恢复中断并执行上下文切换 */
    __asm__ __volatile__("pushl %0; popfl" : : "r"(flags));
    switch_to(prev, next);

    // ⚠️⚠️⚠️ switch_to 返回后：
    //   - 当前栈 = next 的内核栈
    //   - 栈上是恢复后的寄存器（ebx, esi, edi, ebp）
    //   - ret 会返回到调用者（interrupt_exit 或 efficient_scheduler_loop）
    //
    // 对于内核任务：
    //   - 调用者是 efficient_scheduler_loop
    //   - 返回后会继续循环，调用 schedule()
    //
    // 对于用户任务：
    //   - 调用者是 interrupt_exit
    //   - 返回后会恢复 trapframe 并 iret
    printf("[schedule] switch_to returned to caller\n");
    return;
}

// ⚠️⚠️⚠️ switch_to 现在完全由汇编实现 (task_impl.s)
// 原因：
//   1. 需要完整的寄存器保存/恢复 (EBP, EDI, ESI, EBX)
//   2. 需要精确控制栈切换时机
//   3. C 的 inline 版本无法正确处理
//
// 之前 sched.c 中的 static inline void switch_to() 已删除
// 现在统一使用 task_impl.s 中的汇编版本

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
