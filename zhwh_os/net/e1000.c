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

// E1000 网络设备
static net_device_t e1000_dev;
static e1000_priv_t e1000_priv;

// MMIO 读写宏
#define e1000_read32(reg) \
    (*(volatile uint32_t *)(e1000_priv.mmio_base_virt + (reg)))
#define e1000_write32(reg, val) \
    (*(volatile uint32_t *)(e1000_priv.mmio_base_virt + (reg)) = (val))

/**
 * @brief E1000 发送函数
 */
static int e1000_send(net_device_t *dev, uint8_t *data, uint32_t len) {
    if (!data || len == 0 || len > E1000_TX_BUF_SIZE) {
        printf("[e1000] Invalid send parameters\n");
        return -1;
    }

    // 检查 TX 描述符是否可用
    e1000_tx_desc_t *tx_desc = &e1000_priv.tx_desc[e1000_priv.tx_cur];

    // 检查描述符是否已完成 (DD 位)
    if (!(tx_desc->status & E1000_TXD_STAT_DD)) {
        printf("[e1000] TX descriptor %d busy\n", e1000_priv.tx_cur);
        return -1;
    }

    printf("[e1000] Sending %d bytes (desc %d)\n", len, e1000_priv.tx_cur);

    // 复制数据到发送缓冲区
    memcpy(e1000_priv.tx_buffers[e1000_priv.tx_cur], data, len);

    // 设置 TX 描述符
    tx_desc->buffer_addr = (uint64_t)(uint32_t)e1000_priv.tx_buffers[e1000_priv.tx_cur];
    tx_desc->length = (uint16_t)len;
    tx_desc->cmd = E1000_TXD_CMD_EOP | E1000_TXD_CMD_IFCS | E1000_TXD_CMD_RS;
    tx_desc->status = 0;  // 清除 DD 位

    // 内存屏障
    asm volatile("mfence" ::: "memory");

    // 更新 TDT (Tail) 寄存器
    e1000_priv.tx_cur = (e1000_priv.tx_cur + 1) % E1000_NUM_TX_DESC;
    e1000_write32(E1000_TDT, e1000_priv.tx_cur);

    return len;
}

/**
 * @brief E1000 接收函数（中断处理程序调用）
 */
static void e1000_recv(void) {
    printf("[e1000] Receiving packet...\n");

    e1000_rx_desc_t *rx_desc = &e1000_priv.rx_desc[e1000_priv.rx_cur];

    // 检查描述符是否完成
    if (!(rx_desc->status & E1000_RXD_STAT_DD)) {
        printf("[e1000] No packet ready\n");
        return;
    }

    printf("[e1000] RX desc %d: status=0x%x, len=%d\n",
           e1000_priv.rx_cur, rx_desc->status, rx_desc->length);

    // 检查长度
    if (rx_desc->length < ETH_HDR_LEN || rx_desc->length > ETH_MAX_FRAME) {
        printf("[e1000] Invalid packet length: %d\n", rx_desc->length);
        // 清除 DD 位
        rx_desc->status = 0;
        e1000_priv.rx_cur = (e1000_priv.rx_cur + 1) % E1000_NUM_RX_DESC;
        e1000_write32(E1000_RDT, e1000_priv.rx_cur);
        return;
    }

    // 传递给网络栈
    net_rx_packet(&e1000_dev, e1000_priv.rx_buffers[e1000_priv.rx_cur], rx_desc->length);

    // 清除 DD 位并将描述符归还给网卡
    rx_desc->status = 0;
    e1000_priv.rx_cur = (e1000_priv.rx_cur + 1) % E1000_NUM_RX_DESC;
    e1000_write32(E1000_RDT, e1000_priv.rx_cur);
}

/**
 * @brief E1000 中断处理
 */
void e1000_isr(void) {
    printf("[e1000] Interrupt!\n");

    uint32_t icr = e1000_read32(E1000_ICR);

    if (icr == 0) {
        printf("[e1000] Spurious interrupt\n");
        return;
    }

    printf("[e1000] ICR=0x%x\n", icr);

    // 处理接收中断
    if (icr & E1000_ICR_RXT0) {
        printf("[e1000] RX timer interrupt\n");
        e1000_recv();
    }

    if (icr & E1000_ICR_RXDMT0) {
        printf("[e1000] RX minimum threshold\n");
        e1000_recv();
    }

    // 处理发送完成
    if (icr & E1000_ICR_TXDW) {
        printf("[e1000] TX descriptor done\n");
    }

    printf("[e1000] Interrupt handled\n");
}

/**
 * @brief 从 EEPROM 读取 MAC 地址
 */
static void e1000_read_mac_addr(void) {
    // 从 RAL/RAH 寄存器读取 MAC 地址
    uint32_t ral = e1000_read32(E1000_RAL);
    uint32_t rah = e1000_read32(E1000_RAH);

    e1000_priv.mac_addr[0] = ral & 0xFF;
    e1000_priv.mac_addr[1] = (ral >> 8) & 0xFF;
    e1000_priv.mac_addr[2] = (ral >> 16) & 0xFF;
    e1000_priv.mac_addr[3] = (ral >> 24) & 0xFF;
    e1000_priv.mac_addr[4] = rah & 0xFF;
    e1000_priv.mac_addr[5] = (rah >> 8) & 0xFF;

    printf("[e1000] MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           e1000_priv.mac_addr[0], e1000_priv.mac_addr[1],
           e1000_priv.mac_addr[2], e1000_priv.mac_addr[3],
           e1000_priv.mac_addr[4], e1000_priv.mac_addr[5]);
}

/**
 * @brief 初始化 E1000 网卡
 */
int e1000_init_dev(pci_dev_t *pci_dev) {
    printf("[e1000] Initializing E1000...\n");

    // 读取 BAR0 (MMIO 基地址)
    uint32_t bar0 = pci_dev->header.u.h00.bar[0];
    uint32_t mmio_phys = bar0 & 0xFFFFFFF0;  // 低 4 位为类型标志

    printf("[e1000] MMIO base (phys): 0x%x\n", mmio_phys);

    // 将物理地址映射到虚拟地址
    e1000_priv.mmio_base = mmio_phys;

    // 使用高端内存映射函数
    // E1000 MMIO 区域大小约为 0x20000 字节 (128KB)
    void *mapped = map_highmem_physical(mmio_phys, 0x20000, 0);
    if (!mapped) {
        printf("[e1000] Failed to map MMIO region!\n");
        return -1;
    }

    e1000_priv.mmio_base_virt = (uint32_t)mapped;
    printf("[e1000] MMIO base (virt): 0x%x\n", e1000_priv.mmio_base_virt);

    // 读取 IRQ 号
    e1000_priv.irq = pci_dev->header.u.h00.interrupt_line;
    printf("[e1000] IRQ: %d\n", e1000_priv.irq);

    // 启用 PCI 设备
    pci_dev->header.command |= 0x07;  // Bus Master, Memory Space, I/O Space

    // 复位网卡
    printf("[e1000] Resetting...\n");
    e1000_write32(E1000_CTRL, E1000_CTRL_RST);

    // 等待复位完成
    int timeout = 10000;
    while (timeout-- && (e1000_read32(E1000_CTRL) & E1000_CTRL_RST)) {
        // 等待
    }

    if (timeout <= 0) {
        printf("[e1000] Reset timeout!\n");
        return -1;
    }

    printf("[e1000] Reset complete\n");

    // 读取 MAC 地址
    e1000_read_mac_addr();

    // 分配 RX 描述符数组（必须是 16 字节对齐）
    e1000_priv.rx_desc = (e1000_rx_desc_t *)kmalloc(sizeof(e1000_rx_desc_t) * E1000_NUM_RX_DESC + 16);
    if (!e1000_priv.rx_desc) {
        printf("[e1000] Failed to allocate RX descriptors\n");
        return -1;
    }

    // 16 字节对齐
    uint32_t rx_desc_phys = (uint32_t)e1000_priv.rx_desc;
    rx_desc_phys = (rx_desc_phys + 15) & ~15;
    e1000_priv.rx_desc = (e1000_rx_desc_t *)rx_desc_phys;

    printf("[e1000] RX descriptors: 0x%x (virt: 0x%x)\n", rx_desc_phys, (uint32_t)e1000_priv.rx_desc);

    // 分配 RX 缓冲区
    for (int i = 0; i < E1000_NUM_RX_DESC; i++) {
        e1000_priv.rx_buffers[i] = (uint8_t *)kmalloc(E1000_RX_BUF_SIZE);
        if (!e1000_priv.rx_buffers[i]) {
            printf("[e1000] Failed to allocate RX buffer %d\n", i);
            return -1;
        }

        // 清空描述符
        memset(&e1000_priv.rx_desc[i], 0, sizeof(e1000_rx_desc_t));
        e1000_priv.rx_desc[i].buffer_addr = (uint64_t)(uint32_t)e1000_priv.rx_buffers[i];
        e1000_priv.rx_desc[i].status = 0;
    }

    // 分配 TX 描述符数组
    e1000_priv.tx_desc = (e1000_tx_desc_t *)kmalloc(sizeof(e1000_tx_desc_t) * E1000_NUM_TX_DESC + 16);
    if (!e1000_priv.tx_desc) {
        printf("[e1000] Failed to allocate TX descriptors\n");
        return -1;
    }

    // 16 字节对齐
    uint32_t tx_desc_phys = (uint32_t)e1000_priv.tx_desc;
    tx_desc_phys = (tx_desc_phys + 15) & ~15;
    e1000_priv.tx_desc = (e1000_tx_desc_t *)tx_desc_phys;

    printf("[e1000] TX descriptors: 0x%x (virt: 0x%x)\n", tx_desc_phys, (uint32_t)e1000_priv.tx_desc);

    // 分配 TX 缓冲区
    for (int i = 0; i < E1000_NUM_TX_DESC; i++) {
        e1000_priv.tx_buffers[i] = (uint8_t *)kmalloc(E1000_TX_BUF_SIZE);
        if (!e1000_priv.tx_buffers[i]) {
            printf("[e1000] Failed to allocate TX buffer %d\n", i);
            return -1;
        }

        // 清空描述符
        memset(&e1000_priv.tx_desc[i], 0, sizeof(e1000_tx_desc_t));
        e1000_priv.tx_desc[i].status = E1000_TXD_STAT_DD;  // 初始状态为已完成
    }

    // 初始化接收和发送索引
    e1000_priv.rx_cur = 0;
    e1000_priv.tx_cur = 0;
    e1000_priv.tx_tail = 0;

    // 配置接收
    e1000_write32(E1000_RDBAL, rx_desc_phys & 0xFFFFFFFF);
    e1000_write32(E1000_RDBAH, 0);
    e1000_write32(E1000_RDLEN, E1000_NUM_RX_DESC * sizeof(e1000_rx_desc_t));
    e1000_write32(E1000_RDH, 0);
    e1000_write32(E1000_RDT, E1000_NUM_RX_DESC - 1);  // Ring buffer full

    // 配置接收控制寄存器
    e1000_write32(E1000_RCTL,
                  E1000_RCTL_EN |       // Enable receiver
                  E1000_RCTL_SBP |      // Save bad packets
                  E1000_RCTL_UPE |      // Unicast promiscuous
                  E1000_RCTL_MPE |      // Multicast promiscuous
                  E1000_RCTL_BAM |      // Broadcast accept mode
                  E1000_RCTL_SECRC);    // Strip CRC

    // 配置发送
    e1000_write32(E1000_TDBAL, tx_desc_phys & 0xFFFFFFFF);
    e1000_write32(E1000_TDBAH, 0);
    e1000_write32(E1000_TDLEN, E1000_NUM_TX_DESC * sizeof(e1000_tx_desc_t));
    e1000_write32(E1000_TDH, 0);
    e1000_write32(E1000_TDT, 0);

    // 配置发送控制寄存器
    e1000_write32(E1000_TCTL,
                  E1000_TCTL_EN |       // Enable transmitter
                  E1000_TCTL_PSP |      // Pad short packets
                  (0x0F << E1000_TCTL_CT_SHIFT) |  // Collision threshold
                  (0x40 << E1000_TCTL_COLD_SHIFT)); // Collision distance

    // 配置 IPG (Inter-Packet Gap)
    e1000_write32(E1000_TIPG, 0x00602006);  // 默认值

    // 清除所有中断
    e1000_read32(E1000_ICR);  // 读清除

    // 启用发送和接收中断
    e1000_write32(E1000_IMS,
                  E1000_ICR_TXDW |  // TX Descriptor Written Back
                  E1000_ICR_RXT0 |  // RX Timer
                  E1000_ICR_RXDMT0); // RX Minimum Threshold

    printf("[e1000] E1000 initialized\n");

    return 0;
}

/**
 * @brief E1000 探测函数
 */
int e1000_probe(pci_dev_t **devices, unsigned num_devices) {
    printf("[e1000] Probing for E1000...\n");

    for (unsigned i = 0; i < num_devices; i++) {
        pci_dev_t *dev = devices[i];

        if (dev->header.vendor_id == E1000_VENDOR_ID &&
            (dev->header.device_id == E1000_DEVICE_ID ||
             dev->header.device_id == E1000_DEVICE_ID_I82545 ||
             dev->header.device_id == E1000_DEVICE_ID_I82546)) {

            printf("[e1000] Found E1000 device!\n");
            printf("[e1000]   Bus: %d, Device: %d, Function: %d\n",
                   dev->bus_id, dev->dev_id, dev->fn_id);
            printf("[e1000]   Vendor: 0x%x, Device: 0x%x\n",
                   dev->header.vendor_id, dev->header.device_id);

            // 初始化设备
            if (e1000_init_dev(dev) < 0) {
                printf("[e1000] Failed to initialize device\n");
                continue;
            }

            // 注册网络设备
            memset(&e1000_dev, 0, sizeof(e1000_dev));
            strcpy(e1000_dev.name, "eth0");
            memcpy(e1000_dev.mac_addr, e1000_priv.mac_addr, ETH_ALEN);
            e1000_dev.mtu = ETH_MTU;
            e1000_dev.send = e1000_send;
            e1000_dev.recv = NULL;
            e1000_dev.ioctl = NULL;
            e1000_dev.priv = &e1000_priv;

            if (net_device_register(&e1000_dev) < 0) {
                printf("[e1000] Failed to register device\n");
                return -1;
            }

            printf("[e1000] Device registered successfully\n");
            return 0;
        }
    }

    printf("[e1000] No E1000 device found\n");
    return -1;
}

/**
 * @brief E1000 初始化（从 PCI 设备列表探测）
 */
int e1000_init(void) {
    printf("[e1000] E1000 driver init\n");

    // 获取 PCI 设备列表
    pci_dev_t **devices = pci_get_devices();

    // 统计设备数量
    unsigned num_devices = 0;
    while (devices[num_devices] != NULL) {
        num_devices++;
    }

    printf("[e1000] Found %d PCI devices\n", num_devices);

    return e1000_probe(devices, num_devices);
}
