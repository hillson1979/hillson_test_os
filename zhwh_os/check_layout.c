#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

typedef uint32_t pid_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef uint8_t cpu_id_t;
typedef uint64_t task_time_t;

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
    printf("=== Detailed task_t Layout ===\n\n");
    
    // 打印 164 附近的所有字段
    printf("Fields around offset 164:\n");
    printf("  sig_handler:     %3lu\n", OFFSETOF(struct task_t, sig_handler));
    printf("  signal_mask:    %3lu\n", OFFSETOF(struct task_t, signal_mask));
    printf("  pending_signals:%3lu\n", OFFSETOF(struct task_t, pending_signals));
    printf("  tf:             %3lu  <--- Target!\n", OFFSETOF(struct task_t, tf));
    printf("  task_total_count:%3lu\n", OFFSETOF(struct task_t, task_total_count));
    printf("  pde:            %3lu\n", OFFSETOF(struct task_t, pde));
    printf("  kstack:         %3lu\n", OFFSETOF(struct task_t, kstack));
    printf("  iret_frame:     %3lu\n", OFFSETOF(struct task_t, iret_frame));
    
    printf("\nSize of struct trapframe: %lu bytes\n", sizeof(struct trapframe));
    printf("Size of pointer: %lu bytes\n", sizeof(void*));
    
    return 0;
}
