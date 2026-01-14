#include "types.h"
#include "param.h"
//#include "x86/mmu.h"
#include "x86/io.h"
#include "proc.h"
#include "interrupt.h"
#include "printf.h"
#include "page.h"

#include "sched.h"
#include "task.h"
#include "lapic.h"
#include "syscall.h"

extern void alltraps(void);
extern task_t* current_task[8];
// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint64_t vectors[];  // in vectors.S: array of 256 entry pointers
//struct spinlock tickslock;
uint32_t ticks;
void
trap(){}

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  //initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

#define KERNEL_DS (SEG_KDATA << 3)

// 通用中断入口函数
__attribute__((naked))
void alltraps2(void) {
    asm volatile(
        // --- 1. 保存段寄存器 ---
        "pushl %%ds\n\t"
        "pushl %%es\n\t"
        "pushl %%fs\n\t"
        "pushl %%gs\n\t"
          
        // --- 2. 保存通用寄存器 ---
         "pusha\n\t"        // edi, esi, ebp, esp(dummy), ebx, edx, ecx, eax

        // --- 3. 设置内核数据段选择子 ---
        "movl $0x10, %%eax\n\t"       // SEG_KDATA << 3 = 2 << 3 = 0x10
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%fs\n\t"
        "movw %%ax, %%gs\n\t" //

        // --- 4. 调用 C 层处理函数 ---
        "movl %%esp, %%eax\n\t"       // 参数 tf = 当前 esp (指向 trapframe 的 trapno 字段)
        "pushl %%eax\n\t"             // 参数 tf = 当前 esp
        "call do_irq_handler\n\t"
        //"addl $4, %%esp\n\t"          // 弹出参数

        // --- 5. 恢复寄存器 ---
        "popa\n\t"
        "popl %%gs\n\t"
        "popl %%fs\n\t"
        "popl %%es\n\t"
        "popl %%ds\n\t"

        // --- 6. 弹出 trapno 和 errcode (总是 8 字节) ---
        "addl $8, %%esp\n\t"

        // --- 7. 从中断返回 ---
        "iret\n\t"
        :
        :
        : "memory"
    );
}

// 外部声明当前进程（如需调度）
extern struct task_t *current;
extern int need_resched;

// 外部声明do_exit函数
extern void do_exit(int code);

// 处理除零错误 (trapno=0)
void handle_divide_error(struct trapframe *tf) {
    extern task_t *current_task[];

    task_t *task = current_task[logical_cpu_id()];
    if (!task) {
        // ⚠️ 暂时禁用 printf
        // printf("[DIVIDE ERROR] No current task!\n");
        // printf("[DIVIDE ERROR] EIP=0x%x, CS=0x%x\n", tf->eip, tf->cs);
        // 没有当前任务，无法恢复，停止系统
        __asm__ volatile("cli; hlt; jmp .");
        return;
    }

    // ⚠️ 暂时禁用 printf
    // printf("[DIVIDE ERROR] Task %d divided by zero at EIP=0x%x\n",
    //        task->pid, tf->eip);
    // printf("[DIVIDE ERROR] Terminating task...\n");

    // 终止出错的任务
    do_exit(-1);

    // do_exit 会设置 need_resched=1，调度器会在 interrupt_exit 中处理
    // 任务不会返回到这里继续执行
}
//void handle_page_fault(struct trapframe *tf){};
// 时钟中断计数器
static uint32_t timer_ticks = 0;
#define TIME_SLICE 10  // 每10个时钟中断触发一次调度

void handle_timer_interrupt(struct trapframe *tf){
    extern uint32_t ticks;
    ticks++;
    timer_ticks++;

    // 每个时间片检查是否需要调度
    if (timer_ticks >= TIME_SLICE) {
        timer_ticks = 0;

        // 设置需要重新调度标志
        extern int need_resched;
        need_resched = 1;

        // 注意：实际的调度会在中断返回后发生
        // 这样可以避免在中断处理函数中直接调度
    }
};
void handle_keyboard_interrupt(struct trapframe *tf){
    // ⚠️ 暂时禁用 printf
    // printf("enter keyboard interrupt---\n");
};
// ... 其他中断处理函数 ...

// 向8259A发送中断结束信号（EOI）
static void send_eoi(int irq) {
    if (irq >= 8) {
        outb(0xA0, 0x20);  // 从8259A
    }
    outb(0x20, 0x20);      // 主8259A
}
// 系统调用：处理用户进程主动阻塞
static int sys_block(struct trapframe *tf) {
    // 1. 检查当前进程合法性

    current=current_task[logical_cpu_id()];
    if (!current) return -1;

    // 2. 关中断，保护队列操作
    cli();

    // 3. 修改进程状态为阻塞态
    current->state = PS_BLOCKED;

    // 4. 将进程从就绪队列移除（关键：否则调度器仍会选中）
    llist_delete(&current->sched_node);

    // 5. 开中断并触发调度
    sti();
    need_resched = 1;
    schedule();  // 内核中调用调度器，切换到其他进程

    return 0;
}
// 读 CR2: Page Fault 时 CPU 会把出错的虚拟地址放在 CR2
static inline uint32_t readcr2(void) {
    uint32_t val;
    asm volatile("mov %%cr2, %0" : "=r"(val));
    return val;
}

// 刷新单页 TLB
static inline void flush_tlb_single(uint32_t vaddr) {
    asm volatile ("invlpg (%0)" :: "r"(vaddr) : "memory");
}

// COW (Copy-On-Write) 页错误处理
static int handle_cow_fault(uint32_t fault_va, uint32_t err) {
    extern task_t *current_task[];
    extern uint32_t pmm_alloc_page(void);

    // 必须是：USER | WRITE | PRESENT
    if ((err & 0x7) != 0x7)
        return 0;  // 不是 COW

    task_t *cur = current_task[logical_cpu_id()];
    if (!cur)
        return 0;

    // 获取当前页目录
    uint32_t *pd = (uint32_t *)phys_to_virt((uint32_t)cur->cr3);

    uint32_t pdi = fault_va >> 22;
    uint32_t pti = (fault_va >> 12) & 0x3FF;

    // 检查页目录项
    if (!(pd[pdi] & 0x1))
        return 0;  // 页表不存在

    // 获取页表
    uint32_t *pt = (uint32_t *)phys_to_virt(pd[pdi] & ~0xFFF);
    uint32_t pte = pt[pti];

    // 如果已经可写，那不是 COW
    if (!(pte & 0x1) || (pte & 0x2))
        return 0;

    // ===============================
    // 真正的 COW 处理开始
    // ===============================

    // ⚠️ 暂时禁用 printf
    // printf("[COW] Fault at 0x%x, allocating new page...\n", fault_va);

    uint32_t old_phys = pte & ~0xFFF;

    // 分配新物理页
    uint32_t new_phys = pmm_alloc_page();
    if (!new_phys) {
        // ⚠️ 暂时禁用 printf
        // printf("[COW] Out of memory!\n");
        return 0;
    }

    // 拷贝原页面内容
    memcpy(phys_to_virt(new_phys), phys_to_virt(old_phys), PGSIZE);

    // 更新 PTE：指向新页 + 可写
    pt[pti] = new_phys | 0x7;  // PRESENT | WRITABLE | USER

    // 刷新单页 TLB
    flush_tlb_single(fault_va);

    // ⚠️ 暂时禁用 printf
    // printf("[COW] Page copied: 0x%x -> 0x%x\n", old_phys, new_phys);
    return 1;  // COW 处理成功
}

void handle_page_fault(struct trapframe *tf) {
    uint32_t fault_va = readcr2();
    uint32_t err = tf->err;

    // 尝试 COW 处理
    if (handle_cow_fault(fault_va, err)) {
        // COW 处理成功，直接返回用户态继续执行
        return;
    }

    // 不是 COW，按普通页错误处理
    // ⚠️⚠️⚠️ 暂时禁用所有 printf！
    // printf("\n[Page Fault] cr2 = 0x%x\n", fault_va);
    // printf("  err = 0x%x  eip = 0x%x  esp = 0x%x\n",
    //       err, tf->eip, tf->esp);
    // ... (所有其他 printf)

    //panic("Page Fault!\n");
}

void handle_page_fault_(struct trapframe *tf) {
    uint32_t fault_va = readcr2();

    // ⚠️⚠️⚠️ 暂时禁用所有 printf！
    // printf("\n[Page Fault] cr2 = 0x%x\n", fault_va);
    // ... (所有其他 printf)

    /*// 这里你可以根据 err 分析是读/写，用户/内核 等错误
    if ((tf->err & 1) == 0)
        printf("  -> caused by non-present page\n");
    if (tf->err & 2)
        printf("  -> caused by write\n");
    else
        printf("  -> caused by read\n"); */
    // if (tf->err & 4)
    //     printf("  -> caused in user mode\n");
    // else
    //     printf("  -> caused in kernel mode\n");

    // printf("Page Fault!\n");
}

// 打印原始栈内容用于调试
void print_raw_stack(uint32_t *esp) {
    // ⚠️⚠️⚠️ 暂时禁用所有 printf！
    // printf("[RAW] ESP=%p, dumping first 20 dwords:\n", esp);
    // for (int i = 0; i < 20; i++) {
    //     printf("[RAW] esp+%d (0x%x): 0x%x", i*4, i*4, esp[i]);
    //     ... (所有其他 printf)
    // }
    // printf("[RAW] End of dump\n");
}

// 中断处理主函数
void do_irq_handler(struct trapframe *tf) {

    // ⚠️⚠️⚠️ 暂时禁用所有 printf 调试！
    // 原因：printf 会使用 ES 寄存器访问字符串，破坏栈上保存的 ES 值
    // 导致后续恢复时使用错误的 ES 值，造成系统崩溃

    // printf("\n========== IRQ ENTRY ==========\n");
    // printf("[IRQ] tf=%p\n", tf);

    // // ⚠️ 打印原始栈内容来验证结构体布局
    // print_raw_stack((uint32_t*)tf);

    // printf("[IRQ] trapno=%d, err=%d, eip=0x%x, cs=0x%x\n", tf->trapno, tf->err, tf->eip, tf->cs);
    // printf("[IRQ] eax=0x%x, ebx=0x%x, ecx=0x%x, edx=0x%x\n", tf->eax, tf->ebx, tf->ecx, tf->edx);
    // printf("[IRQ] esi=0x%x, edi=0x%x, ebp=0x%x, esp=0x%x\n", tf->esi, tf->edi, tf->ebp, tf->esp);
    // printf("[IRQ] ds=0x%x, es=0x%x, fs=0x%x, gs=0x%x\n", tf->ds, tf->es, tf->fs, tf->gs);
    // printf("[IRQ] eflags=0x%x, user_esp=0x%x, user_ss=0x%x\n", tf->eflags, tf->esp, tf->ss);
    // printf("==============================\n\n");


    if(tf->trapno == T_SYSCALL){
        // ⚠️⚠️⚠️ 暂时禁用 syscall 调试打印！
        // 原因：printf 会使用 ES 寄存器访问字符串，破坏栈上保存的 ES 值
        // printf("[syscall] num=%d (eax=%d, ebx=%x, ecx=%x, edx=%x)\n",
        //        tf->eax, tf->eax, tf->ebx, tf->ecx, tf->edx);
        // printf("[syscall] eip=0x%x, esp=0x%x, cs=0x%x, ds/es/fs/gs=0x%x/0x%x/0x%x/0x%x\n",
        //        tf->eip, tf->esp, tf->cs, tf->ds, tf->es, tf->fs, tf->gs);
        syscall_dispatch(tf);
        return;
     }

    // 2. 根据中断号处理不同类型的中断
    switch (tf->trapno) {
        case 0:  // 除法错误
            handle_divide_error(tf);
            break;
        case 5:  // BOUND异常 - 暂时不处理，直接终止任务
            // ⚠️ BOUND异常可能是伪装的页错误，直接终止
            {
                extern task_t* current_task[];
                extern uint8_t logical_cpu_id(void);
                task_t* cur = current_task[logical_cpu_id()];
                if (cur && cur->user_stack != 0) {
                    // 用户任务，终止它
                    extern void do_exit(int);
                    do_exit(-1);
                } else {
                    // 内核任务，不应该发生
                    printf("[BOUND] Kernel task BOUND exception, halting\n");
                    while(1) __asm__ volatile("hlt");
                }
            }
            break;
        case 14: // 页错误
            // 页错误的err包含详细信息（如读写、存在位等）
            handle_page_fault(tf);
            break;
        case 32: // 时钟中断（IRQ0）
            handle_timer_interrupt(tf);
            // 时钟中断只设置 need_resched 标志
            // 实际调度由 interrupt_exit 在返回用户态前执行
            send_eoi(0);  // 发送EOI
            break;
       case T_IRQ0 + IRQ_SYS_BLOCK:

            
            sys_block(tf);
            lapiceoi();
            break;
        case 33: // 键盘中断（IRQ1）
            // 调用键盘驱动处理程序
            extern void keyboard_handler(void);
            keyboard_handler();
            send_eoi(1);  // 发送EOI
            break; 
        // ... 其他中断类型 ...
        case 13: { // GP Fault - 暂时禁用所有 printf
            extern uint32_t readcr2(void);
            uint32_t cr2 = readcr2();

            // ⚠️⚠️⚠️ 暂时禁用 GP Fault 调试打印！
            // printf("\n=== GP Fault ===\n");
            // printf("EIP=0x%x CS=0x%x\n", tf->eip, tf->cs);
            // printf("EAX=0x%x EBX=0x%x ECX=0x%x EDX=0x%x\n", tf->eax, tf->ebx, tf->ecx, tf->edx);
            // printf("ESI=0x%x EDI=0x%x EBP=0x%x ESP=0x%x\n", tf->esi, tf->edi, tf->ebp, tf->esp);
            // printf("DS=0x%x ES=0x%x FS=0x%x GS=0x%x\n", tf->ds, tf->es, tf->fs, tf->gs);
            // printf("EFLAGS=0x%x\n", tf->eflags);
            // printf("trapno=%d err=0x%x\n", tf->trapno, tf->err);
            // printf("CR2=0x%x (虽然GP Fault通常不用CR2,但打印出来看看)\n", cr2);
            // ... (所有其他 printf)

            // 停止系统,避免无限循环
            while(1) {
                __asm__ volatile("hlt");
            }
            break;
        }
        default:
            // ⚠️⚠️⚠️ 暂时禁用 printf
            // printf("Unhandled interrupt: trapno=%d\n", tf->trapno);
            // 外部中断需要发送EOI，避免阻塞
            if (tf->trapno >= 32 && tf->trapno <= 47) {
                send_eoi(tf->trapno - 32);
            }
            break;
    }
    
}


/*
//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpu_id() == 0){
      //acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      //release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    printf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;
  

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      printf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      //panic("trap");
    }
    // In user space, assume process misbehaved.
    printf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1; 
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}

*/

// ================================
// 调试函数：用于 interrupt_exit
// ================================

extern void debug_print_interrupt_exit_entry(uint32_t esp) {
    printf("[interrupt_exit] ENTRY: ESP=0x%x\n", esp);
    printf("[interrupt_exit] Dumping trapframe at ESP:\n");

    // 读取 trapframe 的关键字段
    uint32_t *tf = (uint32_t *)esp;
    printf("  edi=0x%x, esi=0x%x, ebp=0x%x\n", tf[0], tf[1], tf[2]);
    printf("  ebx=0x%x, edx=0x%x, ecx=0x%x, eax=0x%x\n", tf[4], tf[5], tf[6], tf[7]);
    printf("  gs=0x%x, fs=0x%x, es=0x%x, ds=0x%x\n", tf[8], tf[9], tf[10], tf[11]);
    printf("  trapno=%d, err=%d\n", tf[13], tf[12]);
    printf("  eip=0x%x, cs=0x%x, eflags=0x%x\n", tf[14], tf[15], tf[16]);
    printf("  esp=0x%x, ss=0x%x\n", tf[17], tf[18]);
}

extern void debug_print_before_schedule(uint32_t esp) {
    printf("[interrupt_exit] Before call schedule: ESP=0x%x\n", esp);
    printf("[interrupt_exit] need_resched=%d\n", need_resched);
}

extern void debug_print_after_schedule(uint32_t esp) {
    printf("[interrupt_exit] After schedule returned: ESP=0x%x\n", esp);
    printf("[interrupt_exit] ⚠️ WARNING: Stack may have changed!\n");

    // 读取新的 trapframe
    uint32_t *tf = (uint32_t *)esp;
    printf("  New trapframe: edi=0x%x, esi=0x%x, ebp=0x%x\n", tf[0], tf[1], tf[2]);
    printf("  eip=0x%x, cs=0x%x, eflags=0x%x\n", tf[14], tf[15], tf[16]);
}

extern void debug_print_before_restore_regs(uint32_t esp) {
    printf("[interrupt_exit] Before restore regs: ESP=0x%x\n", esp);
}

extern void debug_print_after_restore_regs(uint32_t esp) {
    printf("[interrupt_exit] After restore regs: ESP=0x%x\n", esp);
    printf("[interrupt_exit] About to check CS and iret...\n");

    // 读取 CS (ESP+4)
    uint32_t *eip_ptr = (uint32_t *)esp;
    uint32_t cs = *(uint32_t *)((uint8_t *)eip_ptr + 4);
    printf("  CS at ESP+4: 0x%x (RPL=%d)\n", cs, cs & 3);
}

// ================================
// 调试函数：用于 trap_entry
// ================================

extern void debug_print_trap_entry(uint32_t marker) {
    if (marker == 0xDEAD0001) {
        printf("[trap_entry] ========== IRQ ENTRY (USER MODE) ==========\n");
    } else if (marker == 0xDEAD0000) {
        printf("[trap_entry] ========== IRQ ENTRY (KERNEL MODE) ==========\n");
    } else {
        printf("[trap_entry] ========== IRQ ENTRY (UNKNOWN MARKER: 0x%x) ==========\n", marker);
    }
}
