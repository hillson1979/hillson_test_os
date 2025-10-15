// acpi.c
#include "acpi.h"
#include "madt.h"
#include "printf.h"
#include "string.h"
#include "kmalloc_early.h"
#include "highmem_mapping.h"
#include "hardware_highmem.h"


// 设备内存的标志位
#define PTE_P (1 << 0) // Present
#define PTE_W (1 << 1) // Writeable
#define PTE_U (1 << 2) // User
#define PTE_PWT (1 << 3) // Write-Through
#define PTE_PCD (1 << 4) // Cache-Disable

#define DEVICE_FLAGS (PTE_P | PTE_W | PTE_PWT | PTE_PCD) 
// acpi_rsdt

static uint8_t lapic_ids[256] = {0}; // CPU core Local APIC IDs
static uint8_t ioapic_ids[256] = {0}; // CPU core Local APIC IDs
static uint16_t numcore = 0;         // number of cores detected
static uint16_t numioapic = 0;
static uint64_t lapic_ptr = 0;       // pointer to the Local APIC MMIO registers
static uint64_t ioapic_ptr[256] = {0};      // pointer to the IO APIC MMIO registers

static acpi_context* ctx = NULL;

acpi_context*
acpi_get_context()
{
    //assert_msg(ctx, "ACPI is not initialized");
    return ctx;
}

// 获取 RSDT 地址
void* get_rsdt_address(void* rsdp_ptr) {
    rsdp_t* rsdp = (rsdp_t*)rsdp_ptr;
    
    if (rsdp->revision < 2) {
        // ACPI 1.0 - 使用 RSDT
        printf("Using RSDT (ACPI 1.0)\n");
        return (void*)(uint32_t)rsdp->rsdt_address;
    } else {
        // ACPI 2.0+ - 使用 XSDT
        rsdp2_t* rsdp2 = (rsdp2_t*)rsdp_ptr;
        printf("Using XSDT (ACPI 2.0+)\n");
        return (void*)(uint32_t)rsdp2->xsdt_address;
    }
}

// 打印表信息
void print_table_info(acpi_sdt_header_t* header) {
    char signature[5] = {0};
    memcpy(signature, header->signature, 4);
    
    printf("Table: %s, Length: %u, Revision: %d\n", 
           signature, header->length, header->revision);
    printf("OEM ID: %s, Table ID: %s\n", 
           header->oemid, header->oem_table_id);
    /*
    // 验证表校验和
    if (verify_checksum(header, header->length)) {
        printf("Checksum: Valid\n");
    } else {
        printf("Checksum: Invalid!\n");
    } */

    printf("------\n");
}

// 枚举所有 ACPI 表
void enumerate_acpi_tables(void* rsdp_ptr) {

    ctx = kmalloc_early(sizeof(acpi_context));
    rsdp_t* rsdp = map_hardware_region(rsdp_ptr,sizeof(rsdp_t),"ACPI ..." );//(rsdp_t*)rsdp_ptr;
    
    void* sdt_address = get_rsdt_address(rsdp);
    printf("rsdt_address is %u\n",sdt_address);

    void *sdt_ptr=rsdp->rsdt_address;
    printf("rsdt_address 2 is %u\n",sdt_ptr);
    void *virt_addr=NULL;
    
    
    acpi_sdt_header_t *sdt_header=map_hardware_region(rsdp->rsdt_address,sizeof(acpi_sdt_header_t),"ACPI ..." );
    printf("RSDP virt address: %x\n",sdt_header );
    
    
    printf("Enumerating ACPI tables from %s...\n", sdt_header->signature);
    
    if (memcmp(sdt_header->signature, ACPI_SIGNATURE_RSDT, 4) == 0) {
        // RSDT - 32位地址
        rsdt_t* rsdt = (rsdt_t*)sdt_header;//sdt_ptr;
        uint32_t entry_count = (rsdt->header.length - sizeof(acpi_sdt_header_t)) / 4;
        
        printf("Found %u tables in RSDT:\n", entry_count);

        entry_count=800;
        
        for (uint32_t i = 0; i < entry_count; i++) {
            
            uint32_t *table_phys = rsdt->entries[i];
            if (table_phys == 0) continue;
            uint32_t *virt_addr=map_hardware_region(table_phys, 0x1000, "ACPI rsdt header");//
            
            //printf("table_phys found ======%u\n",virt_addr);
            
            //print_table_info((uint8_t*)virt_addr);//table);
            
            uint8_t *acpi_table=(uint8_t*)virt_addr;
            if (memcmp(acpi_table, "APIC", 4) == 0) {
                printf("APIC found ======\n");
                
                madt_parse((acpi_madt_t*) acpi_table, ctx);//acpi_madt_t
                break;
              }
            else if(memcmp(acpi_table, "FACP", 4)== 0 ){
                // FADT just a plain structure, no need to parse.
                  //ctx->fadt = *__acpi_fadt(sdthdr);
                printf("ACPI_FADT_SIG found ======\n");
                
                //break;
              }
            else if(memcmp(acpi_table, "MCFG", 4)== 0) {
                  //mcfg_parse(sdthdr, ctx);
                   printf("ACPI_MCFG_SIG found ======\n");
                   
                   //break;
               }
                 //
               
        }

        
        
    } else if (memcmp(sdt_header->signature, ACPI_SIGNATURE_XSDT, 4) == 0) {
        // XSDT - 64位地址
        xsdt_t* xsdt = (xsdt_t*)sdt_header;
        uint32_t entry_count = (xsdt->header.length - sizeof(acpi_sdt_header_t)) / 8;
        
        printf("Found %u tables in XSDT:\n", entry_count);
        
        /*
        for (uint32_t i = 0; i < entry_count; i++) {
            acpi_sdt_header_t* table = (acpi_sdt_header_t*)(uint32_t)xsdt->entries[i];
            uint32_t *virt_addr=map_hardware_region(table, 0x1000, "ACPI xsdt header");//
            print_table_info((acpi_sdt_header_t*)virt_addr);
        }*/
    }
}

// acpi_rsdp

// 在 BIOS 内存区域查找 RSDP
void* find_rsdp(void) {
    // BIOS 内存区域: 0x000E0000 - 0x000FFFFF
    uint8_t* start = (uint8_t*)0x000E0000;
    uint8_t* end = (uint8_t*)0x00100000;
    
    printf("Searching for RSDP in BIOS memory area...\n");
    
    for (uint8_t* ptr = start; ptr < end; ptr += 16) {
        // 检查签名 "RSD PTR "
        if (memcmp(ptr, ACPI_SIGNATURE_RSDP, 8) == 0) {
            printf("RSDP found at physical address: %x\n", ptr);
            return (void*)ptr;
        }
    }
    
    printf("RSDP not found in BIOS memory area\n");
    return NULL;
}

// 验证校验和
int verify_checksum(void* table, uint32_t length) {
    uint8_t* ptr = (uint8_t*)table;
    uint8_t sum = 0;
    
    for (uint32_t i = 0; i < length; i++) {
        sum += ptr[i];
    }
    
    return (sum == 0); // ACPI 校验和应该为0
}

// 验证 RSDP
int verify_rsdp(void* rsdp_ptr) {
    rsdp_t* rsdp = (rsdp_t*)rsdp_ptr;
    
    printf("Verifying RSDP...\n");
    printf("Signature: %s\n", rsdp->signature);
    printf("OEM ID: %s\n", rsdp->oemid);
    printf("Revision: %d\n", rsdp->revision);
    
    // 根据版本确定长度
    uint32_t length = (rsdp->revision < 2) ? sizeof(rsdp_t) : sizeof(rsdp2_t);
    
    if (verify_checksum(rsdp, length)) {
        printf("RSDP checksum valid\n");
        return 1;
    } else {
        printf("RSDP checksum invalid\n");
        return 0;
    }
}


int acpi_init() {
    printf("=== ACPI Table Scanner ===\n\n");
    
    // 1. 查找 RSDP
    void* rsdp = find_rsdp();
    if (!rsdp) {
        printf("Failed to find RSDP\n");
        return 1;
    }
    
    // 2. 验证 RSDP
    /*
    if (!verify_rsdp(rsdp)) {
        printf("RSDP verification failed\n");
        return 1;
    }*/
    
    // 3. 获取 RSDT/XSDT 地址
    void* sdt_address = get_rsdt_address(rsdp);
    if (!sdt_address) {
        printf("Failed to get RSDT/XSDT address\n");
        return 1;
    }
    
    printf("RSDT/XSDT found at physical address: %x\n", sdt_address);
    
    /*
    void* virt_address;
    virt_address=&sdt_address;
    early_remap_page(sdt_address,virt_address,DEVICE_FLAGS);
    printf("RSDT/XSDT found at new virt address: %x\n", virt_address);

    // 4. 验证 RSDT/XSDT
    
    acpi_sdt_header_t* sdt_header = (acpi_sdt_header_t*)virt_address;//sdt_address;
    
    
    if (!verify_checksum(sdt_header, sdt_header->length)) {
        printf( "RSDT/XSDT checksum invalid\n");
        return 1;
    }*/

    // 5. 枚举所有表
    enumerate_acpi_tables(rsdp);//sdt_address);//virt_address);
    
    printf("ACPI scanning completed successfully!\n");
    return 0;
}



