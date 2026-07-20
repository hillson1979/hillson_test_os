#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// 模拟 task.h 中的类型定义
typedef uint32_t pid_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef uint8_t cpu_id_t;
typedef uint32_t time_t;
typedef bool bool_t;

struct llist_header;

struct haybed {
    struct llist_header sleepers;
    time_t wakeup_time;
    time_t alarm_time;
};

typedef void (*activity_callback_t)(struct task_t* task, void* opaque);
typedef void (*sighandler_t)(int);

struct trapframe;

struct task_t {
    uint32_t                esp;
    uint32_t                esp0;
    bool                    has_signal;
    uint32_t                *cr3;
    uint32_t                *user_stack;
    uint32_t                signal_handler;
    int                     idle_flags;
    pid_t                   pid;
    pid_t                   ppid;
    uid_t                   uid;
    gid_t                   gid;
    int                     state;
    int                     has_run_user;
    int                     nice;
    time_t                  start_time;
    pid_t                   waitpid;
    cpu_id_t                cpu;
    const char*             directory;
    const char*             name;
    uint32_t                size;
    const char*             csd;
    int                     load_weight;
    uint32_t                entry;
    uint32_t                time_slice;
    uint64_t                vruntime;
    struct llist_header *sched_node;
    struct haybed sleep;
    struct task_mm* mm;  // 需要定义这个结构
    struct task_t*  prev;
    struct task_t*  next;
    activity_callback_t     check_idle;
    void*                   idle_context;
    sighandler_t sig_handler;
    uint32_t signal_mask;
    uint32_t pending_signals;
    struct trapframe * tf;
    uint32_t task_total_count;
    uint32_t *pde;
    uint32_t *kstack;
    uint32_t iret_frame[5];
};

struct task_mm {
    uint32_t vmroot;
    uint32_t vm_mnt;
    struct task_t* task;
    struct task_mm* guest_mm;
};

int main() {
    printf("Calculating struct task_t field offsets:\n");
    printf("==========================================\n");

    struct task_t *task = NULL;

    printf("offset of esp:          %lu\n", (unsigned long)&task->esp);
    printf("offset of esp0:         %lu\n", (unsigned long)&task->esp0);
    printf("offset of has_signal:   %lu\n", (unsigned long)&task->has_signal);
    printf("offset of cr3:          %lu\n", (unsigned long)&task->cr3);
    printf("offset of user_stack:   %lu\n", (unsigned long)&task->user_stack);
    printf("offset of signal_handler: %lu\n", (unsigned long)&task->signal_handler);
    printf("offset of idle_flags:   %lu\n", (unsigned long)&task->idle_flags);
    printf("offset of pid:          %lu\n", (unsigned long)&task->pid);
    printf("offset of ppid:         %lu\n", (unsigned long)&task->ppid);
    printf("offset of uid:          %lu\n", (unsigned long)&task->uid);
    printf("offset of gid:          %lu\n", (unsigned long)&task->gid);
    printf("offset of state:        %lu\n", (unsigned long)&task->state);
    printf("offset of has_run_user: %lu\n", (unsigned long)&task->has_run_user);
    printf("offset of nice:         %lu\n", (unsigned long)&task->nice);
    printf("offset of start_time:   %lu\n", (unsigned long)&task->start_time);
    printf("offset of waitpid:      %lu\n", (unsigned long)&task->waitpid);
    printf("offset of cpu:          %lu\n", (unsigned long)&task->cpu);
    printf("offset of directory:    %lu\n", (unsigned long)&task->directory);
    printf("offset of name:         %lu\n", (unsigned long)&task->name);
    printf("offset of size:         %lu\n", (unsigned long)&task->size);
    printf("offset of csd:          %lu\n", (unsigned long)&task->csd);
    printf("offset of load_weight:  %lu\n", (unsigned long)&task->load_weight);
    printf("offset of entry:        %lu\n", (unsigned long)&task->entry);
    printf("offset of time_slice:   %lu\n", (unsigned long)&task->time_slice);
    printf("offset of vruntime:     %lu\n", (unsigned long)&task->vruntime);
    printf("offset of sched_node:   %lu\n", (unsigned long)&task->sched_node);
    printf("offset of sleep:        %lu\n", (unsigned long)&task->sleep);
    printf("offset of mm:           %lu\n", (unsigned long)&task->mm);
    printf("offset of prev:         %lu\n", (unsigned long)&task->prev);
    printf("offset of next:         %lu\n", (unsigned long)&task->next);
    printf("offset of check_idle:   %lu\n", (unsigned long)&task->check_idle);
    printf("offset of idle_context: %lu\n", (unsigned long)&task->idle_context);
    printf("offset of sig_handler:  %lu\n", (unsigned long)&task->sig_handler);
    printf("offset of signal_mask:  %lu\n", (unsigned long)&task->signal_mask);
    printf("offset of pending_signals: %lu\n", (unsigned long)&task->pending_signals);
    printf("offset of tf:           %lu  ← THIS IS THE KEY!\n", (unsigned long)&task->tf);
    printf("==========================================\n");
    printf("sizeof(struct task_t) = %lu bytes\n", sizeof(struct task_t));

    return 0;
}
