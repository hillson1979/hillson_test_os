// acpi.h
#ifndef ACPI_H
#define ACPI_H

#include "types.h"
//#include "sdt.h"
#include "madt.h"

// ACPI 表签名
#define ACPI_SIGNATURE_RSDP "RSD PTR "
#define ACPI_SIGNATURE_RSDT "RSDT"
#define ACPI_SIGNATURE_XSDT "XSDT"
#define ACPI_SIGNATURE_FADT "FACP"
#define ACPI_SIGNATURE_MADT "APIC"
#define ACPI_SIGNATURE_HPET "HPET"
#define ACPI_SIGNATURE_MCFG "MCFG"

#define ACPI_MADT_SIG 0x43495041 // 'APIC'
#define ACPI_FADT_SIG 0x50434146 // 'FACP' Notice that it is not 'FADT'.

// 'MCFG' (Not part of ACPI standard. See PCI Firmware Spec.)
#define ACPI_MCFG_SIG 0x4746434d

// RSDP 结构
typedef struct {
    char signature[8];
    uint8_t checksum;
    char oemid[6];
    uint8_t revision;
    uint32_t rsdt_address;      // 物理地址
} __attribute__((packed)) rsdp_t;

// RSDP 扩展 (ACPI 2.0+)
typedef struct {
    rsdp_t v1;
    uint32_t length;
    uint64_t xsdt_address;      // 物理地址
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed)) rsdp2_t;


typedef struct
{
    // Make it as null terminated
    char oem_id[7];
    acpi_madt_toc_t madt;
    //acpi_fadt_t fadt;
    //struct acpi_mcfg_toc mcfg;
} acpi_context;

acpi_context* acpi_get_context();
// 函数声明
void* find_rsdp(void);
int verify_checksum(void* table, uint32_t length);
void* get_rsdt_address(void* rsdp);
void enumerate_acpi_tables(void* sdt_ptr);
void print_table_info(acpi_sdt_header_t* header);
int acpi_init();

#endif // ACPI_H
