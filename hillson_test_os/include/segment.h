void
seginit(void);

// 全局描述符
typedef struct descriptor_t /* 共 8 个字节 */
{
    unsigned short limit_low;      // 段界限 0 ~ 15 位
    unsigned int base_low : 24;    // 基地址 0 ~ 23 位 16M
    unsigned char type : 4;        // 段类型
    unsigned char segment : 1;     // 1 表示代码段或数据段，0 表示系统段
    unsigned char DPL : 2;         // Descriptor Privilege Level 描述符特权等级 0 ~ 3
    unsigned char present : 1;     // 存在位，1 在内存中，0 在磁盘上
    unsigned char limit_high : 4;  // 段界限 16 ~ 19;
    unsigned char available : 1;   // 该安排的都安排了，送给操作系统吧
    unsigned char long_mode : 1;   // 64 位扩展标志
    unsigned char big : 1;         // 32 位 还是 16 位;
    unsigned char granularity : 1; // 粒度 4KB 或 1B
    unsigned char base_high;       // 基地址 24 ~ 31 位
} descriptor_t;

typedef struct tss_t
{
    uint32_t backlink; // 前一个任务的链接，保存了前一个任状态段的段选择子
    uint32_t esp0;     // ring0 的栈顶地址
    uint32_t ss0;      // ring0 的栈段选择子
    uint32_t esp1;     // ring1 的栈顶地址
    uint32_t ss1;      // ring1 的栈段选择子
    uint32_t esp2;     // ring2 的栈顶地址
    uint32_t ss2;      // ring2 的栈段选择子
    uint32_t cr3;
    uint32_t eip;
    uint32_t flags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldtr;          // 局部描述符选择子
    uint16_t trace : 1;     // 如果置位，任务切换时将引发一个调试异常
    uint16_t reversed : 15; // 保留不用
    uint16_t iobase;        // I/O 位图基地址，16 位从 TSS 到 IO 权限位图的偏移
    uint32_t ssp;           // 任务影子栈指针
} tss_t;
void tss_init();
