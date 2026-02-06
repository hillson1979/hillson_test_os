/**
 * @file atheros.h
 * @brief WiFi 网卡驱动头文件
 *
 * 支持:
 * - Intel Centrino Advanced-N 6205
 * - Qualcomm Atheros QCA9377
 */

#ifndef ATHEROS_H
#define ATHEROS_H

#include "types.h"
#include "net.h"
#include "net/wifi/dma.h"

// 前向声明
typedef struct pci_dev pci_dev_t;

// Intel 设备 ID
#define INTEL_VENDOR_ID          0x8086  // Intel Corporation
#define INTEL_CENTRINO_6205_AG   0x0085  // Centrino Advanced-N 6205 AG
#define INTEL_CENTRINO_6205_ABG  0x0082  // Centrino Advanced-N 6205 ABG

// Atheros 设备 ID
#define ATHEROS_VENDOR_ID        0x168C  // Qualcomm Atheros
#define ATHEROS_QCA9377_DEV_ID   0x003E  // QCA9377

// 内存映射 I/O 基地址（从 PCI BAR 读取）
#define ATHEROS_REG_SIZE        0x10000  // 寄存器空间大小

// Atheros WiFi 寄存器偏移（基于 QCA9377 规格）
#define ATHEROS_REG_MAC_ADDR        0x0000  // MAC 地址寄存器
#define ATHEROS_REG_EEPROM          0x0010  // EEPROM 基地址
#define ATHEROS_REG_INT_STATUS      0x0080  // 中断状态
#define ATHEROS_REG_INT_MASK        0x0084  // 中断掩码
#define ATHEROS_REG_FIFO_ADDR       0x0100  // FIFO 地址
#define ATHEROS_REG_RX_DESC_BASE    0x0200  // RX 描述符基址
#define ATHEROS_REG_TX_DESC_BASE    0x0300  // TX 描述符基址

// Atheros 寄存器操作寄存器
#define ATHEROS_REG_RTC_RESET       0x1000  // RTC 复位寄存器
#define ATHEROS_REG_RTC_SYNC        0x1004  // RTC 同步寄存器
#define ATHEROS_REG_RTC_FORCE_WAKE  0x1008  // RTC 强制唤醒
#define ATHEROS_REG_INTR_SYNC       0x1080  // 中断同步寄存器
#define ATHEROS_REG_CPU_PLL         0x1800  // CPU PLL 配置
#define ATHEROS_REG_DMA_CFG         0x2000  // DMA 配置
#define ATHEROS_REG_FIFO_DEPTH      0x2008  // FIFO 深度

// WiFi 控制寄存器
#define ATHEROS_REG_WLAN_CONTROL    0x2100  // WLAN 使能控制
#define ATHEROS_REG_PCIE_CONFIG     0x3000  // PCIe 配置
#define ATHEROS_REG_EEPROM_CTRL     0x3008  // EEPROM 控制

// EEPROM 数据寄存器（用于读取 EEPROM 数据）
#define ATHEROS_REG_EEPROM_DATA     0x300C  // EEPROM 数据

// 中断标志
#define ATHEROS_INT_RX              0x00000001  // 接收中断
#define ATHEROS_INT_TX              0x00000002  // 发送中断
#define ATHEROS_INT_RXORN           0x00000004  // RX 过满
#define ATHEROS_INT_TXURN           0x00000008  // TX 下溢
#define ATHEROS_INT_MIB             0x00000010  // MIB 中断
#define ATHEROS_INT_RXLP            0x00000020  // RX 低优先级
#define ATHEROS_INT_RXHP            0x00000040  // RX 高优先级
#define ATHEROS_INT_TXOK            0x00000100  // TX 完成
#define ATHEROS_INT_TXDESC          0x00000200  // TX 描述符
#define ATHEROS_INT_MISC            0x00000400  // 其他中断
#define ATHEROS_INT_GLOBAL          0x80000000  // 全局中断使能

// DMA 描述符标志
#define ATHEROS_TXDESC_DONE         0x00000001  // TX 完成
#define ATHEROS_TXDESC_ERR          0x00000002  // TX 错误
#define ATHEROS_RXDESC_DONE         0x00000001  // RX 完成
#define ATHEROS_RXDESC_ERR          0x00000002  // RX 错误

// 固件相关
#define ATHEROS_FIRMWARE_SIZE       (64 * 1024)  // 固件最大 64KB
#define ATHEROS_FW_MAGIC            0x544F4F42   // "BOOT" 魔术字 (Atheros)
#define INTEL_FW_MAGIC              0x0A4C5749   // "IWL\n" 魔术字 (Intel，注意 \n 是 0x0A)

// DMA 描述符数量
#define ATHEROS_NUM_TX_DESC         64
#define ATHEROS_NUM_RX_DESC         64
#define ATHEROS_RX_BUF_SIZE         2048
#define ATHEROS_TX_BUF_SIZE         2048

// PCIe 控制位
#define ATHEROS_PCIE_MSI            0x100  // 使能 MSI
#define ATHEROS_PCIE_LTR            0x200  // 使能 LTR

// 私有数据结构
typedef struct {
    uint32_t mem_base;        // 内存映射基地址
    uint32_t irq;             // IRQ 号
    uint8_t mac_addr[ETH_ALEN]; // MAC 地址

    // WiFi 配置
    char ssid[32];            // SSID
    uint8_t channel;          // 信道
    uint8_t security_mode;    // 安全模式
    int enabled;              // 设备是否启用
    int connected;            // 连接状态
    uint8_t ap_mac[ETH_ALEN]; // AP MAC 地址
    int signal_strength;      // 信号强度

    // 硬件状态
    int hw_initialized;       // 硬件是否初始化
    int fw_loaded;            // 固件是否加载
    int dma_enabled;          // DMA 是否使能

    // Atheros DMA 通道
    dma_channel_t *tx_channel; // TX DMA 通道
    dma_channel_t *rx_channel; // RX DMA 通道

    // Intel TX/RX 队列
    void *intel_tx_queue;     // Intel TX 队列
    void *intel_rx_queue;     // Intel RX 队列

    // 中断统计
    uint32_t int_count;       // 中断计数
    uint32_t tx_int_count;    // TX 中断计数
    uint32_t rx_int_count;    // RX 中断计数
} atheros_priv_t;

// 函数声明
int atheros_init(void);
int atheros_probe(pci_dev_t **devices, unsigned num_devices);
int atheros_init_dev(pci_dev_t *pci_dev);

// 硬件初始化
int atheros_hw_init(uint32_t mem_base);
int atheros_hw_reset(uint32_t mem_base);
int atheros_hw_enable_wlan(uint32_t mem_base);
int atheros_hw_enable_interrupts(uint32_t mem_base);

// WiFi 数据包测试
int wifi_send_test_packet(void);
void wifi_show_stats(void);
void wifi_poll(void);
void wifi_timer_callback(void);

// 中断处理
void atheros_interrupt_handler(void);

#endif // ATHEROS_H
