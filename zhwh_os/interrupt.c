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

  for(i = 0; i < 256; i++){
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
    if(i==36){
        printf("[tvinit] IRQ36 gate: offset=0x%x, seg=0x%x, type=%d, dpl=%d, p=%d\n",
               (uint32_t)vectors[36], SEG_KCODE<<3,idt[36].type, idt[36].dpl, idt[36].p);
        sti();
    }
  }
  // 

  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  // 调试：打印系统调用门的设置
  printf("[tvinit] System call gate (IDT[%d]):\n", T_SYSCALL);
  printf("  offset=0x%x, seg=0x%x, type=%d, dpl=%d, present=%d\n",
         (uint32_t)vectors[T_SYSCALL], SEG_KCODE<<3,
         idt[T_SYSCALL].type, idt[T_SYSCALL].dpl, idt[T_SYSCALL].p);
  printf("  vector128 address=0x%p\n", vectors[T_SYSCALL]);

  // 🔥 诊断：打印关键 IDT 项（Trap 13, Trap 19, IRQ 0）
  printf("[tvinit] Trap 13 (GP Fault): offset=0x%x, seg=0x%x, type=%d, dpl=%d, p=%d\n",
         (uint32_t)vectors[13], SEG_KCODE<<3, idt[13].type, idt[13].dpl, idt[13].p);
  printf("[tvinit] Trap 19 (SIMD): offset=0x%x, seg=0x%x, type=%d, dpl=%d, p=%d\n",
         (uint32_t)vectors[19], SEG_KCODE<<3, idt[19].type, idt[19].dpl, idt[19].p);
  printf("[tvinit] IRQ 0 (Timer): offset=0x%x, seg=0x%x, type=%d, dpl=%d, p=%d\n",
         (uint32_t)vectors[32], SEG_KCODE<<3, idt[32].type, idt[32].dpl, idt[32].p);

  //initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

#define KERNEL_DS (SEG_KDATA << 3)

// 外部声明当前进程（如需调度）
extern struct task_t *current;
extern int need_resched;

// 外部声明do_exit函数
extern void do_exit(int code);

// 处理除零错误 (trapno=0)
void handle_divide_error(struct trapframe *tf) {
    extern task_t *current_task[];

    // 🔥 调试：打印 tf 指针和原始栈内容
    uint32_t tf_ptr = (uint32_t)tf;
    uint32_t *raw_stack = (uint32_t *)tf_ptr;

    printf("\n========== DIVIDE ERROR (Kernel Mode) ==========\n");
    printf("  tf pointer = 0x%x\n", tf_ptr);
    printf("  Raw stack[0-4] = 0x%x 0x%x 0x%x 0x%x 0x%x\n",
           raw_stack[0], raw_stack[1], raw_stack[2], raw_stack[3], raw_stack[4]);
    printf("  Raw stack[5-9] = 0x%x 0x%x 0x%x 0x%x 0x%x\n",
           raw_stack[5], raw_stack[6], raw_stack[7], raw_stack[8], raw_stack[9]);
    printf("  Raw stack[10-14] = 0x%x 0x%x 0x%x 0x%x 0x%x\n",
           raw_stack[10], raw_stack[11], raw_stack[12], raw_stack[13], raw_stack[14]);
    printf("==================================================\n");

    task_t *task = current_task[logical_cpu_id()];
    if (!task) {
        // 🔥 内核初始化阶段的除零错误 - 打印详细信息并跳过
        printf("  No current task - this is a kernel divide error!\n");
        printf("  EIP=0x%x, CS=0x%x, EFLAGS=0x%x\n", tf->eip, tf->cs, tf->eflags);
        printf("  EAX=0x%x, EBX=0x%x, ECX=0x%x, EDX=0x%x\n", tf->eax, tf->ebx, tf->ecx, tf->edx);
        printf("  ESI=0x%x, EDI=0x%x, EBP=0x%x, ESP=0x%x\n", tf->esi, tf->edi, tf->ebp, tf->esp);
        printf("  DS=0x%x, ES=0x%x, FS=0x%x, GS=0x%x\n", tf->ds, tf->es, tf->fs, tf->gs);
        printf("==================================================\n");

        // 🔥 尝试跳过触发除零错误的指令
        // 大多数除零指令是 2-3 字节，我们跳过 3 字节
        tf->eip += 3;
        printf("[DIVIDE ERROR] Skipped 3 bytes, continuing at EIP=0x%x\n", tf->eip);
        return;
    }

    // 有任务的除零错误 - 终止任务
    printf("[DIVIDE ERROR] Task %d divided by zero at EIP=0x%x\n",
           task->pid, tf->eip);
    printf("[DIVIDE ERROR] Terminating task...\n");

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
    // 调用键盘驱动的中断处理函数
    extern void keyboard_handler(void);
    keyboard_handler();
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

// ⚠️⚠️⚠️ 关键修复：在中断返回前检查 need_resched 标志
//      用于实现 syscall_yield() 的调度功能
void check_and_schedule(struct trapframe *tf) {
    extern int need_resched;

    // 检查是否需要调度
    if (need_resched) {
        // 清除标志
        need_resched = 0;

        // 只在用户态中断时调度（检查段选择子的 RPL 位）
        if ((tf->cs & 3) == 3) {
            // 用户态中断：调用调度器
            extern void schedule(void);
            schedule();
        }
    }
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
    // ⚠️ CR3 的低 12 位是标志位，需要清除才能得到物理地址
    uint32_t *pd = (uint32_t *)phys_to_virt((uint32_t)cur->cr3 & ~0xFFF);

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

    // 🔍 诊断输出：打印页面错误地址
    extern void printf(const char* fmt, ...);
    printf("[PF] fault_addr=0x%x err=0x%x eip=0x%x\n",
           fault_va, err, tf->eip);

    // 尝试 COW 处理
    if (handle_cow_fault(fault_va, err)) {
        // COW 处理成功，直接返回用户态继续执行
        return;
    }

    // 不是 COW，按普通页错误处理
    // 🔧 修复：未处理的页面错误应该终止任务
    extern void do_exit(int);

    // ⚠️ 关键修复：正确判断是用户态还是内核态页错误
    // 不能使用 user_stack 判断，应该检查 CS 的 RPL 位
    bool is_user_mode = (tf->cs & 3) == 3;

    if (is_user_mode) {
        // 用户任务触发页面错误，可能是程序错误或内存不足
        printf("[PF] User task page fault, terminating\n");
        // 终止任务并返回错误码
        do_exit(-1);  // 使用 -1 表示异常退出
    } else {
        // 内核任务触发页面错误，这是严重的内核 bug
        printf("[PF] Kernel page fault, halting\n");
        printf("[PF] This is a KERNEL BUG - fault in kernel mode!\n");
        printf("[PF] fault_addr=0x%x, eip=0x%x, cs=0x%x\n", fault_va, tf->eip, tf->cs);
        // 停止系统
        __asm__ volatile("cli; hlt; jmp .");
    }
}

void handle_page_fault_(struct trapframe *tf) {
    uint32_t fault_va = readcr2();

    // 🔥 启用页错误调试信息
    printf("\n[Page Fault] cr2 = 0x%x\n", fault_va);
    printf("  EIP=0x%x, ESP=0x%x\n", tf->eip, tf->esp);

    // 这里你可以根据 err 分析是读/写，用户/内核 等错误
    if ((tf->err & 1) == 0)
        printf("  -> caused by non-present page\n");
    if (tf->err & 2)
        printf("  -> caused by write\n");
    else
        printf("  -> caused by read\n");
    if (tf->err & 4)
        printf("  -> caused in user mode\n");
    else
        printf("  -> caused in kernel mode\n");

    printf("Page Fault!\n");
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
// VGA 颜色函数
extern void vga_setcolor(uint8_t fg, uint8_t bg);
#define SET_COLOR_RED()     vga_setcolor(4, 0)   // 红字黑底
// 中断处理主函数
void do_irq_handler(struct trapframe *tf) {
    // 🔥🔥 详细寄存器打印（用于诊断 Trap 19/13 问题）
    // ⚠️⚠️⚠️ 禁用 printf，避免在处理 Trap 19 时再次触发 Trap 19
    if(tf->trapno == 19 || tf->trapno == 13) {
        // 直接使用串口输出，不调用 printf
        // printf("\n========== TRAP %d DEBUG ==========\n", tf->trapno);
        // printf("  EIP=0x%x, CS=0x%x, EFLAGS=0x%x\n", tf->eip, tf->cs, tf->eflags);
        // printf("  ERR=0x%x, ESP=0x%x\n", tf->err, tf->esp);
        // printf("  EAX=0x%x, EBX=0x%x, ECX=0x%x, EDX=0x%x\n", tf->eax, tf->ebx, tf->ecx, tf->edx);
        // printf("  ESI=0x%x, EDI=0x%x, EBP=0x%x\n", tf->esi, tf->edi, tf->ebp);
        // printf("  DS=0x%x, ES=0x%x, FS=0x%x, GS=0x%x\n", tf->ds, tf->es, tf->fs, tf->gs);
        // printf("====================================\n");

        // 🔥 暂时跳过 Trap 19，防止无限递归
        if(tf->trapno == 19) {
            // Trap 19 (SIMD) - 可能是 FPU/SSE 问题
            // 跳过触发异常的指令，继续执行
            tf->eip += 3;  // 跳过 3 字节指令
            return;
        }
    }
    else if(tf->trapno ==32 || tf->trapno ==33 || tf->trapno ==128){
        //
    }
    else{
        //SET_COLOR_RED();
        printf("[IRQ] tf->trapno=%d\n", tf->trapno);
    }
    
    // ⚠️⚠️⚠️ 暂时禁用所有 printf 调试！
    // 原因：printf 会使用 ES 寄存器访问字符串，破坏栈上保存的 ES 值
    // 导致后续恢复时使用错误的 ES 值，造成系统崩溃

    // printf("\n========== IRQ ENTRY ==========\n");
    

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
            // ⚠️ 移除所有printf调试,避免printf中的除法导致二次异常
            handle_divide_error(tf);
            break;
        case 8:  // 🔥 双重故障（Double Fault）- 这是系统重启的主要原因！
            {
                // 🔥 添加调试信息，在重启前显示
                printf("\n\n🔥🔥🔥 DOUBLE FAULT - System will restart! 🔥🔥🔥\n");
                printf("  EIP=0x%x, ESP=0x%x\n", tf->eip, tf->esp);
                printf("  Error code=0x%x\n", tf->err != 0xDEADBEEF ? tf->err : 0);

                // 尝试显示栈信息
                printf("  Stack dump:\n");
                uint32_t *stack = (uint32_t*)tf->esp;
                for (int i = 0; i < 8; i++) {
                    printf("    [esp+%d] = 0x%x\n", i*4, stack[i]);
                }

                // 检查内核栈哨兵
                extern uint32_t stack_base;
                if (*(uint32_t*)&stack_base != 0xDEADBEEF) {
                    printf("  ⚠️ Stack sentinel CORRUPTED! Stack overflow detected!\n");
                } else {
                    printf("  ✓ Stack sentinel OK\n");
                }

                printf("  Halting...\n");
                asm volatile("cli; hlt");
                break;
            }
        case 5:  // BOUND异常 - 暂时不处理，直接终止任务
            // ⚠️ BOUND异常可能是伪装的页错误，直接终止
            {
                printf("[BOUND] BOUND exception at EIP=0x%x\n", tf->eip);
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
        case 33: { // 键盘中断（IRQ1）
            // 调用键盘驱动处理程序
            extern void keyboard_handler(void);
            keyboard_handler();
            // 使用 lapiceoi() 而不是 send_eoi()
            // 因为这个系统使用的是 IOAPIC，不是 8259A PIC
            lapiceoi();
            break;
        }
        // 🔥 WiFi 卡中断处理（IRQ 16-23，常见 PCI 设备 IRQ 范围）
        case T_IRQ0 + 16:  // 48
        case T_IRQ0 + 17:  // 49
        case T_IRQ0 + 18:  // 50
        case T_IRQ0 + 19:  // 51
        case T_IRQ0 + 20:  // 52
        case T_IRQ0 + 21:  // 53
        case T_IRQ0 + 22:  // 54
        case T_IRQ0 + 23:  // 55
        {
            // 🔥 调试：打印 WiFi 中断到达
            printf("[IRQ] WiFi interrupt received! trapno=%d (IRQ%d)\n",
                   tf->trapno, tf->trapno - T_IRQ0);

            // 调用 WiFi 驱动中断处理程序
            extern void atheros_interrupt_handler(void);
            atheros_interrupt_handler();
            // 使用 lapiceoi() 发送 EOI
            lapiceoi();
            break;
        }

        // 🔥 E1000 网卡中断（常见 IRQ: 5, 9, 10, 11）
        
        case 36:  //  -> trapno 36
        case 43:
        {
            
            //printf(">>> got vector 36 from LAPIC!\n");
            extern void e1000_isr(void);
            e1000_isr();
            lapiceoi();
            break;
        }

        // ... 其他中断类型 ...
        case T_SIMDERR: // 19 - SIMD Floating-Point Exception
        case 16: { // x87 FPU Error
            // 🔥 完全静默处理 - 不打印任何信息
            __asm__ volatile("fnclex");
            //tf->eip += 12;  // 跳过 lea (7) + call (5)
            // 不调用 printf，避免递归异常
            break;
        }
        case 13: { // GP Fault - 打印调试信息
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
            // 捕获所有未处理的异常
            printf("[TRAP] Unhandled trap: trapno=%d, eip=0x%x, err=0x%x\n",
                   tf->trapno, tf->eip, tf->err);
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
    // 🔥🔥 暂时禁用定时器中断，避免除零错误循环
    printf("[TIMER] Timer interrupt received, returning immediately\n");
    lapiceoi();
    return;
    // if(cpu_id() == 0){
    //   //acquire(&tickslock);
    //   ticks++;
    //   wakeup(&ticks);
    //   //release(&tickslock);
    // }
    // lapiceoi();
    // break;
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
