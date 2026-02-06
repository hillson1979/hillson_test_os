/**
 * @file e1000.h
 * @brief Intel E1000 网卡驱动头文件
 */

#ifndef E1000_H
#define E1000_H

//#include "types.h"
#include "stdint.h"
#include "net.h"

// 前向声明
typedef struct pci_dev pci_dev_t;

// E1000 设备 ID
#define E1000_VENDOR_ID       0x8086  // Intel
#define E1000_DEVICE_ID       0x100E  // 82540EM (QEMU default)
#define E1000_DEVICE_ID_I82545 0x1004 // 82545EM
#define E1000_DEVICE_ID_I82546 0x1010 // 82546EB
#define E1000_DEVICE_ID_I82579LM 0x1502  // 🔥 新增：82579LM Gigabit Network Connection

// E1000 寄存器偏移（基于内存映射 I/O）
#define E1000_CTRL            0x00000  // Device Control
#define E1000_STATUS          0x00008  // Device Status
#define E1000_EECD            0x00010  // EEPROM/Flash Control
#define E1000_EERD            0x00014  // EEPROM Read
#define E1000_CTRL_EXT        0x00018  // Extended Device Control
#define E1000_MDIC            0x00020  // MDI Control
#define E1000_ICR             0x000C0  // Interrupt Cause Read
#define E1000_ITR             0x000C4  // Interrupt Throttling
#define E1000_ICS             0x000C8  // Interrupt Cause Set
#define E1000_IMS             0x000D0  // Interrupt Mask Set
#define E1000_IMC             0x000D8  // Interrupt Mask Clear
#define E1000_RCTL            0x00100  // RX Control
#define E1000_TCTL            0x00400  // TX Control
#define E1000_TIPG            0x00410  // TX IPG
#define E1000_RDBAL           0x02800  // RX Descriptor Base Low
#define E1000_RDBAH           0x02804  // RX Descriptor Base High
#define E1000_RDLEN           0x02808  // RX Descriptor Length
#define E1000_RDH             0x02810  // RX Descriptor Head
#define E1000_RDT             0x02818  // RX Descriptor Tail
#define E1000_RDTR            0x02820  // RX Delay Timer
#define E1000_RXDCTL          0x02828  // RX Descriptor Control
#define E1000_TDBAL           0x03800  // TX Descriptor Base Low
#define E1000_TDBAH           0x03804  // TX Descriptor Base High
#define E1000_TDLEN           0x03808  // TX Descriptor Length
#define E1000_TDH             0x03810  // TX Descriptor Head
#define E1000_TDT             0x03818  // TX Descriptor Tail
#define E1000_TIDV            0x03820  // TX Interrupt Delay Value
#define E1000_TXDCTL          0x03828  // TX Descriptor Control
//#define E1000_RAL             0x05400  // Receive Address Low
//#define E1000_RAH             0x05404  // Receive Address High
#define E1000_RAL(n)   (0x05400 + (n) * 8)
#define E1000_RAH(n)   (0x05404 + (n) * 8)
#define E1000_MTA             0x05200  // Multicast Table Array
#define E1000_CRCERRS         0x04000  // CRC Error Count
#define E1000_ALGNERRC        0x04004  // Alignment Error Count
#define E1000_SYMERRS         0x04008  // Symbol Error Count
#define E1000_RXERRC          0x0400C  // RX Error Count
#define E1000_MPC             0x04010  // Missed Packets Count
#define E1000_SCC             0x04014  // Single Collision Count
#define E1000_ECOL            0x04018  // Excessive Collisions
#define E1000_MCC             0x0401C  // Multiple Collision Count
#define E1000_LATECOL         0x04020  // Late Collisions
#define E1000_COLC            0x04028  // Collision Count
#define E1000_DC              0x04030  // Defer Count
#define E1000_TNCRS           0x04034  // TX No CRS
#define E1000_SEC             0x04038  // Sequence Error Count
#define E1000_CEXTERR         0x0403C  // Carrier Extension Error
#define E1000_RLEC            0x04040  // Receive Length Error
#define E1000_XONRXC          0x04048  // XON Received
#define E1000_XONTXC          0x0404C  // XON Transmitted
#define E1000_XOFFRXC         0x04050  // XOFF Received
#define E1000_XOFFTXC         0x04054  // XOFF Transmitted
#define E1000_FCRUC           0x04058  // FC Received Unsupported
#define E1000_PRC64           0x0405C  // Packets Received (64 bytes)
#define E1000_PRC127          0x04060  // Packets Received (65-127 bytes)
#define E1000_PRC255          0x04064  // Packets Received (128-255 bytes)
#define E1000_PRC511          0x04068  // Packets Received (256-511 bytes)
#define E1000_PRC1023         0x0406C  // Packets Received (512-1023 bytes)
#define E1000_PRC1522         0x04070  // Packets Received (1024-1522 bytes)
#define E1000_GPRC            0x04074  // Good Packets Received
#define E1000_BPRC            0x04078  // Broadcast Packets Received
#define E1000_MPRC            0x0407C  // Multicast Packets Received
#define E1000_GPTC            0x04080  // Good Packets Transmitted
#define E1000_GORCL           0x04088  // Good Octets Received Low
#define E1000_GORCH           0x0408C  // Good Octets Received High
#define E1000_GOTCL           0x04090  // Good Octets Transmitted Low
#define E1000_GOTCH           0x04094  // Good Octets Transmitted High
#define E1000_RNBC            0x040A0  // RX No Buffers Count
#define E1000_RUC             0x040A4  // RX Undersize Count
#define E1000_RFC             0x040A8  // RX Fragment Count
#define E1000_ROC             0x040AC  // RX Oversize Count
#define E1000_RJC             0x040B0  // RX Jabber Count
#define E1000_MGTPRC          0x040B4  // Management Packets RX
#define E1000_MGTPDC          0x040B8  // Management Packets Dropped
#define E1000_MGTPTC          0x040BC  // Management Packets TX
#define E1000_TORL            0x040C0  // Total Octets RX Low
#define E1000_TORH            0x040C4  // Total Octets RX High
#define E1000_TOTL            0x040C8  // Total Octets TX Low
#define E1000_TOTH            0x040CC  // Total Octets TX High
#define E1000_TPR             0x040D0  // Total Packets RX
#define E1000_TPT             0x040D4  // Total Packets TX
#define E1000_PTC64           0x040D8  // Packets TX (64 bytes)
#define E1000_PTC127          0x040DC  // Packets TX (65-127 bytes)
#define E1000_PTC255          0x040E0  // Packets TX (128-255 bytes)
#define E1000_PTC511          0x040E4  // Packets TX (256-511 bytes)
#define E1000_PTC1023         0x040E8  // Packets TX (512-1023 bytes)
#define E1000_PTC1522         0x040EC  // Packets TX (1024-1522 bytes)
#define E1000_MPTC            0x040F0  // Multicast Packets TX
#define E1000_BPTC            0x040F4  // Broadcast Packets TX
#define E1000_TSCTC           0x040F8  // TCP Segments TX
#define E1000_TSCTFC          0x040FC  // TCP Segments TX Fail
#define E1000_IAC             0x04100  // Interrupt Assertion Count
#define E1000_ICRXPTC         0x04104  // Interrupt Cause RX Pkt Timer
#define E1000_ICRXATC         0x04108  // Interrupt Cause RX Abs Timer
#define E1000_ICTXPTC         0x0410C  // Interrupt Cause TX Pkt Timer
#define E1000_ICTXATC         0x04110  // Interrupt Cause TX Abs Timer
#define E1000_ICTXQEC         0x04118  // Interrupt Cause TX Queue Empty
#define E1000_ICTXQMTC        0x0411C  // Interrupt Cause TX Queue Min
#define E1000_ICRXDMTC        0x04120  // Interrupt Cause RX Desc Min
#define E1000_ICRXOC          0x04124  // Interrupt Cause RX Overrun

// CTRL 寄存器位
#define E1000_CTRL_RST        0x04000000  // Reset
#define E1000_CTRL_ASDE       0x00000020  // Auto-Speed Detection
#define E1000_CTRL_SLU        0x00000040  // Set Link Up
#define E1000_CTRL_LRST       0x00000008  // Link Reset
#define E1000_CTRL_FRCSPD     0x00000200  // Force Speed
#define E1000_CTRL_FRCDPX     0x00000100  // Force Duplex
#define E1000_CTRL_VME        0x40000000  // VLAN Mode Enable

// STATUS 寄存器位
#define E1000_STATUS_LU       0x00000002  // Link Up
#define E1000_STATUS_TXOFF    0x00000010  // TX Off
#define E1000_STATUS_SPEED_10 0x00000000  // 10 Mbps
#define E1000_STATUS_SPEED_100 0x00000080 // 100 Mbps
#define E1000_STATUS_SPEED_1000 0x00000100 // 1000 Mbps
#define E1000_STATUS_NVM_DONE 0x00080000  // NVM auto-load done (82579LM需要等待此位)

// EEPROM 寄存器位
#define E1000_EECD_SK        0x00000001  // EEPROM Clock
#define E1000_EECD_CS        0x00000002  // EEPROM Chip Select
#define E1000_EECD_DI        0x00000004  // EEPROM Data In
#define E1000_EECD_DO        0x00000008  // EEPROM Data Out

// EERD 寄存器位（EEPROM Read）
#define E1000_EERD_START     0x00000001  // Start EEPROM read
#define E1000_EERD_DONE      0x00000010  // Read done
#define E1000_EERD_ADDR_SHIFT 8          // Address offset
#define E1000_EERD_ADDR_MASK  0x0000FF00 // Address mask
#define E1000_EERD_DATA_SHIFT 16         // Data offset
#define E1000_EERD_DATA_MASK  0xFFFF0000 // Data mask

// RCTL 寄存器位
#define E1000_RCTL_RST        0x00000001  // Receiver Reset
#define E1000_RCTL_EN         0x00000002  // Receiver Enable
#define E1000_RCTL_SBP        0x00000004  // Save Bad Packets
#define E1000_RCTL_UPE        0x00000008  // Unicast Promiscuous
#define E1000_RCTL_MPE        0x00000010  // Multicast Promiscuous
#define E1000_RCTL_LPE        0x00000020  // Long Packet Reception
#define E1000_RCTL_LBM_NO     0x00000000  // No Loopback
#define E1000_RCTL_LBM_MAC    0x00000040  // MAC Loopback
#define E1000_RCTL_RDMTS_HALF 0x00000000  // Free Buffer Threshold
#define E1000_RCTL_RDMTS_QUAT 0x00000100  // ... Quarter
#define E1000_RCTL_RDMTS_EIGTH 0x00000200 // ... Eighth
#define E1000_RCTL_MO_SHIFT   12          // Multicast Offset
#define E1000_RCTL_BAM        0x00008000  // Broadcast Accept Mode
#define E1000_RCTL_VFE        0x00010000  // VLAN Filter Enable
#define E1000_RCTL_CFIEN      0x00020000  // CFI Enable
#define E1000_RCTL_CFI        0x00040000  // Canonical Form Ind
#define E1000_RCTL_DPF        0x00080000  // Discard Pause Frames
#define E1000_RCTL_PMCF       0x00100000  // Pass MAC Control Frames
#define E1000_RCTL_BSEX       0x00200000  // Buffer Size Extension
#define E1000_RCTL_SECRC      0x00400000  // Strip Ethernet CRC
#define E1000_RCTL_FLXBUF_MASK 0x78000000 // Flexbuffer size
#define E1000_RCTL_FLXBUF_SHIFT 27

// TCTL 寄存器位
#define E1000_TCTL_RST        0x00000001  // Transmitter Reset
#define E1000_TCTL_EN         0x00000002  // Transmit Enable
#define E1000_TCTL_BCE        0x00000004  // Busy Check Enable
#define E1000_TCTL_PSP        0x00000008  // Pad Short Packets
#define E1000_TCTL_CT_SHIFT   4           // Collision Threshold
#define E1000_TCTL_CT_MASK    0x000000F0
#define E1000_TCTL_COLD_SHIFT 12          // Collision Distance
#define E1000_TCTL_COLD_MASK  0x00003FF0
#define E1000_TCTL_SWXOFF     0x00400000  // Software XOFF Transmission
#define E1000_TCTL_PBE        0x00800000  // Programmable Burst Enable
#define E1000_TCTL_RTLC       0x01000000  // Re-transmit on Late Collision
#define E1000_TCTL_NRTU       0x02000000  // No Re-transmit on Underrrun

// 中断位
#define E1000_ICR_TXDW        0x00000001  // TX Descriptor Written Back
#define E1000_ICR_TXQE        0x00000002  // TX Queue Empty
#define E1000_ICR_LSC         0x00000004  // Link Status Change
#define E1000_ICR_RXSEQ       0x00000008  // RX Sequence Error
#define E1000_ICR_RXDMT0      0x00000010  // RX Descriptor Minimum Threshold
#define E1000_ICR_RXO         0x00000040  // RX Overrun
#define E1000_ICR_RXT0        0x00000080  // RX Timer Interrupt
#define E1000_ICR_VMMB        0x00000100  // VM Mailbox
#define E1000_ICR_GPI_EN0     0x00000800  // General Purpose Interrupt
#define E1000_ICR_GPI_EN1     0x00001000
#define E1000_ICR_GPI_EN2     0x00002000
#define E1000_ICR_GPI_EN3     0x00004000
#define E1000_ICR_TXD_LOW     0x00008000  // TX Descriptor Low Threshold
#define E1000_ICR_SRPD        0x00010000  // Small Receive Packet Detected
#define E1000_ICR_ACK         0x00020000  // Receive ACK
#define E1000_ICR_MNG         0x00040000  // Manageability Event
#define E1000_ICR_DOUTSYNC    0x10000000  // DMA Doorbell sync

// 描述符相关
#define E1000_NUM_RX_DESC     64
#define E1000_NUM_TX_DESC     64
#define E1000_RX_BUF_SIZE     2048
#define E1000_TX_BUF_SIZE     2048
#define E1000_RCTL_BSIZE_2048   0x00000000

// TX 描述符状态位
#define E1000_TXD_STAT_DD     0x00000001  // Descriptor Done

// RX 描述符状态位
#define E1000_RXD_STAT_DD     0x00000001  // Descriptor Done
#define E1000_RXD_STAT_EOP    0x00000002  // End of Packet

// E1000 RX 描述符（Legacy 格式，32位系统专用）
//
// 🔥 32位系统优化：buffer_addr 改为 32 位
typedef struct __attribute__((packed)) {
    uint32_t buffer_addr;     // Bytes 0-3:  Buffer 物理地址（32位）
    uint32_t padding;         // Bytes 4-7:  填充（保持16字节对齐）
    uint16_t length;          // Bytes 8-9:  包长度
    uint16_t csum;            // Bytes 10-11: 校验和
    uint8_t  status;          // Byte 12:    状态 (DD, EOP)
    uint8_t  errors;          // Byte 13:    错误标志
    uint16_t special;         // Bytes 14-15: VLAN/特殊
} e1000_rx_desc_t;

// E1000 TX 描述符（32位系统专用）
typedef struct __attribute__((packed)) {
    uint32_t buffer_addr;     // Bytes 0-3:  Buffer 物理地址（32位）
    uint32_t padding;         // Bytes 4-7:  填充
    uint16_t length;          // Bytes 8-9:  包长度
    uint8_t  cso;             // Byte 10:    校验和偏移
    uint8_t  cmd;             // Byte 11:    命令
    uint8_t  status;          // Byte 12:    状态
    uint8_t  css;             // Byte 13:    校验和起始
    uint16_t vlan;            // Bytes 14-15: VLAN 标签
} e1000_tx_desc_t;

// 🔥 静态断言：确保大小为 16 字节
_Static_assert(sizeof(e1000_rx_desc_t) == 16, "e1000_rx_desc_t must be 16 bytes");
_Static_assert(sizeof(e1000_tx_desc_t) == 16, "e1000_tx_desc_t must be 16 bytes");

// TX 命令位（用于 cmd 字段，uint8_t）
#define E1000_TXD_CMD_EOP     0x01  // End of Packet
#define E1000_TXD_CMD_IFCS    0x02  // Insert FCS
#define E1000_TXD_CMD_IC      0x04  // Insert Checksum
#define E1000_TXD_CMD_RS      0x08  // Report Status
#define E1000_TXD_CMD_RPS     0x10  // Report Packet Sent
#define E1000_TXD_CMD_VLE     0x40  // VLAN Packet Enable
#define E1000_TXD_CMD_IDE     0x80  // Interrupt Delay Enable

// E1000 私有数据结构
typedef struct {
    uint32_t mmio_base;        // MMIO 基地址（物理地址）
    uint32_t mmio_base_virt;   // MMIO 基地址（虚拟地址）
    uint32_t irq;              // IRQ 号

    e1000_rx_desc_t *rx_desc;  // RX 描述符数组
    uint32_t rx_desc_phys;     // 🔥 RX 描述符物理地址（必须保存!）
    uint8_t *rx_buffers[E1000_NUM_RX_DESC];  // RX 缓冲区
    uint16_t rx_cur;           // 当前 RX 描述符索引

    e1000_tx_desc_t *tx_desc;  // TX 描述符数组
    uint32_t tx_desc_phys;     // 🔥 TX 描述符物理地址
    uint8_t *tx_buffers[E1000_NUM_TX_DESC];  // TX 缓冲区
    uint32_t tx_buffers_dma[E1000_NUM_TX_DESC]; // 🔥 TX 缓冲区 DMA 物理地址
    uint16_t tx_cur;           // 当前 TX 描述符索引
    uint16_t tx_tail;          // TX tail 指针

    uint8_t mac_addr[ETH_ALEN]; // MAC 地址

    // 🔥 调试统计（用于 dump_rx 显示）
    uint32_t intr_count;       // 中断触发次数
    uint32_t recv_call_count;  // e1000_recv 调用次数
    uint32_t empty_recv_count; // 空接收次数（cur == RDH）
    uint32_t packets_processed;// 实际处理的包数
} e1000_priv_t;

// 函数声明
int e1000_init(const char *dev_name);
int e1000_probe(pci_dev_t **devices, unsigned num_devices, const char *dev_name);
int e1000_init_dev(pci_dev_t *pci_dev, const char *dev_name);
void e1000_isr(void);
void e1000_poll_rx(net_device_t *dev);
void e1000_dump_regs(void);
void e1000_dump_rx_regs(void);
int e1000_ifup(const char *dev_name);
int e1000_loopback_test(void);  // 🔥 硬件 loopback 测试（轮询版本）
int e1000_loopback_test_interrupt(void);  // 🔥 硬件 loopback 测试（中断版本）

// 导出 IRQ 号
extern int e1000_irq;

#endif // E1000_H
