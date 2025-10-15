#include "mmdetect.h"
#include "multiboot.h"
//#include "printf.h"

struct multiboot_mmap_entry {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed));

const char *get_memory_type_name(uint32_t type) {
    switch (type) {
        case 1: return "Available RAM";
        case 2: return "Reserved (maybe APIC)";
        case 3: return "ACPI Reclaimable";
        case 4: return "ACPI NVS";
        case 5: return "Bad Memory";
        default: return "Unknown";
    }
}

/*
int
mm_detect(struct multiboot * mbi)
{
        printf("memory detect!\n");
        parse_memory_map(mbi->mmap_addr, mbi->mmap_length);
}

void parse_memory_map(uint32_t mmap_addr, uint32_t mmap_length) {
    struct multiboot_mmap_entry *mmap = (struct multiboot_mmap_entry*)mmap_addr;
    uint32_t offset = 0;
    
    printf("=== Memory Map (mp) ===\n");
    
    while (offset < mmap_length) {
        printf("Addr: %x, Len: %x, Type: %d (%s)\n",
               mmap->addr, mmap->len, mmap->type,
               get_memory_type_name(mmap->type));
        
        // 检查多处理器相关内存区域
        if (mmap->addr >= 0xFEE00000 && mmap->addr < 0xFEF00000) {
            printf(" APIC  detected!\n");
        }
        
        if (mmap->type == 2 && mmap->addr >= 0xFEC00000 && mmap->addr < 0xFED00000) {
            printf(" I/O APIC  detected!\n");
        }
        
        offset += mmap->size + sizeof(mmap->size);
        mmap = (struct multiboot_mmap_entry*)(mmap_addr + offset);
    }
}


*/
 

/**
 * 解析Multiboot提供的内存映射信息
 * 针对32位系统做了适配
 */
void parse_memory_map(uint32_t mmap_addr, uint32_t mmap_length) {
    struct multiboot_mmap_entry *entry = (struct multiboot_mmap_entry*)mmap_addr;
    uint32_t current_offset = 0;
    
    printf("=== Memory Map (x86 32-bit) ===\n");
    printf("Entries start at: 0x%x, Total length: 0x%x\n", mmap_addr, mmap_length);
    
    while (current_offset < mmap_length) {
        // 对于32位系统，将64位地址拆分为高32位和低32位显示
        uint32_t addr_low = (uint32_t)(entry->addr & 0xFFFFFFFF);
        uint32_t addr_high = (uint32_t)(entry->addr >> 32);
        uint32_t len_low = (uint32_t)(entry->len & 0xFFFFFFFF);
        uint32_t len_high = (uint32_t)(entry->len >> 32);
        
        // 大多数32位系统只使用低32位地址
        printf("Addr: 0x%x", addr_low);
        if (addr_high != 0) {
            early_print(" (high: 0x%x)", addr_high);  // 仅当高32位非零时显示
        }
        
        early_print(", Len: 0x%x", len_low);
        if (len_high != 0) {
            printf(" (high: 0x%x)", len_high);
        }
        
        early_print(", Type: %d (%s)\n", entry->type, get_memory_type_name(entry->type));
        
        // 检测APIC相关内存区域 (在32位系统中的典型位置)
        if (addr_high == 0 && addr_low >= 0xFEE00000 && addr_low < 0xFEF00000) {
            early_print("  - Local APIC detected in this region\n");
        }
        
        // 检测I/O APIC相关内存区域
        if (entry->type == 2 && addr_high == 0 && 
            addr_low >= 0xFEC00000 && addr_low < 0xFED00000) {
            early_print("  - I/O APIC detected in this region\n");
        }
        
        // 移动到下一个条目
        current_offset += entry->size + sizeof(entry->size);
        entry = (struct multiboot_mmap_entry*)(mmap_addr + current_offset);
    }
}


int mm_detect_(struct multiboot *mbi) {
    if (!mbi) {
        early_print("Error: Invalid multiboot info structure\n");
        return -1;
    }
    
    early_print("Starting memory detection (x86 32-bit)...\n");
    
    if (mbi->mmap_addr == 0 || mbi->mmap_length == 0) {
        early_print("Warning: No memory map information available\n");
        return -1;
    }
    
    // 在32位系统中检查地址有效性
    if ((uint32_t)mbi->mmap_addr + mbi->mmap_length > 0xFFFFFFFF) {
        early_print("Warning: Memory map exceeds 32-bit address space\n");
        return -1;
    }
    
    parse_memory_map(mbi->mmap_addr, mbi->mmap_length);
    return 0;
}

// Multiboot 头文件中的定义
#define MULTIBOOT_MEMORY_AVAILABLE 1
#define MULTIBOOT_MEMORY_RESERVED 2

// 打印内存映射信息
void print_memory_map(struct multiboot_mmap_entry *mmap, uint32_t length) {
    uint32_t i = 0;
    
    early_print("Memory Map:\n");
    early_print("Address         Length           Type\n");
    early_print("--------------  --------------   ----\n");
    
    while (i < length) {
        early_print("0x%x 0x%x ",
               (unsigned long long)mmap->addr,
               (unsigned long long)mmap->len);
        
        switch (mmap->type) {
            case MULTIBOOT_MEMORY_AVAILABLE:
                early_print("Available RAM\n");
                break;
            case MULTIBOOT_MEMORY_RESERVED:
                early_print("Reserved\n");
                break;
            default:
                early_print("Type %u\n", mmap->type);
                break;
        }
        
        // 移动到下一个条目
        i += mmap->size + sizeof(mmap->size);
        mmap = (struct multiboot_mmap_entry*)((uint8_t*)mmap + mmap->size + sizeof(mmap->size));
    }
}

// 在 Multiboot 信息处理中使用
void mm_detect( uint32_t *mb_info) { 
    early_console_init();
    // 检查内存映射标志位
    if (!(mb_info[0] & 0x40)) {
        early_print("No memory map provided\n");
        return;
    }
    
    uint32_t mmap_length = mb_info[11];
    uint32_t mmap_addr = mb_info[12];
    
    print_memory_map((struct multiboot_mmap_entry*)mmap_addr, mmap_length);
}

// 简单的早期控制台初始化（使用VGA文本模式）
void early_console_init() {
    // 初始化VGA文本模式缓冲区
    // 这可以在分页前安全执行，因为它使用固定的物理地址
    volatile uint16_t *vga_buffer = (volatile uint16_t*)0xB8000;
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = (0x07 << 8) | ' ';
    }
}

// 早期打印函数（分页前使用）
void early_print(const char *str) {
    static int cursor_x = 0, cursor_y = 0;
    volatile uint16_t *vga_buffer = (volatile uint16_t*)0xB8000;
    
    while (*str) {
        if (*str == '\n') {
            cursor_x = 0;
            cursor_y++;
        } else {
            vga_buffer[cursor_y * 80 + cursor_x] = (0x07 << 8) | *str;
            cursor_x++;
            if (cursor_x >= 80) {
                cursor_x = 0;
                cursor_y++;
            }
        }
        str++;
    }
}
