/**
 * @file rtl8139.c
 * @brief RTL8139 网卡驱动实现
 *
 * 支持 RealTek RTL8139 系列网卡
 * 常见于 QEMU 虚拟机
 */

#include "net.h"
#include "rtl8139.h"
#include "../include/printf.h"
#include "../include/string.h"
#include "../include/kmalloc.h"
#include "../include/pci.h"
#include "../include/x86/io.h"

// RTL8139 网络设备
static net_device_t rtl8139_dev;
static rtl8139_priv_t rtl8139_priv;

// 🔥 保存设备名称（用于提示等）
static char rtl8139_dev_name[16] = "eth0";

// I/O 操作宏
#define rtl8139_read8(reg)      inb(rtl8139_priv.io_base + (reg))
#define rtl8139_read16(reg)     inw(rtl8139_priv.io_base + (reg))
#define rtl8139_read32(reg)     inl(rtl8139_priv.io_base + (reg))
#define rtl8139_write8(reg, val) outb(rtl8139_priv.io_base + (reg), (val))
#define rtl8139_write16(reg, val) outw(rtl8139_priv.io_base + (reg), (val))
#define rtl8139_write32(reg, val) outl(rtl8139_priv.io_base + (reg), (val))

// 内存操作宏（用于访问缓冲区）
#define RTL8139_TX_PHYS_ADDR(i) (rtl8139_priv.tx_buffer[i])
#define RTL8139_RX_PHYS_ADDR()  (rtl8139_priv.rx_buffer)

/**
 * @brief RTL8139 发送函数
 */
static int rtl8139_send(net_device_t *dev, uint8_t *data, uint32_t len) {
    if (!data || len == 0 || len > RTL8139_TX_BUF_SIZE) {
        printf("[rtl8139] Invalid send parameters\n");
        return -1;
    }

    // 检查发送缓冲区是否可用
    uint32_t tx_status = rtl8139_read32(RTL8139_REG_TXSTAT0 + (rtl8139_priv.tx_cur * 4));

    // 如果位 13 (OWN) 被设置，说明 DMA 还在占用
    if (tx_status & RTL8139_TSR_OWN) {
        printf("[rtl8139] TX buffer %d busy\n", rtl8139_priv.tx_cur);
        return -1;
    }

    printf("[rtl8139] Sending %d bytes (buffer %d)\n", len, rtl8139_priv.tx_cur);

    // 复制数据到发送缓冲区
    memcpy(rtl8139_priv.tx_buffer[rtl8139_priv.tx_cur], data, len);

    // 设置发送长度（低 11 位为长度，位 13 为 OWN）
    uint32_t tx_cmd = len | RTL8139_TSR_OWN;

    // 写入发送命令
    rtl8139_write32(RTL8139_REG_TXSTAT0 + (rtl8139_priv.tx_cur * 4), tx_cmd);

    // 切换到下一个发送缓冲区
    rtl8139_priv.tx_cur = (rtl8139_priv.tx_cur + 1) % 4;

    return len;
}

/**
 * @brief RTL8139 接收函数（中断处理程序调用）
 */
static void rtl8139_recv(void) {
    printf("[rtl8139] Receiving packet...\n");

    // 读取当前接收缓冲区指针
    uint16_t capr = rtl8139_read16(RTL8139_REG_CAPR);

    // 计算下一个包的位置
    uint16_t rx_offset = (capr + 16) & RTL8139_RX_BUF_MASK;

    uint8_t *rx_buf = rtl8139_priv.rx_buffer;

    // 读取接收包头
    rtl8139_rx_hdr_t *hdr = (rtl8139_rx_hdr_t *)&rx_buf[rx_offset];

    uint16_t status = ntohs(hdr->status);
    uint16_t length = ntohs(hdr->length);

    printf("[rtl8139] RX status=0x%x, len=%d\n", status, length);

    // 检查接收状态
    if (!(status & RTL8139_RX_STATUS_ROK)) {
        printf("[rtl8139] RX error: status=0x%x\n", status);
        return;
    }

    // 检查长度
    if (length < ETH_HDR_LEN || length > ETH_MAX_FRAME) {
        printf("[rtl8139] Invalid packet length: %d\n", length);
        return;
    }

    // 计算数据位置（跳过包头）
    uint16_t data_offset = (rx_offset + RTL8139_RX_HDR_SIZE) & RTL8139_RX_BUF_MASK;

    // 传递给网络栈
    net_rx_packet(&rtl8139_dev, &rx_buf[data_offset], length - 4);  // 减去 CRC

    // 更新 CAPR 指针
    uint16_t new_capr = (capr + 4 + length + 3) & ~3;  // 4 字节对齐
    new_capr &= RTL8139_RX_BUF_MASK;
    rtl8139_write16(RTL8139_REG_CAPR, new_capr);
}

/**
 * @brief RTL8139 中断处理
 */
void rtl8139_isr(void) {
    printf("[rtl8139] Interrupt!\n");

    uint16_t isr = rtl8139_read16(RTL8139_REG_ISR);

    if (isr == 0) {
        printf("[rtl8139] Spurious interrupt\n");
        return;
    }

    // 处理接收中断
    if (isr & RTL8139_IRQ_RX_OK) {
        printf("[rtl8139] RX OK interrupt\n");
        rtl8139_recv();
    }

    // 处理接收错误
    if (isr & RTL8139_IRQ_RX_ERR) {
        printf("[rtl8139] RX error interrupt\n");
    }

    // 处理发送完成
    if (isr & RTL8139_IRQ_TX_OK) {
        printf("[rtl8139] TX OK interrupt\n");
    }

    // 处理发送错误
    if (isr & RTL8139_IRQ_TX_ERR) {
        printf("[rtl8139] TX error interrupt\n");
    }

    // 清除中断
    rtl8139_write16(RTL8139_REG_ISR, isr);

    printf("[rtl8139] Interrupt handled\n");
}

/**
 * @brief 初始化 RTL8139 网卡
 */
int rtl8139_init_dev(pci_dev_t *pci_dev) {
    printf("[rtl8139] Initializing RTL8139...\n");

    // 读取 I/O 基地址（BAR0）
    uint32_t bar0 = pci_dev->header.u.h00.bar[0];
    rtl8139_priv.io_base = bar0 & 0xFFFE;  // 低位为 1 表示 I/O 空间

    printf("[rtl8139] I/O base: 0x%x\n", rtl8139_priv.io_base);

    // 读取 IRQ 号
    rtl8139_priv.irq = pci_dev->header.u.h00.interrupt_line;
    printf("[rtl8139] IRQ: %d\n", rtl8139_priv.irq);

    // 启用 PCI 设备
    pci_dev->header.command |= 0x05;  // 使能总线主控和 I/O 空间

    // 复位网卡
    printf("[rtl8139] Resetting...\n");
    rtl8139_write8(RTL8139_REG_CMD, RTL8139_CMD_RESET);

    // 等待复位完成
    int timeout = 10000;
    while (timeout-- && (rtl8139_read8(RTL8139_REG_CMD) & RTL8139_CMD_RESET)) {
        // 等待
    }

    if (timeout <= 0) {
        printf("[rtl8139] Reset timeout!\n");
        return -1;
    }

    printf("[rtl8139] Reset complete\n");

    // 分配接收缓冲区（必须是 8K + 16 字节对齐）
    uint8_t *rx_buf_temp = (uint8_t *)kmalloc(RTL8139_RX_BUF_SIZE + 16);
    if (!rx_buf_temp) {
        printf("[rtl8139] Failed to allocate RX buffer\n");
        return -1;
    }

    // 16 字节对齐
    uint32_t rx_buf_phys = (uint32_t)rx_buf_temp;
    rx_buf_phys = (rx_buf_phys + 15) & ~15;
    rtl8139_priv.rx_buffer = (uint8_t *)rx_buf_phys;

    printf("[rtl8139] RX buffer: 0x%x\n", rx_buf_phys);

    // 分配 4 个发送缓冲区
    for (int i = 0; i < 4; i++) {
        rtl8139_priv.tx_buffer[i] = (uint8_t *)kmalloc(RTL8139_TX_BUF_SIZE);
        if (!rtl8139_priv.tx_buffer[i]) {
            printf("[rtl8139] Failed to allocate TX buffer %d\n", i);
            return -1;
        }
        printf("[rtl8139] TX buffer %d: 0x%x\n", i, (uint32_t)rtl8139_priv.tx_buffer[i]);
    }

    // 设置接收缓冲区地址
    rtl8139_write32(RTL8139_REG_RXBUF, rx_buf_phys);

    // 初始化接收位置
    rtl8139_priv.rx_cur = 0;
    rtl8139_write16(RTL8139_REG_CAPR, 0);

    // 读取 MAC 地址
    for (int i = 0; i < ETH_ALEN; i++) {
        rtl8139_priv.mac_addr[i] = rtl8139_read8(RTL8139_REG_IDR0 + i);
    }

    printf("[rtl8139] MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           rtl8139_priv.mac_addr[0], rtl8139_priv.mac_addr[1],
           rtl8139_priv.mac_addr[2], rtl8139_priv.mac_addr[3],
           rtl8139_priv.mac_addr[4], rtl8139_priv.mac_addr[5]);

    // 配置发送：DMA 阈值 = 1536 字节
    rtl8139_write32(RTL8139_REG_TXCFG, 0x0300);

    // 配置接收：环绕模式，FIFO 阈值，接收广播和多播
    rtl8139_write32(RTL8139_REG_RXCFG,
                    RTL8139_RXCFG_WRAP |
                    RTL8139_RXCFG_FIFO_MAX |
                    RTL8139_RXCFG_ACCEPT_BROADCAST |
                    RTL8139_RXCFG_ACCEPT_MULTICAST |
                    RTL8139_RXCFG_ACCEPT_MYphys);

    // 清除所有中断状态
    rtl8139_write16(RTL8139_REG_ISR, 0xFFFF);

    // 启用接收和发送完成中断
    rtl8139_write16(RTL8139_REG_IMR,
                    RTL8139_IRQ_TX_OK |
                    RTL8139_IRQ_RX_OK);

    // 启用发送和接收
    rtl8139_write8(RTL8139_REG_CMD,
                   RTL8139_CMD_TX_ENABLE |
                   RTL8139_CMD_RX_ENABLE);

    printf("[rtl8139] RTL8139 initialized\n");

    return 0;
}

/**
 * @brief RTL8139 探测函数
 */
int rtl8139_probe(pci_dev_t **devices, unsigned num_devices) {
    printf("[rtl8139] Probing for RTL8139...\n");

    for (unsigned i = 0; i < num_devices; i++) {
        pci_dev_t *dev = devices[i];

        if (dev->header.vendor_id == RTL8139_VENDOR_ID &&
            dev->header.device_id == RTL8139_DEVICE_ID) {

            printf("[rtl8139] Found RTL8139 device!\n");
            printf("[rtl8139]   Bus: %d, Device: %d, Function: %d\n",
                   dev->bus_id, dev->dev_id, dev->fn_id);

            // 初始化设备
            if (rtl8139_init_dev(dev) < 0) {
                printf("[rtl8139] Failed to initialize device\n");
                continue;
            }

            // 注册网络设备
            memset(&rtl8139_dev, 0, sizeof(rtl8139_dev));
            // 🔥 动态命名：eth0, eth1, eth2...
            extern int net_get_device_count(void);
            int dev_num = net_get_device_count();
            snprintf(rtl8139_dev.name, sizeof(rtl8139_dev.name), "eth%d", dev_num);
            memcpy(rtl8139_dev.mac_addr, rtl8139_priv.mac_addr, ETH_ALEN);
            rtl8139_dev.mtu = ETH_MTU;
            rtl8139_dev.send = rtl8139_send;
            rtl8139_dev.recv = NULL;
            rtl8139_dev.ioctl = NULL;
            rtl8139_dev.priv = &rtl8139_priv;
            rtl8139_dev.pci_dev = dev;  // 保存 PCI 设备指针

            if (net_device_register(&rtl8139_dev) < 0) {
                printf("[rtl8139] Failed to register device\n");
                return -1;
            }

            printf("[rtl8139] Device registered as %s\n", rtl8139_dev.name);
            return 0;
        }
    }

    printf("[rtl8139] No RTL8139 device found\n");
    return -1;
}

/**
 * @brief RTL8139 初始化（从 PCI 设备列表探测）
 */
int rtl8139_init(void) {
    printf("[rtl8139] RTL8139 driver init\n");

    // 获取 PCI 设备列表
    pci_dev_t **devices = pci_get_devices();

    // 统计设备数量
    unsigned num_devices = 0;
    while (devices[num_devices] != NULL) {
        num_devices++;
    }

    printf("[rtl8139] Found %d PCI devices\n", num_devices);

    return rtl8139_probe(devices, num_devices);
}
