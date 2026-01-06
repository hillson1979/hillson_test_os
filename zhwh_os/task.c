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
/**
 * @brief The currently running taskess on each CPU
 */

#define MAX_CPUS 8

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
        //current_task[logical_cpu_id()]=init;
        task_total_count++;
        return init;
}

/* 用户任务主函数 */
void user_task_main(struct task_t* th) {
    printf("User task started\n");
    int a=0;  
    while (1) {
        //printf("User task running...\n");
        
        //for (volatile int i = 0; i < 1000000; i++,a++);
        a++;
        /* 主动让出CPU */
        //yield();
        //break;
        
        if(a%1==0){
          // printf("User task runed some unit time, give up cpu\n");
          //task_to_user_mode(th);  // 调用系统调用，陷入内核阻塞,for test
          break;
          }
    }
    task_prepare_pde();
    task_to_user_mode();
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
    // printf("[copy_kernel_mappings_to_pd] START: pd_user=0x%x\n", (uint32_t)pd_user);
    // kernel occupies PDE indexes 768..1023 (0xC0000000 >> 22 == 768)
    for (int i = 768; i < 1024; ++i) {
        // printf("[copy_kernel_mappings_to_pd] Loop i=%u\n", i);
        uint32_t entry = pd[i];
        // printf("[copy_kernel_mappings_to_pd] pd[%u]=0x%x\n", i, entry);
        // 如果原来 pd_kernel[i] 为 0，就跳过
        if ((entry & PTE_P) == 0) {
            // printf("[copy_kernel_mappings_to_pd] Skipping (not present)\n");
            pd_user[i] = 0;
            continue;
        }
        // 清除用户位（U/S），保持内核只在 supervisor 模式访问
        // printf("[copy_kernel_mappings_to_pd] Writing to pd_user[%u]=0x%x\n", i, entry & ~PTE_U);
        pd_user[i] = entry & ~PTE_U;
        // printf("[copy_kernel_mappings_to_pd] Wrote pd_user[%u]=0x%x\n", i, pd_user[i]);
    }
    // printf("[copy_kernel_mappings_to_pd] END\n");
}

#define PHYS_VIDEO 0xB8000
#define VIRT_USER_VIDEO 0xB8000

void task_prepare_pde(){

       // printf("[task_prepare_pde] START\n");
       page_t pde = alloc_page_table_();
       // printf("[task_prepare_pde] alloc_page_table_ returned: phys=0x%x, virt=0x%x\n", (uint32_t)pde.phys, (uint32_t)pde.virt);
       th_u->pde = (uint32_t*)pde.phys;   // CR3 必须用物理地址
       uint32_t *pd_user = pde.virt;      // 内核里可以直接访问 PDE 内容

       // printf("[task_prepare_pde] th_u->pde=0x%x, pd_user=0x%x\n", (uint32_t)th_u->pde, (uint32_t)pd_user);

       th_u->cr3 = pd_user;

      /* // 比如把用户虚拟地址 0x08000000 映射到物理地址 0x00400000
       map_page(th_u->pde, VIRT_MEM_BASE_USER,PHYS_MEM_BASE_USER, PAGE_USER | PAGE_WRITE);

       // 还可以继续映射多个页
       for(int i=1;i<4;i++){
          map_page(th_u->pde, VIRT_MEM_BASE_USER+0x1000 * i,PHYS_MEM_BASE_USER+ 0x1000 *i, PAGE_USER | PAGE_WRITE);
         }
       */

       load_module_to_user(th_u->pde);

       // 复制内核高端映射，保证 kernel addresses 仍可访问
       copy_kernel_mappings_to_pd(pd_user);

       // 映射到用户虚拟空间
       //map_page(th_u->pde, VIRT_USER_STACK_TOP - PAGE_SIZE, th_u->user_stack, PTE_P|PTE_W|PTE_U);

       // 用户栈顶虚拟地址（用户程序开始时 esp 指向这里）
       //th_u->tf->esp = VIRT_USER_STACK_TOP;

       //printf("[task_load] user stack mapped at 0x%x (PA=0x%x)\n",VIRT_USER_STACK_TOP - PAGE_SIZE, th_u->user_stack);
       //iframe=(struct trapframe *)(th_u->tf);
       // 不要在这里切换 CR3，否则后续的 printf 和其他内核操作会失败
       // CR3 将在 task_to_user_mode 中切换
       //asm volatile("movl %%eax, %%cr3\n" ::"a"(th_u->pde));

       map_page(th_u->pde, VIRT_USER_VIDEO, PHYS_VIDEO, PTE_P | PTE_W | PTE_U);//video in user

}


/* 假定这些宏与你内核一致 */
#define USER_CS  ((SEG_UCODE << 3) | DPL_USER)   /* 0x1b in your case */
#define USER_DS  ((SEG_UDATA << 3) | DPL_USER)   /* 0x23 in your case */
#define FL_IF    0x00000200

void task_to_user_mode()
{
    task_t *task = th_u;

    struct trapframe *tf = task->tf;

    // 简单调试：输出一个字符表示进入 task_to_user_mode
    extern void vga_putc(char c);
    vga_putc('1'); // 标记进入 task_to_user_mode

    tf->edi = 1;
    tf->esi = 2;
    tf->ebp = 3;
    tf->ebx = 5;
    tf->edx = 6;
    tf->ecx = 7;
    tf->eax = 8;

    // 修复: gs 不能设置为 0 (null selector),会导致 GP Fault
    // 应该设置为用户数据段选择子
    tf->gs = (SEG_UDATA << 3) | DPL_USER;
    tf->ds = (SEG_UDATA << 3) | DPL_USER;
    tf->es = (SEG_UDATA << 3) | DPL_USER;
    tf->fs = (SEG_UDATA << 3) | DPL_USER;
    tf->ss = (SEG_UDATA << 3) | DPL_USER;
    tf->cs = (SEG_UCODE << 3) | DPL_USER;

    // 注意：不要覆盖 load_module_to_user() 设置的 eip 和 esp
    // 因为 tf 结构体可能被重新分配，或者需要确保值的一致性
    // 但 load_module_to_user() 已经正确设置了这些值

    // tf->esp = VIRT_USER_STACK_TOP;
    // tf->eip = 0x80000000;  // 不要硬编码！使用 load_module_to_user() 设置的值
    tf->eflags = FL_IF; // 开中断，测试系统调用是否正常

    // 设置当前CPU的TSS - 更新全局TSS的esp0
    extern struct tss_t tss;  // 在segment.c中定义的全局TSS
    tss.ss0 = SEG_KDATA << 3;
    tss.esp0 = (uint32_t)th_u->kstack + KSTACKSIZE;  // 栈顶是基址+大小
    printf("[task_to_user_mode] TSS.esp0=0x%x (kstack=0x%x, KSTACKSIZE=%d)\n",
           tss.esp0, th_u->kstack, KSTACKSIZE);

    // 检查内核栈是否在当前页表（用户页表）中可访问
    uint32_t current_cr3 = 0;
    asm volatile("movl %%cr3, %0" : "=r"(current_cr3));
    printf("[task_to_user_mode] Current CR3=0x%x (physical)\n", current_cr3);
    printf("[task_to_user_mode] Task PDE=0x%x (physical)\n", (uint32_t)th_u->pde);
    printf("[task_to_user_mode] Kernel stack at 0x%x (virtual)\n", (uint32_t)th_u->kstack + KSTACKSIZE);

    // 注意:不需要再次调用ltr(),TSS已经在tss_init()中加载过了

    // 关键修复：在切换 CR3 之前,将 trapframe 复制到当前栈上
    // 这样切换页表后,栈上的 trapframe 仍然可访问
    printf("[task_to_user_mode] Before memcpy: tf->eip=0x%x\n", tf->eip);

    struct trapframe stack_tf;
    memcpy(&stack_tf, tf, sizeof(struct trapframe));

    printf("[task_to_user_mode] After memcpy: stack_tf.eip=0x%x\n", stack_tf.eip);
    printf("[task_to_user_mode] stack_tf.esp=0x%x, stack_tf.cs=0x%x, stack_tf.ss=0x%x\n", stack_tf.esp, stack_tf.cs, stack_tf.ss);
    printf("[task_to_user_mode] tf->esp=0x%x, tf->cs=0x%x, tf->ss=0x%x\n", tf->esp, tf->cs, tf->ss);

    // 实验性修改：不切换 CR3，使用内核页表运行用户进程
    // 因为用户地址空间也应该被映射到内核页表中
    printf("[task_to_user_mode] Keeping kernel page table (NOT switching CR3)\n");
    // asm volatile ("movl %0, %%cr3" :: "r"(task->pde) : "memory");

    // 从struct trapframe返回用户模式
    // struct trapframe字段顺序:
    //   [0] edi [4] esi [8] ebp [12] oesp [16] ebx [20] edx
    //   [24] ecx [28] eax [32] gs  [36] fs   [40] es   [44] ds
    //   [48] trapno [52] err [56] eip [60] cs [64] eflags
    //   [68] esp [72] ss

    asm volatile (
        "cli\n\t"                                    // 禁中断
        "movl %0, %%esp\n\t"                         // ESP = &stack_tf

        // 恢复通用寄存器（使用 popa）
        "popa\n\t"

        // 现在ESP指向 gs，手动恢复段寄存器
        "popl %%eax\n\t"                             // 弹出 gs
        "movw %%ax, %%gs\n\t"
        "popl %%eax\n\t"                             // 弹出 fs
        "movw %%ax, %%fs\n\t"
        "popl %%eax\n\t"                             // 弹出 es
        "movw %%ax, %%es\n\t"
        "popl %%eax\n\t"                             // 弹出 ds
        "movw %%ax, %%ds\n\t"

        // 跳过 trapno 和 err
        "addl $8, %%esp\n\t"

        // iret会弹出eip,cs,eflags,esp,ss并跳转到用户代码
        "iret\n\t"
        :: "r"(&stack_tf)
        : "eax", "memory"
    );

    // 不会执行到这里
}


/* 内核任务主函数 */
void kernel_task_main(struct task_t* th) {
    printf("Kernel task started\n");
    int i=0;
    while (1) {
        if (++i==1)
           printf("Kernel task running...\n");
        break;
        
        /* 内核任务工作 */
        //system_maintenance();
        
        /* 短暂延迟 */
        for (volatile int i = 0; i < 500000; i++);
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

   if (th->entry != NULL) {
        entry_kernel(th);  // 调用回调函数，传递任务结构体作为参数
    }
   th->user_stack=0;
}


void
start_task(struct task_t* th, task_entry_callback_t entry) {

    //if(th && !task_terminated(th))return;
    th->entry=entry;

    //llist_append(sched_root->sched_node, th->sched_node);
    
    if (sched_root) {
        llist_append(sched_root, th->sched_node);
    } else {
        sched_root = th->sched_node;
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
        th_u=newtask;
        // trapframe 紧随其后
        //newtask->tf = (struct trapframe*)(newtask + 1);

        // 栈放在页尾（从高地址往下长）
        newtask->kstack = (uint32_t*)((uint8_t*)page + PAGE_SIZE);

        // trapframe 在栈顶下方
        newtask->tf = (struct trapframe *)((uint8_t*)newtask->kstack - sizeof(struct trapframe));
	newtask->waitpid = 0;
	newtask->name = "task_";//strdup(fsi->filename);
	newtask->pid = nextid++;
	newtask->directory = "";//strdup(fullpath);
	newtask->csd = "csd";//strdup(csd);
	newtask->size = 4096;//fsi->size;  4k

        newtask->pde = pd;//copy_pde(newtask);
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
}*/
