// The local APIC manages internal (non-I/O) interrupts.
// See Chapter 8 & Appendix C of Intel processor manual volume 3.

#include "param.h"
#include "types.h"
#include "lapic.h"
#include "string.h"
#include "date.h"
#include "memlayout.h"
#include "interrupt.h"

#include "x86/io.h"

// Local APIC registers, divided by 4 for use as uint[] indices.
#define ID      (0x0020/4)   // ID
#define VER     (0x0030/4)   // Version
#define TPR     (0x0080/4)   // Task Priority
#define EOI     (0x00B0/4)   // EOI
#define SVR     (0x00F0/4)   // Spurious Interrupt Vector
  #define ENABLE     0x00000100   // Unit Enable
#define ESR     (0x0280/4)   // Error Status
#define ICRLO   (0x0300/4)   // Interrupt Command
  #define INIT       0x00000500   // INIT/RESET
  #define STARTUP    0x00000600   // Startup IPI
  #define DELIVS     0x00001000   // Delivery status
  #define ASSERT     0x00004000   // Assert interrupt (vs deassert)
  #define DEASSERT   0x00000000
  #define LEVEL      0x00008000   // Level triggered
  #define BCAST      0x00080000   // Send to all APICs, including self.
  #define BUSY       0x00001000
  #define FIXED      0x00000000
#define ICRHI   (0x0310/4)   // Interrupt Command [63:32]
#define TIMER   (0x0320/4)   // Local Vector Table 0 (TIMER)
  #define X1         0x0000000B   // divide counts by 1
  #define PERIODIC   0x00020000   // Periodic
#define PCINT   (0x0340/4)   // Performance Counter LVT
#define LINT0   (0x0350/4)   // Local Vector Table 1 (LINT0)
#define LINT1   (0x0360/4)   // Local Vector Table 2 (LINT1)
#define ERROR   (0x0370/4)   // Local Vector Table 3 (ERROR)
  #define MASKED     0x00010000   // Interrupt masked
#define TICR    (0x0380/4)   // Timer Initial Count
#define TCCR    (0x0390/4)   // Timer Current Count
#define TDCR    (0x03E0/4)   // Timer Divide Configuration

volatile uint32_t *lapic;  // Initialized in mp.c
extern struct cpu *cpus[NCPU];
uint64_t get_apic_base_32bit(void)
{
    uint32_t eax, edx;
    
    __asm__ volatile (
        "movl $0x1B, %%ecx\n\t"
        "rdmsr"
        : "=a"(eax), "=d"(edx)
        : "c" (0x1B)
    );
    
    // 小心组合64位值
    uint64_t result = edx;
    result = (result << 32) | eax;
    result &= 0xFFFFFFFFFFFFF000ULL;
    
    return result;
}

//PAGEBREAK!
static void
lapicw(int index, int value)
{
  lapic[index] = value;
  lapic[ID];  // wait for write to finish, by reading
}

void
lapicinit(void)
{
  uint64_t lapic_addr_64 = get_apic_base_32bit();
  uint32_t lapic_addr =  (uint32_t)(lapic_addr_64 & 0xFFFFF000);

  printf("---lapic_addr value is 0x%x---\n", lapic_addr);
  printf("---lapic value is 0x%x---\n", lapic);

  // 🔥🔥🔥 关键修复：Identity map 整个 LAPIC 窗口（64KB）
  // 原因：MSI 写的是物理地址，必须确保该物理地址在页表中有 identity mapping
  // MSI 地址格式：0xFEE00000 | (lapic_id << 12)
  //   - lapic_id = 0 → 0xFEE00000
  //   - lapic_id = 1 → 0xFEE01000
  //   - lapic_id = 2 → 0xFEE02000
  // 所以必须映射至少 64KB (0x10000) 才能覆盖所有可能的 lapic_id
  //
  // 标志位：
  //   PAGE_PRESENT (0x001)  - 页面存在
  //   PAGE_RW      (0x002)  - 可读写
  //   PAGE_PCD     (0x010)  - 禁用缓存（MMIO 必须）
  //   PAGE_PWT     (0x008)  - 写通过（可选，MMIO 推荐）

  #define PAGE_PRESENT  0x001
  #define PAGE_RW       0x002
  #define PAGE_PCD      0x010
  #define PAGE_PWT      0x008

  printf("[lapicinit] Mapping LAPIC window: phys=0x%x -> virt=0x%x (size=64KB)\n",
         lapic_addr, lapic_addr);

  // 映射 64KB (16 个 4KB 页)
  for (uint32_t offset = 0; offset < 0x10000; offset += 0x1000) {
    map_4k_page(lapic_addr + offset, lapic_addr + offset,
                PAGE_PRESENT | PAGE_RW | PAGE_PCD | PAGE_PWT);
  }

  printf("[lapicinit] LAPIC identity mapping complete\n");

  // 设置 lapic 指针为物理地址（identity mapping 后可以直接使用）
  lapic = (volatile uint32_t *)lapic_addr;

  // 🔍 验证映射是否成功（测试访问）
  printf("[lapicinit] Verifying LAPIC access... ");
  uint32_t test_read = lapic[ID];  // 读取 LAPIC ID 寄存器
  printf("LAPIC ID = 0x%x\n", test_read);

  // Enable local APIC; set spurious interrupt vector.
  lapicw(SVR, ENABLE | (T_IRQ0 + IRQ_SPURIOUS));

  // The timer repeatedly counts down at bus frequency
  // from lapic[TICR] and then issues an interrupt.
  // If xv6 cared more about precise timekeeping,
  // TICR would be calibrated using an external time source.
  lapicw(TDCR, X1);
  // TEMPORARILY DISABLE TIMER FOR DEBUGGING
  lapicw(TIMER, MASKED); // Disable timer
  // lapicw(TIMER, PERIODIC | (T_IRQ0 + IRQ_TIMER));
  // lapicw(TICR, 10000000);

  // Disable logical interrupt lines.
  lapicw(LINT0, MASKED);
  lapicw(LINT1, MASKED);

  // Disable performance counter overflow interrupts
  // on machines that provide that interrupt entry.
  if(((lapic[VER]>>16) & 0xFF) >= 4)
    lapicw(PCINT, MASKED);

  // Map error interrupt to IRQ_ERROR.
  lapicw(ERROR, T_IRQ0 + IRQ_ERROR);

  // Clear error status register (requires back-to-back writes).
  lapicw(ESR, 0);
  lapicw(ESR, 0);

  // Ack any outstanding interrupts.
  lapicw(EOI, 0);

  // Send an Init Level De-Assert to synchronise arbitration ID's.
  lapicw(ICRHI, 0);
  lapicw(ICRLO, BCAST | INIT | LEVEL);
  while(lapic[ICRLO] & DELIVS)
    ;

  // Enable interrupts on the APIC (but not on the processor).
  lapicw(TPR, 0);

  printf("[lapicinit] LAPIC initialized successfully\n");

}

int
lapicid(void)
{
  if (!lapic)
    return 0;
  return lapic[ID] >> 24;
}

// Acknowledge interrupt.
void
lapiceoi(void)
{
  if(lapic)
    lapicw(EOI, 0);
}

// Spin for a given number of microseconds.
// On real hardware would want to tune this dynamically.
void
microdelay(int us)
{
}

#define CMOS_PORT    0x70
#define CMOS_RETURN  0x71

// Start additional processor running entry code at addr.
// See Appendix B of MultiProcessor Specification.
void
lapicstartap(uint8_t apicid, uint32_t addr)
{
  int i;
  uint16_t *wrv;

  // "The BSP must initialize CMOS shutdown code to 0AH
  // and the warm reset vector (DWORD based at 40:67) to point at
  // the AP startup code prior to the [universal startup algorithm]."
  outb(CMOS_PORT, 0xF);  // offset 0xF is shutdown code
  outb(CMOS_PORT+1, 0x0A);
  wrv = (uint16_t*)P2V((0x40<<4 | 0x67));  // Warm reset vector
  wrv[0] = 0;
  wrv[1] = addr >> 4;

  // "Universal startup algorithm."
  // Send INIT (level-triggered) interrupt to reset other CPU.
  lapicw(ICRHI, apicid<<24);
  lapicw(ICRLO, INIT | LEVEL | ASSERT);
  microdelay(200);
  lapicw(ICRLO, INIT | LEVEL);
  microdelay(100);    // should be 10ms, but too slow in Bochs!

  // Send startup IPI (twice!) to enter code.
  // Regular hardware is supposed to only accept a STARTUP
  // when it is in the halted state due to an INIT.  So the second
  // should be ignored, but it is part of the official Intel algorithm.
  // Bochs complains about the second one.  Too bad for Bochs.
  for(i = 0; i < 2; i++){
    lapicw(ICRHI, apicid<<24);
    lapicw(ICRLO, STARTUP | (addr>>12));
    microdelay(200);
  }
}

#define CMOS_STATA   0x0a
#define CMOS_STATB   0x0b
#define CMOS_UIP    (1 << 7)        // RTC update in progress

#define SECS    0x00
#define MINS    0x02
#define HOURS   0x04
#define DAY     0x07
#define MONTH   0x08
#define YEAR    0x09

static uint32_t
cmos_read(uint32_t reg)
{
  outb(CMOS_PORT,  reg);
  microdelay(200);

  return inb(CMOS_RETURN);
}

static void
fill_rtcdate(struct rtcdate *r)
{
  r->second = cmos_read(SECS);
  r->minute = cmos_read(MINS);
  r->hour   = cmos_read(HOURS);
  r->day    = cmos_read(DAY);
  r->month  = cmos_read(MONTH);
  r->year   = cmos_read(YEAR);
}

// qemu seems to use 24-hour GWT and the values are BCD encoded
void
cmostime(struct rtcdate *r)
{
  struct rtcdate t1, t2;
  int sb, bcd;

  sb = cmos_read(CMOS_STATB);

  bcd = (sb & (1 << 2)) == 0;

  // make sure CMOS doesn't modify time while we read it
  for(;;) {
    fill_rtcdate(&t1);
    if(cmos_read(CMOS_STATA) & CMOS_UIP)
        continue;
    fill_rtcdate(&t2);
    if(memcmp(&t1, &t2, sizeof(t1)) == 0)
      break;
  }

  // convert
  if(bcd) {
#define    CONV(x)     (t1.x = ((t1.x >> 4) * 10) + (t1.x & 0xf))
    CONV(second);
    CONV(minute);
    CONV(hour  );
    CONV(day   );
    CONV(month );
    CONV(year  );
#undef     CONV
  }

  *r = t1;
  r->year += 2000;
}

/*
uint32_t apic_read(uint64_t reg) {
	//uint64_t lapic_base = get_apic_base_32bit();
	return *(volatile uint32_t *)(lapic + reg);	
}

uint32_t cpu_id(void) {
	// xAPIC: 8-bit ID from MMIO register (0x20 >> 24)
	return (apic_read(ID) >> 24) & 0xFF;
}
*/

//cpus[NCPU]
uint8_t get_cpu_id_from_lapic_id(uint32_t lapic_id) {
	for (uint8_t x = 0; x < NCPU - 1; ++x) {
		if (cpus[x] == lapic_id) {
			return x;
		}
	}
	return INVALID_CPU_ID;
}


uint8_t logical_cpu_id(void) {
	return get_cpu_id_from_lapic_id(lapicid());
}

#define LAPIC_BASE 0xFEE00000
#define LAPIC_ID   0x020

// 导出版本的 lapic_read，供 e1000.c 使用
uint32_t lapic_read(int index) {
    return *((volatile uint32_t*)(LAPIC_BASE + index));
}

uint8_t lapicid2(void) {
    return (uint8_t)(lapic_read(LAPIC_ID) >> 24);
}

#define LAPIC_ICRLO   0x300
#define LAPIC_ICRHI   0x310
#define LAPIC_EOI     0x0B0

static inline void lapic_send_ipi(uint8_t apicid, uint8_t vector)
{
    // 等待之前的 IPI 发送完成
    while (lapic[LAPIC_ICRLO/4] & (1 << 12)) {
        /* DELIVS = bit12 */
    }

    // 目标 APIC ID 写到 ICRHI[31:24]
    lapic[LAPIC_ICRHI/4] = ((uint32_t)apicid) << 24;

    // ICRLO:
    // bits 7:0   = vector
    // bits 10:8  = delivery mode = 000 (Fixed)
    // bit 14     = level = 1 (assert)
    // bit 15     = trigger = 0 (edge)
    lapic[LAPIC_ICRLO/4] = vector | (1 << 14);

    // 等待发送完成
    while (lapic[LAPIC_ICRLO/4] & (1 << 12)) {
    }
}

void lapic_send_ipi_(uint8_t apicid, uint8_t vector){
  printf("LAPIC ID=%d\n", lapicid2());
  printf("ICRLO before1=0x%x\n", lapic[LAPIC_ICRLO/4]);

  lapic_send_ipi(apicid, vector);

  printf("LAPIC ID=%d\n", lapicid2());
  printf("ICRLO before2=0x%x\n", lapic[LAPIC_ICRLO/4]);

}
