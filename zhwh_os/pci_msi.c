/**
 * @file pci_msi.c
 * @brief PCI MSI (Message Signaled Interrupt) 支持
 *
 * 用于现代 PCIe 设备的中断，绕过传统的 IOAPIC
 */

#include "types.h"
#include "printf.h"
#include "io.h"

// PCI Capabilities
#define PCI_CAP_ID_MSI  0x05
#define PCI_STATUS_CAP_LIST  (1 << 4)

// MSI 寄存器偏移（相对于 capability 指针）
#define MSI_CAP_CTRL    0x02  // Message Control
#define MSI_CAP_ADDR    0x04  // Message Address (Low)
#define MSI_CAP_DATA    0x08  // Message Data
#define MSI_CAP_MASK    0x0C  // Mask Bits (可选)
#define MSI_CAP_PENDING 0x10  // Pending Bits (可选)



#define MSI_CAP_ADDR_LO 0x04
#define MSI_CAP_ADDR_HI 0x08   // only if 64-bit

#define MSI_CAP_DATA_32 0x08
#define MSI_CAP_DATA_64 0x0C


// MSI Control 位
#define MSI_CTRL_ENABLE (1 << 0)     // MSI Enable
#define MSI_CTRL_64BIT  (1 << 7)     // 64-bit Address Capable
#define MSI_CTRL_MASK   (1 << 8)     // Per-Vector Masking Capable

// MSI 地址（LAPIC）
#define MSI_ADDRESS_BASE 0xFEE00000

// 外部 PCI 读取函数
extern uint8_t pci_read_config_byte(unsigned bus, unsigned dev, unsigned fn, unsigned reg);
extern uint16_t pci_read_config_word(unsigned bus, unsigned dev, unsigned fn, unsigned reg);
extern uint32_t pci_read_config_dword(unsigned bus, unsigned dev, unsigned fn, unsigned reg);
extern void pci_write_config_word(unsigned bus, unsigned dev, unsigned fn, unsigned reg, uint16_t value);
extern void pci_write_config_dword(unsigned bus, unsigned dev, unsigned fn, unsigned reg, uint32_t value);

/**
 * @brief 启用 PCI 设备的 MSI 中断
 *
 * @param bus PCI 总线号
 * @param dev 设备号
 * @param func 功能号
 * @param vector 中断向量 (0x20-0xFF)
 * @return 成功返回 0，失败返回 -1
 */
int pci_enable_msi(unsigned bus, unsigned dev, unsigned func, uint8_t vector) {
    printf("\n");
    // printf("╔════════════════════════════════════════════════════════════╗\n");
    // printf("║              PCI MSI Configuration                          ║\n");
    // printf("╚════════════════════════════════════════════════════════════╝\n");

    // 1. 检查 Capabilities List
    uint16_t status = pci_read_config_word(bus, dev, func, 0x06);
    if (!(status & PCI_STATUS_CAP_LIST)) {
        printf("[MSI] ✗ No capabilities list\n");
        return -1;
    }
    printf("[MSI] ✓ Capabilities list present\n");

    // 2. 查找 MSI Capability
    uint8_t cap_ptr = pci_read_config_byte(bus, dev, func, 0x34) & 0xFC;
    printf("[MSI] Capabilities pointer = 0x%x\n", cap_ptr);

    uint8_t msi_cap = 0;
    while (cap_ptr != 0) {
        uint8_t cap_id = pci_read_config_byte(bus, dev, func, cap_ptr);
        printf("[MSI]   Cap at 0x%x: ID=0x%x", cap_ptr, cap_id);

        if (cap_id == PCI_CAP_ID_MSI) {
            msi_cap = cap_ptr;
            printf(" ✓ MSI found!\n");
            break;
        }
        printf("\n");

        // 下一个 capability
        cap_ptr = pci_read_config_byte(bus, dev, func, cap_ptr + 1);
        cap_ptr &= ~0x3;
    }

    if (msi_cap == 0) {
        printf("[MSI] ✗ MSI capability not found\n");
        return -1;
     }
    //else{
    //     while(1) {
    //             __asm__ volatile("hlt");
    //         }
    // }

    // 3. 读取 MSI Control
    uint16_t msi_ctrl = pci_read_config_word(bus, dev, func, msi_cap + MSI_CAP_CTRL);
    printf("[MSI] MSI Control = 0x%04x\n", msi_ctrl);
    printf("[MSI]   64-bit capable: %s\n", (msi_ctrl & MSI_CTRL_64BIT) ? "Yes" : "No");
    printf("[MSI]   Per-vector masking: %s\n", (msi_ctrl & MSI_CTRL_MASK) ? "Yes" : "No");
    printf("[MSI]   Currently enabled: %s\n", (msi_ctrl & MSI_CTRL_ENABLE) ? "Yes" : "No");

    // 4. 计算 MSI 地址
    // 格式: 0xFEE00000 | (LAPIC_ID << 12)
    // 🔥 关键：对于多核系统，需要指定目标 CPU 的 LAPIC ID
    extern uint64_t get_apic_base_32bit(void);
    extern uint8_t lapicid2(void);
    extern uint8_t logical_cpu_id(void);
    extern void map_4k_page(uint32_t phys_addr, uint32_t virt_addr, uint32_t flags);

    uint32_t lapic_id = (uint32_t)lapicid2();  // 获取当前 CPU 的 LAPIC ID
    uint64_t lapic_base = get_apic_base_32bit();  // 返回 0xFEE00900
    uint32_t msg_addr = (uint32_t)(lapic_base & 0xFFFFF000) | (lapic_id << 12);

    //uint32_t msg_addr = MSI_ADDRESS_BASE | (lapic_id << 12);

    // 🔥🔥🔥 关键修复：确保 MSI 目标地址已映射到页表
    // 原因：MSI 写的是物理地址，CPU 直接访问该物理地址时，页表必须有映射
    // 否则会触发 Page Fault
    #define PAGE_PRESENT  0x001
    #define PAGE_RW       0x002
    #define PAGE_PCD      0x010  // 禁用缓存（MMIO 必须）
    #define PAGE_PWT      0x008

    printf("[MSI] Ensuring MSI target address is mapped: 0x%x\n", msg_addr);

    // 映射 MSI 目标页（4KB）
    //map_4k_page(msg_addr, msg_addr, PAGE_PRESENT | PAGE_RW | PAGE_PCD | PAGE_PWT);
    printf("[MSI] MSI target address mapped successfully\n");

    //uint16_t msg_data = (uint16_t)vector;
    uint16_t msg_data = vector | (0 << 8); // Fixed delivery

    printf("[MSI] Current CPU LAPIC ID = %d\n", lapic_id);
    printf("[MSI] Message Address = 0x%08x (0xFEE00000 | %d << 12)\n",
           msg_addr, lapic_id);
    printf("[MSI] Message Data = 0x%04x (vector = 0x%02x, decimal %d)\n",
           msg_data, msg_data & 0xFF, msg_data & 0xFF);

    // ===== 新增：先 disable MSI =====
    msi_ctrl &= ~MSI_CTRL_ENABLE;
    pci_write_config_word(bus, dev, func, msi_cap + MSI_CAP_CTRL, msi_ctrl);
    printf("[MSI] MSI temporarily disabled\n");
    // ================================


    // 5. 写入 MSI 地址和数据
    //pci_write_config_dword(bus, dev, func, msi_cap + MSI_CAP_ADDR, msg_addr);

    if (msi_ctrl & MSI_CTRL_64BIT) {
        pci_write_config_dword(bus, dev, func, msi_cap + MSI_CAP_ADDR_LO, msg_addr);
        pci_write_config_dword(bus, dev, func, msi_cap + MSI_CAP_ADDR_HI, 0);
        pci_write_config_word (bus, dev, func, msi_cap + MSI_CAP_DATA_64, msg_data);
        printf("[MSI] 64-bit MSI mode configured\n");
    } else {
        pci_write_config_dword(bus, dev, func, msi_cap + MSI_CAP_ADDR_LO, msg_addr);
        pci_write_config_word (bus, dev, func, msi_cap + MSI_CAP_DATA_32, msg_data);
        printf("[MSI] 32-bit MSI mode configured\n");
    }


    // if (msi_ctrl & MSI_CTRL_64BIT) {
    //     // 64-bit 地址模式
    //     pci_write_config_dword(bus, dev, func, msi_cap + MSI_CAP_ADDR + 4, 0);
    //     pci_write_config_word(bus, dev, func, msi_cap + MSI_CAP_DATA + 4, msg_data);
    //     printf("[MSI] ✓ 64-bit MSI mode configured\n");
    // } else {
    //     // 32-bit 地址模式
    //     pci_write_config_word(bus, dev, func, msi_cap + MSI_CAP_DATA, msg_data);
    //     printf("[MSI] ✓ 32-bit MSI mode configured\n");
    // }

    // 6. 启用 MSI 并禁用 INTx
    // 先读取 PCI Command 寄存器
    uint16_t pci_cmd = pci_read_config_word(bus, dev, func, 0x04);
    printf("[MSI] PCI Command before: 0x%04x\n", pci_cmd);

    pci_cmd |= (1 << 2); // Bus Master Enable


    // 禁用 INTx (bit 10)
    pci_cmd |= (1 << 10);  // Interrupt Disable
    pci_write_config_word(bus, dev, func, 0x04, pci_cmd);
    printf("[MSI] INTx disabled (bit 10 set)\n");

    if (msi_ctrl & MSI_CTRL_MASK) {
        uint32_t mask_offset;

        if (msi_ctrl & MSI_CTRL_64BIT)
            mask_offset = msi_cap + 0x10; // 64bit: Mask Bits
        else
            mask_offset = msi_cap + 0x0C; // 32bit: Mask Bits

        uint32_t mask = pci_read_config_dword(bus, dev, func, mask_offset);
        printf("[MSI] Mask bits before = 0x%08x\n", mask);

        // unmask vector 0
        mask &= ~(1 << 0);
        pci_write_config_dword(bus, dev, func, mask_offset, mask);

        printf("[MSI] Mask bits after  = 0x%08x\n", mask);
    }


    // 启用 MSI
    msi_ctrl |= MSI_CTRL_ENABLE;
    pci_write_config_word(bus, dev, func, msi_cap + MSI_CAP_CTRL, msi_ctrl);
    printf("[MSI]  MSI Enabled\n");

    // 7. 验证
    msi_ctrl = pci_read_config_word(bus, dev, func, msi_cap + MSI_CAP_CTRL);
    pci_cmd = pci_read_config_word(bus, dev, func, 0x04);
    printf("[MSI] PCI Command after: 0x%04x (INTx %s)\n",
           pci_cmd, (pci_cmd & (1 << 10)) ? "disabled" : "enabled");

    if (msi_ctrl & MSI_CTRL_ENABLE) {
        // printf("╔════════════════════════════════════════════════════════════╗\n");
        // printf("║  ✓ MSI Configuration SUCCESS                                ║\n");
        // printf("║  Vector: 0x%02x (%d decimal)                               \n", vector, vector);
        // printf("║  INTx: Disabled                                             \n");
        // printf("╚════════════════════════════════════════════════════════════╝\n");
        printf("\n");
        return 0;
    } else {
        printf("[MSI]  Failed to enable MSI\n");
        return -1;
    }
}

/**
 * @brief 禁用 PCI 设备的 MSI 中断
 */
void pci_disable_msi(unsigned bus, unsigned dev, unsigned func) {
    uint8_t cap_ptr = pci_read_config_byte(bus, dev, func, 0x34) & 0xFC;

    while (cap_ptr != 0) {
        uint8_t cap_id = pci_read_config_byte(bus, dev, func, cap_ptr);
        if (cap_id == PCI_CAP_ID_MSI) {
            uint16_t msi_ctrl = pci_read_config_word(bus, dev, func, cap_ptr + MSI_CAP_CTRL);
            msi_ctrl &= ~MSI_CTRL_ENABLE;
            pci_write_config_word(bus, dev, func, cap_ptr + MSI_CAP_CTRL, msi_ctrl);
            printf("[MSI] Disabled\n");
            return;
        }
        cap_ptr = pci_read_config_byte(bus, dev, func, cap_ptr + 1) & 0xFC;
    }
}


// #define CONFIG_ADDRESS 0xCF8
// #define CONFIG_DATA    0xCFC

// extern volatile uint32_t *lapic;
// extern uint32_t lapicid(void);
// extern void map_4k_page(uint32_t phys_addr, uint32_t virt_addr, uint32_t flags);
// extern uint16_t pci_read16(unsigned bus, unsigned dev, unsigned fn, unsigned reg);

// #define MSI_ADDRESS_BASE 0xFEE00000
// #define PAGE_PRESENT 0x001
// #define PAGE_RW      0x002
// #define PAGE_PCD     0x010
// #define PAGE_PWT     0x008

// #define PCI_STATUS_CAP_LIST 0x10
// #define PCI_CAP_ID_MSI       0x05
// #define MSI_CAP_CTRL         0x2
// #define MSI_CAP_ADDR_LO      0x4
// #define MSI_CAP_ADDR_HI      0x8
// #define MSI_CAP_DATA_32      0x8
// #define MSI_CTRL_ENABLE      0x1
// #define MSI_CTRL_64BIT       0x80

// int pci_enable_msi_safe_1(unsigned bus, unsigned dev, unsigned fn, uint8_t vector)
// {
//     // 1. 检查 Capabilities List
//     uint16_t status = pci_read_config_word(bus, dev, fn, 0x06);
//     if (!(status & PCI_STATUS_CAP_LIST))
//         return -1;

//     // 2. 查找 MSI Capability
//     uint8_t cap_ptr = pci_read_config_byte(bus, dev, fn, 0x34) & 0xFC;
//     uint8_t msi_cap = 0;
//     while (cap_ptr != 0) {
//         uint8_t cap_id = pci_read_config_byte(bus, dev, fn, cap_ptr);
//         if (cap_id == PCI_CAP_ID_MSI) {
//             msi_cap = cap_ptr;
//             break;
//         }
//         cap_ptr = pci_read_config_byte(bus, dev, fn, cap_ptr + 1) & 0xFC;
//     }
//     if (msi_cap == 0)
//         return -1;

//     // 3. 读取 MSI Control
//     uint16_t msi_ctrl = pci_read_config_word(bus, dev, fn, msi_cap + MSI_CAP_CTRL);

//     // 4. Identity map LAPIC 64KB (覆盖所有 lapic_id)
//     for (uint32_t offset = 0; offset < 0x10000; offset += 0x1000) {
//         map_4k_page(MSI_ADDRESS_BASE + offset, MSI_ADDRESS_BASE + offset,
//                     PAGE_PRESENT | PAGE_RW | PAGE_PCD | PAGE_PWT);
//     }

//     // 5. 计算 MSI Address & Data
//     uint32_t lapic_id = lapicid();
//     uint32_t msg_addr = MSI_ADDRESS_BASE | (lapic_id << 12);
//     uint16_t msg_data = vector;  // Fixed delivery

//     // 6. 写 MSI Address & Data
//     if (msi_ctrl & MSI_CTRL_64BIT) {
//         pci_write_config_dword(bus, dev, fn, msi_cap + MSI_CAP_ADDR_LO, msg_addr);
//         pci_write_config_dword(bus, dev, fn, msi_cap + MSI_CAP_ADDR_HI, 0);
//         pci_write_config_word (bus, dev, fn, msi_cap + MSI_CAP_DATA_32, msg_data);
//     } else {
//         pci_write_config_dword(bus, dev, fn, msi_cap + MSI_CAP_ADDR_LO, msg_addr);
//         pci_write_config_word (bus, dev, fn, msi_cap + MSI_CAP_DATA_32, msg_data);
//     }

//     // 7. 禁用 INTx + 启用 Bus Master
//     uint16_t pci_cmd = pci_read_config_word(bus, dev, fn, 0x04);
//     pci_cmd |= (1 << 2) | (1 << 10);  // Bus Master + Interrupt Disable
//     pci_write_config_word(bus, dev, fn, 0x04, pci_cmd);

//     // 8. 启用 MSI
//     msi_ctrl |= MSI_CTRL_ENABLE;
//     pci_write_config_word(bus, dev, fn, msi_cap + MSI_CAP_CTRL, msi_ctrl);

//     return 0;
// }

// int pci_enable_msi_safe(unsigned bus, unsigned dev, unsigned fn, uint8_t vector)
// {
//     uint16_t status = pci_read_config_word(bus, dev, fn, 0x06);
//     if (!(status & PCI_STATUS_CAP_LIST))
//         return -1;

//     uint8_t cap_ptr = pci_read_config_byte(bus, dev, fn, 0x34) & 0xFC;
//     uint8_t msi_cap = 0;
//     while (cap_ptr != 0) {
//         uint8_t cap_id = pci_read_config_byte(bus, dev, fn, cap_ptr);
//         if (cap_id == PCI_CAP_ID_MSI) {
//             msi_cap = cap_ptr;
//             break;
//         }
//         cap_ptr = pci_read_config_byte(bus, dev, fn, cap_ptr + 1) & 0xFC;
//     }
//     if (msi_cap == 0)
//         return -1;

//     uint16_t msi_ctrl = pci_read_config_word(bus, dev, fn, msi_cap + MSI_CAP_CTRL);

//     uint32_t lapic_id = lapicid();
//     uint32_t msg_addr = MSI_ADDRESS_BASE | (lapic_id << 12);
//     uint16_t msg_data = vector;  // Fixed delivery, edge-triggered

//     if (msi_ctrl & MSI_CTRL_64BIT) {
//         pci_write_config_dword(bus, dev, fn, msi_cap + MSI_CAP_ADDR_LO, msg_addr);
//         pci_write_config_dword(bus, dev, fn, msi_cap + MSI_CAP_ADDR_HI, 0);
//         pci_write_config_word (bus, dev, fn, msi_cap + MSI_CAP_DATA_64, msg_data);
//     } else {
//         pci_write_config_dword(bus, dev, fn, msi_cap + MSI_CAP_ADDR_LO, msg_addr);
//         pci_write_config_word (bus, dev, fn, msi_cap + MSI_CAP_DATA_32, msg_data);
//     }

//     uint16_t pci_cmd = pci_read_config_word(bus, dev, fn, 0x04);
//     pci_cmd |= (1 << 2) | (1 << 10);  // Bus Master + Interrupt Disable (禁用 INTx)
//     pci_write_config_word(bus, dev, fn, 0x04, pci_cmd);

//     msi_ctrl |= MSI_CTRL_ENABLE;
//     pci_write_config_word(bus, dev, fn, msi_cap + MSI_CAP_CTRL, msi_ctrl);

//     return 0;
// }
