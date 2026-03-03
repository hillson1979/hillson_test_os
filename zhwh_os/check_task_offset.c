#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

// 从 task.h 复制的定义
typedef uint32_t uint32_t;
typedef uint8_t uint8_t;
typedef uint8_t bool;

typedef uint32_t pid_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef uint8_t cpu_id_t;

struct llist_header {
    struct llist_header *next;
    struct llist_header *prev;
};

struct haybed {
    struct llist_header sleepers;
    uint32_t wakeup_time;
    uint32_t alarm_time;
};

struct task_mm {
    uint32_t vmroot;
    uint32_t vm_mnt;
    struct task_mm* task;
    struct task_mm* guest_mm;
};

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

typedef bool (*activity_callback_t)(void* task, void* opaque);
typedef void (*sighandler_t)(int);

typedef struct task_t {
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
    uint32_t                start_time;
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
    struct task_mm* mm;
    struct task_t* prev;
    struct task_t* next;
    activity_callback_t check_idle;
    void* idle_context;
    sighandler_t sig_handler;
    uint32_t signal_mask;
    uint32_t pending_signals;
    struct trapframe * tf;
    uint32_t task_total_count;
    uint32_t *pde;
    uint32_t *kstack;
    uint32_t iret_frame[5];
} task_t;

int main() {
    printf("=== task_t offsets ===\n");
    printf("offsetof(task_t, tf) = %u (0x%x)\n",
           (unsigned)offsetof(struct task_t, tf), (unsigned)offsetof(struct task_t, tf));
    printf("sizeof(task_t) = %u\n", (unsigned)sizeof(struct task_t));
    printf("sizeof(struct trapframe) = %u\n", (unsigned)sizeof(struct trapframe));

    // 计算给定地址的 tf 偏移
    uint32_t task_addr = 0xC02F8680;
    uint32_t tf_addr = 0xC02F9634;
    uint32_t actual_offset = tf_addr - task_addr;
    printf("\nActual offset from log: 0x%x (%u)\n", actual_offset, actual_offset);

    return 0;
}
