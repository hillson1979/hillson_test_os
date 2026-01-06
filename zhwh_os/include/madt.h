#ifndef __LUNAIX_ACPI_MADT_H
#define __LUNAIX_ACPI_MADT_H

#include "sdt.h"
#define ACPI_TABLE_PACKED __attribute__((packed))

#define ACPI_MADT_LAPIC 0x0  // Local APIC
#define ACPI_MADT_IOAPIC 0x1 // I/O APIC
#define ACPI_MADT_INTSO 0x2  // Interrupt Source Override

/**
 * @brief ACPI Interrupt Controller Structure (ICS) Header
 *
 */
typedef struct
{
    uint8_t type;
    uint8_t length;
} ACPI_TABLE_PACKED acpi_ics_hdr_t;
//#define __acpi_ics_hdr(acpi_ptr)   ((acpi_ics_hdr_t*)__ptr(acpi_ptr))

/**
 * @brief ACPI Processor Local APIC Structure (PLAS)
 * This structure tell information about our Local APIC per processor. Including
 * the MMIO addr.
 *
 */
typedef struct
{
    acpi_ics_hdr_t header;
    uint8_t processor_id;
    uint8_t apic_id;
    uint32_t flags;
} ACPI_TABLE_PACKED acpi_apic_t;
#define __acpi_apic(acpi_ptr)   ((acpi_apic_t*)__ptr(acpi_ptr))

/**
 * @brief ACPI IO APIC Structure (IOAS)
 *
 * This structure tell information about our I/O APIC on motherboard. Including
 * the MMIO addr.
 *
 */
typedef struct
{
    acpi_ics_hdr_t header;
    uint8_t ioapic_id;
    uint8_t reserved;
    uint32_t ioapic_addr;
    // The global system interrupt offset for this IOAPIC. (Kind of IRQ offset
    // for a slave IOAPIC)
    uint32_t gis_offset;
} ACPI_TABLE_PACKED acpi_ioapic_t;
//#define __acpi_ioapic(acpi_ptr)   ((acpi_ioapic_t*)__ptr(acpi_ptr))

/**
 * @brief ACPI Interrupt Source Override (INTSO)
 *
 * According to the ACPI Spec, the IRQ config between APIC and 8259 PIC can be
 * assumed to be identically mapped. However, some manufactures may have their
 * own preference and hence expections may be introduced. This structure provide
 * information on such exception.
 *
 */
typedef struct
{
    acpi_ics_hdr_t header;
    uint8_t bus;
    // source, which is the original IRQ back in the era of IBM PC/AT, the 8259
    // PIC
    uint8_t source;
    // global system interrupt. The override of source in APIC mode
    uint32_t gsi;
    uint16_t flags;
} ACPI_TABLE_PACKED acpi_intso_t;
//#define __acpi_intso(acpi_ptr)   ((acpi_intso_t*)__ptr(acpi_ptr))

typedef struct
{
    acpi_sdt_header_t header;
    uint32_t apic_addr;
    uint32_t flags;
    // Here is a bunch of packed ICS reside here back-to-back.
} ACPI_TABLE_PACKED acpi_madt_t;
//#define __acpi_madt(acpi_ptr)   ((acpi_madt_t*)__ptr(acpi_ptr))

typedef struct
{
    uint32_t apic_addr;
    acpi_apic_t* apic;
    acpi_ioapic_t* ioapic;
    acpi_intso_t** irq_exception;
} ACPI_TABLE_PACKED acpi_madt_toc_t;

//void madt_parse(acpi_madt_t* madt, acpi_context* toc);

#endif /* __LUNAIX_ACPI_MADT_H */
