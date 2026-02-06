// hardware_highmem.c
#include "highmem_mapping.h"

// 统一的硬件访问接口
void* map_hardware_region(uint32_t phys_base, uint32_t size, const char* name) {

    #define printf(...)
    // 设备内存通常需要uncached访问
    uint32_t flags = 0x3 | 0x10; // Present + RW + Uncached

    void* mapped_addr = map_highmem_physical(phys_base, size, flags);
    if (mapped_addr) {
        printf("%s mapped: phys 0x%x -> virt 0x%x\n", name, phys_base, mapped_addr);
    } else {
        printf("ERROR: Failed to map %s at 0x%x\n", name, phys_base);
    }
    return mapped_addr;
}

// 具体硬件映射函数
void* map_apic(void) {
    return map_hardware_region(APIC_PHYS_BASE, 0x100000, "APIC");
}

void* map_pci_config(void) {
    return map_hardware_region(PCI_CONFIG_PHYS, 0x1000000, "PCI Config");
}

void* map_pci_memory(uint32_t phys_base, uint32_t size) {
    char name[32];
    printf(name, sizeof(name), "PCI Memory 0x%x", phys_base);
    return map_hardware_region(phys_base, size, name);
}

void* map_ioapic(uint32_t phys_base) {
    return map_hardware_region(phys_base, 0x1000, "IOAPIC");
}

