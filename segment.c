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
struct tss_t *tss;                  // 任务状态段
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
    //struct cpu *c = &cpus[logical_cpu_id()];
    tss=(struct tss_t *)kmalloc_early(sizeof (struct tss_t));//phys_to_virt(pmm_alloc_page());
    memset(&tss, 0, sizeof(tss));

    // 设置内核栈
    uint32_t esp0;
    asm volatile("mov %%esp, %0" : "=r"(esp0));
    tss->esp0 = esp0;
    tss->ss0 = SEG_KDATA << 3;
    tss->iobase = sizeof(tss);

    struct cpu *c = &cpus[logical_cpu_id()];

    c->gdt[SEG_TSS] = SEG16(0x89, &c->ts,sizeof(c->ts)-1, 0);
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
    printf("TSS init: addr=0x%x, ESP0=0x%x\n", &tss, tss->esp0);
    printf("TSS init:selector=0x%x \n", SEG_TSS << 3);
    //ltr(SEG_TSS << 3);
    //uint16_t tss_selector = SEG_TSS << 3;
    //asm volatile("ltr %0" : : "r"(tss_selector));
}
