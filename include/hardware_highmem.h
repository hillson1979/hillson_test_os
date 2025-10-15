// hardware_highmem.h
void* map_hardware_region(uint32_t phys_base, uint32_t size, const char* name);

// 具体硬件映射函数
void* map_apic(void);

void* map_pci_config(void);

void* map_pci_memory(uint32_t phys_base, uint32_t size);

void* map_ioapic(uint32_t phys_base) ;
