// x86 trap and interrupt constants.
// ⚠️ 这些偏移必须与 struct trapframe 的布局完全匹配！
// ⚠️ 关键修复：压栈顺序是 DS→ES→FS→GS→pusha→trapno→err→eip→cs→eflags→esp→ss
// ⚠️ ESP指向trapframe的起始位置(DS)，不是pusha的起始位置！
#define TRAPFRAME_DS 0       // ds 偏移 (alltraps先压入，低地址)
#define TRAPFRAME_ES 4       // es 偏移
#define TRAPFRAME_FS 8       // fs 偏移
#define TRAPFRAME_GS 12      // gs 偏移 (alltraps后压入)
#define TRAPFRAME_EAX 16     // eax 偏移 (pusha先压)
#define TRAPFRAME_ECX 20     // ecx 偏移
#define TRAPFRAME_EDX 24     // edx 偏移
#define TRAPFRAME_EBX 28     // ebx 偏移
#define TRAPFRAME_OESP 32    // oesp 偏移 (pusha压入的dummy ESP)
#define TRAPFRAME_EBP 36     // ebp 偏移
#define TRAPFRAME_ESI 40     // esi 偏移
#define TRAPFRAME_EDI 44     // edi 偏移 (pusha后压)
#define TRAPFRAME_TRAPNO 48  // trapno 偏移
#define TRAPFRAME_ERR 52     // err 偏移
#define TRAPFRAME_EIP 56     // eip 偏移
#define TRAPFRAME_CS 60      // cs 偏移
#define TRAPFRAME_EFLAGS 64  // eflags 偏移
#define TRAPFRAME_ESP 68     // esp 偏移 (仅用户态)
#define TRAPFRAME_SS 72      // ss 偏移 (仅用户态)
#define TRAPFRAME_PDE 544    // th_u->pde 偏移

// Processor-defined:
#define T_DIVIDE         0      // divide error
#define T_DEBUG          1      // debug exception
#define T_NMI            2      // non-maskable interrupt
#define T_BRKPT          3      // breakpoint
#define T_OFLOW          4      // overflow
#define T_BOUND          5      // bounds check
#define T_ILLOP          6      // illegal opcode
#define T_DEVICE         7      // device not available
#define T_DBLFLT         8      // double fault
// #define T_COPROC      9      // reserved (not used since 486)
#define T_TSS           10      // invalid task switch segment
#define T_SEGNP         11      // segment not present
#define T_STACK         12      // stack exception
#define T_GPFLT         13      // general protection fault
#define T_PGFLT         14      // page fault
// #define T_RES        15      // reserved
#define T_FPERR         16      // floating point error
#define T_ALIGN         17      // aligment check
#define T_MCHK          18      // machine check
#define T_SIMDERR       19      // SIMD floating point error

// These are arbitrarily chosen, but with care not to overlap
// processor defined exceptions or interrupt vectors.
#define T_SYSCALL       128 //64      // system call
#define T_DEFAULT      500      // catchall

#define T_IRQ0          32      // IRQ 0 corresponds to int T_IRQ

#define IRQ_TIMER        0
#define IRQ_KBD          1
#define IRQ_COM1         4
#define IRQ_IDE         14
#define IRQ_ERROR       19

#define IRQ_SYS_BLOCK   123 // SYS_block=20
#define IRQ_SPURIOUS    31


//  trapframe 定义
struct trapframe {
    // 段寄存器
    uint32_t ds;
    uint32_t es;
    uint32_t fs;
    uint32_t gs;

    // pusha 顺序（必须这样）
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t oesp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;

    // vector.S 压入
    uint32_t trapno;
    uint32_t err;

    // CPU 自动压入
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;
    uint32_t ss;
} __attribute__((packed));

struct trapframe_bak {
    uint32_t ds;
    uint32_t es;
    uint32_t fs;
    uint32_t gs;

    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t oesp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t trapno;
    uint32_t err;

    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;
    uint32_t ss;
}__attribute__((packed));
void
tvinit(void);

extern void alltraps(void);


