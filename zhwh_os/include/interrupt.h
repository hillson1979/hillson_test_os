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
// ⚠️ 关键修复：必须与 xv6 的 trapframe 布局完全匹配！
//    trap_entry.S 的 alltraps 压栈顺序：DS→ES→FS→GS → pusha
//    在栈上：DS/ES/FS/GS 在低地址，pusha(EDI..EAX) 在高地址
//    但是！ESP指向pusha的起始位置（edi），不是DS！
//    正确的栈布局（从ESP往高地址）：
//      [0-31]   edi..eax (pusha)
//      [32-47]  gs..ds (段寄存器，注意：GS在低地址，DS在高地址！)
//      [48-51]  err (vectors.S 先压)
//      [52-55]  trapno (vectors.S 后压)
//      [56-59]  eip
//      [60-63]  cs
//      [64-67]  eflags
//      [68-71]  esp (仅用户态)
//      [72-75]  ss (仅用户态)
struct trapframe {
  // ⚠️ pusha 压入的通用寄存器在低地址（ESP指向这里）
  // pusha 压入顺序：EDI, ESI, EBP, ESP(dummy), EBX, EDX, ECX, EAX
  uint32_t edi;     // offset 0
  uint32_t esi;     // offset 4
  uint32_t ebp;     // offset 8
  uint32_t oesp;    // offset 12 (pusha压入的原始ESP，无用)
  uint32_t ebx;     // offset 16
  uint32_t edx;     // offset 20
  uint32_t ecx;     // offset 24
  uint32_t eax;     // offset 28

  // ⚠️ alltraps 压入的段寄存器（在pusha之后）
  // 压栈顺序（trap_entry.S）: DS → ES → FS → GS
  // 栈向下生长，先压入的在低地址，后压入的在高地址
  // 实际内存布局：DS=32, ES=36, FS=40, GS=44
  uint32_t ds;      // offset 32 (alltraps 先压入)
  uint32_t es;      // offset 36
  uint32_t fs;      // offset 40
  uint32_t gs;      // offset 44 (alltraps 最后压入)

  // vectors.S 压入的值 (8字节)
  // ⚠️ vectors.S 的压栈顺序：
  //    pushl $0       # errcode (第一条push)
  //    pushl $128     # trapno (第二条push)
  //    栈向下生长，所以 trapno 在 offset 48，errcode 在 offset 52
  uint32_t trapno;   // trap number (vectors.S 后 push, offset 48，低地址)
  uint32_t err;      // error code (vectors.S 先 push, offset 52，高地址)

  // CPU 硬件压入的值 (12字节)
  uint32_t eip;      // offset 56
  uint32_t cs;       // offset 60
  uint32_t eflags;   // offset 64

  // 仅在特权级改变时压入 (8字节)
  // ⚠️ 内核态异常时 CPU 不会压入这两项!
  uint32_t esp;      // 用户态 ESP (offset 68)
  uint32_t ss;       // 用户态 SS (offset 72)
} __attribute__((packed));

void
tvinit(void);

extern void alltraps(void);


