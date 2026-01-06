#include "types.h"
#include "multiboot.h"

// EBDA 中常见的数据结构偏移量
#define EBDA_SIZE_OFFSET       0x0000  // EBDA 大小
#define EBDA_PRINTER_OFFSET    0x0008  // 打印机端口
#define EBDA_EQUIPMENT_OFFSET  0x0010  // 设备列表
#define EBDA_MEMORY_OFFSET     0x0013  // 内存大小

struct multiboot_mmap_entry {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed));

// EBDA 通常在 0x80000-0x9FFFF 范围内
#define EBDA_BASE_ADDR 0x0009FC00  // 典型的 EBDA 起始地址
#define EBDA_MAX_ADDR  0x0009FFFF

void dump_ebda_contents(uint32_t ebda_addr, uint32_t length) {
    uint8_t *ebda = (uint8_t*)ebda_addr;
    uint32_t bytes_to_dump = (length > 256) ? 256 : length; // 限制转储大小
    
    printf("\n=== EBDA 内容转储 (前 %d 字节) ===\n", bytes_to_dump);
    
    for (uint32_t i = 0; i < bytes_to_dump; i += 16) {
        printf("0x%04X: ", i);
        
        // 十六进制转储
        for (uint32_t j = 0; j < 16; j++) {
            if (i + j < bytes_to_dump) {
                printf("%02X ", ebda[i + j]);
            } else {
                printf("   ");
            }
        }
        
        printf(" ");
        
        // ASCII 转储
        for (uint32_t j = 0; j < 16; j++) {
            if (i + j < bytes_to_dump) {
                uint8_t c = ebda[i + j];
                printf("%c", (c >= 32 && c <= 126) ? c : '.');
            } else {
                printf(" ");
            }
        }
        
        printf("\n");
    }
}

// 解析 EBDA 中的特定数据结构
void parse_ebda_structures(uint32_t ebda_addr) {
    uint8_t *ebda = (uint8_t*)ebda_addr;
    
    printf("\n=== EBDA 结构解析 ===\n");
    
    // 检查 EBDA 大小（通常在前几个字节）
    uint16_t ebda_size = *((uint16_t*)ebda);
    printf("EBDA 大小: %d 字节 (0x%04X)\n", ebda_size, ebda_size);
    
    // 查找可能的 BIOS 数据结构
    // 这里可以添加更多特定的解析逻辑
}


void parse_specific_ebda_data(uint64_t ebda_addr) {
    uint8_t *ebda = (uint8_t*)ebda_addr;
    
    printf("\n=== EBDA struct ===\n");
    
    // EBDA 大小
    uint16_t ebda_size = *((uint16_t*)&ebda[EBDA_SIZE_OFFSET]);
    printf("EBDA size: %d KB\n", ebda_size);
    
    // 设备列表
    uint16_t equipment = *((uint16_t*)&ebda[EBDA_EQUIPMENT_OFFSET]);
    printf("device flag: 0x%04X\n", equipment);
    
    // 内存信息
    uint16_t memory_kb = *((uint16_t*)&ebda[EBDA_MEMORY_OFFSET]);
    printf("basic mm: %d KB\n", memory_kb);
    
    // 打印机端口
    uint16_t lpt1 = *((uint16_t*)&ebda[EBDA_PRINTER_OFFSET]);
    uint16_t lpt2 = *((uint16_t*)&ebda[EBDA_PRINTER_OFFSET + 2]);
    uint16_t lpt3 = *((uint16_t*)&ebda[EBDA_PRINTER_OFFSET + 4]);
    
    printf("lpt port: LPT1=0x%03X, LPT2=0x%03X, LPT3=0x%03X\n", 
           lpt1, lpt2, lpt3);
}

uint16_t get_ebda_segment() {
    // 读取 40:0E 处的 EBDA 段地址
    uint16_t ebda_segment;
    
    // 在实模式下或通过其他方式读取
    // 这里只是示意，实际需要适当的上下文
    __asm__ volatile (
        "mov $0x40, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov 0x0E, %%ax\n"
        : "=a" (ebda_segment)
        :
        : "memory"
    );
    
    return ebda_segment;
}

uint32_t get_ebda_physical_address() {
    uint16_t segment = get_ebda_segment();
    return (uint32_t)segment << 4;  // 段地址 * 16 = 物理地址
}

uint64_t find_ebda_in_mmap(uint32_t mmap_addr, uint32_t mmap_length) {
    struct multiboot_mmap_entry *mmap = (struct multiboot_mmap_entry*)mmap_addr;
    uint32_t offset = 0;
    
    printf("===  search EBDA  ===\n");
    (void)printf("+ multiboot_mmap length is %d...",mmap->len);
    (void)printf("+ multiboot_mmap type is %d...",mmap->type);
    

    while (offset < mmap_length) {
        // 检查是否为保留内存（类型2），且地址在 EBDA 范围内
        if (mmap->type == 2 && 
            mmap->addr >= EBDA_BASE_ADDR && 
            mmap->addr < EBDA_MAX_ADDR) {
            
            printf("EBDA find:\n");
            printf("  addr: 0x%016llX\n", mmap->addr);
            printf("  size: 0x%016llX\n", mmap->len);
            printf("  type: %d (Reserved)\n", mmap->type);
            
            // 进一步验证是否为真正的 EBDA
            if (verify_ebda_region(mmap->addr)) {
                printf(" is EBDA area\n");
                //dump_ebda_contents(mmap->addr, mmap->len);
                break;
            }
        }
        
        offset += mmap->size + sizeof(mmap->size);
        mmap = (struct multiboot_mmap_entry*)(mmap_addr + offset);
    }

    return mmap->addr;
}

int verify_ebda_region(uint64_t addr) {
    // 简单的 EBDA 验证：检查是否有已知的 BIOS 数据结构
    uint8_t *ebda_ptr = (uint8_t*)addr;
    
    // EBDA 通常包含 BIOS 数据，可能会有特定的模式
    // 这里只是一个简单的示例验证
    return (ebda_ptr[0] != 0 && ebda_ptr[1] != 0); // 非零数据可能表示有效区域
}




void find_ebda_in_acpi(struct multiboot *mbi) {
    if (mbi->flags & (1 << 8)) {  // 检查配置表标志
        uint32_t *config_table = (uint32_t*)mbi->config_table;
        
        // 搜索 ACPI 表
        for (uint32_t i = 0; i < 1024; i += 16) {
            if (config_table[i] == 0x20445352 &&  // "RSD "
                config_table[i+1] == 0x20525450) { // "PTR "
                
                struct acpi_rsdp *rsdp = (struct acpi_rsdp*)&config_table[i];
                parse_acpi_for_ebda(rsdp);
                break;
            }
        }
    }
}

void parse_acpi_for_ebda(struct acpi_rsdp *rsdp) {
    // ACPI 表可能包含对 EBDA 的引用
    printf("ACPI RSDP at: %x\n", rsdp);
    
    // 某些 ACPI 表可能包含 EBDA 相关信息
    // 这里需要具体的 ACPI 表解析逻辑
}

