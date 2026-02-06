/**
 * @file e1000.c
 * @brief Intel E1000 网卡驱动实现
 *
 * 支持 Intel E1000 系列网卡
 * 常见于 QEMU 虚拟机 (82540EM)
 */

#include "net.h"
#include "e1000.h"
#include "../include/printf.h"
#include "../include/string.h"
#include "../include/kmalloc.h"
#include "../include/pci.h"
#include "../include/highmem_mapping.h"
#include "../include/page.h"
#include "../include/io.h"
#include "../include/pci_msi.h"

// 类型定义
#ifndef size_t
typedef uint32_t size_t;
#endif

extern void vga_setcolor(uint8_t fg, uint8_t bg);
#define SET_COLOR_RED()     vga_setcolor(4, 0)   // 红字黑底
#define SET_COLOR_GREEN()     vga_setcolor(2, 0)   // 绿字黑底

// E1000 网络设备
static net_device_t e1000_dev;
static e1000_priv_t e1000_priv;

// 全局 MMIO 基地址
volatile uint32_t *e1000_mmio_base = NULL;

// E1000 Interrupt Cause Read bits (必须在使用前定义)
// 注意：不同 E1000 型号位定义可能不同
#define E1000_ICR_TXDW     (1 << 0)   // TX Descriptor Written Back
#define E1000_ICR_RXDMT0   (1 << 4)   // RX Descriptor Minimum Threshold
#define E1000_ICR_RXO      (1 << 6)   // RX Overrun
#define E1000_ICR_RXT0     (1 << 7)   // RX Timer Interrupt (旧型号)
#define E1000_ICR_LSC      (1 << 5)   // Link Status Change
#define E1000_ICR_RXT0_ALT (1 << 31)  // RX Timer Interrupt (新型号，如 82540EM)

// IRQ 号
int e1000_irq = 0;

// MMIO 读写宏
#define e1000_read32(reg) \
    (*(volatile uint32_t *)(e1000_priv.mmio_base_virt + (reg)))
#define e1000_write32(reg, val) \
    (*(volatile uint32_t *)(e1000_priv.mmio_base_virt + (reg)) = (val))

// 兼容旧代码的宏
#define e1000_reg_read32(reg) e1000_read32(reg)
#define e1000_reg_write32(reg, val) e1000_write32(reg, val)

// 全局变量（兼容旧代码）
// 🔥 注意：已废弃，使用 e1000_priv.rx_buffers 代替
// static uint8_t *e1000_rx_buffers[E1000_NUM_RX_DESC];
static e1000_tx_desc_t *e1000_tx_desc;

// 🔥 全局变量用于loopback测试的同步
static volatile int loopback_rx_received = 0;
static volatile int loopback_tx_done = 0;

// DMA 方向定义（Linux 风格）
#define DMA_TO_DEVICE   0
#define DMA_FROM_DEVICE 1

// ==================== DMA Coherent 内存管理（使用 page.c 中的实现）===================
// DMA 函数声明（在 page.c 中实现）
extern void dma_map_region(void);
extern void *dma_alloc_coherent(size_t size, uint32_t *dma_handle);
extern void dma_free_coherent(void *cpu_addr, size_t size);


// 辅助函数：打印 MAC 地址（带前导零）
static void print_mac_addr(const uint8_t *mac) {
    printf("%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// 辅助函数：打印 IP 地址
static void print_ip_addr(uint32_t ip) {
    printf("%d.%d.%d.%d",
           (ip >> 24) & 0xFF,
           (ip >> 16) & 0xFF,
           (ip >> 8) & 0xFF,
           ip & 0xFF);
}

/**
 * @brief E1000 发送函数
 */
static int e1000_send(net_device_t *dev, uint8_t *data, uint32_t len) {
    if (!data || len == 0 || len > E1000_TX_BUF_SIZE) {
        printf("[e1000] Invalid send parameters\n");
        return -1;
    }

    // 🔥 调试：检查链路状态
    uint32_t status = e1000_read32(E1000_STATUS);
    if (!(status & E1000_STATUS_LU)) {
        printf("[e1000] WARNING: Link is DOWN! Packet may not be sent.\n");
    }

    // 检查 TX 描述符是否可用
    e1000_tx_desc_t *tx_desc = &e1000_priv.tx_desc[e1000_priv.tx_cur];

    // 🔥 调试：打印发送前描述符状态
    printf("[e1000] TX desc %d: status=0x%x (before send)\n",
           e1000_priv.tx_cur, tx_desc->status);

    // 检查描述符是否已完成 (DD 位)
    if (!(tx_desc->status & E1000_TXD_STAT_DD)) {
        printf("[e1000] TX descriptor %d busy (DD=0)\n", e1000_priv.tx_cur);
        return -1;
    }

    printf("[e1000] Sending %d bytes (desc %d)\n", len, e1000_priv.tx_cur);

    // 🔥 打印前 16 字节（以太网头）
    printf("[e1000] TX data: ");
    for (int i = 0; i < 16 && i < len; i++) {
        printf("%02x ", data[i]);
    }
    printf("\n");

    // 复制数据到发送缓冲区
    memcpy(e1000_priv.tx_buffers[e1000_priv.tx_cur], data, len);

    // 🔥 设置 TX 描述符（使用 DMA 物理地址）
    tx_desc->buffer_addr = e1000_priv.tx_buffers_dma[e1000_priv.tx_cur];
    tx_desc->length = (uint16_t)len;
    tx_desc->cmd = E1000_TXD_CMD_EOP | E1000_TXD_CMD_IFCS | E1000_TXD_CMD_RS;
    tx_desc->status = 0;  // 清除 DD 位

    // 内存屏障
    asm volatile("mfence" ::: "memory");

    // 更新 TDT (Tail) 寄存器
    e1000_priv.tx_cur = (e1000_priv.tx_cur + 1) % E1000_NUM_TX_DESC;
    e1000_write32(E1000_TDT, e1000_priv.tx_cur);

    // 🔥 调试：打印发送后的 TDT/TDH
    uint32_t tdt = e1000_read32(E1000_TDT);
    uint32_t tdh = e1000_read32(E1000_TDH);
    printf("[e1000] After send: TDT=%d, TDH=%d\n", tdt, tdh);

    return len;
}

/**
 * @brief E1000 接收函数（中断处理程序或轮询调用）
 * 处理所有可用的接收包（Intel 推荐方式）
 *
 * Intel 手册明确说明：
 * - 不要用 RDH/RDT 判断是否有包
 * - 只检查描述符的 DD 位
 * - 使用软件维护的 rx_cur 指针
 */
static void e1000_recv(net_device_t *dev) {
    uint32_t total_packets = 0;

    // 🔥 统计：记录调用次数
    e1000_priv.recv_call_count++;

    printf("[e1000] recv: rx_cur=%d\n", e1000_priv.rx_cur);

    // 🔥 Intel 推荐方式：从软件 rx_cur 开始，只检查 DD 位
    while (1) {
        // 🔥 内存屏障：确保读取到硬件最新写入的数据
        asm volatile("mfence" ::: "memory");

        e1000_rx_desc_t *rx_desc = &e1000_priv.rx_desc[e1000_priv.rx_cur];

        // ✅ 唯一可靠的判断：DD 位
        if (!(rx_desc->status & E1000_RXD_STAT_DD)) {
            // 描述符未就绪，没有更多包了
            printf("[e1000] RX desc %d not ready (DD=0)\n", e1000_priv.rx_cur);
            break;
        }

        printf("[e1000] RX desc %d: status=0x%x, len=%d\n",
               e1000_priv.rx_cur, rx_desc->status, rx_desc->length);

        // 获取有效长度
        uint16_t pkt_len = rx_desc->length;
        SET_COLOR_RED();
        // 打印 raw 数据（使用 e1000_priv.rx_buffers）
        for (int i = 0; i < pkt_len; i++) {
            printf("%02x ", e1000_priv.rx_buffers[e1000_priv.rx_cur][i]);
        }
        SET_COLOR_GREEN();
        printf("\n");

        // 检查长度
        if (rx_desc->length < ETH_HDR_LEN || rx_desc->length > ETH_MAX_FRAME) {
            printf("[e1000] Invalid packet length: %d, dropping\n", rx_desc->length);
            // 🔥 清除 DD 位，归还描述符给硬件
            rx_desc->status = 0;
            e1000_priv.rx_cur = (e1000_priv.rx_cur + 1) % E1000_NUM_RX_DESC;
            total_packets++;
            continue;
        }

        // 🔥 检查以太网类型（在驱动层就过滤掉无效包）
        uint8_t *data = e1000_priv.rx_buffers[e1000_priv.rx_cur];

        // 🔥 再次内存屏障，确保读取最新数据
        asm volatile("mfence" ::: "memory");

        uint16_t eth_type = (data[12] << 8) | data[13];

        // 有效的以太网类型：IPv4 (0x0800), ARP (0x0806), IPv6 (0x86DD), VLAN (0x8100)
        if (eth_type != 0x0800 && eth_type != 0x0806 &&
            eth_type != 0x86DD && eth_type != 0x8100) {
            printf("[e1000] DROP: Invalid EtherType 0x%04x (not IPv4/ARP/IPv6/VLAN)\n", eth_type);
            printf("[e1000]   data[12]=0x%02x, data[13]=0x%02x\n", data[12], data[13]);

            // 🔥 调试：显示前 16 字节
            printf("[e1000]   Raw data: ");
            for (int i = 0; i < 16 && i < rx_desc->length; i++) {
                printf("%02x ", data[i]);
            }
            printf("\n");

            // 🔥 清除 DD 位，归还描述符给硬件
            rx_desc->status = 0;
            e1000_priv.rx_cur = (e1000_priv.rx_cur + 1) % E1000_NUM_RX_DESC;
            total_packets++;
            continue;
        }

        // 传递给网络栈
        printf("[e1000] -> Calling net_rx_packet: len=%d, type=0x%04x\n", rx_desc->length, eth_type);
        net_rx_packet(dev, data, rx_desc->length);
        printf("[e1000] net_rx_packet returned\n");

        // 🔥 关键：必须清除 DD 位，归还描述符给硬件
        // Intel 手册：Software must clear the DD bit to make the descriptor available again
        rx_desc->status = 0;

        // 移动到下一个描述符
        e1000_priv.rx_cur = (e1000_priv.rx_cur + 1) % E1000_NUM_RX_DESC;
        total_packets++;
    }

    // 🔥 更新 RDT 寄存器（让硬件知道描述符可用了）
    if (total_packets > 0) {
        // RDT 应该指向"软件已经处理完的最后一个描述符"
        // 即：rx_cur - 1
        uint32_t new_rdt = (e1000_priv.rx_cur == 0)
            ? (E1000_NUM_RX_DESC - 1)
            : (e1000_priv.rx_cur - 1);

        printf("[e1000] Updating RDT to %d (processed %d packets)\n", new_rdt, total_packets);
        e1000_write32(E1000_RDT, new_rdt);

        e1000_priv.packets_processed += total_packets;
        printf("[e1000] Processed %d packets\n", total_packets);
    } else {
        e1000_priv.empty_recv_count++;
    }
}


/**
 * @brief E1000 中断处理
 */
void e1000_isr(void) {
    // 🔥 使用全局 dev 指针（因为中断处理程序无法传递参数）
    net_device_t *dev = &e1000_dev;

    // 🔥 统计中断次数
    e1000_priv.intr_count++;
    printf("[e1000] MSI ISR ENTER <<<\n");
    printf("[e1000] ISR: dev=0x%x, dev->name=%s\n", (uint32_t)dev, dev->name ? dev->name : "NULL");

    // 🔥 读取中断原因寄存器（读取会自动清除）
    uint32_t icr = e1000_read32(E1000_ICR);

    if (icr == 0) {
        // 不是我们的中断
        printf("[e1000] ISR: ICR=0 (spurious interrupt)\n");
        return;
    }

    // 🔥 调试：打印每次中断
    printf("[e1000] ISR: ICR=0x%08x\n", icr);

    // 🔥 详细的中断位分析
    if (icr & E1000_ICR_TXDW) printf("[e1000]   - TX Descriptor Written Back\n");
    if (icr & E1000_ICR_RXT0) printf("[e1000]   - RX Timer interrupt (bit 7)\n");
    if (icr & E1000_ICR_RXT0_ALT) printf("[e1000]   - RX Timer interrupt (bit 31)\n");
    if (icr & E1000_ICR_RXDMT0) printf("[e1000]   - RX minimum threshold\n");
    if (icr & E1000_ICR_RXO) printf("[e1000]   - RX overrun\n");
    if (icr & E1000_ICR_LSC) printf("[e1000]   - Link status change\n");

    // 🔥 如果 ICR=0，这是虚假中断（spurious interrupt）
    if (icr == 0) {
        printf("[e1000] WARNING: Spurious interrupt (ICR=0)\n");
        return;
    }

    // 🔥🔥 Loopback 测试：检查 TX 完成中断
    if (icr & E1000_ICR_TXDW) {
        // printf("[e1000] ISR: TX done interrupt!\n");
        loopback_tx_done = 1;  // 🔥 设置标志
    }

    // 处理接收中断（合并所有 RX 中断类型，包括 bit 7 和 bit 31）
    if (icr & (E1000_ICR_RXT0 | E1000_ICR_RXT0_ALT | E1000_ICR_RXDMT0 | E1000_ICR_RXO)) {
        SET_COLOR_RED();
        printf("[e1000] ISR: RX interrupt detected\n");
        SET_COLOR_GREEN();
    
        // 🔥🔥 Loopback 测试：检查是否是我们的测试包
        // 先调用接收函数（使用局部 dev 变量）
        e1000_recv(dev);

        // 🔥 设置标志：收到 RX 中断
        loopback_rx_received = 1;

        // 🔥 调试：显示收到的数据
        // printf("[e1000] ISR: Loopback RX flag set!\n");
    }

    // 链路状态变化
    if (icr & E1000_ICR_LSC) {
       // printf("[e1000] ISR: Link status change\n");
    }
    printf("[e1000] MSI ISR EXIT >>>\n");
}

/**
 * @brief 从 EEPROM 读取 MAC 地址
 */
static void e1000_read_mac_addr(void) {
    // 从 RAL/RAH 寄存器读取 MAC 地址
    uint32_t ral = e1000_read32(E1000_RAL(0));
    uint32_t rah = e1000_read32(E1000_RAH(0));

    e1000_priv.mac_addr[0] = ral & 0xFF;
    e1000_priv.mac_addr[1] = (ral >> 8) & 0xFF;
    e1000_priv.mac_addr[2] = (ral >> 16) & 0xFF;
    e1000_priv.mac_addr[3] = (ral >> 24) & 0xFF;
    e1000_priv.mac_addr[4] = rah & 0xFF;
    e1000_priv.mac_addr[5] = (rah >> 8) & 0xFF;

    // 🔥 更新全局 local_mac（用于 net_rx_packet 过滤）
    extern uint8_t local_mac[ETH_ALEN];
    memcpy(local_mac, e1000_priv.mac_addr, ETH_ALEN);

    printf("[e1000] MAC: ");
    print_mac_addr(e1000_priv.mac_addr);
    printf("\n");
}

/**
 * @brief 禁用 MSI，强制使用 Legacy INTx
 */
static void e1000_disable_msi(pci_dev_t *pci_dev) {
    printf("[e1000] disable MSI ...\n");
    uint8_t cap_ptr = pci_read_config_byte(pci_dev->bus_id,
                                       (pci_dev->dev_id >> 3) & 0x1F,
                                       pci_dev->fn_id, 0x34);

    while (cap_ptr) {
        uint8_t cap_id = pci_read_config_byte(pci_dev->bus_id,
                                              (pci_dev->dev_id >> 3) & 0x1F,
                                              pci_dev->fn_id, cap_ptr);
        if (cap_id == 0x05) {
            uint16_t msi_ctrl = pci_read_config_word(pci_dev->bus_id,
                                                     (pci_dev->dev_id >> 3) & 0x1F,
                                                     pci_dev->fn_id, cap_ptr + 2);
            msi_ctrl &= ~(1 << 0); // clear MSI Enable
            pci_write_config_word(pci_dev->bus_id,
                                  (pci_dev->dev_id >> 3) & 0x1F,
                                  pci_dev->fn_id, cap_ptr + 2, msi_ctrl);
            printf("[e1000] MSI disabled for %02x:%02x.%x\n",
                   pci_dev->bus_id, pci_dev->dev_id >> 3, pci_dev->fn_id);
            return;
        }

        // 读取 next capability pointer
        cap_ptr = pci_read_config_byte(pci_dev->bus_id,
                                       (pci_dev->dev_id >> 3) & 0x1F,
                                       pci_dev->fn_id, cap_ptr + 1);
    }
    printf("[e1000] No MSI capability found, using Legacy INTx\n");
    return;
}

/**
 * @brief 初始化 E1000 网卡
 */
int e1000_init_dev(pci_dev_t *pci_dev, const char *dev_name)
{
    uint32_t bar0;
    uint32_t mac_low, mac_high;
    uint8_t mac[6];

    printf("[e1000] Initializing device %s\n", dev_name);

    /* 1. 取 BAR0 (MMIO) */
    bar0 = pci_read_config_dword(pci_dev->bus_id, pci_dev->dev_id, pci_dev->fn_id, PCI_BAR0);
    bar0 &= ~0xF;  // mask flags

    printf("[e1000] MMIO phys base = 0x%x\n", bar0);

    // 映射物理地址到虚拟地址
    uint32_t mmio_size = 0x20000;  // 128KB should be enough for E1000
    void *mmio_virt = map_highmem_physical(bar0, mmio_size, 0);

    if (mmio_virt == NULL) {
        printf("[e1000] ERROR: Failed to map MMIO region!\n");
        return -1;
    }

    e1000_mmio_base = (volatile uint32_t *)mmio_virt;
    e1000_priv.mmio_base = bar0;
    e1000_priv.mmio_base_virt = (uint32_t)mmio_virt;

    printf("[e1000] MMIO virt base = 0x%x\n", (uint32_t)mmio_virt);

    /* 2. 打开 PCI bus mastering */
    uint16_t cmd = pci_read_config_word(pci_dev->bus_id, pci_dev->dev_id, pci_dev->fn_id, PCI_COMMAND);
    cmd |= (1 << 2); // Bus Master Enable
    pci_write_config_word(pci_dev->bus_id, pci_dev->dev_id, pci_dev->fn_id, PCI_COMMAND, cmd);

    /* 2.5 读取并配置中断 */
    printf("[e1000] E1000 82579LM detected!\n");

    //e1000_disable_msi(pci_dev);

    // 🔥 从 PCI 配置空间读取 Interrupt Pin（offset 0x3C）
    uint32_t irq_info = pci_read_config_dword(pci_dev->bus_id, pci_dev->dev_id, pci_dev->fn_id, 0x3C);
    uint8_t interrupt_pin = (irq_info >> 8) & 0xFF;  // Interrupt Pin (1=INTA, 2=INTB, etc)
    uint8_t interrupt_line = irq_info & 0xFF;        // Interrupt Line (IRQ，由 BIOS 填写)

    printf("[e1000] Interrupt Pin: %d (1=INTA, 2=INTB, 3=INTC, 4=INTD)\n", interrupt_pin);
    printf("[e1000] Interrupt Line (from BIOS): %d\n", interrupt_line);

    e1000_priv.irq = interrupt_line;
    if (interrupt_line != 0 && interrupt_line != 0xFF) {
        printf("[e1000] Using IRQ from BIOS: %d\n", e1000_priv.irq);
    } else {
        e1000_priv.irq = 11;  // 默认值
        printf("[e1000] BIOS didn't configure IRQ, using default: %d\n", e1000_priv.irq);
    }

    /* 3. Reset 网卡 */
    e1000_reg_write32(E1000_CTRL, E1000_CTRL_RST);
    for (volatile int i = 0; i < 1000000; i++);

    printf("[e1000] Reset done\n");

    /* 4. 等待 EEPROM / NVM auto-load */
    uint32_t status;
    int timeout = 10000;
    int nvm_done = 0;

    // 检查设备类型，使用不同的等待策略
    uint16_t device_id = pci_dev->header.device_id;

    printf("[e1000] Device ID: 0x%x\n", device_id);

    do {
        status = e1000_reg_read32(E1000_STATUS);

        // 对于旧款 E1000 (82540EM, 82545EM, 82546EB)，检查 bit 5
        if (device_id == 0x100E || device_id == 0x1004 || device_id == 0x1010) {
            if (status & (1 << 5)) {  // EEPROM_DONE
                nvm_done = 1;
                printf("[e1000] EEPROM auto-load done (bit 5)\n");
                break;
            }
        }
        // 对于 Intel 82579LM，检查 bit 19 (NVM_DONE)
        else if (device_id == 0x1502) {
            if (status & E1000_STATUS_NVM_DONE) {  // 0x00080000
                nvm_done = 1;
                printf("[e1000] NVM auto-load done (bit 19)\n");
                break;
            }
        }
        // 通用方法：尝试两种标志
        else {
            if (status & (1 << 5)) {
                nvm_done = 1;
                printf("[e1000] EEPROM auto-load done (bit 5)\n");
                break;
            }
            if (status & E1000_STATUS_NVM_DONE) {
                nvm_done = 1;
                printf("[e1000] NVM auto-load done (bit 19)\n");
                break;
            }
        }
    } while (--timeout);

    if (!timeout && !nvm_done) {
        printf("[e1000] WARNING: NVM/EEPROM auto-load timeout (status=0x%x)\n", status);
        printf("[e1000] Continuing anyway...\n");
        // 不返回错误，继续尝试初始化
    }

    /* 5. 读取 MAC 地址 (RAR0) */
    mac_low  = e1000_reg_read32(E1000_RAL(0));
    mac_high = e1000_reg_read32(E1000_RAH(0));

    mac[0] = mac_low & 0xFF;
    mac[1] = (mac_low >> 8) & 0xFF;
    mac[2] = (mac_low >> 16) & 0xFF;
    mac[3] = (mac_low >> 24) & 0xFF;
    mac[4] = mac_high & 0xFF;
    mac[5] = (mac_high >> 8) & 0xFF;

    // 检查 MAC 地址是否有效（不全为0或全为F）
    int mac_valid = 1;
    if ((mac_low == 0 || mac_low == 0xFFFFFFFF) &&
        (mac_high == 0 || (mac_high & 0xFFFF) == 0xFFFF)) {
        mac_valid = 0;
    }

    if (!mac_valid) {
        printf("[e1000] WARNING: Invalid MAC address from RAR0\n");
        printf("[e1000] RAL=0x%x, RAH=0x%x\n", mac_low, mac_high);

        // 使用临时 MAC 地址 (Intel OUI: 00:15:17:xx:xx:xx)
        mac[0] = 0x00;
        mac[1] = 0x15;
        mac[2] = 0x17;
        mac[3] = 0x00;
        mac[4] = 0x00;
        mac[5] = 0x01;

        printf("[e1000] Using temporary MAC: ");
        print_mac_addr(mac);
        printf("\n");

        // 将临时 MAC 地址写回 RAR0
        e1000_reg_write32(E1000_RAL(0),
            mac[0] | (mac[1] << 8) | (mac[2] << 16) | (mac[3] << 24));
        e1000_reg_write32(E1000_RAH(0),
            mac[4] | (mac[5] << 8) | 0x8000);  // Bit 15 = Address Valid
    } else {
        printf("[e1000] MAC = ");
        print_mac_addr(mac);
        printf("\n");
    }

    // 🔥 复制 MAC 地址到 e1000_priv
    memcpy(e1000_priv.mac_addr, mac, ETH_ALEN);

    // 🔥 复制 MAC 地址到 e1000_dev
    memcpy(e1000_dev.mac_addr, mac, ETH_ALEN);

    // 🔥 复制 MAC 地址到全局 local_mac（用于 net_rx_packet 过滤）
    extern uint8_t local_mac[ETH_ALEN];
    memcpy(local_mac, mac, ETH_ALEN);

    // 🔥 验证：打印复制后的 MAC
    printf("[e1000] MAC copied to e1000_priv.mac_addr: ");
    print_mac_addr(e1000_priv.mac_addr);
    printf("\n");
    printf("[e1000] MAC copied to e1000_dev.mac_addr:   ");
    print_mac_addr(e1000_dev.mac_addr);
    printf("\n");
    printf("[e1000] MAC copied to local_mac:             ");
    print_mac_addr(local_mac);
    printf("\n");
    printf("[e1000] MAC configuration verified\n");

    /* 6. 初始化 RX ring（使用 Linux 风格 DMA API）*/
    printf("[e1000] Initializing RX ring with dma_alloc_coherent\n");

    // 分配 RX 描述符数组
    uint32_t rx_desc_dma;
    e1000_priv.rx_desc = dma_alloc_coherent(
        sizeof(e1000_rx_desc_t) * E1000_NUM_RX_DESC,
        &rx_desc_dma
    );

    if (!e1000_priv.rx_desc) {
        printf("[e1000] ERROR: Failed to allocate RX descriptors\n");
        return -1;
    }

    // 清空描述符并设置缓冲区
    for (int i = 0; i < E1000_NUM_RX_DESC; i++) {
        memset(&e1000_priv.rx_desc[i], 0, sizeof(e1000_rx_desc_t));

        // 分配 RX 缓冲区
        uint32_t buf_dma;
        e1000_priv.rx_buffers[i] = dma_alloc_coherent(E1000_RX_BUF_SIZE, &buf_dma);

        if (!e1000_priv.rx_buffers[i]) {
            printf("[e1000] ERROR: Failed to allocate RX buffer %d\n", i);
            return -1;
        }

        // 🔹 清空 RX buffer，避免残留垃圾
        memset(e1000_priv.rx_buffers[i], 0, E1000_RX_BUF_SIZE);

        // 设置缓冲区物理地址
        e1000_priv.rx_desc[i].buffer_addr = buf_dma;

        // 调试：打印前几个描述符的信息
        if (i < 3) {
            printf("[e1000] RX desc %d: buf_virt=0x%x, buf_dma=0x%x\n",
                   i, (uint32_t)e1000_priv.rx_buffers[i], buf_dma);
        }
        // 🔹 确保描述符状态位初始化为 0 (DD=0)
        e1000_priv.rx_desc[i].status = 0;
    }

    printf("[e1000] RX desc array: virt=0x%x, dma=0x%x\n",
           (uint32_t)e1000_priv.rx_desc, rx_desc_dma);

    // 设置 RX 描述符寄存器
    e1000_reg_write32(E1000_RDBAL, rx_desc_dma & 0xFFFFFFFF);
    e1000_reg_write32(E1000_RDBAH, 0);
    e1000_reg_write32(E1000_RDLEN, E1000_NUM_RX_DESC * sizeof(e1000_rx_desc_t));
    e1000_reg_write32(E1000_RDH, 0);
    e1000_reg_write32(E1000_RDT, E1000_NUM_RX_DESC - 1);

    /* 7. 初始化 TX ring（使用 Linux 风格 DMA API）*/
    printf("[e1000] Initializing TX ring with dma_alloc_coherent\n");

    // 分配 TX 描述符数组
    uint32_t tx_desc_dma;
    e1000_priv.tx_desc = dma_alloc_coherent(
        sizeof(e1000_tx_desc_t) * E1000_NUM_TX_DESC,
        &tx_desc_dma
    );

    if (!e1000_priv.tx_desc) {
        printf("[e1000] ERROR: Failed to allocate TX descriptors\n");
        return -1;
    }

    // 初始化 TX 描述符
    for (int i = 0; i < E1000_NUM_TX_DESC; i++) {
        memset(&e1000_priv.tx_desc[i], 0, sizeof(e1000_tx_desc_t));
        e1000_priv.tx_desc[i].status = (1 << 0); // DD
    }

    printf("[e1000] TX desc array: virt=0x%x, dma=0x%x\n",
           (uint32_t)e1000_priv.tx_desc, tx_desc_dma);

    // 🔥 分配 TX 缓冲区（使用 DMA coherent memory）
    for (int i = 0; i < E1000_NUM_TX_DESC; i++) {
        e1000_priv.tx_buffers[i] = dma_alloc_coherent(E1000_TX_BUF_SIZE, &e1000_priv.tx_buffers_dma[i]);

        if (!e1000_priv.tx_buffers[i]) {
            printf("[e1000] ERROR: Failed to allocate TX buffer %d\n", i);
            return -1;
        }

        // 清空 TX buffer
        memset(e1000_priv.tx_buffers[i], 0, E1000_TX_BUF_SIZE);

        // 调试：打印前几个描述符的信息
        if (i < 3) {
            printf("[e1000] TX buffer %d: virt=0x%x, dma=0x%x\n",
                   i, (uint32_t)e1000_priv.tx_buffers[i], e1000_priv.tx_buffers_dma[i]);
        }
    }

    // 设置 TX 描述符寄存器
    e1000_reg_write32(E1000_TDBAL, tx_desc_dma & 0xFFFFFFFF);
    e1000_reg_write32(E1000_TDBAH, 0);
    e1000_reg_write32(E1000_TDLEN, E1000_NUM_TX_DESC * sizeof(e1000_tx_desc_t));
    e1000_reg_write32(E1000_TDH, 0);
    e1000_reg_write32(E1000_TDT, 0);

    /* 8. 设置 RCTL */
    /* 🔥 82579LM 必须确保使用 Legacy RX Descriptor 格式 (DTYP=0) */
    /* 🔥 关闭混杂模式：移除 UPE 和 MPE 标志 */
    e1000_reg_write32(E1000_RCTL,
        E1000_RCTL_EN  |      // Enable RX
        E1000_RCTL_SBP |      // Save Bad Packets
        // E1000_RCTL_UPE |  // ❌ Unicast Promiscuous (混杂模式) - 已移除
        // E1000_RCTL_MPE |  // ❌ Multicast Promiscuous - 已移除
        E1000_RCTL_BAM |      // Broadcast Accept Mode
        E1000_RCTL_SECRC |    // Strip CRC
        E1000_RCTL_BSIZE_2048  // Buffer size 2048
        /* DTYP bits [11:10] = 00 (Legacy descriptor, 默认) */
    );

    /* 9. 设置 TCTL */
    e1000_reg_write32(E1000_TCTL,
        E1000_TCTL_EN |
        E1000_TCTL_PSP |
        (0x10 << E1000_TCTL_CT_SHIFT) |
        (0x40 << E1000_TCTL_COLD_SHIFT)
    );

    /* 10. 启用中断 */
    printf("[e1000] Enabling interrupts\n");

    // 清除所有挂起的中断
    e1000_reg_write32(E1000_ICR, 0xFFFFFFFF);

    // 🔥 启用我们需要的中断：
    // - TXDW: 发送完成
    // - RXDMT0: RX 描述符阈值（队列快满了）
    // - RXT0: RX 定时器中断
    // 中断使能
    #define IMS_RXT0 0x00000040  // RX Timer
    #define IMS_RXDMT0 0x00000020 // RX Descriptor Minimum Threshold
    #define IMS_TXDW 0x00000001   // TX Descriptor Written Back
    #define IMS_LSC 0x00000010    // Link Status Change

    e1000_reg_write32(E1000_IMS, IMS_RXT0 | IMS_RXDMT0 | IMS_TXDW | IMS_LSC);

    e1000_reg_write32(E1000_IMS,
        E1000_ICR_TXDW |      // TX Descriptor Written Back
        E1000_ICR_RXDMT0 |    // RX Descriptor Minimum Threshold
        E1000_ICR_RXT0  |      // RX Timer Interrupt
        E1000_ICR_LSC    // link
    );

    printf("[e1000] Interrupts enabled (TXDW, RXDMT0, RXT0)\n");
    printf("[e1000] IMS = 0x%08x\n", e1000_read32(E1000_IMS));

    /* 启用 MSI 中断 */
    #define E1000_MSI_VECTOR 0x24  // MSI 中断向量

    printf("[e1000] ===== Enabling MSI (Message Signaled Interrupt) =====\n");

    // 🔥 关键诊断：打印 APIC BASE MSR
    uint32_t eax, edx;
    __asm__ volatile ("rdmsr" : "=a"(eax), "=d"(edx) : "c"(0x1B));
    uint64_t apic_base = ((uint64_t)edx << 32) | eax;
    
    printf("[e1000] APIC BASE MSR = 0x%x\n", (unsigned long long)apic_base);
    printf("[e1000]   Bit 11 (Enable) = %d %s\n",
       (int)((apic_base >> 11) & 1),
       ((apic_base >> 11) & 1) ? " LAPIC Enabled" : " LAPIC Disabled");

    // uint64_t apic_base;
    // __asm__ volatile ("rdmsr" : "=a"(apic_base) : "c"(0x1B));
    // printf("[e1000] APIC BASE MSR = 0x%llx\n", apic_base);
    // printf("[e1000]   Bit 11 (Enable) = %d %s\n",
    //        (int)((apic_base >> 11) & 1),
    //        ((apic_base >> 11) & 1) ? "✓ LAPIC Enabled" : "✗ LAPIC Disabled");

    int msi_ret = pci_enable_msi(pci_dev->bus_id, pci_dev->dev_id,
                                  pci_dev->fn_id, E1000_MSI_VECTOR);
    //extern void pci_print_command(unsigned bus, unsigned dev, unsigned fn) ;
    //pci_print_command(pci_dev->bus_id, pci_dev->dev_id,pci_dev->fn_id);
    
    if (msi_ret == 0) {
        printf("[e1000]  MSI enabled successfully!\n");
        printf("[e1000] Using MSI vector 0x%02x instead of legacy IRQ %d\n",
               E1000_MSI_VECTOR, e1000_priv.irq);

        // 🔥 关键诊断：打印 MSI 配置
        printf("[e1000] ===== MSI Configuration Summary =====\n");
        printf("[e1000] MSI addr = 0xFEE00000 (LAPIC base)\n");
        printf("[e1000] MSI data = 0x%04x (vector = 0x%02x)\n",
               E1000_MSI_VECTOR, E1000_MSI_VECTOR);
        printf("[e1000] Path: E1000 -> Write(0xFEE00000, 0x%04x) ->  LAPIC -> CPU\n",
               E1000_MSI_VECTOR);
        // extern void ioapicenable(int irq, int cpu);
        // printf("[e1000] Enabling IRQ %d in IOAPIC (routing to CPU 0)...\n", 64);
        // ioapicenable(64, 0);
        // printf("[e1000] E1000_MSI_VECTOR %d enabled in IOAPIC\n", 64);
        printf("[e1000] ========================================\n");
    } else {
        printf("[e1000] ✗ MSI failed, falling back to IOAPIC IRQ %d\n", e1000_priv.irq);

        /* 注册中断处理函数到 IOAPIC */
        extern void ioapicenable(int irq, int cpu);
        printf("[e1000] Enabling IRQ %d in IOAPIC (routing to CPU 0)...\n", e1000_priv.irq);
        ioapicenable(e1000_priv.irq, 0);
        printf("[e1000] IRQ %d enabled in IOAPIC\n", e1000_priv.irq);
    }
    printf("[e1000] =======================================================\n");
    // 注意: mmio_base 和 mmio_base_virt 已经在前面初始化了
    e1000_priv.tx_cur = 0;
    e1000_priv.rx_cur = 0;

    // 初始化私有数据（已经在 DMA 分配时设置好了）

    // 初始化网络设备结构
    memset(&e1000_dev, 0, sizeof(e1000_dev));
    strcpy(e1000_dev.name, dev_name);
    memcpy(e1000_dev.mac_addr, mac, 6);
    e1000_dev.mtu = ETH_MTU;
    e1000_dev.send = e1000_send;
    e1000_dev.recv = NULL;
    e1000_dev.ioctl = NULL;
    e1000_dev.priv = &e1000_priv;
    e1000_dev.pci_dev = pci_dev;

    // 注册到网络核心
    if (net_device_register(&e1000_dev) < 0) {
        printf("[e1000] Failed to register device\n");
        return -1;
    }

    printf("[e1000] Device registered as %s\n", dev_name);

    //msi_debug_flow(pci_dev->bus_id, pci_dev->dev_id, pci_dev->fn_id, E1000_MSI_VECTOR, 0);

    // 🔥🔥 最终验证：在初始化完成时打印所有 MAC 配置
    printf("\n[e1000] ========== FINAL MAC VERIFICATION ==========\n");
    printf("[e1000] HW RAL(0):         0x%08x\n", e1000_read32(E1000_RAL(0)));
    printf("[e1000] HW RAH(0):         0x%08x\n", e1000_read32(E1000_RAH(0)));
    printf("[e1000] e1000_priv.mac:    ");
    print_mac_addr(e1000_priv.mac_addr);
    printf("\n[e1000] e1000_dev.mac:     ");
    print_mac_addr(e1000_dev.mac_addr);
    printf("\n[e1000] global local_mac:  ");
    extern uint8_t local_mac[ETH_ALEN];
    print_mac_addr(local_mac);
    printf("\n[e1000] ==============================================\n\n");

    // 🔥 最终 RX 状态检查
    printf("[e1000] ========== FINAL RX STATUS ==========\n");
    printf("[e1000] RCTL   = 0x%08x (EN=%d)\n", e1000_read32(E1000_RCTL),
           (e1000_read32(E1000_RCTL) & E1000_RCTL_EN) ? 1 : 0);
    printf("[e1000] RDH    = %d\n", e1000_read32(E1000_RDH));
    printf("[e1000] RDT    = %d\n", e1000_read32(E1000_RDT));
    printf("[e1000] RX desc 0 status = 0x%04x\n", e1000_priv.rx_desc[0].status);
    printf("[e1000] ===========================================\n\n");

    return 0;
}

/**
 * @brief E1000 探测函数
 */
int e1000_probe(pci_dev_t **devices, unsigned num_devices, const char *dev_name) {
    printf("[e1000] Probing for E1000...\n");

    for (unsigned i = 0; i < num_devices; i++) {
        pci_dev_t *dev = devices[i];

        if (dev->header.vendor_id == E1000_VENDOR_ID &&
            (dev->header.device_id == E1000_DEVICE_ID ||
             dev->header.device_id == E1000_DEVICE_ID_I82545 ||
             dev->header.device_id == E1000_DEVICE_ID_I82546 ||
             dev->header.device_id == E1000_DEVICE_ID_I82579LM)) {  // 🔥 支持 82579LM

            printf("[e1000] Found E1000 device!\n");
            printf("[e1000]   Bus: %d, Device: %d, Function: %d\n",
                   dev->bus_id, dev->dev_id, dev->fn_id);
            printf("[e1000]   Vendor: 0x%x, Device: 0x%x\n",
                   dev->header.vendor_id, dev->header.device_id);

            // 初始化设备
            if (e1000_init_dev(dev, dev_name) < 0) {
                printf("[e1000] Failed to initialize device\n");
                continue;
            }

            printf("[e1000] Device initialized successfully\n");
            return 0;
        }
    }

    printf("[e1000] No E1000 device found\n");
    return -1;
}

/**
 * @brief E1000 初始化（从 PCI 设备列表探测）
 */
int e1000_init(const char *dev_name) {
    printf("[e1000] E1000 driver init\n");

    // 获取 PCI 设备列表
    pci_dev_t **devices = pci_get_devices();

    // 统计设备数量
    unsigned num_devices = 0;
    while (devices[num_devices] != NULL) {
        num_devices++;
    }

    printf("[e1000] Found %d PCI devices\n", num_devices);

    return e1000_probe(devices, num_devices, dev_name);
}

/**
 * @brief E1000 轮询接收函数
 */
void e1000_poll_rx(net_device_t *dev) {
    e1000_recv(dev);
}

/**
 * @brief E1000 发送包函数 (兼容旧代码)
 */
static int e1000_send_packet(net_device_t *dev, uint8_t *data, uint32_t len) {
    return e1000_send(dev, data, len);
}

/**
 * @brief E1000 完整寄存器转储
 */
void e1000_dump_regs(void) {

    // printf("sending IPI...\n");
    // extern uint8_t lapicid2(void);
    // extern void lapic_send_ipi_(uint8_t apicid, uint8_t vector);
    // lapic_send_ipi_(lapicid2(), 36);
    
    printf("[e1000] === Register Dump ===\n");
    printf("[e1000] CTRL   = 0x%08x\n", e1000_read32(E1000_CTRL));
    uint32_t status = e1000_read32(E1000_STATUS);
    printf("[e1000] STATUS = 0x%08x\n", status);

    // 🔥 检查链路状态
    if (status & E1000_STATUS_LU) {
        printf("[e1000] Link: UP ✅\n");
        if (status & E1000_STATUS_SPEED_1000) {
            printf("[e1000] Speed: 1000 Mbps\n");
        } else if (status & E1000_STATUS_SPEED_100) {
            printf("[e1000] Speed: 100 Mbps\n");
        } else {
            printf("[e1000] Speed: 10 Mbps\n");
        }
    } else {
        printf("[e1000] Link: DOWN ❌ (no cable connected or link partner not ready)\n");
    }
    printf("[e1000] RCTL   = 0x%08x\n", e1000_read32(E1000_RCTL));
    printf("[e1000] TCTL   = 0x%08x\n", e1000_read32(E1000_TCTL));
    printf("[e1000] RDBAL  = 0x%08x\n", e1000_read32(E1000_RDBAL));
    printf("[e1000] RDBAH  = 0x%08x\n", e1000_read32(E1000_RDBAH));
    printf("[e1000] RDLEN  = 0x%08x\n", e1000_read32(E1000_RDLEN));
    printf("[e1000] RDH    = 0x%08x\n", e1000_read32(E1000_RDH));
    printf("[e1000] RDT    = 0x%08x\n", e1000_read32(E1000_RDT));
    printf("[e1000] TDBAL  = 0x%08x\n", e1000_read32(E1000_TDBAL));
    printf("[e1000] TDBAH  = 0x%08x\n", e1000_read32(E1000_TDBAH));
    printf("[e1000] TDLEN  = 0x%08x\n", e1000_read32(E1000_TDLEN));
    printf("[e1000] TDH    = 0x%08x\n", e1000_read32(E1000_TDH));
    printf("[e1000] TDT    = 0x%08x\n", e1000_read32(E1000_TDT));
    printf("[e1000] IMS    = 0x%08x\n", e1000_read32(E1000_IMS));
    printf("[e1000] ICR    = 0x%08x\n", e1000_read32(E1000_ICR));

}

extern volatile uint32_t *lapic;
/**
 * @brief E1000 RX 寄存器转储（详细）
 */
void e1000_dump_rx_regs(void) {
    uint32_t rdh, rdt;

    printf("\n[e1000] ==================== RX Status ====================\n");

    // RX 控制寄存器
    uint32_t rctl = e1000_read32(E1000_RCTL);
    printf("[e1000] RCTL   = 0x%08x ", rctl);
    if (rctl & E1000_RCTL_EN)  printf("EN ");
    if (rctl & E1000_RCTL_UPE) printf("UPE ");
    if (rctl & E1000_RCTL_MPE) printf("MPE ");
    if (rctl & E1000_RCTL_BAM) printf("BAM ");
    printf("\n");

    // RX 描述符队列
    printf("[e1000] RDBAL  = 0x%08x (phys)\n", e1000_read32(E1000_RDBAL));
    printf("[e1000] RDBAH  = 0x%08x\n", e1000_read32(E1000_RDBAH));
    printf("[e1000] RDLEN fact = 0x%08x (%d descriptors)\n",
           e1000_read32(E1000_RDLEN), e1000_read32(E1000_RDLEN) / 16);
    printf("[e1000] RDLEN rsc  = %d (%d sizeof descriptors)\n",E1000_NUM_RX_DESC,sizeof(e1000_rx_desc_t));

    // RX 头尾指针
    rdh = e1000_read32(E1000_RDH);
    rdt = e1000_read32(E1000_RDT);
    printf("[e1000] RDH    = %d (hw head)\n", rdh);
    printf("[e1000] RDT    = %d (sw tail)\n", rdt);
    printf("[e1000] RXDCTL = 0x%08x\n", e1000_read32(E1000_RXDCTL));

    // 计算 pending 包数量：硬件写好但软件未处理的
    // pending = (RDH - RDT + RING_SIZE) % RING_SIZE
    int pending = (rdh - rdt + E1000_NUM_RX_DESC) % E1000_NUM_RX_DESC;
    printf("[e1000] Pending packets to process: %d\n", pending);

    // 软件状态
    printf("[e1000] Software RX cur: %d\n", e1000_priv.rx_cur);

    // // 🔥 手动触发一次 RX 处理（用于诊断）
    // printf("[e1000] Manually polling RX...\n");
    // //e1000_recv(&e1000_dev);
    // printf("[e1000] After manual poll:\n");
    // printf("[e1000]   recv_call_count:   %d\n", e1000_priv.recv_call_count);
    // printf("[e1000]   packets_processed: %d\n", e1000_priv.packets_processed);

    // 🔥 统计信息（放在最后，方便查看）
    printf("\n[e1000] ============== Interrupt Statistics ==============\n");
    printf("[e1000] Interrupts triggered:     %d\n", e1000_priv.intr_count);
    printf("[e1000] e1000_recv() called:      %d\n", e1000_priv.recv_call_count);
    printf("[e1000] Empty receives (cur=RDH): %d\n", e1000_priv.empty_recv_count);
    printf("[e1000] Packets processed:        %d\n", e1000_priv.packets_processed);

    // 🔥 读取 ICR 寄存器（查看是否有挂起的中断）
    uint32_t icr = e1000_read32(E1000_ICR);
    printf("[e1000] ICR (pending interrupts): 0x%08x\n", icr);
    if (icr != 0) {
        printf("[e1000] ⚠️  WARNING: Interrupts pending but not delivered!\n");
        if (icr & E1000_ICR_RXT0) printf("[e1000]    - RX Timer interrupt\n");
        if (icr & E1000_ICR_RXDMT0) printf("[e1000]    - RX minimum threshold\n");
        if (icr & E1000_ICR_TXDW) printf("[e1000]    - TX done\n");
    }
    // extern uint32_t lapic_read(int index);
    // printf("LAPIC IRR0 = %08x  ", lapic_read(0x200));
    // printf("LAPIC ISR0 = %08x\n", lapic_read(0x100));

    // for (int i=0; i<8; i++) {
    //     printf("IRR[%d] = %08x  ", i, lapic[0x200/4 + i]);
    //     printf("ISR[%d] = %08x  ", i, lapic[0x100/4 + i]);
    // }
    printf("[e1000] ==================================================\n");

    // 显示最近几个 RX 描述符的状态
    
    printf("\n[e1000] RX Descriptor Ring (last 3 entries):\n");
    for (int i = 0; i < 3; i++) {
        int idx = (e1000_priv.rx_cur - 1 - i + E1000_NUM_RX_DESC) % E1000_NUM_RX_DESC;
        e1000_rx_desc_t *desc = &e1000_priv.rx_desc[idx];

        // 🔥 Raw dump: 显示原始 16 字节
        printf("[e1000]   [%2d] RAW: ", idx);
        uint8_t *p = (uint8_t *)desc;
        for (int j = 0; j < 16; j++) {
            printf("%02x ", p[j]);
            if (j == 7) printf(" ");
        }
        printf("\n");

        // 显示结构体解析的值
        printf("[e1000]        => buf=0x%08x len=0x%04x stat=0x%02x",
               desc->buffer_addr, desc->length, desc->status);

        if (desc->status & E1000_RXD_STAT_DD) printf(" DD");
        if (desc->status & E1000_RXD_STAT_EOP) printf(" EOP");
        printf("\n");
    }
     
    //printf("[e1000] ==================================================\n\n");
}

/**
 * @brief E1000 重新启动网络接口
 */
int e1000_ifup(const char *dev_name) {
    printf("[e1000] Bringing up interface %s\n", dev_name);

    // 重新使能接收器（非混杂模式）
    e1000_reg_write32(E1000_RCTL,
        E1000_RCTL_EN  |
        E1000_RCTL_SBP |
        // E1000_RCTL_UPE |  // ❌ 混杂模式已移除
        // E1000_RCTL_MPE |  // ❌ 混杂模式已移除
        E1000_RCTL_BAM |
        E1000_RCTL_SECRC |
        E1000_RCTL_BSIZE_2048
    );

    // 重新使能发送器
    e1000_reg_write32(E1000_TCTL,
        E1000_TCTL_EN |
        E1000_TCTL_PSP |
        (0x10 << E1000_TCTL_CT_SHIFT) |
        (0x40 << E1000_TCTL_COLD_SHIFT)
    );

    printf("[e1000] Interface %s is up\n", dev_name);
    return 0;
}

/**
 * @brief E1000 硬件 Loopback 测试（轮询版本）
 *
 * 测试流程：
 * 1. 启用 MAC level loopback 模式
 * 2. 发送一个测试包
 * 3. 检查 RX 描述符是否收到
 * 4. 恢复正常模式
 *
 * 这个测试可以验证：
 * - TX 描述符和 DMA
 * - RX 描述符和 DMA
 * - 网卡内部数据路径
 */
int e1000_loopback_test(void) {
    printf("\n");
    printf("[e1000] ===============================================\n");
    printf("[e1000]   Hardware Loopback Test\n");
    printf("[e1000] ===============================================\n");

    uint32_t ctrl, rctl;

    // 1. 保存原始控制寄存器值
    ctrl = e1000_read32(E1000_CTRL);
    rctl = e1000_read32(E1000_RCTL);

    printf("[e1000] Original CTRL: 0x%08x\n", ctrl);
    printf("[e1000] Original RCTL: 0x%08x\n", rctl);

    // 2. 启用 MAC level loopback (VLAN loopback)
    // 设置 CTRL.LB (bit 16) = 01b for MAC loopback
    printf("[e1000] Enabling MAC loopback mode...\n");

    uint32_t new_ctrl = ctrl;
    new_ctrl &= ~(0x3 << 16);  // 清除 LB 位
    new_ctrl |= (0x1 << 16);    // 设置 LB = 01b (MAC loopback)

    e1000_write32(E1000_CTRL, new_ctrl);
    printf("[e1000] Set CTRL: 0x%08x (LB bit set)\n", new_ctrl);

    // 设置 RCTL.LBM (bit 7:6) = 01b for MAC loopback
    uint32_t new_rctl = rctl;
    new_rctl &= ~(0x3 << 6);   // 清除 LBM 位
    new_rctl |= (0x1 << 6);    // 设置 LBM = 01b (MAC loopback)

    e1000_write32(E1000_RCTL, new_rctl);
    printf("[e1000] Set RCTL: 0x%08x (LBM bit set)\n", new_rctl);

    // 3. 等待网卡稳定
    printf("[e1000] Waiting for link to stabilize...\n");
    for (volatile int i = 0; i < 1000000; i++) {
        __asm__ volatile("nop");
    }

    // 4. 构造测试包
    uint8_t test_packet[128];
    memset(test_packet, 0, sizeof(test_packet));

    // 以太网头
    eth_hdr_t *eth = (eth_hdr_t *)test_packet;
    memset(eth->eth_dst, 0xFF, ETH_ALEN);  // 广播
    memcpy(eth->eth_src, e1000_dev.mac_addr, ETH_ALEN);
    eth->eth_type = htons(ETH_P_IP);

    // IP 头
    ip_hdr_t *ip = (ip_hdr_t *)(test_packet + ETH_HDR_LEN);
    ip->ip_verhlen = 0x45;
    ip->ip_tos = 0;
    ip->ip_len = htons(20 + 8 + 4);  // IP + ICMP + data
    ip->ip_id = htons(0xABCD);
    ip->ip_off = 0;  // 片偏移
    ip->ip_ttl = 64;
    ip->ip_proto = IPPROTO_ICMP;
    ip->ip_src = htonl(0xC0A80101);  // 192.168.1.1
    ip->ip_dst = htonl(0xC0A80101);

    // ICMP 头 (Echo Request)
    icmp_hdr_t *icmp = (icmp_hdr_t *)(test_packet + ETH_HDR_LEN + 20);
    icmp->icmp_type = ICMP_ECHO_REQUEST;
    icmp->icmp_code = 0;
    icmp->icmp_id = htons(0x1234);
    icmp->icmp_seq = htons(1);

    // 数据部分
    uint8_t *data = test_packet + ETH_HDR_LEN + 20 + 8;
    const char *test_data = "TEST";
    memcpy(data, test_data, 4);

    int pkt_len = ETH_HDR_LEN + 20 + 8 + 4;

    printf("[e1000] Sending test packet (%d bytes)...\n", pkt_len);
    printf("[e1000]   ETH: dst=%02x:%02x:%02x:%02x:%02x:%02x src=%02x:%02x:%02x:%02x:%02x:%02x type=0x%04x\n",
           eth->eth_dst[0], eth->eth_dst[1], eth->eth_dst[2],
           eth->eth_dst[3], eth->eth_dst[4], eth->eth_dst[5],
           eth->eth_src[0], eth->eth_src[1], eth->eth_src[2],
           eth->eth_src[3], eth->eth_src[4], eth->eth_src[5],
           ntohs(eth->eth_type));

    // 5. 发送包
    int tx_ret = e1000_dev.send(&e1000_dev, test_packet, pkt_len);
    if (tx_ret < 0) {
        printf("[e1000] FAILED to send packet\n");
        goto restore;
    }

    printf("[e1000] Packet sent successfully\n");

    // 6. 等待 RX
    printf("[e1000] Waiting for RX (polling)...\n");
    int rx_count = 0;
    int found = 0;

    for (int i = 0; i < 10000000; i++) {
        // 检查 RX 描述符
        e1000_rx_desc_t *desc = &e1000_priv.rx_desc[e1000_priv.rx_cur];

        if (desc->status & E1000_RXD_STAT_DD) {
            printf("[e1000] ✓ RX descriptor %d marked as DD!\n", e1000_priv.rx_cur);
            printf("[e1000]   buffer_addr = 0x%08x\n", desc->buffer_addr);
            printf("[e1000]   length = %d bytes\n", desc->length);
            printf("[e1000]   status = 0x%02x", desc->status);

            if (desc->status & E1000_RXD_STAT_DD) printf(" DD");
            if (desc->status & E1000_RXD_STAT_EOP) printf(" EOP");
            printf("\n");

            // 验证数据
            uint8_t *rx_buf = e1000_priv.rx_buffers[e1000_priv.rx_cur];
            printf("[e1000]   First 16 bytes: ");
            for (int j = 0; j < 16 && j < desc->length; j++) {
                printf("%02x ", rx_buf[j]);
            }
            printf("\n");

            // 检查是否是我们的测试包
            eth_hdr_t *rx_eth = (eth_hdr_t *)rx_buf;
            if (rx_eth->eth_type == htons(ETH_P_IP)) {
                printf("[e1000]   ✓ ETH type matches (0x%04x)\n", ntohs(rx_eth->eth_type));
            }

            // 清除 DD 位
            desc->status = 0;

            // 更新 RDT 寄存器
            e1000_priv.rx_cur = (e1000_priv.rx_cur + 1) % E1000_NUM_RX_DESC;
            e1000_write32(E1000_RDT, e1000_priv.rx_cur);

            found = 1;
            rx_count++;
            break;
        }

        if (i % 1000000 == 0 && i > 0) {
            printf("[e1000]   Still waiting... (%d iterations)\n", i);
        }
    }

    // 7. 恢复原始设置
restore:
    printf("[e1000] Restoring original settings...\n");
    e1000_write32(E1000_CTRL, ctrl);
    e1000_write32(E1000_RCTL, rctl);
    printf("[e1000] Restored CTRL: 0x%08x\n", ctrl);
    printf("[e1000] Restored RCTL: 0x%08x\n", rctl);

    


    // 8. 报告结果
    printf("[e1000] ===============================================\n");
    uint32_t icr = e1000_read32(E1000_ICR);
    printf("ICR=0x%x\n", icr);
    if (found) {
        printf("[e1000] ✓✓✓ LOOPBACK TEST PASSED! ✓✓✓\n");
        printf("[e1000]     TX descriptors: OK\n");
        printf("[e1000]     TX DMA: OK\n");
        printf("[e1000]     RX descriptors: OK\n");
        printf("[e1000]     RX DMA: OK\n");
        printf("[e1000]     Hardware loopback: OK\n");
        printf("[e1000] ===============================================\n");
        return 0;
    } else {
        printf("[e1000] ✗✗✗ LOOPBACK TEST FAILED ✗✗✗\n");
        printf("[e1000]     Did not receive packet on RX\n");
        printf("[e1000]     Possible issues:\n");
        printf("[e1000]       - TX descriptor not processed\n");
        printf("[e1000]       - RX descriptor not set up correctly\n");
        printf("[e1000]       - MSI interrupt not firing\n");
        printf("[e1000]       - Hardware loopback not enabled\n");
        printf("[e1000] ===============================================\n");
        return -1;
    }
}
#define E1000_CTRL   0x0000
#define E1000_RCTL   0x0100
#define E1000_TCTL   0x0400
#define E1000_IMS    0x00D0
#define E1000_ICR    0x00C0

#define E1000_RCTL_EN       (1 << 1)
#define E1000_RCTL_LBM_MAC  (1 << 6)

#define E1000_TCTL_EN (1 << 1)
#define E1000_TCTL_PSP (1 << 3)

// E1000 描述符配置
#define E1000_NUM_TX_DESC 1
#define E1000_NUM_RX_DESC 16   // 🔥 增加到 16 个 RX 描述符，确保 RX 中断能正确触发
#define E1000_RX_BUF_SIZE 2048



void e1000_loopback_enable(void) {
    uint32_t rctl = e1000_read32(E1000_RCTL);
    rctl |= E1000_RCTL_LBM_MAC; // MAC loopback
    rctl |= E1000_RCTL_EN;
    e1000_write32(E1000_RCTL, rctl);

    uint32_t tctl = e1000_read32(E1000_TCTL);
    tctl |= E1000_TCTL_EN | E1000_TCTL_PSP;
    e1000_write32(E1000_TCTL, tctl);

    // 开 RX/TX 中断
    e1000_write32(E1000_IMS, E1000_ICR_RXT0 | E1000_ICR_TXDW);

    printf("[e1000] Loopback + interrupts enabled\n");
}






/**
 * @brief E1000 硬件 Loopback 测试（中断驱动版本）
 *
 * 这是真正的MSI中断测试！
 *
 * 测试流程：
 * 1. 启用 MAC level loopback 模式
 * 2. 启用 TX 和 RX 中断
 * 3. 发送一个测试包
 * 4. **等待MSI中断**触发
 * 5. 在ISR中验证收到包
 * 6. 恢复正常模式
 *
 * 这个测试可以验证：
 * - TX 描述符和 DMA
 * - RX 描述符和 DMA
 * - **MSI 中断路径** ✓
 * - **ISR 被调用** ✓
 * - 网卡内部数据路径
 */
int e1000_loopback_test_interrupt(void) {
    printf("\n");
    printf("[e1000] ===============================================\n");
    printf("[e1000]   Hardware Loopback Test (INTERRUPT DRIVEN)\n");
    printf("[e1000] ===============================================\n");
    printf("[e1000] This test will verify MSI interrupt path!\n\n");

    int found = 0;  // 测试结果标志

    // 重置同步标志
    loopback_rx_received = 0;
    loopback_tx_done = 0;

    // 1. 保存原始控制寄存器值
    uint32_t ctrl = e1000_read32(E1000_CTRL);
    uint32_t rctl = e1000_read32(E1000_RCTL);
    uint32_t ims = e1000_read32(E1000_IMS);

    // 🔥 诊断：检查当前中断状态
    printf("[e1000] === Pre-Test Diagnosis ===\n");
    uint32_t icr = e1000_read32(E1000_ICR);
    uint32_t status = e1000_read32(E1000_STATUS);

    printf("[e1000] ICR (current): 0x%08x\n", icr);
    printf("[e1000] IMS (current): 0x%08x\n", ims);
    printf("[e1000] CTRL: 0x%08x\n", ctrl);
    printf("[e1000] RCTL: 0x%08x\n", rctl);
    printf("[e1000] STATUS: 0x%08x\n", status);
    printf("[e1000] ===================================\n\n");

    printf("[e1000] Original CTRL: 0x%08x\n", ctrl);
    printf("[e1000] Original RCTL: 0x%08x\n", rctl);
    printf("[e1000] Original IMS:  0x%08x\n", ims);

    // 2. 重置同步标志（已在上面完成）

    // 3. 启用 MAC level loopback (VLAN loopback)
    printf("[e1000] Enabling MAC loopback mode...\n");

    uint32_t new_ctrl = ctrl;
    new_ctrl &= ~(0x3 << 16);  // 清除 LB 位
    new_ctrl |= (0x1 << 16);    // 设置 LB = 01b (MAC loopback)
    e1000_write32(E1000_CTRL, new_ctrl);
    printf("[e1000] Set CTRL: 0x%08x (LB bit set)\n", new_ctrl);

    // 设置 RCTL.LBM (bit 7:6) = 01b for MAC loopback
    uint32_t new_rctl = rctl;
    new_rctl &= ~(0x3 << 6);   // 清除 LBM 位
    new_rctl |= (0x1 << 6);    // 设置 LBM = 01b (MAC loopback)
    e1000_write32(E1000_RCTL, new_rctl);
    printf("[e1000] Set RCTL: 0x%08x (LBM bit set)\n", new_rctl);

    // 4. 🔥 启用 TX 和 RX 中断
    printf("[e1000] Enabling TX/RX interrupts...\n");
    e1000_write32(E1000_IMS, E1000_ICR_TXDW | E1000_ICR_RXT0);
    printf("[e1000] IMS set to: 0x%08x (TXDW + RXT0)\n",
           E1000_ICR_TXDW | E1000_ICR_RXT0);

    // 5. 等待网卡稳定
    printf("[e1000] Waiting for link to stabilize...\n");
    for (volatile int i = 0; i < 1000000; i++) {
        __asm__ volatile("nop");
    }

    // 6. 构造测试包
    uint8_t test_packet[128];
    memset(test_packet, 0, sizeof(test_packet));

    // 以太网头
    eth_hdr_t *eth = (eth_hdr_t *)test_packet;
    memset(eth->eth_dst, 0xFF, ETH_ALEN);  // 广播
    memcpy(eth->eth_src, e1000_dev.mac_addr, ETH_ALEN);
    eth->eth_type = htons(ETH_P_IP);

    // IP 头
    ip_hdr_t *ip = (ip_hdr_t *)(test_packet + ETH_HDR_LEN);
    ip->ip_verhlen = 0x45;
    ip->ip_tos = 0;
    ip->ip_len = htons(20 + 8 + 4);  // IP + ICMP + data
    ip->ip_id = htons(0xABCD);
    ip->ip_off = 0;  // 片偏移
    ip->ip_ttl = 64;
    ip->ip_proto = IPPROTO_ICMP;
    ip->ip_src = htonl(0xC0A80001);  // 192.168.0.1
    ip->ip_dst = htonl(0xC0A80001);

    // ICMP 头 (Echo Request)
    icmp_hdr_t *icmp = (icmp_hdr_t *)(test_packet + ETH_HDR_LEN + 20);
    icmp->icmp_type = ICMP_ECHO_REQUEST;
    icmp->icmp_code = 0;
    icmp->icmp_id = htons(0x1234);
    icmp->icmp_seq = htons(1);

    // 数据部分 - 🔥 特殊标记用于验证
    uint8_t *data = test_packet + ETH_HDR_LEN + 20 + 8;
    const char *test_data = "INTR";  // 标记这是中断测试
    memcpy(data, test_data, 4);

    int pkt_len = ETH_HDR_LEN + 20 + 8 + 4;

    printf("[e1000] Sending test packet (%d bytes)...\n", pkt_len);
    printf("[e1000]   Data payload: '%s' (marker for interrupt test)\n", test_data);

    // 7. 发送包
    int tx_ret = e1000_dev.send(&e1000_dev, test_packet, pkt_len);
    if (tx_ret < 0) {
        printf("[e1000] FAILED to send packet\n");
        found = 0;
        goto restore;
    }

    printf("[e1000] ✓ Packet sent successfully\n");
    printf("[e1000] Now waiting for MSI interrupt...\n");

    // 8. 🔥 等待中断（而不是轮询）
    // 我们给中断一些时间来触发
    int timeout = 1000000000;  // 100M 次循环约等于几秒

    for (int i = 0; i < timeout; i++) {
        // 检查是否收到中断
        if (loopback_tx_done && loopback_rx_received) {
            printf("[e1000] ✓✓✓ BOTH interrupts fired!\n");
            printf("[e1000]     TX done interrupt: YES\n");
            printf("[e1000]     RX interrupt: YES\n");
            found = 1;
            break;
        }

        // 每 10M 次循环打印一次等待信息
        if (i % 10000000 == 0 && i > 0) {
            //printf("[e1000]   Still waiting for interrupt... (%d/%d)\n",
                 //  i / 10000000, timeout / 10000000);
        }

        // 小延时避免 CPU 100%
        for (volatile int j = 0; j < 10; j++) {
            __asm__ volatile("nop");
        }
    }

    // 9. 恢复原始设置
restore:
    printf("[e1000] Restoring original settings...\n");
    e1000_write32(E1000_CTRL, ctrl);
    e1000_write32(E1000_RCTL, rctl);

    // 恢复中断掩码
    if (ims != 0) {
        e1000_write32(E1000_IMS, ims);
    } else {
        e1000_write32(E1000_IMC, 0xFFFFFFFF);  // 禁用所有中断
    }
    /*
    printf("[e1000] Restored CTRL: 0x%08x\n", ctrl);
    printf("[e1000] Restored RCTL: 0x%08x\n", rctl);
    printf("[e1000] Restored IMS:  0x%08x\n", ims);

    // 10. 报告结果
    printf("[e1000] ===============================================\n");
    if (found) {
        printf("[e1000] ✓✓✓ INTERRUPT LOOPBACK TEST PASSED! ✓✓✓\n");
        printf("[e1000] ===============================================\n");
        printf("[e1000] Verified components:\n");
        printf("[e1000]     ✓ TX descriptors: OK\n");
        printf("[e1000]     ✓ TX DMA: OK\n");
        printf("[e1000]     ✓ TX MSI interrupt: OK\n");
        printf("[e1000]     ✓ RX descriptors: OK\n");
        printf("[e1000]     ✓ RX DMA: OK\n");
        printf("[e1000]     ✓ RX MSI interrupt: OK\n");
        printf("[e1000]     ✓ ISR (Interrupt Service Routine): OK\n");
        printf("[e1000]     ✓ Hardware loopback: OK\n");
        printf("[e1000] ===============================================\n");
        printf("[e1000] ✅ MSI interrupt path is fully functional!\n");
        printf("[e1000] ===============================================\n");
        return 0;
    } else {
        printf("[e1000] ✗✗✗ INTERRUPT LOOPBACK TEST FAILED ✗✗✗\n");
        printf("[e1000] ===============================================\n");
        printf("[e1000]     Did not receive expected interrupts\n");
        printf("[e1000] \n");
        printf("[e1000] TX interrupt fired: %s\n",
               loopback_tx_done ? "YES ✓" : "NO ✗");
        printf("[e1000] RX interrupt fired: %s\n",
               loopback_rx_received ? "YES ✓" : "NO ✗");
        printf("[e1000] \n");
        printf("[e1000] Possible issues:\n");
        if (!loopback_tx_done) {
            printf("[e1000]       1. TX descriptor not processed\n");
            printf("[e1000]       2. TX MSI not enabled/configured\n");
        }
        if (!loopback_rx_received) {
            printf("[e1000]       3. RX descriptor not set up correctly\n");
            printf("[e1000]       4. RX MSI not enabled/configured\n");
            printf("[e1000]       5. ISR not registered in IDT\n");
            printf("[e1000]       6. APIC not enabled\n");
            printf("[e1000]       7. MSI address/data incorrect\n");
        }
        printf("[e1000]       8. Hardware loopback not enabled\n");
        printf("[e1000] ===============================================\n");
        return -1;
    }*/
}

/**
 * @brief 打印 MAC 地址配置（用于调试）
 */
void e1000_debug_print_mac(void) {
    printf("[e1000] === MAC Address Configuration ===\n");
    printf("[e1000] e1000_priv.mac_addr:  ");
    print_mac_addr(e1000_priv.mac_addr);
    printf("\n");

    printf("[e1000] e1000_dev.mac_addr:   ");
    print_mac_addr(e1000_dev.mac_addr);
    printf("\n");

    extern uint8_t local_mac[ETH_ALEN];
    printf("[e1000] global local_mac:      ");
    print_mac_addr(local_mac);
    printf("\n");

    // 从硬件寄存器重新读取
    uint32_t ral = e1000_read32(E1000_RAL(0));
    uint32_t rah = e1000_read32(E1000_RAH(0));
    printf("[e1000] HW RAL/RAH:           %02x:%02x:%02x:%02x:%02x:%02x\n",
           ral & 0xFF, (ral >> 8) & 0xFF, (ral >> 16) & 0xFF, (ral >> 24) & 0xFF,
           rah & 0xFF, (rah >> 8) & 0xFF);

    printf("[e1000] ======================================\n");
}

/**
 * @brief 手动轮询 RX（用于调试）
 * 在 shell 中调用此函数检查是否有包在 RX 描述符中
 */
void e1000_debug_poll_rx(void) {
    printf("[e1000] === Manual RX Poll Debug ===\n");

    uint32_t rdh = e1000_read32(E1000_RDH);
    uint32_t rdt = e1000_read32(E1000_RDT);
    uint32_t rctl = e1000_read32(E1000_RCTL);
    printf("[e1000] RDH=%d, RDT=%d, rx_cur=%d\n", rdh, rdt, e1000_priv.rx_cur);
    printf("[e1000] RCTL=0x%08x (RX enabled: %s)\n", rctl, (rctl & E1000_RCTL_EN) ? "YES" : "NO");

    // 检查所有描述符的原始数据
    printf("[e1000] RX Descriptors:\n");
    for (int i = 0; i < E1000_NUM_RX_DESC; i++) {
        e1000_rx_desc_t *desc = &e1000_priv.rx_desc[i];
        printf("[e1000]   desc[%d]: buffer=0x%08x, status=0x%04x, len=%d, errors=0x%04x, DD=%d\n",
               i, desc->buffer_addr, desc->status, desc->length, desc->errors,
               (desc->status & E1000_RXD_STAT_DD) ? 1 : 0);

        // 如果 DD 位设置了，显示前 16 字节
        if (desc->status & E1000_RXD_STAT_DD) {
            uint8_t *data = e1000_priv.rx_buffers[i];
            printf("[e1000]     Data: ");
            for (int j = 0; j < 16 && j < desc->length; j++) {
                printf("%02x ", data[j]);
            }
            printf("\n");
        }
    }

    // 尝试接收
    extern net_device_t e1000_dev;
    printf("[e1000] Attempting to receive packets...\n");
    e1000_recv(&e1000_dev);

    printf("[e1000] ==============================\n");
}