// The I/O APIC manages hardware interrupts for an SMP system.
// http://www.intel.com/design/chipsets/datashts/29056601.pdf
// See also picirq.c.

#include "types.h"
#include "printf.h"
#include "ioapic.h"
#include "acpi.h"
#include "interrupt.h"
#include "page.h"

#define IOAPIC  0xFEC00000   // Default physical address of IO APIC

#define REG_ID     0x00  // Register index: ID
#define REG_VER    0x01  // Register index: version
#define REG_TABLE  0x10  // Redirection table base

// The redirection table starts at REG_TABLE and uses
// two registers to configure each interrupt.
// The first (low) register in a pair contains configuration bits.
// The second (high) register contains a bitmask telling which
// CPUs can serve that interrupt.
#define INT_DISABLED   0x00010000  // Interrupt disabled
#define INT_LEVEL      0x00008000  // Level-triggered (vs edge-)
#define INT_ACTIVELOW  0x00002000  // Active low (vs high)
#define INT_LOGICAL    0x00000800  // Destination is CPU id (vs APIC ID)

#define IOAPIC_IOREGSEL 0x00
#define IOAPIC_IOWIN 0x10
#define IOAPIC_REG_SEL *((volatile uint32_t*)(_ioapic_base + IOAPIC_IOREGSEL))
#define IOAPIC_REG_WIN *((volatile uint32_t*)(_ioapic_base + IOAPIC_IOWIN))
// 设备内存的标志位
#define PTE_P (1 << 0) // Present
#define PTE_W (1 << 1) // Writeable
#define PTE_U (1 << 2) // User
#define PTE_PWT (1 << 3) // Write-Through
#define PTE_PCD (1 << 4) // Cache-Disable

#define DEVICE_FLAGS (PTE_P | PTE_W | PTE_PWT | PTE_PCD) 
//#define DEVICE_FLAGS (0x00000000 | (1 << 4) | (1 << 3)) // PCD=1, PWT=1

// IO APIC MMIO structure: write reg, then read or write data.
typedef struct  {
  uint32_t index;
  uint32_t pad[3];
  uint32_t data;
}ioapic_t;

//volatile static struct ioapic_t *ioapic=NULL;
static volatile uint32_t _ioapic_base;


static uint32_t
ioapicread(uint32_t reg)
{ 
  if (_ioapic_base == NULL) {
        printf("IOAPIC not initialized! Call ioapic_init() first");
    }

  ioapic_t *ioapic = (ioapic_t*)_ioapic_base;
  ioapic->index = reg;
  return ioapic->data;
}


static void
ioapicwrite(int reg, uint32_t data)
{
  /*(&ioapic)->reg = reg;
  (&ioapic)->data = data; */
  IOAPIC_REG_SEL = reg;
  IOAPIC_REG_WIN = data;
}

// 读取ID寄存器
uint32_t get_ioapic_id(void) {
    uint32_t id_reg = ioapicread(0x00);
    return (id_reg >> 24) & 0x0F; 
}

// 读取版本寄存器
uint8_t get_ioapic_version(void) {
    uint32_t ver_reg = ioapicread(0x01);
    return ver_reg & 0xFF;
}

// 在尝试读取之前，先检查基本可访问性
void check_ioapic_accessible(void)
{
  // 尝试读取版本寄存器（应该总是可读的）
  uint8_t version = get_ioapic_version();
  printf("I/O APIC Version: %u\n", version);
  
  if (version == 0 || version == 0xFFFFFFFF) {
    printf("ERROR: I/O APIC not accessible!\n");
    // 这里需要处理错误
  }
}


void
ioapicinit(void)
{
  uint32_t i,id, maxintr;
  uint32_t phys_addr;
  
  acpi_context* acpi_ctx = acpi_get_context();

  if(acpi_ctx==NULL || acpi_ctx->madt.ioapic==NULL){
    phys_addr=IOAPIC;    
    printf("phys_addr IOAPIC is : 0x%x\n", phys_addr); 
  }else{
  acpi_ioapic_t * temp=map_hardware_region(acpi_ctx->madt.ioapic,sizeof(acpi_ioapic_t),"IOAPIC ...");
  phys_addr=temp->ioapic_addr;
  printf("phys_addr in acpi_context is : 0x%x\n", phys_addr); 
  }

  //uint32_t virt_addr=&_ioapic_base;//&ioapic;

  _ioapic_base=map_hardware_region(phys_addr,sizeof(ioapic_t),"IOAPIC ...");//early_remap_page(phys_addr,virt_addr,DEVICE_FLAGS);

  //ioapic = (volatile struct ioapic*)virt_addr;
  // 验证映射
  if (_ioapic_base == NULL) {
        printf("Failed to map IOAPIC\n");
    }
    
  printf("IOAPIC mapped at: 0x%x\n", _ioapic_base);
  
  //check_ioapic_accessible();
  //maxintr = (ioapicread(REG_VER) >> 16) & 0xFF;get_ioapic_version

  uint32_t ver_reg = ioapicread(REG_VER);
  uint8_t version = ver_reg & 0xFF;
  printf("I/O APIC Version: %u\n", version);
  maxintr = (ver_reg >> 16) & 0xFF;

  id = ioapicread(REG_ID) >> 24;
  //id=get_ioapic_id();

  printf("===id value is %u===\n",id);
  printf("===ioapicid  value is %u===\n",ioapicid);
  if(id != ioapicid )//ioapicid)
    printf("ioapicinit: id isn't equal to ioapicid; not a MP\n");

  

  
  // Mark all interrupts edge-triggered, active high, disabled,
  // and not routed to any CPUs.
  for(i = 0; i <= maxintr; i++){
    ioapicwrite(REG_TABLE+2*i, INT_DISABLED | (T_IRQ0 + i));
    ioapicwrite(REG_TABLE+2*i+1, 0);
  }
}

void
ioapicenable(int irq, int cpunum)
{
  // Mark interrupt edge-triggered, active high,
  // enabled, and routed to the given cpunum,
  // which happens to be that cpu's APIC ID.

  // 🔥 调试：打印配置信息
  printf("[ioapicenable] Enabling IRQ%d on CPU%d\n", irq, cpunum);
  printf("[ioapicenable] Writing to REG_TABLE+%d (0x%x)\n", 2*irq, REG_TABLE+2*irq);
  printf("[ioapicenable] Vector = %d (0x%x)\n", T_IRQ0 + irq, T_IRQ0 + irq);

  uint32_t low_before = ioapicread(REG_TABLE+2*irq);
  uint32_t high_before = ioapicread(REG_TABLE+2*irq+1);
  printf("[ioapicenable] Before: low=0x%x high=0x%x\n", low_before, high_before);

  // 🔥 关键：设置中断模式
  // Bit 16: Mask (0 = enabled, 1 = masked)
  // Bit 15: Trigger mode (0 = edge, 1 = level)
  // Bit 13: Polarity (0 = active high, 1 = active low)
  // PCI 中断应该是：edge-triggered, active high
  uint32_t low = T_IRQ0 + irq;  // Vector
  low &= ~(1 << 16);             // Unmask
  // low &= ~(1 << 15);           // Edge triggered (默认)
  // low &= ~(1 << 13);           // Active high (默认)

  ioapicwrite(REG_TABLE+2*irq, low);
  ioapicwrite(REG_TABLE+2*irq+1, cpunum << 24);

  uint32_t low_after = ioapicread(REG_TABLE+2*irq);
  uint32_t high_after = ioapicread(REG_TABLE+2*irq+1);
  printf("[ioapicenable] After: low=0x%x high=0x%x\n", low_after, high_after);

  // 🔥 检查是否被 mask
  if (low_after & (1 << 16)) {
    printf("[ioapicenable] ⚠️  WARNING: IRQ %d is still MASKED!\n", irq);
  }

  printf("[ioapicenable] Done!\n");
}


