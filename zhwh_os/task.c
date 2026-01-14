#include "types.h"
//#include "task.h"
#include "lapic.h"
#include "sched.h"
#include "interrupt.h"
//#include "x86/mmu.h"
#include "x86/io.h"
//#include "page.h"
#include "multiboot.h"
#include "memlayout.h"
#include "highmem_mapping.h"
#include "mm.h"
#include "segment.h"
//#include "page.h"
#include "param.h"
#include "proc.h"
#include "userboot.h"
#include "printf.h"
/**
 * @brief The currently running taskess on each CPU
 */

#define MAX_CPUS 8

// ⚠️ 调试函数：打印进入task_to_user_mode_with_task
void debug_print_enter(void) {
    printf("[DEBUG] ===== Entering task_to_user_mode_with_task =====\n");
}

// ⚠️ 调试函数：打印tf指针
void debug_print_tf_ptr(uint32_t tf_ptr) {
    printf("[DEBUG] task->tf = 0x%x\n", tf_ptr);
}

// ⚠️ 调试函数：打印从tf读取的值
void debug_print_tf_values(uint32_t eip, uint32_t esp, uint32_t eflags) {
    printf("[DEBUG] Read from tf: eip=0x%x, esp=0x%x, eflags=0x%x\n", eip, esp, eflags);
}

extern struct tss_t tss;
uint32_t* pd_user;

task_t* current_task[MAX_CPUS] = { NULL };

task_t* task_list[MAX_CPUS] = { NULL };
task_t* combined_task_list = NULL;

//spinlock_t task_lock[MAX_CPUS] = { 0 };

//struct hashmap* task_by_pid;

uint32_t nextid = 1;

uint32_t task_count = 0;

idle_timer_t* task_idles = NULL, *timer_idles = NULL;

extern int llist_empty(struct llist_header *list);
/* 定义任务入口点 
uint32_t user_entry = (uint32_t)&user_task_main;   // 用户任务入口
uint32_t kernel_entry = (uint32_t)&kernel_task_main; // 内核任务入口*/

//struct task_t *sched_sleep;
uint32_t task_total_count=0;

struct trapframe *iframe;
uint32_t addr;


//extern struct cpu *cpus[MAX_CPUS];

struct task_t* init_task(bool with_ustack)
{

        //initlock(&tasklocks.lock, "task");
        //tasklocks.locking = 1;

	printf("task init\n");
	task_t* init = task_load("task init", 0, with_ustack);
	if (!init) {
		printf("task init missing or invalid!\n");
	}
        // ⚠️ 关键修复：设置 current_task，这样 start_task 可以找到第一个任务
//         current_task[logical_cpu_id()]=init;
        task_total_count++;
        return init;
}

/* 用户任务主函数 */
void user_task_main(struct task_t* th) {
    printf("[user_task_main] START: th=0x%x\n", th);
    printf("User task started (task=0x%x)\n", th);

    // 准备用户页表和加载模块
    printf("[user_task_main] Calling task_prepare_pde for th=0x%x\n", th);
    task_prepare_pde(th);  // 传递任务指针！

    printf("[user_task_main] Initialization complete, returning to scheduler\n");

    // 返回到调度器，调度器会决定何时跳转到用户态
    // 注意：不要在这里调用 task_to_user_mode_with_task
    // 因为调用后 iret 会直接返回用户态，不应该再回到这里
}

// 返回一个新页（页对齐），既给你物理地址也给你虚拟地址
typedef struct {
    uint32_t *phys;  // 物理地址 (给 CR3 / PDE 用)
    uint32_t *virt;  // 内核虚拟地址 (给 memset / 访问用)
} page_t;

page_t alloc_page_table_() {
    page_t pg;
    uint32_t phys = pmm_alloc_page();           // 物理地址
    pg.phys = (uint32_t*)phys;

    // 确保这个物理页在内核页目录中有映射（按需映射）
    uint32_t virt_addr = (uint32_t)phys_to_virt(phys);
    uint32_t kernel_pd_index = virt_addr >> 22;
    uint32_t kernel_pt_index = (virt_addr >> 12) & 0x3FF;

    // 检查内核页目录是否有对应的页表
    extern uint32_t pd[];
    if (!(pd[kernel_pd_index] & PAGE_PRESENT)) {
        // 内核页目录也没有页表，需要创建一个
        printf("[alloc_page_table_] Creating kernel page table for pd_idx=%u\n", kernel_pd_index);
        // 直接在内核页目录中创建映射
        // 使用 map_4k_page() 来创建页表
        extern void map_4k_page(uint32_t, uint32_t, uint32_t);
        map_4k_page(phys, virt_addr, 0x3);
    } else {
        // 页表存在，检查具体的页映射
        uint32_t *kernel_pt = (uint32_t*)phys_to_virt(pd[kernel_pd_index] & ~0xFFF);
        if (!(kernel_pt[kernel_pt_index] & PAGE_PRESENT)) {
            // 为内核创建这个物理页的映射
            printf("[alloc_page_table_] Mapping phys=0x%x to kernel space at 0x%x\n", phys, virt_addr);
            kernel_pt[kernel_pt_index] = phys | 0x3;
            // 刷新 TLB
            __asm__ volatile ("invlpg (%0)" : : "r" (virt_addr) : "memory");
        }
    }

    pg.virt = (uint32_t*)virt_addr;  // 转换到内核虚拟地址
    printf("[alloc_page_table_] Before memset: pg.virt=0x%x, phys=0x%x\n", pg.virt, phys);
    memset(pg.virt, 0, PAGE_SIZE);
    printf("[alloc_page_table_] After memset succeeded\n");
    return pg;
}
void copy_kernel_mappings_to_pd(uint32_t *pd_user) {
    printf("[copy_kernel_mappings_to_pd] START: pd_user=0x%x\n", (uint32_t)pd_user);
    // kernel occupies PDE indexes 768..1023 (0xC0000000 >> 22 == 768)
    int mapped_count = 0;
    for (int i = 768; i < 1024; ++i) {
        uint32_t entry = pd[i];
        // 如果原来 pd_kernel[i] 为 0，就跳过
        if ((entry & PTE_P) == 0) {
            pd_user[i] = 0;
            continue;
        }
        // ⚠️ 关键修复：复制内核PDE并强制设置U位=1
        // 允许用户态访问内核地址空间(用于中断处理)
        pd_user[i] = entry | PDE_USER;

        mapped_count++;
        if (mapped_count <= 5) {
            printf("[copy_kernel_mappings_to_pd] Copied pd[%u]=0x%x -> pd_user[%u]=0x%x (cleared PTE.U bits)\n",
                   i, pd[i], i, pd_user[i]);
        }
    }
    printf("[copy_kernel_mappings_to_pd] DONE: copied %u kernel mappings\n", mapped_count);
}

#define PHYS_VIDEO 0xB8000
#define VIRT_USER_VIDEO 0xB8000

void task_prepare_pde(struct task_t *task){
       printf("[task_prepare_pde] START for task=0x%x\n", task);
       page_t pde = alloc_page_table_();
       printf("[task_prepare_pde] alloc_page_table_ returned: phys=0x%x, virt=0x%x\n", (uint32_t)pde.phys, (uint32_t)pde.virt);
       task->pde = (uint32_t*)pde.phys;   // CR3 必须用物理地址
       uint32_t *pd_user = pde.virt;      // 内核里可以直接访问 PDE 内容

       printf("[task_prepare_pde] task->pde=0x%x, pd_user=0x%x\n", (uint32_t)task->pde, (uint32_t)pd_user);

       task->cr3 = (uint32_t*)pde.phys;   // CR3 字段存储物理地址，用于汇编 switch_to()

       printf("[task_prepare_pde] task->cr3=0x%x (CR3 initialized!)\n", (uint32_t)task->cr3);

       // ⚠️ 关键修复：先复制内核高端映射，再调用 load_module_to_user
       // 这样 load_module_to_user 中分配的新页表才能被内核访问
       copy_kernel_mappings_to_pd(pd_user);
       printf("[task_prepare_pde] Kernel mappings copied\n");

       printf("[task_prepare_pde] Calling load_module_to_user...\n");
       load_module_to_user(task, task->pde);
       printf("[task_prepare_pde] load_module_to_user returned, checking EIP...\n");

       printf("[task_prepare_pde] task->tf=0x%x, task->tf->eip=0x%x\n",
              (uint32_t)task->tf, task->tf ? task->tf->eip : 0);

       // 验证 trapframe 的关键字段
       if (task->tf) {
           printf("[task_prepare_pde] Verifying trapframe:\n");
           printf("  eip=0x%x, cs=0x%x, eflags=0x%x, esp=0x%x\n",
                  task->tf->eip, task->tf->cs, task->tf->eflags, task->tf->esp);
       }

       // ⚠️⚠️⚠️ 用户栈已经在 load_module_to_user 中映射了（4页 = 16KB）
       // 不要在这里重复映射，否则可能会覆盖之前的映射
       printf("[task_prepare_pde] User stack already mapped by load_module_to_user (4 pages = 16KB)\n");
       printf("[task_prepare_pde] User stack VA range: 0x%x - 0x%x\n",
              VIRT_USER_STACK_TOP - 4*PAGE_SIZE, VIRT_USER_STACK_TOP);

       // 用户栈顶虚拟地址（用户程序开始时 esp 指向这里）
       th_u->tf->esp = VIRT_USER_STACK_TOP;

       //printf("[task_load] user stack mapped at 0x%x (PA=0x%x)\n",VIRT_USER_STACK_TOP - PAGE_SIZE, th_u->user_stack);
       //iframe=(struct trapframe *)(th_u->tf);
       // 不要在这里切换 CR3，否则后续的 printf 和其他内核操作会失败
       // CR3 将在 task_to_user_mode 中切换
       //asm volatile("movl %%eax, %%cr3\n" ::"a"(th_u->pde));

       // ⚠️ 不要在这里映射 VGA！原因：
       // 1. th_u->pde 是物理地址，需要先切换 CR3 才能访问
       // 2. 用户页目录的物理地址还没有被映射到内核虚拟地址空间
       // 3. VGA 映射应该在 load_module_to_user 中完成（如果需要的话）
       // map_page(th_u->pde, VIRT_USER_VIDEO, PHYS_VIDEO, PTE_P | PTE_W | PTE_U);

}


/* 假定这些宏与你内核一致 */
#define USER_CS  ((SEG_UCODE << 3) | DPL_USER)   /* 0x1b in your case */
#define USER_DS  ((SEG_UDATA << 3) | DPL_USER)   /* 0x23 in your case */
#define FL_IF    0x00000200

/* 内核任务主函数 */
void kernel_task_main(struct task_t* th) {
    printf("Kernel task started\n");
    int i=0;
    if (++i==1)
        printf("Kernel task running...\n");

    /* 内核任务工作 */
    //system_maintenance();

    /* 短暂延迟 */
    for (volatile int j = 0; j < 500000; j++);

    /* 内核任务只执行一次，然后返回让其他任务初始化 */
    printf("Kernel task initialization complete\n");

    // ⚠️⚠️⚠️ 关键修复：内核任务不应该直接返回！
    // 直接返回会导致 CPU 弹出错误的返回地址（栈上的垃圾数据）
    // 正确的做法：检查用户任务是否存在，如果存在则让出CPU

    extern task_t* current_task[];
    extern struct llist_header *sched_root;

    printf("[kernel_task_main] Checking if user task exists...\n");

    // 检查是否有用户任务
    task_t* user_task = NULL;
    if (th->next != NULL && th->next != th) {
        user_task = th->next;  // 假设下一个任务是用户任务
        printf("[kernel_task_main] Found user task: pid=%d, state=%d\n",
               user_task->pid, user_task->state);
    }

    if (user_task != NULL && user_task->state != PS_TERMNAT) {
        printf("[kernel_task_main] User task exists, calling schedule()...\n");

        // 让出 CPU，让调度器选择用户任务
        extern void schedule(void);
        schedule();

        printf("[kernel_task_main] Returned from schedule()\n");
    } else {
        printf("[kernel_task_main] No user task or user task dead, entering idle loop...\n");

        // 进入 idle 循环
        while (1) {
            __asm__ volatile("hlt");  // 节能
        }
    }

    // 永远不会到这里
    printf("[kernel_task_main] ERROR: Should not reach here!\n");
    while (1) {
        __asm__ volatile("hlt");
    }
}

void start_task_user(struct task_t* th, task_entry_callback_t  entry_user){
  start_task( th,  entry_user);
  if (th->entry != NULL) {
        entry_user(th);  // 调用回调函数，传递任务结构体作为参数
    }
   
}


void start_task_kernel(struct task_t* th, task_entry_callback_t  entry_kernel){
   start_task( th,entry_kernel);

   // ⚠️⚠️⚠️ 关键修复：在内核任务的栈上设置初始上下文
   // 这样调度器切换到这个任务时，会"跳转"到 entry_kernel
   //
   // switch_to 的保存顺序（task_impl.s line 43-46）：
   //   pushl %ebp, pushl %edi, pushl %esi, pushl %ebx
   //
   // 所以栈布局（从ESP往高地址）应该是：
   //   [ebp] [edi] [esi] [ebx] [ret_addr]  <- ESP 指向这里（在 ret 之前）
   //
   // switch_to 恢复顺序（task_impl.s line 104-112）：
   //   popl %ebx, popl %esi, popl %edi, popl %ebp, ret
   //
   // 当 switch_to 执行 ret 时，会弹出 ret_addr 并跳转

   uint32_t *kstack = (uint32_t*)th->esp;  // 获取内核栈顶

   // 为寄存器腾出空间（4个寄存器 + 返回地址 = 5个 uint32_t）
   kstack -= 5;

   // 设置初始值（可以是任意值，因为内核任务会覆盖它们）
   kstack[0] = 0;  // ebp
   kstack[1] = 0;  // edi
   kstack[2] = 0;  // esi
   kstack[3] = 0;  // ebx

   // 设置返回地址：指向 entry_kernel
   kstack[4] = (uint32_t)entry_kernel;

   th->esp = (uint32_t)kstack;  // 更新 ESP

   printf("[start_task_kernel] Set up initial stack for task %d: esp=0x%x, ret_addr=0x%x\n",
          th->pid, th->esp, (uint32_t)entry_kernel);

   th->user_stack=0;
}


void
start_task(struct task_t* th, task_entry_callback_t entry) {

    //if(th && !task_terminated(th))return;
    th->entry=entry;

    //llist_append(sched_root->sched_node, th->sched_node);

    // 使用 llist_append 管理 sched_node 链表
    if (sched_root) {
        llist_append(sched_root, th->sched_node);
    } else {
        sched_root = th->sched_node;
    }

    // 关键修复：手动设置 next/prev 指针构建双向链表
    extern struct llist_header *sched_root;
    extern task_t* current_task[];
    struct task_t *first_task = current_task[0];

    printf("[start_task] DEBUG: pid=%d, first_task=0x%x, current_task[0]=0x%x\n",
           th->pid, (uint32_t)first_task, (uint32_t)current_task[0]);

    if (first_task == NULL) {
        // ⚠️ 第一个任务：设置 current_task[0] 但不构建循环链表
        current_task[0] = th;
        th->next = NULL;
        th->prev = NULL;
        printf("[start_task] First task pid=%d, next=NULL, prev=NULL\n", th->pid);
    } else {
        // ⚠️ 后续任务：添加到链表末尾，构建循环链表
        struct task_t *last = first_task;
        while (last->next != NULL && last->next != first_task) {
            last = last->next;
        }

        // 将 th 添加到链表末尾
        last->next = th;
        th->prev = last;
        th->next = first_task;  // 循环链表
        first_task->prev = th;

        printf("[start_task] Added task pid=%d to chain: prev->pid=%d, next->pid=%d\n",
               th->pid, th->prev ? th->prev->pid : -1, th->next ? th->next->pid : -1);
    }

    th->state = PS_READY;
}


void handle_idle_state(uint8_t cpu) {
    if (task_list[cpu] == NULL) __asm__ __volatile__("sti; hlt; cli");
}


task_t* task_load(const char* fullpath, pid_t parent_pid, bool with_ustack)
{
        /*
	task_t* newtask =(task_t*)kmalloc_early(sizeof (struct task_t));//phys_to_virt(pmm_alloc_page());//

	if (!newtask) {
		printf("Out of memory starting new taskess\n");
		return NULL;
	}
        memset(newtask, 0, PAGE_SIZE);
	char* error = "Unknown error";*/
	// 分配 1 页
        void *page = kmalloc_early(PAGE_SIZE);
        if (!page) return NULL;
        memset(page, 0, PAGE_SIZE);

        // 任务结构体放在页首
        task_t *newtask = (task_t*)page;
        // 只在第一次创建用户任务时设置全局 th_u
        if (th_u == NULL) {
            th_u = newtask;
        }
        // trapframe 紧随其后
        //newtask->tf = (struct trapframe*)(newtask + 1);

        // 栈放在页尾（从高地址往下长）
        // 注意: kmalloc_early 返回的是内核虚拟地址
        newtask->kstack = (uint32_t*)((uint8_t*)page + PAGE_SIZE);

        // 关键修复：初始化 esp0 为内核栈顶（Ring 0→Ring 3 时 CPU 使用）
        newtask->esp0 = (uint32_t)newtask->kstack;

        // 关键修复：初始化 esp 为内核栈顶（调度器使用它来切换栈）
        newtask->esp = (uint32_t)newtask->kstack;

        // 调试: 输出kstack的虚拟地址
        printf("[init_task] kstack virt=0x%x\n", (uint32_t)newtask->kstack);

        // trapframe 在栈顶下方
        newtask->tf = (struct trapframe *)((uint8_t*)newtask->kstack - sizeof(struct trapframe));
	newtask->waitpid = 0;
	newtask->name = "task_";//strdup(fsi->filename);
	newtask->pid = nextid++;
	newtask->directory = "";//strdup(fullpath);
	newtask->csd = "csd";//strdup(csd);
	newtask->size = 4096;//fsi->size;  4k

        newtask->pde = pd;//copy_pde(newtask);

        // 关键修复：初始化 cr3 字段为 0（表示未初始化）
        // 用户任务的 cr3 会在 task_prepare_pde() 中设置
        // 内核任务使用全局 pd 的物理地址
        newtask->cr3 = NULL;

        if (!with_ustack) {
            // 内核任务：使用内核页目录
            extern uint32_t read_cr3(void);
            newtask->cr3 = (uint32_t*)read_cr3();
            printf("[task_load] kernel task: cr3=0x%x (from current CR3)\n", (uint32_t)newtask->cr3);
        } else {
            printf("[task_load] user task: cr3=NULL (will be set by task_prepare_pde)\n");
        }

	newtask->start_time = (time_t)2025;//time(NULL);
	//newtask->state = PS_RUNNING;
	newtask->ppid = parent_pid;
	newtask->cpu = logical_cpu_id();
	newtask->check_idle = NULL;
        newtask->load_weight = calculate_weight(0);//(nice_value);
        
        if (with_ustack) {
          // 分配物理页作为用户栈
          uint32_t stack_pa = pmm_alloc_page();
          if (!stack_pa) {
             printf("Failed to allocate user stack\n");
             return NULL;
          }
          newtask->user_stack=stack_pa;

          
        }

	
	//acquire(&tasklocks.lock);
	//acquire(&task_lock[newtask->cpu]);
	if (task_list[newtask->cpu] == NULL) {
		/* First taskess */
		task_list[newtask->cpu] = newtask;
		newtask->next = NULL;
		newtask->prev = NULL;
	} else {
		/* Any other tasks */
		newtask->next = task_list[newtask->cpu];
		newtask->prev = NULL;
		task_list[newtask->cpu]->prev = newtask;
		task_list[newtask->cpu] = newtask;
	}
	if (combined_task_list == NULL) {
		/* First taskess */
		combined_task_list = newtask;
		newtask->next = NULL;
		newtask->prev = NULL;
	} else {
		/* Any other taskess */
		newtask->next = combined_task_list;
		newtask->prev = NULL;
		combined_task_list->prev = newtask;
		combined_task_list = newtask;
	}
	// No current task? Make it the only task.
	if (current_task[newtask->cpu] == NULL) {
		current_task[newtask->cpu] = task_list[newtask->cpu];
	}

	task_count++;
	//release(&tasklocks.lock);
	//release(&task_lock[newtask->cpu]);

        llist_init_head(&newtask->sleep.sleepers);
        if (llist_empty(&sched_sleep)) {
            llist_append(&sched_sleep, &newtask->sleep.sleepers);
        }
        llist_init_head(&newtask->sched_node);

        if (llist_empty(&sched_root)) {
            llist_append(&sched_root, &newtask->sched_node);
            newtask->sched_node->next=&sched_root;
        }else{
            llist_append(&sched_root, &newtask->sched_node);
            newtask->sched_node->next=&sched_root;
        }
	return newtask;
}


/*
//call this function in driver program later
void task_register_idle(task_idle_timer_t handler, uint32_t type)
{
	printf("Register idler: %d\n", type);
        int IDLE_FOREGROUND=0;
	idle_timer_t* newidle = kmalloc(sizeof(idle_timer_t));
	if (!newidle) {
		return;
	}
	newidle->func = handler;
	if (type == IDLE_FOREGROUND) {
		newidle->next = task_idles;
		task_idles = newidle;
	} else {
		newidle->next = timer_idles;
		timer_idles = newidle;
	}
}

/*
 * do_fork() - fork 系统调用的实现
 * 创建当前进程的副本，包括独立的地址空间
 *
 * 返回值：
 * - 在父进程中：返回子进程的 task_t 指针
 * - 在子进程中：返回 NULL (通过设置 tf->eax = 0)
 *
 * 参考：
 * - Linux 0.11 do_fork()
 * - xv6 fork()
 * - OSDev wiki "Process Forking"
 */
task_t* do_fork(void) {
    extern task_t *current_task[];
    task_t *parent = current_task[logical_cpu_id()];

    if (!parent) {
        printf("[do_fork] ERROR: No current task!\n");
        return NULL;
    }

    printf("[do_fork] Parent PID=%d forking...\n", parent->pid);

    // 1. 分配新任务结构
    uint32_t child_phys = pmm_alloc_page();
    if (!child_phys) {
        printf("[do_fork] ERROR: Failed to allocate task structure!\n");
        return NULL;
    }
    task_t *child = (task_t*)phys_to_virt(child_phys);
    memset(child, 0, PAGE_SIZE);

    // 2. 复制任务结构的基本字段
    child->pid = nextid++;
    child->ppid = parent->pid;
    child->uid = parent->uid;
    child->gid = parent->gid;
    child->state = PS_CREATED;
    child->cpu = parent->cpu;
    child->nice = parent->nice;
    child->start_time = 0;  // 将在调度时设置
    child->time_slice = parent->time_slice;
    child->vruntime = 0;
    child->load_weight = parent->load_weight;
    child->directory = parent->directory;
    child->name = parent->name;
    child->size = parent->size;
    child->entry = parent->entry;
    child->signal_handler = parent->signal_handler;
    child->signal_mask = parent->signal_mask;
    child->pending_signals = 0;
    // ⚠️ intr_depth 字段已删除
    child->has_signal = false;
    child->idle_flags = 0;

    // 3. 分配独立的内核栈
    uint32_t kstack_phys = pmm_alloc_page();
    if (!kstack_phys) {
        printf("[do_fork] ERROR: Failed to allocate kernel stack!\n");
        // 释放任务结构
        pmm_free_page(child_phys);
        return NULL;
    }
    uint32_t kstack_virt = phys_to_virt(kstack_phys);
    child->kstack = (uint32_t*)kstack_virt;
    child->esp0 = kstack_virt + PAGE_SIZE;
    child->esp = kstack_virt + PAGE_SIZE;  // 关键修复：初始化 esp，调度器使用它来切换栈

    printf("[do_fork] Child task struct virt=0x%x, kstack_virt=0x%x, esp=0x%x\n",
           (uint32_t)child, kstack_virt, child->esp);

    // 4. 创建独立的页目录 (CR3)
    page_t pde_child = alloc_page_table_();
    child->pde = (uint32_t*)pde_child.phys;
    child->cr3 = (uint32_t*)pde_child.phys;
    child->directory = child->cr3;  // 关键修复：directory 指向子进程自己的页目录
    uint32_t *pd_child_virt = pde_child.virt;

    printf("[do_fork] Child page dir: phys=0x%x, virt=0x%x\n",
           pde_child.phys, pd_child_virt);

    // 5. 复制内核高端映射 (0xC0000000+)
    copy_kernel_mappings_to_pd(pd_child_virt);
    printf("[do_fork] Kernel mappings copied\n");

    // 6. 复制用户空间页表 (实现 Copy-on-Write)
    // 获取父进程的页目录虚拟地址
    uint32_t *pd_parent = (uint32_t*)phys_to_virt((uint32_t)parent->cr3);

    int copied_ptes = 0;
    int copied_pts = 0;

    for (int i = 0; i < 768; i++) {  // 用户空间：0-767
        if (pd_parent[i] & PAGE_PRESENT) {
            // 获取父进程的页表物理地址
            uint32_t parent_pt_phys = pd_parent[i] & ~0xFFF;

            // 分配新的页表
            page_t pt_child = alloc_page_table_();
            uint32_t *pt_parent_virt = (uint32_t*)phys_to_virt(parent_pt_phys);
            uint32_t *pt_child_virt = pt_child.virt;

            // 复制页表项，父子进程都标记为只读 (Copy-on-Write)
            for (int j = 0; j < 1024; j++) {
                if (pt_parent_virt[j] & PAGE_PRESENT) {
                    uint32_t pte = pt_parent_virt[j];

                    // Copy-on-Write: 父子进程都清除写位
                    pte &= ~PAGE_WRITABLE;  // 清除写位

                    // 关键修复：父进程页表也要标记为只读
                    pt_parent_virt[j] = pte;
                    pt_child_virt[j] = pte;
                    copied_ptes++;
                }
            }

            // 设置子进程页目录项
            pd_child_virt[i] = (uint32_t)pt_child.phys | (pd_parent[i] & 0xFFF);
            copied_pts++;
        }
    }

    // 刷新 TLB，确保页表修改生效
    asm volatile("movl %%cr3, %%eax; movl %%eax, %%cr3" ::: "%eax");

    printf("[do_fork] Copied %d page tables, %d page entries (COW)\n",
           copied_pts, copied_ptes);

    // 7. 复制 trapframe 并构建正确的内核栈布局
    // ⚠️⚠️⚠️ 关键修复：子进程的内核栈布局必须匹配 switch_to + interrupt_exit 的期望
    //
    // 当子进程第一次被调度时，流程是：
    //   1. switch_to(prev, child)  - 切换到子进程的内核栈
    //      - 恢复 [ebx, esi, edi, ebp] 从栈上
    //      - ret 返回到 schedule()
    //   2. schedule() 返回到 interrupt_exit
    //   3. interrupt_exit 恢复 trapframe 并 iret
    //
    // 因此，子进程的内核栈布局应该是（从低地址到高地址）：
    //   [低地址]
    //   ebp (0)                     <- child->esp 指向这里
    //   edi (0)
    //   esi (0)
    //   ebx (0)
    //   ret_addr (dummy, 指向 schedule 中 switch_to 之后的代码)
    //   ---- trapframe 开始 ----
    //   edi, esi, ebp, oesp, ebx, edx, ecx, eax (pusha)
    //   gs, fs, es, ds (段寄存器)
    //   err, trapno
    //   eip, cs, eflags, esp, ss (CPU 自动压入)
    //   ---- trapframe 结束 ----
    //   [高地址] kstack + PAGE_SIZE

    // 计算 trapframe 位置（在栈顶，76字节）
    child->tf = (struct trapframe*)(kstack_virt + PAGE_SIZE - sizeof(struct trapframe));
    memcpy(child->tf, parent->tf, sizeof(struct trapframe));

    // 子进程返回 0
    child->tf->eax = 0;

    printf("[do_fork] Trapframe copied: eip=0x%x, esp=0x%x\n",
           child->tf->eip, child->tf->esp);
    printf("[do_fork] tf at 0x%x, sizeof(tf)=%d\n",
           (uint32_t)child->tf, sizeof(struct trapframe));

    // ⚠️⚠️⚠️ 在 trapframe 下方构建 switch_to 帧（4个寄存器 + 1个返回地址）
    uint32_t *frame_bottom = (uint32_t*)child->tf - 5;  // 向下移动 5 个 dword

    // 栈布局（从低地址到高地址）：
    //   frame_bottom[0] = ebp
    //   frame_bottom[1] = edi
    //   frame_bottom[2] = esi
    //   frame_bottom[3] = ebx
    //   frame_bottom[4] = ret_addr (指向 schedule 中 switch_to 之后的代码)

    frame_bottom[0] = 0;  // ebp (初始值都是 0)
    frame_bottom[1] = 0;  // edi
    frame_bottom[2] = 0;  // esi
    frame_bottom[3] = 0;  // ebx
    // frame_bottom[4] 是返回地址，会被 switch_to 的 ret 使用
    // 但我们不需要精确的地址，因为 schedule() 会直接 return 到 interrupt_exit

    // 设置子进程的 ESP 指向 switch_to 帧的底部（ebp 的位置）
    child->esp = (uint32_t)&frame_bottom[0];

    printf("[do_fork] Child ESP=0x%x (switch_to frame bottom)\n", child->esp);
    printf("[do_fork] Stack: [switch_to frame: 20 bytes][trapframe: %d bytes][stack top]\n",
           20 + (int)sizeof(struct trapframe));

    // 8. 将子进程加入调度队列
    // 添加到任务链表
    if (combined_task_list == NULL) {
        combined_task_list = child;
        child->next = NULL;
        child->prev = NULL;
    } else {
        child->next = combined_task_list;
        child->prev = NULL;
        combined_task_list->prev = child;
        combined_task_list = child;
    }

    // 添加到 CPU 的任务列表
    if (task_list[child->cpu] == NULL) {
        task_list[child->cpu] = child;
    }

    task_count++;
    task_total_count++;

    // 初始化睡眠和调度节点
    extern int llist_empty(struct llist_header *list);
    extern void llist_init_head(struct llist_header *node);
    extern void llist_append(struct llist_header *head, struct llist_header *node);

    llist_init_head(&child->sleep.sleepers);
    if (llist_empty(&sched_sleep)) {
        llist_append(&sched_sleep, &child->sleep.sleepers);
    }

    llist_init_head(&child->sched_node);
    if (llist_empty(&sched_root)) {
        llist_append(&sched_root, &child->sched_node);
        child->sched_node->next = &sched_root;
    } else {
        llist_append(&sched_root, &child->sched_node);
        child->sched_node->next = &sched_root;
    }

    // 设置子进程状态
    // 注意：fork 创建的子进程不需要 PS_CREATED 状态
    // 因为它直接从 fork 系统调用返回，不需要初始化
    child->state = PS_READY;

    printf("[do_fork] Child PID=%d created successfully, state=PS_READY\n", child->pid);

    // 返回子进程指针（父进程中会使用）
    return child;
}

// ================================
// 调试函数：用于 task_to_user_mode_with_task
// ================================

extern void debug_print_task_to_user_entry(task_t *task) {
    printf("[task_to_user_mode] ENTRY: task=0x%x, pid=%d\n", (uint32_t)task, task->pid);
    printf("[task_to_user_mode] task->esp=0x%x, task->esp0=0x%x\n", task->esp, task->esp0);
    printf("[task_to_user_mode] task->tf=0x%x, task->user_stack=0x%x\n",
           (uint32_t)task->tf, task->user_stack);
    printf("[task_to_user_mode] task->state=%d, task->cr3=0x%x\n", task->state, (uint32_t)task->cr3);
}

extern void debug_print_task_to_user_tf(struct trapframe *tf) {
    printf("[task_to_user_mode] trapframe at 0x%x:\n", (uint32_t)tf);
    printf("  eip=0x%x, cs=0x%x, eflags=0x%x\n", tf->eip, tf->cs, tf->eflags);
    printf("  esp=0x%x, ss=0x%x\n", tf->esp, tf->ss);
    printf("  ds=0x%x, es=0x%x, fs=0x%x, gs=0x%x\n", tf->ds, tf->es, tf->fs, tf->gs);
}

extern void debug_print_task_to_user_before_iret(uint32_t eip, uint32_t cs,
                                                  uint32_t eflags, uint32_t esp, uint32_t ss) {
    printf("[task_to_user_mode] About to iret:\n");
    printf("  EIP=0x%x, CS=0x%x (RPL=%d)\n", eip, cs, cs & 3);
    printf("  EFLAGS=0x%x (IF=%d)\n", eflags, (eflags & 0x200) ? 1 : 0);
    printf("  ESP=0x%x, SS=0x%x (RPL=%d)\n", esp, ss, ss & 3);
}

extern void debug_print_task_to_user_after_tss(task_t *task) {
    printf("[task_to_user_mode] TSS.esp0 updated to 0x%x (task->esp0)\n", task->esp0);
    printf("[task_to_user_mode] Segment registers set to 0x%x (USER_DS)\n", USER_DS);
}

// 调试消息字符串
const char debug_msg_regs_restored[] = "[task_to_user_mode] General registers restored from trapframe\n";
const char debug_msg_iret_frame_ready[] = "[task_to_user_mode] IRET frame prepared on stack\n";
const char debug_msg_before_cr3[] = "[task_to_user_mode] Before CR3 switch: CR3=0x%x\n";
const char debug_msg_after_cr3[] = "[task_to_user_mode] After CR3 switch: new CR3=0x%x, TLB flushed\n";
const char debug_msg_seg_restored[] = "[task_to_user_mode] Segment registers restored: DS=ES=FS=GS=0x%x\n";
const char debug_msg_before_iret[] = "[task_to_user_mode] *** ABOUT TO EXECUTE IRET ***\n";

// ================================
// C包装函数：解决调用约定问题
// ================================

void task_to_user_mode_with_task_wrapper(struct task_t *task) {
    printf("[wrapper] Calling task_to_user_mode_with_task with task=0x%x\n", (uint32_t)task);
    printf("[wrapper] Before inline asm, EAX will be set to 0x%x\n", (uint32_t)task);

    // ⚠️⚠️⚠️ 调试：检查 trapframe 的值
    if (task->tf) {
        printf("[wrapper] trapframe BEFORE task_to_user_mode_with_task:\n");
        printf("  eip=0x%x, esp=0x%x\n", task->tf->eip, task->tf->esp);
        printf("  cs=0x%x, ss=0x%x, eflags=0x%x\n", task->tf->cs, task->tf->ss, task->tf->eflags);
    }

    // ⚠️⚠️⚠️ 关键修复：使用 volatile 变量防止编译器优化
    // 编译器可能会优化掉 task 参数，导致 EAX = 0
    volatile uint32_t task_volatile = (uint32_t)task;

    // ⚠️⚠️⚠️ 关键修复：在调用之前确保段寄存器指向内核数据段！
    // 因为调用者可能在用户态，DS/ES可能指向用户数据段
    // 如果不修复，jmp指令读取目标地址时会崩溃
    __asm__ volatile(
        "movw $0x10, %%ax\n\t"
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%fs\n\t"
        "movw %%ax, %%gs\n\t"
        : : : "ax", "memory"
    );

    // ⚠️⚠️⚠️ 关键修复：使用 jmp 而不是 call！
    // 原因：
    //   1. task_to_user_mode_with_task 最终执行 iret，永不返回
    //   2. call 会压入返回地址，破坏栈布局
    //   3. jmp 是尾跳转，不会压栈，保持栈干净
    __asm__ volatile(
        "movl %0, %%eax\n\t"
        "jmp task_to_user_mode_with_task\n\t"
        :
        : "r"(task_volatile)  // ⚠️ 使用 volatile 变量
        : "eax", "ecx", "edx", "memory"
    );

    // 不会返回到这里
    printf("[wrapper] ERROR: Returned from task_to_user_mode_with_task!\n");
    while(1) {
        __asm__ volatile("hlt");
    }
}
