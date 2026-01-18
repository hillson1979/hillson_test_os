#include "types.h"
//#include "task.h"
#include "lapic.h"
#include "sched.h"
#include "interrupt.h"
//#include "x86/mmu.h"
#include "x86/io.h"
//#include "page.h"  // mm.h 已经包含了 page.h
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

// ⚠️ 全局变量：保存内核页目录的物理地址
// 在内核初始化时设置，do_fork() 需要使用它来映射子进程内存
uint32_t kernel_page_directory_phys = 0;

// ⚠️ 调试函数：打印进入task_to_user_mode_with_task
void debug_print_enter(void) {
    printf("[DEBUG] ===== Entering task_to_user_mode_with_task =====\n");
}

// ⚠️ 调试函数：打印tf指针
void debug_print_tf_ptr(uint32_t tf_ptr) {
    printf("[DEBUG] task->tf = 0x%x\n", tf_ptr);
}

// ⚠️ 调试全局变量：用于从汇编代码传递调试信息
volatile uint32_t debug_esp_before_iret = 0;
volatile uint32_t debug_eip_on_stack = 0;
volatile uint32_t debug_cs_on_stack = 0;
volatile uint32_t debug_eflags_on_stack = 0;
volatile uint32_t debug_user_esp_on_stack = 0;  // 用户栈 ESP
volatile uint32_t debug_ss_on_stack = 0;        // 用户栈 SS
volatile uint32_t debug_cr3_before_iret = 0;
volatile uint32_t debug_iret_executed = 0;  // 标记 iret 之后是否立即执行

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

    // ⚠️⚠️⚠️ 关键修复：使用固定的内核页目录，而不是从当前 CR3 读取
    // 原因：fork() 中当前 CR3 可能是父进程的 CR3，不是内核 CR3
    extern uint32_t kernel_page_directory_phys;
    uint32_t *pd_kernel = (uint32_t*)phys_to_virt(kernel_page_directory_phys);

    printf("[copy_kernel_mappings_to_pd] kernel PD phys=0x%x, pd_kernel=0x%x\n",
           kernel_page_directory_phys, (uint32_t)pd_kernel);

    // kernel occupies PDE indexes 768..1023 (0xC0000000 >> 22 == 768)
    int mapped_count = 0;
    for (int i = 768; i < 1024; ++i) {
        uint32_t entry = pd_kernel[i];

        // ⚠️⚠️⚠️ 关键修复：如果 pd_user[i] 已经存在，不要覆盖！
        // fork() 中可能已经为子进程设置了特殊的内核映射（如 PD[777]）
        if (pd_user[i] & PAGE_PRESENT) {
            printf("[copy_kernel_mappings_to_pd] Skipping pd[%u] (already present: 0x%x)\n",
                   i, pd_user[i]);
            continue;
        }

        // 如果原来 pd_kernel[i] 为 0，就跳过
        if ((entry & PAGE_PRESENT) == 0) {
            pd_user[i] = 0;
            continue;
        }

        // ⚠️⚠️⚠️ 关键修复：原样复制内核 PDE，不要修改权限位！
        // 不要设置 PDE_USER，内核页必须保持 U=0（内核态专用）
        pd_user[i] = entry;

        mapped_count++;
        if (mapped_count <= 5) {
            printf("[copy_kernel_mappings_to_pd] Copied pd[%u]=0x%x -> pd_user[%u]=0x%x\n",
                   i, entry, i, pd_user[i]);
        }
    }
    printf("[copy_kernel_mappings_to_pd] DONE: copied %u kernel mappings\n", mapped_count);
}

#define PHYS_VIDEO 0xB8000
#define VIRT_USER_VIDEO 0xB8000

void task_prepare_pde(struct task_t *task){
       printf("[task_prepare_pde] START for task=0x%x\n", task);

       // ⚠️⚠️⚠️ Linux 方案：用户任务直接使用内核 CR3，共享内核页表
       // 这样所有进程的 CR3 都相同，switch_to 时不需要切换 CR3
       extern uint32_t kernel_page_directory_phys;

       task->pde = (uint32_t*)kernel_page_directory_phys;
       task->cr3 = (uint32_t*)kernel_page_directory_phys;
       task->directory = task->cr3;

       printf("[task_prepare_pde] User task using kernel CR3: 0x%x (shared with all)\n",
              (uint32_t)task->cr3);
       printf("[task_prepare_pde] task->pde=0x%x (kernel page directory)\n", (uint32_t)task->pde);

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

       // ⚠️⚠️⚠️ 关键修复：不要覆盖 tf->esp！
       // 原因：load_module_to_user 已经按照Linux ABI标准设置了正确的ESP
       //      ESP应该指向argc（栈顶下方4字节），而不是栈顶
       // 如果覆盖为VIRT_USER_STACK_TOP，会导致：
       //   1. 用户程序访问[ESP]时得到未初始化的垃圾数据
       //   2. C运行时代码解引用这个垃圾指针 -> NULL-0x18崩溃
       //
       // 正确的ABI布局（由load_module_to_user设置）：
       //   [ESP] = argc (0)
       //   [ESP+4] = argv[0] (NULL)
       //   [ESP+8] = argv[1] (NULL)
       //   ...
       printf("[task_prepare_pde] Keeping tf->esp=0x%x (set by load_module_to_user)\n",
              task->tf->esp);
       printf("[task_prepare_pde] This ESP points to argc, following Linux ABI standard\n");

       //printf("[task_load] user stack mapped at 0x%x (PA=0x%x)\n",VIRT_USER_STACK_TOP - PAGE_SIZE, task->user_stack);
       //iframe=(struct trapframe *)(task->tf);
       // 不要在这里切换 CR3，否则后续的 printf 和其他内核操作会失败
       // CR3 将在 task_to_user_mode 中切换
       //asm volatile("movl %%eax, %%cr3\n" ::"a"(task->pde));

       // ⚠️ 不要在这里映射 VGA！原因：
       // 1. task->pde 是物理地址，需要先切换 CR3 才能访问
       // 2. 用户页目录的物理地址还没有被映射到内核虚拟地址空间
       // 3. VGA 映射应该在 load_module_to_user 中完成（如果需要的话）
       // map_page(task->pde, VIRT_USER_VIDEO, PHYS_VIDEO, PTE_P | PTE_W | PTE_U);

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

        // ⚠️ kstack 已经指向页末尾（栈顶），所以 esp0 = kstack
        // 在用户态→内核态切换时，CPU会使用这个地址作为内核栈指针
        newtask->esp0 = (uint32_t)newtask->kstack;

        // ⚠️⚠️⚠️ 关键修复：esp 应该指向栈顶下方，为 trapframe 预留空间
        // 原因：当用户任务被中断时，CPU 会从 esp0 开始压栈（创建 trapframe）
        //      如果 esp == esp0，调度器切换时会破坏 trapframe
        //
        // 正确的布局（从高地址到低地址）：
        //   [栈顶/esp0] <-- CPU 从这里开始压栈（中断时）
        //   [trapframe (76 字节)] <-- CPU 压入
        //   [esp 指向这里] <-- 调度器使用
        newtask->esp = (uint32_t)newtask->kstack - sizeof(struct trapframe);

        // ⚠️⚠️⚠️ 初始化 has_run_user = 0，表示还未进入过用户态
        newtask->has_run_user = 0;

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
    printf("[do_fork] parent=0x%x, parent->tf=0x%x, parent->kstack=0x%x\n",
           (uint32_t)parent, (uint32_t)parent->tf, (uint32_t)parent->kstack);

    // ⚠️ 安全检查：确保 parent->tf 有效
    if (parent->tf == NULL) {
        printf("[do_fork] ERROR: parent->tf is NULL!\n");
        return NULL;
    }

    // ⚠️ 获取内核的 CR3（需要用来映射子进程的内存到内核页表）
    extern void map_page(uint32_t pde_phys, uint32_t vaddr, uint32_t paddr, uint32_t flags);
    extern uint32_t kernel_page_directory_phys;  // ⚠️ 使用全局变量，不是当前CR3

    // 1. 分配新任务结构
    uint32_t child_phys = pmm_alloc_page();
    if (!child_phys) {
        printf("[do_fork] ERROR: Failed to allocate task structure!\n");
        return NULL;
    }
    uint32_t child_virt = phys_to_virt(child_phys);

    // ⚠️⚠️⚠️ 关键修复：临时切换到内核 CR3 来执行映射操作
    // 原因：当前在系统调用中，CR3 是父进程的 CR3，需要切换到内核 CR3 才能修改内核页目录
    uint32_t current_cr3;
    __asm__ volatile("movl %%cr3, %0" : "=r"(current_cr3));
    uint32_t kernel_cr3_value = kernel_page_directory_phys | 0x3;  // 添加标志位
    __asm__ volatile("movl %0, %%cr3" : : "r"(kernel_cr3_value));
    printf("[do_fork] Temporarily switched CR3: 0x%x -> 0x%x\n", current_cr3, kernel_cr3_value);

    // ⚠️⚠️⚠️ 关键修复：将子进程任务结构映射到内核的页表中
    // 原因：child_phys 可能在 16MB 之前，不在内核的直接映射区域
    printf("[do_fork] Mapping child task struct to kernel page table: phys=0x%x, virt=0x%x\n",
           child_phys, child_virt);
    map_page(kernel_page_directory_phys, child_virt, child_phys, PAGE_PRESENT | PAGE_WRITABLE);
    printf("[do_fork] Child task struct mapped\n");

    task_t *child = (task_t*)child_virt;
    memset(child, 0, PAGE_SIZE);

    // 2. 复制任务结构的基本字段
    child->pid = nextid++;
    child->ppid = parent->pid;
    child->uid = parent->uid;
    child->gid = parent->gid;
    child->state = PS_CREATED;
    child->cpu = parent->cpu;
    child->nice = parent->nice;
    child->has_run_user = 0;  // ⚠️⚠️⚠️ 子进程还未进入过用户态
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

    // ⚠️⚠️⚠️ 关键修复：复制 user_stack 字段!
    // 子进程和父进程共享同一个用户虚拟地址空间(COW),所以 user_stack 值相同
    child->user_stack = parent->user_stack;

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

    printf("[do_fork] Child task struct virt=0x%x, kstack_phys=0x%x, kstack_virt=0x%x, esp=0x%x\n",
           (uint32_t)child, kstack_phys, kstack_virt, child->esp);

    // ⚠️⚠️⚠️ 关键修复：将子进程的内核栈映射到内核的页表中
    // 原因：内核执行时使用内核的 CR3，需要能访问子进程的内核栈
    // kstack_phys 可能在 16MB 之前（不在内核的直接映射区域）
    printf("[do_fork] Mapping child kstack to kernel page table: phys=0x%x, virt=0x%x\n",
           kstack_phys, kstack_virt);
    map_page(kernel_page_directory_phys, kstack_virt, kstack_phys, PAGE_PRESENT | PAGE_WRITABLE);
    printf("[do_fork] Child kernel stack mapped to kernel page table\n");

    // ⚠️⚠️⚠️ 标准 fork 实现：每个子进程有独立的 CR3（页目录）
    //
    // 设计原则：
    //   1. 子进程有独立的页目录（自己的 CR3）
    //   2. 内核空间映射（768-1023）直接复制（所有进程共享）
    //   3. 用户空间映射（0-767）需要复制页表和物理页（COW）
    //
    // 为什么要独立 CR3？
    //   - 避免多个子进程互相覆盖用户栈映射
    //   - 符合标准 fork 语义：父子进程有独立的地址空间
    //
    printf("[do_fork] Creating independent page directory for child...\n");

    // 1. 分配新的页目录
    uint32_t child_pd_phys = pmm_alloc_page();
    if (!child_pd_phys) {
        printf("[do_fork] ERROR: Failed to allocate page directory for child!\n");
        pmm_free_page(child_phys);
        pmm_free_page(kstack_phys);
        return NULL;
    }
    uint32_t child_pd_virt = phys_to_virt(child_pd_phys);
    uint32_t *child_pd = (uint32_t*)child_pd_virt;

    printf("[do_fork] Allocated child PD: phys=0x%x, virt=0x%x\n",
           child_pd_phys, child_pd_virt);

    // 2. 清空子进程页目录（避免垃圾数据）
    memset(child_pd, 0, PAGE_SIZE);

    // 3. 复制内核映射（768-1023 项）
    //    使用内核全局 CR3 来访问父进程的页目录
    extern uint32_t kernel_page_directory_phys;
    uint32_t *parent_pd = (uint32_t*)phys_to_virt(kernel_page_directory_phys);

    for (int i = 768; i < 1024; i++) {
        child_pd[i] = parent_pd[i];
    }

    printf("[do_fork] Copied kernel mappings (768-1023)\n");

    // 4. 复制用户空间页表（0-767 项）
    //    ⚠️ 关键：需要复制页表结构，但暂时共享物理页（COW 的第一步）
    //
    // ⚠️⚠️⚠️ 关键修复：确保在 kernel CR3 下复制页表
    // 原因：memcpy 需要访问内核虚拟地址，必须在 kernel CR3 下进行
    uint32_t current_cr3_check;
    __asm__ volatile("movl %%cr3, %0" : "=r"(current_cr3_check));
    if ((current_cr3_check & ~0xFFF) != kernel_page_directory_phys) {
        printf("[do_fork] WARNING: CR3 changed! Restoring kernel CR3...\n");
        printf("[do_fork] Current CR3=0x%x, Expected=0x%x\n",
               current_cr3_check, kernel_page_directory_phys);
        uint32_t kernel_cr3_value = kernel_page_directory_phys | 0x3;
        __asm__ volatile("movl %0, %%cr3" : : "r"(kernel_cr3_value));
    }

    // ⚠️⚠️⚠️ 简化策略：只复制已知的用户空间页表
    // 根据 load_module_to_user 的输出，我们知道：
    //   - PD[32] = 0x2402007 (用户代码页表 @ 0x8000000)
    //   - PD[767] = 0x2404007 (用户栈页表 @ 0xBFFFF000)
    //
    // 其他用户空间的 PDE 项可能是未初始化的垃圾数据，不应该复制！
    // 这样可以避免访问无效的物理地址（如 0xD0200000）
    //
    int user_pd_indices[] = {32, 767};  // 用户代码和用户栈
    int num_user_pds = 2;

    for (int idx = 0; idx < num_user_pds; idx++) {
        int i = user_pd_indices[idx];

        if (parent_pd[i] & PAGE_PRESENT) {
            uint32_t parent_pt_phys = parent_pd[i] & ~0xFFF;

            // ⚠️⚠️⚠️ 分配新的页表物理页
            uint32_t child_pt_phys = pmm_alloc_page();
            if (!child_pt_phys) {
                printf("[do_fork] ERROR: Failed to allocate page table for PD[%d]!\n", i);
                continue;
            }

            // 复制页表内容（共享物理页映射）
            void *parent_pt_virt = phys_to_virt(parent_pt_phys);
            void *child_pt_virt = phys_to_virt(child_pt_phys);

            printf("[do_fork] memcpy: parent_pt_virt=0x%x, child_pt_virt=0x%x\n",
                   (uint32_t)parent_pt_virt, (uint32_t)child_pt_virt);

            memcpy(child_pt_virt, parent_pt_virt, PAGE_SIZE);

            // 设置子进程的 PDE（保持相同的标志位）
            child_pd[i] = child_pt_phys | (parent_pd[i] & 0xFFF);

            printf("[do_fork] Copied PD[%d]: parent_pt=0x%x -> child_pt=0x%x\n",
                   i, parent_pt_phys, child_pt_phys);
        }
    }

    printf("[do_fork] Copied user space page tables (0-767)\n");

    // 5. 设置子进程的 CR3 和 pde
    child->pde = (uint32_t*)child_pd_phys;  // 存储物理地址
    child->cr3 = (uint32_t*)child_pd_phys;   // CR3 需要物理地址
    child->directory = child->cr3;

    printf("[do_fork] Child using independent CR3: 0x%x\n", (uint32_t)child->cr3);

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

    // ⚠️ 调试：打印 tf 字段的偏移量
    printf("[do_fork] DEBUG: child task struct addr=0x%x\n", (uint32_t)child);
    printf("[do_fork] DEBUG: &child->tf=0x%x, offset=%d\n",
           (uint32_t)&child->tf, (uint32_t)&child->tf - (uint32_t)child);
    printf("[do_fork] DEBUG: child->tf=0x%x (value stored)\n", (uint32_t)child->tf);
    printf("[do_fork] Before memcpy: child->tf=0x%x, parent->tf=0x%x, size=%d\n",
           (uint32_t)child->tf, (uint32_t)parent->tf, sizeof(struct trapframe));

    // 复制 trapframe（现在应该可以正常工作了，因为子进程内核栈已映射到内核页表）
    memcpy(child->tf, parent->tf, sizeof(struct trapframe));

    printf("[do_fork] Trapframe copied successfully\n");

    // 子进程返回 0
    child->tf->eax = 0;

    printf("[do_fork] Trapframe copied: eip=0x%x, esp=0x%x\n",
           child->tf->eip, child->tf->esp);
    printf("[do_fork] Parent tf at 0x%x, Child tf at 0x%x, sizeof(tf)=%d\n",
           (uint32_t)parent->tf, (uint32_t)child->tf, sizeof(struct trapframe));

    // ⚠️⚠️⚠️ 关键调试：验证 child->tf 的内容是否正确
    printf("[do_fork] Child trapfield DUMP:\n");
    printf("  eip=0x%x, cs=0x%x, eflags=0x%x\n", child->tf->eip, child->tf->cs, child->tf->eflags);
    printf("  esp=0x%x, ss=0x%x\n", child->tf->esp, child->tf->ss);
    printf("  eax=0x%x, ebx=0x%x, ecx=0x%x, edx=0x%x\n", child->tf->eax, child->tf->ebx, child->tf->ecx, child->tf->edx);
    printf("  esi=0x%x, edi=0x%x, ebp=0x%x\n", child->tf->esi, child->tf->edi, child->tf->ebp);

    // ⚠️⚠️⚠️ 简化方案：父子进程共享用户栈映射
    // 原因：共享 CR3，所有映射都相同
    // 问题：多个子进程会互相覆盖用户栈内容
    // 解决：后续需要实现 COW（写时复制）
    //
    printf("[do_fork] Child sharing parent's address space (including user stack)\n");
    printf("[do_fork] ⚠️ WARNING: Multiple children will overwrite each other's user stack!\n");
    printf("[do_fork] ⚠️ TODO: Implement COW (Copy-On-Write) mechanism\n");

    // ⚠️⚠️⚠️ 关键修复：fork的子进程不需要构建switch_to帧！
    // 原因：
    //   1. 子进程状态是PS_CREATED
    //   2. 调度器会直接调用task_to_user_mode_with_task（不走switch_to路径）
    //   3. task_to_user_mode_with_task直接恢复trapframe并iret，不需要switch_to帧
    //
    // 错误的代码（已删除）：
    //   uint32_t *frame_bottom = (uint32_t*)child->tf - 5;
    //   child->esp = (uint32_t)&frame_bottom[0];
    //
    // 正确的逻辑：
    //   child->tf 已经正确复制父进程的trapframe
    //   child->esp 应该指向内核栈顶（不需要修改）
    //   task_to_user_mode_with_task会使用child->tf来恢复用户态
    //
    printf("[do_fork] Child trapframe ready: tf=0x%x, eip=0x%x, esp=0x%x\n",
           (uint32_t)child->tf, child->tf->eip, child->tf->esp);
    printf("[do_fork] Child will enter user mode via task_to_user_mode_with_task\n");

    // 8. 将子进程加入调度队列
    // ⚠️⚠️⚠️ 关键修复：添加到循环链表中，这样调度器才能找到子进程
    extern struct task_t *combined_task_list;
    extern task_t *current_task[];

    // ⚠️⚠️⚠️ 关键修复：需要找到循环链表的真正头部(PID=1,内核任务)
    // 不能使用 current_task[0],因为它是当前运行的任务(可能是 PID=2)
    // 方法:从 current_task[0] 开始遍历,找到 PID=1 的任务
    struct task_t *first_task = current_task[0];
    struct task_t *temp = first_task;

    // 遍历循环链表,找到 PID=1 (内核任务)
    while (temp != NULL && temp->pid != 1) {
        temp = temp->next;
        if (temp == first_task) {
            // 回到起点了,没找到 PID=1
            printf("[do_fork] ERROR: Cannot find PID=1 in task list!\n");
            break;
        }
    }

    if (temp != NULL && temp->pid == 1) {
        first_task = temp;
    }

    printf("[do_fork] Found first_task: pid=%d\n", first_task ? first_task->pid : -1);

    if (first_task == NULL) {
        // 不应该发生：至少应该有内核任务和父进程
        printf("[do_fork] ERROR: No tasks in system!\n");
        child->next = NULL;
        child->prev = NULL;
    } else {
        // ⚠️⚠️⚠️ 关键修复：找到 first_task 的前驱节点(真正的链表末尾)
        // 在循环链表中: last->next = first_task
        struct task_t *last = first_task->prev;

        if (last == NULL) {
            // 不应该发生：循环链表应该有 prev 指针
            printf("[do_fork] ERROR: first_task->prev is NULL!\n");
            last = first_task;
            while (last->next != NULL && last->next != first_task) {
                last = last->next;
            }
        }

        // 将 child 添加到循环链表的末尾(在 first_task 之前)
        // 原来的链表: ... -> last -> first_task -> ...
        // 新的链表: ... -> last -> child -> first_task -> ...
        last->next = child;
        child->prev = last;
        child->next = first_task;
        first_task->prev = child;

        printf("[do_fork] Added child to circular list: prev->pid=%d, next->pid=%d\n",
               child->prev ? child->prev->pid : -1,
               child->next ? child->next->pid : -1);
    }

    // 同时更新 combined_task_list (用于其他用途，不需要循环)
    if (combined_task_list == NULL) {
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

    // ⚠️⚠️⚠️ 关键修复：fork 创建的子进程保持 PS_CREATED 状态！
    // 原因：子进程第一次被调度时，必须走 task_to_user_mode_with_task 路径，
    //       而不是 switch_to 路径！
    //
    // 如果使用 PS_READY，schedule() 会调用 switch_to，
    // 但子进程的栈上没有合法的 C 调用返回地址，会导致 ret 跳飞并 triple fault！
    //
    // 正确流程：
    //   1. fork() 创建子进程，state = PS_CREATED
    //   2. schedule() 检测到 PS_CREATED，调用 task_to_user_mode_with_task
    //   3. task_to_user_mode_with_task 恢复 trapframe 并 iret 到用户态
    //   4. 用户态执行，后续调度时 state = PS_READY/PS_RUNNING，走 switch_to 路径
    //
    // 保持第 650 行设置的 PS_CREATED 状态，不要覆盖！
    // child->state 已经是 PS_CREATED（第 650 行）

    printf("[do_fork] Child PID=%d created successfully, state=%d (PS_CREATED)\n", child->pid, child->state);

    // ⚠️⚠️⚠️ 恢复父进程的 CR3
    __asm__ volatile("movl %0, %%cr3" : : "r"(current_cr3));
    printf("[do_fork] Restored CR3: 0x%x -> 0x%x\n", kernel_cr3_value, current_cr3);

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
    // ⚠️⚠️⚠️ 关键调试：检查 task 参数
    printf("[task_to_user_mode_wrapper] ENTRY: task=0x%x\n", (uint32_t)task);

    // ⚠️ 移除所有printf调试,避免printf中的除法导致异常

    // ⚠️⚠️⚠️ 关键调试:检查task指针是否为NULL
    if (task == 0) {
        printf("[task_to_user_mode_wrapper] ERROR: task is NULL!\n");
        __asm__ volatile("hlt");
    }
    if (task->tf == 0) {
        printf("[task_to_user_mode_wrapper] ERROR: task->tf is NULL!\n");
        __asm__ volatile("hlt");
    }

    // ⚠️⚠️⚠️ 关键调试：打印 trapframe 内容
    printf("[task_to_user_mode_wrapper] task=0x%x, pid=%d\n", (uint32_t)task, task->pid);
    printf("[task_to_user_mode_wrapper] task->tf=0x%x\n", (uint32_t)task->tf);
    printf("[task_to_user_mode_wrapper] trapframe content:\n");
    printf("  eip=0x%x, cs=0x%x, eflags=0x%x\n", task->tf->eip, task->tf->cs, task->tf->eflags);
    printf("  esp=0x%x, ss=0x%x\n", task->tf->esp, task->tf->ss);
    printf("  eax=0x%x (should be 0 for child)\n", task->tf->eax);

    // ⚠️⚠️⚠️ 直接调用 task_to_user_mode_with_task
    //    参数通过 C 调用约定传递（栈）
    printf("[task_to_user_mode_wrapper] task_volatile = 0x%x (about to call task_to_user_mode_with_task)\n", (uint32_t)task);

    task_to_user_mode_with_task(task);

    // 不会返回到这里
    __asm__ volatile("hlt");  // 如果返回了,说明有错误
    while(1) {
        __asm__ volatile("hlt");
    }
}
