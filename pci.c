//#include <stddef.h>
#include "x86/hal.h"
#include "hal.h"
//#include "stdio.h"
//#include "kmalloc.h"
#include "vga.h"
#include "kmalloc_early.h"
#include "x86/io.h"
#include "pci.h"

#include "pci/pci_ids.h"

#define CONFIG_ADDRESS 0xCF8
#define CONFIG_DATA    0xCFC
// 地址转换宏（内核直接映射）
#define KERNEL_VA_OFFSET 0xC0000000   // 内核虚拟地址偏移
#define phys_to_virt(pa) ((void*)((uint32_t)(pa) + KERNEL_VA_OFFSET))
#define virt_to_phys(va) ((uint32_t)(va) - KERNEL_VA_OFFSET)

/*
31     30    24 23 16 15   11 10      8 7       2 10
Enable Reserved Bus # Device# Function# Register# 00
*/

#define ENABLE_BIT (1U << 31)
#define BUS_M      0xFF
#define BUS_S      16
#define DEV_M      0x1F
#define DEV_S      11
#define FN_M       0x7
#define FN_S       8
#define REG_M      0x3F
#define REG_S      2

/* Set in the header type field if the device has multiple functions. */
#define HEADER_TYPE_MF 0x80

#define NUM_CLASS_CODE_STRS 18
const char *class_code_strs[NUM_CLASS_CODE_STRS] = {
  "Very old device",
  "Mass storage controller",
  "Network controller",
  "Display controller",
  "Multimedia controller",
  "Memory controller",
  //"Bridge device",
  //"Simple communication controller",
  //"Base system peripheral",
  "Input device",
  "Docking station",
  "Processor",
  "Serial bus controller",
  "Wireless controller",
  "Intelligent I/O controller",
  "Satellite communication controller",
  "Encryption/Decryption controller",
  "Data acquisition o signal processing controller",
};

/* Reads a 32-bit value from the PCI configuration space. */
static uint32_t pci_read32(unsigned bus, unsigned dev, unsigned fn, unsigned reg) {
  uint32_t addr = ((bus&BUS_M) << BUS_S) |
    ((dev&DEV_M) << DEV_S) |
    ((fn&FN_M) << FN_S) |
    ((reg&REG_M) << REG_S) |
    ENABLE_BIT;

  outl(CONFIG_ADDRESS, addr);
  return inl(CONFIG_DATA);
}

#define MAX_PCI_DEVICES 64
static pci_dev_t *devices[MAX_PCI_DEVICES] = {NULL};
static unsigned num_devices = 0;

static const char *get_vendor_name(uint16_t id, unsigned verbose) {
  //printf("PCI_VENTABLE_LEN is %u\n",PCI_VENTABLE_LEN);

  //printf("in get_vendor_name %u:---:\n", id); 
  for (unsigned i = 0; i < PCI_VENTABLE_LEN; ++i) {
    if (PciVenTable[i].VenId == id)
      return verbose ? PciVenTable[i].VenFull : PciVenTable[i].VenShort;
  }
  return NULL;
}

static const char *get_device_name(uint16_t vendor_id, uint16_t id,
                                   unsigned verbose) {
  //printf("First entry: VenId=%u, DevId=%u\n", PciDevTable[0].VenId, PciDevTable[0].DevId);
  //printf("PCI_DEVTABLE_LEN is %u\n",PCI_DEVTABLE_LEN);
  //printf("1 in get_device_name %u:---%u:\n", vendor_id,id); 
  
  //printf("my000: Chip='%s', ChipDesc='%s'\n",
    //           PciDevTable[0].Chip, PciDevTable[0].ChipDesc);
  
  for (unsigned i = 0; i < PCI_DEVTABLE_LEN; ++i) {
    /*if(i>1730 && i<1750){
       printf("--1--"); 

       printf("%u:",i);  
       printf("%u:",PciDevTable[i].VenId);
       printf("%u:",PciDevTable[i].DevId);
       printf("my000: id='%u', id2='%u'\n",vendor_id,id);
       printf("my000: Chip='%s', ChipDesc='%s'\n",
               PciDevTable[i].Chip, PciDevTable[i].ChipDesc);
       } 
    if(i>1750 && i<1850){
       printf("--2--");
       printf("%u:",i);}
    if(i>1850 && i<1900){
       printf("--3--");
       printf("%u:",i);}*/

    if (PciDevTable[i].VenId == vendor_id &&
        PciDevTable[i].DevId == id){
        //printf("3 in get_device_name %u:---:%u\n", vendor_id,PciDevTable[i].VenId);
       // printf("Raw strings: Chip='%s', ChipDesc='%s'\n",
         //      PciDevTable[i].Chip, PciDevTable[i].ChipDesc);
        return verbose ? PciDevTable[i].ChipDesc : PciDevTable[i].Chip;
    }
      
  }
  return NULL;
}

static const char *get_class_code(uint8_t class) {
  if (class >= NUM_CLASS_CODE_STRS)
    return NULL;
  return class_code_strs[class];
}

static void print_device_brief(pci_header_t *h) {
  /*printf("0x%04x:0x%04x:%s: %s %s\n", h->vendor_id, h->device_id,
          get_class_code(h->class),
          get_vendor_name(h->vendor_id, 1),
          get_device_name(h->vendor_id, h->device_id, 1));*/
  //printf("brief %u:---%u:\n", h->vendor_id, h->device_id);
  printf("%s ,",get_class_code(h->class));
  //printf("%s ,",get_vendor_name(h->vendor_id, 1));
  char* i=get_device_name(h->vendor_id, h->device_id, 1);
  printf("%s ===.",i);
  //printf("\n");
}

void pci_print_device(pci_dev_t *d) {
  printf("%02x:%02x:%02x - %04x:%04x\n", d->bus_id, d->dev_id, d->fn_id,
          d->header.vendor_id, d->header.device_id);
  printf("class %x subclass %x progIF %x int_line %x int_pin %x\n",
          d->header.class, d->header.subclass, d->header.prog_if,
          d->header.u.h00.interrupt_line, d->header.u.h00.interrupt_pin);
  for (unsigned i = 0; i < 6 ; ++i)
    printf("BAR%d: %08x\n", i, d->header.u.h00.bar[i]);
}

pci_dev_t **pci_get_devices() {
  return devices;
}

static pci_dev_t *pci_probe(unsigned bus, unsigned dev, unsigned fn) {
  if (pci_read32(bus, dev, fn, 0) == 0xFFFFFFFF)
    return NULL;

  pci_dev_t *d = (pci_dev_t *)kmalloc_early(sizeof(pci_header_t));//phys_to_virt(pmm_alloc_page());//

  d->bus_id = bus;
  d->dev_id = dev;
  d->fn_id = fn;
  uint32_t *h32 = (uint32_t*)&d->header;
  for (unsigned i = 0; i < 0x10; ++i)
    h32[i] = pci_read32(bus, dev, fn, i);

  devices[num_devices++] = d;
  return d;
}

int pci_init() {

  /* Scan PCI buses. */
  for (unsigned bus = 0; bus < 256; ++bus) {
    for (unsigned dev = 0; dev < 32; ++dev) {
      pci_dev_t *d = pci_probe(bus, dev, 0);
      if (!d) continue;
      print_device_brief(&d->header);

      if (d->header.header_type & HEADER_TYPE_MF) {
        for (unsigned fn = 1; fn < 8; ++fn) {
          d = pci_probe(bus, dev, fn);
          if (!d) continue;
          print_device_brief(&d->header);
        }
      }
    }
  }

  return 0;
}

void check_pci_table_size(void) {
    unsigned int actual_entries = 0;
    const struct _PCI_DEVTABLE *entry = &PciDevTable[0];
    
    // 计算实际非空条目数（假设以全0条目结束）
    while (entry->VenId != 0 || entry->DevId != 0) {
        actual_entries++;
        entry++;
    }
    
    printf("PCI stats:\n");
    //printf("pci_dev_table_size  - 理论条目数: %u\n", pci_dev_table_size);
    printf("actual_entries  - actual lines: %u\n", actual_entries);
    printf("sizeof(pci_dev_table)  - total: %u bytes\n", sizeof(PciDevTable));
    printf("avg  - avg : %u bytes\n", 
           sizeof(PciDevTable) / actual_entries);
}

static prereq_t prereqs[] = { {"kmalloc",NULL}, {NULL,NULL} };
static module_t x run_on_startup = {
  .name = "x86/pci",
  .required = prereqs,
  .load_after = NULL,
  .init = &pci_init,
  .fini = NULL
};
