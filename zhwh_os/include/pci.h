#ifndef X86_PCI_H
#define X86_PCI_H

#define PCI_CLASS_MASS_STORAGE 0x01

#define PCI_SUBCLASS_IDE  0x01

/* PCI configuration registers */
#define PCI_COMMAND        0x04    /* Command register */
#define PCI_STATUS         0x06    /* Status register */
#define PCI_BAR0           0x10    /* Base Address Register 0 */
#define PCI_BAR1           0x14    /* Base Address Register 1 */
#define PCI_BAR2           0x18    /* Base Address Register 2 */
#define PCI_BAR3           0x1C    /* Base Address Register 3 */
#define PCI_BAR4           0x20    /* Base Address Register 4 */
#define PCI_BAR5           0x24    /* Base Address Register 5 */
#define PCI_INTERRUPT_LINE 0x3C    /* Interrupt line register */

/* PCI command bits */
#define PCI_COMMAND_IO      0x01   /* Enable I/O space */
#define PCI_COMMAND_MEMORY  0x02   /* Enable Memory space */
#define PCI_COMMAND_MASTER  0x04   /* Enable bus mastering */

/* Header structure for a device with header type 0x00, which is most
   devices. */
typedef struct pci_header_00 {
  uint32_t bar[6];
  uint32_t cardbus_cis_ptr;
  uint16_t subsys_vendor_id, subsys_id;
  uint32_t expansion_rom_addr;
  uint8_t  capabilities, resvd1[3];
  uint32_t resvd2;
  uint8_t  interrupt_line, interrupt_pin, min_grant, max_latency;
} pci_header_00_t;

/* Header structure for a device with header type 0x01, which is
   generally a PCI-to-PCI bridge */
typedef struct pci_header_01 {
  uint32_t bar[2];
  uint8_t  pri_bus_num, sec_bus_num, sub_bus_num, secondary_latency_timer;
  uint8_t  io_base, io_limit;
  uint16_t sec_status;
  uint16_t memory_base, memory_limit;
  uint16_t prefetch_memory_base, prefetch_memory_limit;
  uint32_t prefetchable_base_hi32;
  uint32_t prefetchable_base_lo32;
  uint16_t io_limit_lo16, io_limit_hi16;
  uint8_t  capabilities, resvd[3];
  uint32_t expansion_rom_addr;
  uint8_t  interrupt_line, interrupt_pin;
  uint16_t bridge_ctl;
} pci_header_01_t;

/* Header structure for a PCI device - common to all header types. */
typedef struct pci_header {
  uint16_t vendor_id, device_id;
  uint16_t command, status;
  uint8_t  revision_id, prog_if, subclass, class;
  uint8_t  cache_line_sz, latency_timer, header_type, bist;
  union {
    struct pci_header_00 h00;
    struct pci_header_01 h01;
  } u;
} pci_header_t;

/* A PCI device descriptor. */
typedef struct pci_dev {
  uint16_t bus_id, dev_id, fn_id;
  pci_header_t header;
} pci_dev_t;

/* Returns a NULL-terminated array of PCI devices detected. */
pci_dev_t **pci_get_devices();

/* Prints out a PCI device descriptor in detail. */
void pci_print_device(pci_dev_t *d);

/* Get PCI vendor and device names from IDs */
const char *pci_get_vendor_name(uint16_t vendor_id);
const char *pci_get_device_name(uint16_t vendor_id, uint16_t device_id);

/* PCI configuration space read/write functions */
uint32_t pci_read_config_dword(unsigned bus, unsigned dev, unsigned fn, unsigned reg);
uint16_t pci_read_config_word(unsigned bus, unsigned dev, unsigned fn, unsigned reg);
uint8_t pci_read_config_byte(unsigned bus, unsigned dev, unsigned fn, unsigned reg);
void pci_write_config_word(unsigned bus, unsigned dev, unsigned fn, unsigned reg, uint16_t value);
void pci_write_config_dword(unsigned bus, unsigned dev, unsigned fn, unsigned reg, uint32_t value);

int pci_init();

#endif
