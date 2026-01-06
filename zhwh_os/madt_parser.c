#include "types.h"
#include "madt.h"
#include "acpi.h"

//#include <lunaix/mm/valloc.h>

void
madt_parse(acpi_madt_t* madt,acpi_context* toc)
{
    //toc->madt.apic_addr = madt->apic_addr;

    // FUTURE: make madt.{apic,ioapic} as array or linked list.
    uint64_t ics_start = (uint64_t)madt + sizeof(acpi_madt_t);
    uint64_t ics_end = (uint64_t)madt + madt->header.length;

    // Cosidering only one IOAPIC present (max 24 pins)
    //toc->madt.irq_exception =(acpi_intso_t**)vcalloc(24, sizeof(acpi_intso_t*));

    //uint32_t so_idx = 0;

    int i=0,j=0,k=0;
    while (ics_start < ics_end) {
        acpi_ics_hdr_t* entry = (acpi_ics_hdr_t*)(ics_start);
        switch (entry->type) {
            case ACPI_MADT_LAPIC:
                toc->madt.apic = (acpi_apic_t*)entry;
                i++;
                if(i==1){printf("ACPI_MADT_LAPIC found ======\n");}
                         
                
                //printf("ACPI_MADT_LAPIC found ======\n");
                break;
            case ACPI_MADT_IOAPIC:
                j++;
                if(j==1){printf("ACPI_MADT_IOAPIC found ======\n");}
                        
                //printf("ACPI_MADT_IOAPIC found ======\n");
                toc->madt.ioapic = (acpi_ioapic_t*)entry;
                break;
            case ACPI_MADT_INTSO: {
                k++;
                if(k==1){printf("ACPI_MADT_INTSO found ======\n");}
                        
                //printf("ACPI_MADT_INTSO found ======\n");
                //acpi_intso_t* intso_tbl = __acpi_intso(entry);
                //toc->madt.irq_exception[intso_tbl->source] = intso_tbl;
                break;
            }
            default:
                break;
        }

        ics_start += entry->length;
    }
}
