// Set up CPU's kernel segment descriptors.
// Run once on entry on each CPU.
#include "types.h"
#include "param.h"
//#include "x86/mmu.h"
#include "x86/io.h"
#include "proc.h"
#include "lapic.h"
#include "segment.h"
#include "printf.h"
#include "string.h"
extern struct tss_t tss;                  // 任务状态段
#define KERNEL_VA_OFFSET 0xC0000000   // 内核虚拟地址偏移

// 地址转换宏（内核直接映射）
#define phys_to_virt(pa) ((void*)((uint32_t)(pa) + KERNEL_VA_OFFSET))
#define virt_to_phys(va) ((uint32_t)(va) - KERNEL_VA_OFFSET)

void seginit(void)
{
  struct cpu *c;

  // Map "logical" addresses to virtual addresses using identity map.
  // Cannot share a CODE descriptor for both kernel and user
  // because it would have to have DPL_USR, but the CPU forbids
  // an interrupt from CPL=0 to DPL=3.
  c = &cpus[logical_cpu_id()];
  c->gdt[SEG_KCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, 0);
  c->gdt[SEG_KDATA] = SEG(STA_W, 0, 0xffffffff, 0);
  c->gdt[SEG_UCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, DPL_USER);
  c->gdt[SEG_UDATA] = SEG(STA_W, 0, 0xffffffff, DPL_USER);
  //c->gdt[SEG_TSS] = SEG16(0, 0, 0, 0);
  
  lgdt(c->gdt, sizeof(c->gdt));
}

void descriptor_init(struct descriptor_t *desc, uint32_t base, uint32_t limit)
{
    desc->base_low = base & 0xffffff;
    desc->base_high = (base >> 24) & 0xff;
    desc->limit_low = limit & 0xffff;
    desc->limit_high = (limit >> 16) & 0xf;
}

void tss_init()
{
    // 🔥 临时禁用所有 printf，避免在早期启动时崩溃
    // printf("tss_init: starting\n");
    // printf("tss address: 0x%x, sizeof(tss): %u\n", &tss, sizeof(tss));

    // 🔥 关键修复：不要清零整个 TSS！
    // 原因：boot.s 中已经设置了 tss.esp0 和 tss.ss0
    //       memset 会把它们清零，导致异常处理时 Triple Fault
    //
    // 只清零需要初始化的字段
    tss.backlink = 0;
    tss.cr3 = 0;
    tss.eip = 0;
    tss.flags = 0;
    tss.eax = 0;
    tss.ecx = 0;
    tss.edx = 0;
    tss.ebx = 0;
    tss.esp = 0;
    tss.ebp = 0;
    tss.esi = 0;
    tss.edi = 0;
    tss.es = 0;
    tss.cs = 0;
    tss.ss = 0;
    tss.ds = 0;
    tss.fs = 0;
    tss.gs = 0;
    tss.ldtr = 0;
    tss.trace = 0;
    tss.reversed = 0;
    tss.iobase = sizeof(tss);
    tss.ssp = 0;

    // ⚠️ 不要覆盖 boot.s 中设置的 esp0 和 ss0！
    // 检查它们是否有效
    if (tss.esp0 == 0) {
        // printf("tss_init: WARNING! esp0 was 0 (boot.s failed?), using current ESP\n");
        uint32_t current_esp;
        asm volatile("mov %%esp, %0" : "=r"(current_esp));
        tss.esp0 = current_esp;
    }

    if (tss.ss0 == 0) {
        // printf("tss_init: WARNING! ss0 was 0, setting to 0x10\n");
        tss.ss0 = SEG_KDATA << 3;
    }

    // printf("tss_init: preserved ESP0=0x%x, SS0=0x%x\n", tss.esp0, tss.ss0);

    struct cpu *c = &cpus[logical_cpu_id()];

    c->gdt[SEG_TSS] = SEG16(0x89, &tss, sizeof(tss)-1, 0);
    c->gdt[SEG_TSS].s = 0;

    // 使用descriptor_init初始化TSS描述符
    /*
    struct descriptor_t *desc = (struct descriptor_t *)&c->gdt[SEG_TSS];
    descriptor_init(desc, (uint32_t)&tss, sizeof(tss) - 1);
    desc->segment = 0;     // 系统段
    desc->granularity = 0; // 字节粒度
    desc->big = 0;
    desc->long_mode = 0;
    desc->present = 1;
    desc->DPL = 0;
    //desc->type = 0x9;      // 32位可用TSS
    */
    // printf("TSS init: addr=0x%x, ESP0=0x%x, SS0=0x%x\n",
    //        &tss, tss.esp0, tss.ss0);
    // printf("TSS init:selector=0x%x \n", SEG_TSS << 3);

    // 加载 TSS 到任务寄存器（重要！）
    uint16_t tss_selector = SEG_TSS << 3;
    asm volatile("ltr %0" : : "r"(tss_selector));
    // printf("TSS loaded into task register\n");
}
