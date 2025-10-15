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
    pg.phys = (uint32_t*)pmm_alloc_page();       // 物理地址
    pg.virt = (uint32_t*)phys_to_virt(pg.phys);  // 转换到内核虚拟地址
    memset(pg.virt, 0, PAGE_SIZE);
    return pg;
}
void copy_kernel_mappings_to_pd(uint32_t *pd_user) {
    // kernel occupies PDE indexes 768..1023 (0xC0000000 >> 22 == 768)
    for (int i = 768; i < 1024; ++i) {
        uint32_t entry = pd[i];
        // 如果原来 pd_kernel[i] 为 0，就跳过
        if ((entry & PTE_P) == 0) {
            pd_user[i] = 0;
            continue;
        }
        // 清除用户位（U/S），保持内核只在 supervisor 模式访问
        pd_user[i] = entry & ~PTE_U;
    }
}

#define PHYS_VIDEO 0xB8000
#define VIRT_USER_VIDEO 0xB8000

void task_prepare_pde(){

       page_t pde = alloc_page_table_();
       th_u->pde = (uint32_t*)pde.phys;   // CR3 必须用物理地址
       uint32_t *pd_user = pde.virt;      // 内核里可以直接访问 PDE 内容
       
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
       copy_kernel_mappings_to_pd(th_u->pde);

       // 映射到用户虚拟空间
       //map_page(th_u->pde, VIRT_USER_STACK_TOP - PAGE_SIZE, th_u->user_stack, PTE_P|PTE_W|PTE_U);

       // 用户栈顶虚拟地址（用户程序开始时 esp 指向这里）
       //th_u->tf->esp = VIRT_USER_STACK_TOP;

       //printf("[task_load] user stack mapped at 0x%x (PA=0x%x)\n",VIRT_USER_STACK_TOP - PAGE_SIZE, th_u->user_stack);
       //iframe=(struct trapframe *)(th_u->tf);
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

    tf->edi = 1;
    tf->esi = 2;
    tf->ebp = 3;
    tf->ebx = 5;
    tf->edx = 6;
    tf->ecx = 7;
    tf->eax = 8;

    tf->gs = 0;
    tf->ds = (SEG_UDATA << 3) | DPL_USER;
    tf->es = (SEG_UDATA << 3) | DPL_USER;
    tf->fs = (SEG_UDATA << 3) | DPL_USER;
    tf->ss = (SEG_UDATA << 3) | DPL_USER;
    tf->cs = (SEG_UCODE << 3) | DPL_USER;

    //tf->esp = VIRT_USER_STACK_TOP;
    //tf->eip = VIRT_MEM_BASE_USER;
    tf->eflags = FL_IF; // 开中断

    // 设置当前CPU的TSS
    struct cpu *c = &cpus[logical_cpu_id()];
    c->ts.ss0 = SEG_KDATA << 3;
    c->ts.esp0 = (uint32_t)th_u->kstack;     // 指向栈顶 (uint32_t)task->kstack + sizeof(task->kstack);
    ltr(SEG_TSS << 3);

    // 切换用户页表
    asm volatile ("movl %0, %%cr3" :: "r"(task->pde) : "memory");
    //outb(inb(0x21) | 0x01, 0x21);  // 置位 bit0 屏蔽 IRQ0
    // 切换堆栈到 trapframe，执行 iret 进入用户空间
    asm volatile (
        "cli\n\t"                // 禁中断
        "movl %0, %%esp\n\t"     // 切换栈指针到 trapframe
       "jmp interrupt_exit\n\t"
       :: "r"(tf) : "memory"
 
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

        newtask->tf = (struct trapframe *)((uint8_t*)newtask->kstack + 4096 - sizeof(struct trapframe));
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
