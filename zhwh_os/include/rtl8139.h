/**
 * @file rtl8139.h
 * @brief RTL8139 网卡驱动头文件
 */

#ifndef RTL8139_H
#define RTL8139_H

#include "types.h"
#include "net.h"

// 前向声明
typedef struct pci_dev pci_dev_t;

// RTL8139 寄存器定义
#define RTL8139_REG_IDR0        0x00    // MAC 地址第0字节
#define RTL8139_REG_IDR1        0x01    // MAC 地址第1字节
#define RTL8139_REG_IDR2        0x02    // MAC 地址第2字节
#define RTL8139_REG_IDR3        0x03    // MAC 地址第3字节
#define RTL8139_REG_IDR4        0x04    // MAC 地址第4字节
#define RTL8139_REG_IDR5        0x05    // MAC 地址第5字节
#define RTL8139_REG_MAR0        0x08    // 多播地址掩码
#define RTL8139_REG_MAR4        0x0C    // 多播地址掩码

#define RTL8139_REG_TCR         0x40    // 发送命令寄存器
#define RTL8139_REG_RCR         0x44    // 接收命令寄存器
#define RTL8139_REG_TSR         0x58    // 发送状态寄存器
#define RTL8139_REG_RCR         0x44    // 接收命令寄存器

#define RTL8139_REG_CMD         0x37    // 命令寄存器
#define RTL8139_REG_CAPR        0x38    // 当前接收缓冲区地址
#define RTL8139_REG_CBR         0x3A    // 当前缓冲区地址
#define RTL8139_REG_IMR         0x3C    // 中断掩码寄存器
#define RTL8139_REG_ISR         0x3E    // 中断状态寄存器
#define RTL8139_REG_TXCFG       0x40    // 发送配置寄存器
#define RTL8139_REG_RXCFG       0x44    // 接收配置寄存器
#define RTL8139_REG_TCTR        0x48    // 定时器计数寄存器

#define RTL8139_REG_TXADDR0     0x20    // 发送缓冲区地址0
#define RTL8139_REG_TXADDR1     0x24    // 发送缓冲区地址1
#define RTL8139_REG_TXADDR2     0x28    // 发送缓冲区地址2
#define RTL8139_REG_TXADDR3     0x2C    // 发送缓冲区地址3

#define RTL8139_REG_TXSTAT0     0x10    // 发送状态0
#define RTL8139_REG_TXSTAT1     0x14    // 发送状态1
#define RTL8139_REG_TXSTAT2     0x18    // 发送状态2
#define RTL8139_REG_TXSTAT3     0x1C    // 发送状态3

#define RTL8139_REG_RXBUF       0x30    // 接收缓冲区地址

#define RTL8139_REG_RXCONFIG    0x44    // 接收配置
#define RTL8139_REG_RXMISS      0x4C    // 接收丢失包计数

// 命令寄存器位
#define RTL8139_CMD_BUF_EMPTY   0x01    // 发送缓冲区空
#define RTL8139_CMD_TX_ENABLE   0x04    // 发送使能
#define RTL8139_CMD_RX_ENABLE   0x08    // 接收使能
#define RTL8139_CMD_RESET       0x10    // 复位

// 发送配置寄存器位
#define RTL8139_TXCFG_LOOPBACK  0x0006  // 回环模式
#define RTL8139_TXCFG_DMA       0x0700  // DMA 阈值

// 接收配置寄存器位
#define RTL8139_RXCFG_WRAP      0x0007  // 环绕模式
#define RTL8139_RXCFG_FIFO_MAX  0x0F00  // FIFO 阈值
#define RTL8139_RXCFG_ACCEPT_ERR 0x20   // 接收错误帧
#define RTL8139_RXCFG_ACCEPT_RUNT 0x40  // 接收小包
#define RTL8139_RXCFG_ACCEPT_BROADCAST 0x08  // 接收广播
#define RTL8139_RXCFG_ACCEPT_MULTICAST 0x04  // 接收多播
#define RTL8139_RXCFG_ACCEPT_MYphys 0x02  // 接收匹配物理地址

// 中断状态/掩码寄存器位
#define RTL8139_IRQ_TX_OK       0x04    // 发送完成
#define RTL8139_IRQ_RX_OK       0x01    // 接收完成
#define RTL8139_IRQ_RX_ERR      0x02    // 接收错误
#define RTL8139_IRQ_TX_ERR      0x08    // 发送错误
#define RTL8139_IRQ_SYSTEM_ERR  0x8000  // 系统错误

// 发送状态寄存器位
#define RTL8139_TSR_OWN         0x2000  // DMA 拥有
#define RTL8139_TSR_TX_OK       0x8000  // 发送成功

// 接收包头结构
#define RTL8139_RX_HDR_SIZE     4       // 接收包头大小
#define RTL8139_RX_STATUS_ROK   0x01    // 接收成功
#define RTL8139_RX_STATUS_FAE   0x02    // 帧对齐错误
#define RTL8139_RX_STATUS_CRC   0x04    // CRC 错误
#define RTL8139_RX_STATUS_LONG  0x08    // 包太长
#define RTL8139_RX_STATUS_RUNT  0x10    // 包太短
#define RTL8139_RX_STATUS_ISE   0x20    // 符号错误
#define RTL8139_RX_STATUS_BAR   0x10000000 // 广播地址
#define RTL8139_RX_STATUS_PAM   0x20000000 // 物理地址匹配
#define RTL8139_RX_STATUS_MAR   0x40000000 // 多播地址匹配

// RTL8139 设备 ID
#define RTL8139_VENDOR_ID       0x10EC  // RealTek
#define RTL8139_DEVICE_ID       0x8139  // RTL8139

// 缓冲区大小
#define RTL8139_TX_BUF_SIZE     2048    // 发送缓冲区大小
#define RTL8139_RX_BUF_SIZE     8192    // 接收缓冲区大小 (8K + 16 字节对齐)
#define RTL8139_RX_BUF_MASK     (RTL8139_RX_BUF_SIZE - 1)

// RTL8139 私有数据结构
typedef struct {
    uint32_t io_base;           // I/O 基地址
    uint32_t irq;               // IRQ 号

    uint8_t *tx_buffer[4];      // 发送缓冲区（4个描述符）
    uint8_t *rx_buffer;         // 接收缓冲区
    uint16_t rx_cur;            // 当前接收位置

    uint8_t tx_cur;             // 当前发送描述符

    uint8_t mac_addr[ETH_ALEN]; // MAC 地址
} rtl8139_priv_t;

// RTL8139 接收包头
typedef struct {
    uint16_t status;            // 状态
    uint16_t length;            // 包长度
} __attribute__((packed)) rtl8139_rx_hdr_t;

// 函数声明
int rtl8139_init(void);
int rtl8139_probe(pci_dev_t **devices, unsigned num_devices);
int rtl8139_init_dev(pci_dev_t *pci_dev);

#endif // RTL8139_H
