#include "types.h"
#include "llist.h"
#include "time.h"
//#include "spinlock.h"

/*
typedef enum task_state_t {
	PS_RUNNING,	
	PS_SUSPENDED,	
	//PS_IO_BOUND,  
        PS_READY ,
        PS_RUNNING ,
        PS_TERMNAT ,
        PS_DESTROY ,
        PS_PAUSED ,
        PS_BLOCKED ,
        PS_STOPPED ,
        PS_CREATED 

} task_state_t;*/


#define PS_READY 0
#define PS_RUNNING 1
#define PS_TERMNAT 2
#define PS_DESTROY 4
#define PS_PAUSED 8
#define PS_BLOCKED 16
#define PS_STOPPED 32
#define PS_CREATED 64

#define PS_GrBP (PS_PAUSED | PS_BLOCKED | PS_STOPPED)
#define PS_GrDT (PS_TERMNAT | PS_DESTROY)
#define PS_Rn (PS_RUNNING | PS_CREATED)

#define task_terminated(task) \
            (!(task) || ((task)->state) & PS_GrDT)


#define proc_hanged(proc) \
            ((proc)  && ((proc)->state) & PS_BLOCKED)
#define task_runnable(task) \
            ((task) && (!(task)->state || !(((task)->state) & ~PS_Rn)))

struct haybed {
    struct llist_header sleepers;
    time_t wakeup_time;
    time_t alarm_time;
};


typedef uint32_t pid_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef uint8_t cpu_id_t;

struct task_t;
typedef void (*task_idle_timer_t)(void);

/**
 * @brief Represents an idle timer callback.
 */
typedef struct idle_timer {
	task_idle_timer_t func;       /**< Function pointer for callback */
	struct idle_timer* next;      /**< Next idle timer in list */
} idle_timer_t;

typedef bool (*activity_callback_t)(struct task_t* task, void* opaque);

// 通过编译器计算结构体偏移量
#define OFFSETOF(type, member) ((unsigned long)&((type *)0)->member)

// 线程结构体偏移量
#define TASK_ESP         OFFSETOF(struct task_t, esp)
#define TASK_ESP0        OFFSETOF(struct task_t, esp0)
#define TASK_HAS_SIGNAL  OFFSETOF(struct task_t, has_signal)
#define TASK_USP         OFFSETOF(struct task_t, usp)

// 任务结构体偏移量  
#define TASK_CR3          OFFSETOF(struct task_t, cr3)
#define TASK_THREAD       OFFSETOF(struct task_t, thread)
#define TASK_STATE        OFFSETOF(struct task_t, state)
#define TASK_SIG_HANDLER  OFFSETOF(struct task_t, signal_handler)
#define TASK_INTR_DEPTH   OFFSETOF(struct task_t, intr_depth)

#define TASK_IFRAME   OFFSETOF(struct task_t, tf)

#define KSTACK_SIZE 4096

// 信号处理函数类型定义
typedef void (*sighandler_t)(int);

typedef struct task_t {

        uint32_t                esp;
        uint32_t                esp0;
        bool                    has_signal;
        uint32_t                *cr3;
        uint32_t                *user_stack;//user stack
        uint32_t                signal_handler;
        int                     idle_flags;
        uint32_t                intr_depth;
	pid_t			pid;        /**< Unique taskess ID */
	pid_t			ppid;       /**< Parent taskess ID */
	uid_t			uid;        /**< User ID (future use) */
	gid_t			gid;        /**< Group ID (future use) */
	int		state;      /**< Running state */
	
        int                     nice;

        time_t			start_time; /**< Start time (UNIX epoch) */
	pid_t			waitpid;    /**< PID being waited on */
	cpu_id_t		cpu;        /**< Logical CPU this taskess is assigned to */
	const char*		directory;  /**< Directory of program */
	const char*		name;       /**< Filename of program */
	uint32_t		size;       /**< Size of program in bytes */
	const char*		csd;        /**< Current selected directory */
        int                     load_weight; //for calculate_weight(nice_value);
	uint32_t                entry;
        uint32_t                time_slice;
        uint64_t                vruntime;
        struct llist_header *sched_node;  // 调度链表节点
        struct haybed sleep;
        struct task_mm* mm;
        struct task_t*	prev;       /**< Previous taskess in doubly linked list */
	struct task_t*	next;       /**< Next taskess in doubly linked list */
	activity_callback_t	check_idle; /**< If non-null, called to check if the taskess should remain idle */
	void*			idle_context; /**< Opaque context passed to the check_idle callback */

        sighandler_t sig_handler; // 20: 信号处理函数
    
        uint32_t signal_mask;  // 信号掩码 (bitmask)
        uint32_t pending_signals; // 待处理信号 (bitmask)
        struct trapframe * tf;
        uint32_t task_total_count;
        uint32_t *pde;

        uint32_t *kstack;//kernel stack
} task_t;


struct task_mm
{
    // virtual memory root (i.e. root page table)
    uint32_t             vmroot;
    uint32_t             vm_mnt;       // current mount point
   // vm_regions_t      regions;

    //struct mm_region* heap;
    struct task_t* task;
    struct task_mm*   guest_mm;     // vmspace mounted by this vmspace
};

/*
static struct {
  struct spinlock lock;
  int locking;
} tasklocks;
*/
extern struct task_t *th_u;
task_t* task_load(const char* fullpath, pid_t parent_pid, bool with_ustack);
typedef void (*task_entry_callback_t)(void*);
struct task_t* init_task(bool with_ustack);
void start_task_user(struct task_t* th, task_entry_callback_t  user_entry);
void start_task_kernel(struct task_t* th, task_entry_callback_t  kernel_entry);
void efficient_scheduler_loop();
void user_task_main();
void kernel_task_main();
void handle_idle_state(uint8_t cpu);

void ok_here();

