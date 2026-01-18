#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// 不包含 time.h，避免类型冲突
// 模拟 task.h 中的定义
typedef uint32_t pid_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef uint8_t cpu_id_t;
typedef uint64_t task_time_t;  // 改名避免与系统 time_t 冲突

#define OFFSETOF(type, member) ((unsigned long)&((type *)0)->member)

struct llist_header {
    int dummy;
};

struct haybed {
    struct llist_header sleepers;
    task_time_t wakeup_time;
    task_time_t alarm_time;
};

struct task_mm {
    uint32_t vmroot;
    uint32_t vm_mnt;
    struct task_t* task;
    struct task_mm* guest_mm;
};

typedef void (*sighandler_t)(int);
typedef void (*activity_callback_t)(struct task_t*, void*);

struct trapframe {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t oesp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t ds;
    uint32_t es;
    uint32_t fs;
    uint32_t gs;
    uint32_t trapno;
    uint32_t err;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;
    uint32_t ss;
} __attribute__((packed));

struct task_t {
    uint32_t esp;
    uint32_t esp0;
    bool has_signal;
    uint32_t *cr3;
    uint32_t *user_stack;
    uint32_t signal_handler;
    int idle_flags;
    pid_t pid;
    pid_t ppid;
    uid_t uid;
    gid_t gid;
    int state;
    int has_run_user;
    int nice;
    task_time_t start_time;
    pid_t waitpid;
    cpu_id_t cpu;
    const char* directory;
    const char* name;
    uint32_t size;
    const char* csd;
    int load_weight;
    uint32_t entry;
    uint32_t time_slice;
    uint64_t vruntime;
    struct llist_header *sched_node;
    struct haybed sleep;
    struct task_mm* mm;
    struct task_t* prev;
    struct task_t* next;
    activity_callback_t check_idle;
    void* idle_context;
    sighandler_t sig_handler;
    uint32_t signal_mask;
    uint32_t pending_signals;
    struct trapframe *tf;
    uint32_t task_total_count;
    uint32_t *pde;
    uint32_t *kstack;
    uint32_t iret_frame[5];
};

int main() {
    printf("=== task_t Structure Offset Analysis ===\n\n");
    int TASK_USER_STACK_OFFSET = OFFSETOF(struct task_t, user_stack);
    int TASK_TF_OFFSET         = OFFSETOF(struct task_t, tf);
    printf("Key field offsets 1------:%d\n",TASK_USER_STACK_OFFSET);
    printf("Key field offsets 2------:%d\n",TASK_TF_OFFSET);
    printf("Size of struct task_t: %lu bytes\n\n", sizeof(struct task_t));
    
    printf("Key field offsets:\n");
    printf("  esp:         %3lu\n", OFFSETOF(struct task_t, esp));
    printf("  esp0:        %3lu\n", OFFSETOF(struct task_t, esp0));
    printf("  has_signal:  %3lu\n", OFFSETOF(struct task_t, has_signal));
    printf("  cr3:         %3lu\n", OFFSETOF(struct task_t, cr3));
    printf("  pid:         %3lu\n", OFFSETOF(struct task_t, pid));
    printf("  state:       %3lu\n", OFFSETOF(struct task_t, state));
    printf("  has_run_user:%3lu\n", OFFSETOF(struct task_t, has_run_user));
    printf("  vruntime:    %3lu\n", OFFSETOF(struct task_t, vruntime));
    printf("  sched_node:  %3lu\n", OFFSETOF(struct task_t, sched_node));
    printf("  mm:          %3lu\n", OFFSETOF(struct task_t, mm));
    printf("  tf:          %3lu  <--- CRITICAL!\n", OFFSETOF(struct task_t, tf));
    printf("  pde:         %3lu\n", OFFSETOF(struct task_t, pde));
    printf("  kstack:      %3lu\n", OFFSETOF(struct task_t, kstack));
    printf("  iret_frame:  %3lu\n", OFFSETOF(struct task_t, iret_frame));
    
    printf("\n");
    printf("TASK_IFRAME should be: %lu\n", OFFSETOF(struct task_t, tf));
    printf("Current TASK_IFRAME in task_impl.s: 152\n");
    
    if (OFFSETOF(struct task_t, tf) == 152) {
        printf("\n✓ OFFSET IS CORRECT!\n");
    } else {
        printf("\n✗ OFFSET MISMATCH! Need to update task_impl.s\n");
        printf("  Correct value: .set TASK_IFRAME, %lu\n", OFFSETOF(struct task_t, tf));
    }
    
    return 0;
}
