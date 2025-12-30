#include "types.h"
#include "param.h"
//#include "x86/mmu.h"
#include "x86/io.h"
#include "proc.h"
#include "interrupt.h"
#include "printf.h"

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

// 声明中断处理函数
void handle_divide_error(struct trapframe *tf){};
//void handle_page_fault(struct trapframe *tf){};
void handle_timer_interrupt(struct trapframe *tf){
    // 简单的时钟中断处理 - 只增加计数器
    extern uint32_t ticks;
    ticks++;
};
void handle_keyboard_interrupt(struct trapframe *tf){
printf("enter keyboard interrupt---\n");

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

void handle_page_fault(struct trapframe *tf) {
    uint32_t fault_va = readcr2();
    uint32_t err = tf->err;

    printf("\n[Page Fault] cr2 = 0x%x\n", fault_va);
    printf("  err = 0x%x  eip = 0x%x  esp = 0x%x\n",
          err, tf->eip, tf->esp);

    // 解析错误码
    if ((err & 1) == 0)
        printf("  -> caused by NON-PRESENT page ()\n");
    else
        printf("  -> caused by PAGE-PROTECTION violation ()\n");

    if (err & 2)
        printf("  -> access type: WRITE\n");
    else
        printf("  -> access type: READ\n");

    if (err & 4)
        printf("  -> in USER mode\n");
    else
        printf("  -> in KERNEL mode\n");

    if (err & 8)
        printf("  -> reserved bit violation\n");

    if (err & 16)
        printf("  -> instruction fetch (maybe NX forbit exe)\n");

    //panic("Page Fault!\n");
}

void handle_page_fault_(struct trapframe *tf) {
    uint32_t fault_va = readcr2();

    //printf("\n[Page Fault] cr2 = 0x%x\n", fault_va);
    //printf("err = 0x%x  eip = 0x%x\n",   tf->err, tf->eip);
    //printf("err = 0x%x  esp = 0x%x\n",   tf->err, tf->esp);

    /*// 这里你可以根据 err 分析是读/写，用户/内核 等错误
    if ((tf->err & 1) == 0)
        printf("  -> caused by non-present page\n");
    if (tf->err & 2)
        printf("  -> caused by write\n");
    else
        printf("  -> caused by read\n"); */
    if (tf->err & 4)
        printf("  -> caused in user mode\n");
    else
        printf("  -> caused in kernel mode\n");

    printf("Page Fault!\n");
}

// 中断处理主函数
void do_irq_handler(struct trapframe *tf) {

    // 调试:打印地址和值
    // printf("[IRQ] tf=%p, &tf->eax=%p, tf->eax=%d\n", tf, &tf->eax, tf->eax);
    // printf("  &tf->ecx=%p, tf->ecx=%d\n", &tf->ecx, tf->ecx);
    // printf("  &tf->trapno=%p, tf->trapno=%d\n", &tf->trapno, tf->trapno);

    // 1. 调试信息：打印中断基本信息
    // printf("[IRQ] trapno=%d (0x%x), err=%d, eip=0x%x, cs=0x%x\n",tf->trapno, tf->trapno, tf->err, tf->eip, tf->cs);

    if(tf->trapno == T_SYSCALL){
        // 额外调试:直接打印EAX的值
        // printf("[IRQ T_SYSCALL] eax=%d\n", tf->eax);
        syscall_dispatch(tf);
        return;
     }

    // 2. 根据中断号处理不同类型的中断
    switch (tf->trapno) {
        case 0:  // 除法错误
            handle_divide_error(tf);
            break;
        case 14: // 页错误
            // 页错误的err包含详细信息（如读写、存在位等）
            handle_page_fault(tf);
            break;
        case 32: // 时钟中断（IRQ0）
            handle_timer_interrupt(tf);
            // 时钟中断中检查是否需要调度
            if (current && need_resched) {
                //schedule();  // 调用进程调度器
            }
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
        case 13: // GP Fault - 打印详细信息以便调试
            printf("\n=== GP Fault ===\n");
            printf("EIP=0x%x CS=0x%x\n", tf->eip, tf->cs);
            printf("EAX=0x%x EBX=0x%x ECX=0x%x EDX=0x%x\n", tf->eax, tf->ebx, tf->ecx, tf->edx);
            printf("ESI=0x%x EDI=0x%x EBP=0x%x ESP=0x%x\n", tf->esi, tf->edi, tf->ebp, tf->esp);
            printf("DS=0x%x ES=0x%x FS=0x%x GS=0x%x\n", tf->ds, tf->es, tf->fs, tf->gs);
            printf("EFLAGS=0x%x\n", tf->eflags);
            printf("trapno=%d err=%d\n", tf->trapno, tf->err);
            printf("==================\n");
            // 停止系统,避免无限循环
            while(1) {
                __asm__ volatile("hlt");
            }
            break;
        default:
            printf("Unhandled interrupt: trapno=%d\n", tf->trapno);
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

