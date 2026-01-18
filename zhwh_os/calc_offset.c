#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

// 不包含 time.h，避免类型冲突
#define trapframe offset_trapframe
#define task_t offset_task_t

struct offset_trapframe {
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

struct offset_llist_header {
    int dummy;
};

struct offset_haybed {
    struct offset_llist_header sleepers;
    uint32_t wakeup_time;
    uint32_t alarm_time;
};

struct offset_task_mm {
    uint32_t vmroot;
    uint32_t vm_mnt;
    struct offset_task_t* task;
    struct offset_task_mm* guest_mm;
};

struct offset_task_t;

typedef void (*offset_activity_callback_t)(struct offset_task_t*, void*);

struct offset_task_t {
    uint32_t                esp;
    uint32_t                esp0;
    bool                    has_signal;
    uint32_t                *cr3;
    uint32_t                *user_stack;
    uint32_t                signal_handler;
    int                     idle_flags;
    uint32_t                pid;
    uint32_t                ppid;
    uint32_t                uid;
    uint32_t                gid;
    int                     state;
    int                     has_run_user;
    int                     nice;
    uint64_t                start_time;
    uint32_t                waitpid;
    uint8_t                 cpu;
    const char*             directory;
    const char*             name;
    uint32_t                size;
    const char*             csd;
    int                     load_weight;
    uint32_t                entry;
    uint32_t                time_slice;
    uint64_t                vruntime;
    void*                   sched_node;
    struct offset_haybed    sleep;
    struct offset_task_mm*  mm;
    struct offset_task_t*   prev;
    struct offset_task_t*   next;
    offset_activity_callback_t check_idle;
    void*                   idle_context;
    uint32_t                sig_handler;
    uint32_t                signal_mask;
    uint32_t                pending_signals;
    struct offset_trapframe* tf;
    uint32_t                task_total_count;
    uint32_t*               pde;
    uint32_t*               kstack;
    uint32_t                iret_frame[5];
};

int main() {
    printf("=== task_t Structure Offset Analysis ===\n\n");

    printf("Size of struct offset_task_t: %zu bytes\n\n", sizeof(struct offset_task_t));

    printf("Key field offsets:\n");
    printf("  esp:          %3zu\n", __builtin_offsetof(struct offset_task_t, esp));
    printf("  esp0:         %3zu\n", __builtin_offsetof(struct offset_task_t, esp0));
    printf("  has_signal:   %3zu\n", __builtin_offsetof(struct offset_task_t, has_signal));
    printf("  cr3:          %3zu\n", __builtin_offsetof(struct offset_task_t, cr3));
    printf("  pid:          %3zu\n", __builtin_offsetof(struct offset_task_t, pid));
    printf("  state:        %3zu\n", __builtin_offsetof(struct offset_task_t, state));
    printf("  has_run_user: %3zu\n", __builtin_offsetof(struct offset_task_t, has_run_user));
    printf("  vruntime:     %3zu\n", __builtin_offsetof(struct offset_task_t, vruntime));
    printf("  sched_node:   %3zu\n", __builtin_offsetof(struct offset_task_t, sched_node));
    printf("  mm:           %3zu\n", __builtin_offsetof(struct offset_task_t, mm));
    printf("  tf:           %3zu  <--- CRITICAL!\n", __builtin_offsetof(struct offset_task_t, tf));
    printf("  pde:          %3zu\n", __builtin_offsetof(struct offset_task_t, pde));
    printf("  kstack:       %3zu\n", __builtin_offsetof(struct offset_task_t, kstack));
    printf("  iret_frame:   %3zu\n", __builtin_offsetof(struct offset_task_t, iret_frame));

    printf("\n");
    printf("TASK_IFRAME should be: %zu\n", __builtin_offsetof(struct offset_task_t, tf));
    printf("Current TASK_IFRAME in task_impl.s: 152\n");

    size_t tf_offset = __builtin_offsetof(struct offset_task_t, tf);
    if (tf_offset == 152) {
        printf("\n✓ OFFSET IS CORRECT! No changes needed.\n");
    } else {
        printf("\n✗ OFFSET MISMATCH! Need to update task_impl.s\n");
        printf("  Current value: .set TASK_IFRAME, 152\n");
        printf("  Should be:     .set TASK_IFRAME, %zu\n", tf_offset);
        printf("\nTo fix, run in Git Bash or MSYS:\n");
        printf("  sed -i 's/.set TASK_IFRAME,.*/.set TASK_IFRAME, %zu/' task_impl.s\n", tf_offset);
    }

    return 0;
}
