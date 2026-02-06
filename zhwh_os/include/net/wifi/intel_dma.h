/**
 * @file intel_dma.h
 * @brief Intel WiFi DMA 数据结构
 *
 * 基于 Linux iwlwifi 驱动的 DMA 结构
 */

#ifndef INTEL_DMA_H
#define INTEL_DMA_H

#include "types.h"

// ==================== Intel TX 描述符结构 ====================

/**
 * @brief TFD (Transmit Frame Descriptor) - Intel 特定的 TX 描述符
 *
 * Intel 使用两套描述符系统：
 * 1. TFD - 主描述符，包含指向 TB 的指针
 * 2. TB (Transfer Buffer) - 实际的数据缓冲区
 */

// TFD 控制位
#define IWL_TFD_CMD                0x00000001  // 命令帧
#define IWL_TFD_DATA              0x00000002  // 数据帧
#define IWL_TFD_TB_COUNT           0x0000001F  // TB 数量掩码

// TFD 最大 TB 数量
#define IWL_MAX_TBS                20

/**
 * @brief TFD (Transmit Frame Descriptor) 结构
 * 每个 TFD 指向多个 TB (Transfer Buffer)
 */
typedef struct {
    uint32_t tb1_addr;        // TB1 物理地址
    uint16_t tb1_len;         // TB1 长度
    uint8_t tb1_flags;        // TB1 标志
    uint8_t num_tbs;          // TB 总数 (0-19)

    uint32_t tb2_addr;        // TB2 物理地址
    uint16_t tb2_len;         // TB2 长度
    uint8_t tb2_flags;        // TB2 标志
    uint8_t reserved;

    // 可以有更多 TB (最多 20 个)
    // 这里简化为只使用前 2 个 TB
} __attribute__((packed)) intel_tfd_t;

// ==================== Intel RX 缓冲区结构 ====================

/**
 * @brief RX buffer 描述符
 */
typedef struct {
    uint32_t addr;            // 缓冲区物理地址
    uint16_t len;             // 缓冲区长度
    uint16_t reserved;        // 保留
} __attribute__((packed)) intel_rbd_t;

// RX 缓冲区大小
#define IWL_RX_BUF_SIZE         4096    // 4KB RX 缓冲区
#define IWL_NUM_RX_BUFS         256     // RX 缓冲区数量

// ==================== Intel TX/RX 环形队列 ====================

/**
 * @brief TX 队列配置
 */
#define IWL_NUM_TX_QUEUES       8       // TX 队列数量
#define IWL_TX_QUEUE_SIZE       256     // 每个 TX 队列的 TFD 数量
#define IWL_CMD_QUEUE_SIZE      256     // 命令队列大小

// 队列类型
typedef enum {
    IWL_TX_QUEUE_CMD = 0,      // 命令队列
    IWL_TX_QUEUE_DATA = 1,     // 数据队列
    IWL_TX_QUEUE_MGMT = 2      // 管理队列
} intel_tx_queue_type_t;

/**
 * @brief Intel TX 队列
 */
typedef struct {
    intel_tfd_t *tfd_base;     // TFD 数组基址（虚拟）
    uint32_t tfd_base_phys;    // TFD 数组基址（物理）

    uint8_t *tb_buffers;       // TB 缓冲区（虚拟）
    uint32_t tb_buffers_phys;  // TB 缓冲区（物理）

    uint16_t write_ptr;        // 写指针
    uint16_t read_ptr;         // 读指针

    uint16_t queue_size;       // 队列大小
    intel_tx_queue_type_t type; // 队列类型
} intel_tx_queue_t;

/**
 * @brief Intel RX 队列
 */
typedef struct {
    intel_rbd_t *rbd_base;     // RBD 数组基址（虚拟）
    uint32_t rbd_base_phys;    // RBD 数组基址（物理）

    uint8_t *buffers;          // RX 缓冲区（虚拟）
    uint32_t buffers_phys;     // RX 缓冲区（物理）

    uint16_t write_ptr;        // 写指针
    uint16_t read_ptr;         // 读指针

    uint16_t num_rbs;          // RBD 数量
} intel_rx_queue_t;

// ==================== CSR 寄存器定义 ====================

// FH (Flow Handler) 寄存器 - 用于 DMA 控制
#define CSR_FBHB_SIZE0         0x0400  // RX RB 的大小基址 0
#define CSR_FBHB_SIZE1         0x0404  // RX RB 的大小基址 1
#define CSR_FBHB_BASE0         0x0500  // RX RB 基址 0
#define CSR_FBHB_BASE1         0x0504  // RX RB 基址 1

// TX 队列寄存器
#define FH_MEM_CBBC_QUEUE0     0x1D00  // Command queue base
#define FH_MEM_CBBC_QUEUE1     0x1D04  // Data queue 1 base
#define FH_MEM_CBBC_QUEUE2     0x1D08  // Data queue 2 base
#define FH_MEM_CBBC_QUEUE3     0x1D0C  // Data queue 3 base
#define FH_MEM_CBBC_QUEUE4     0x1D10  // Data queue 4 base
#define FH_MEM_CBBC_QUEUE5     0x1D14  // Data queue 5 base
#define FH_MEM_CBBC_QUEUE6     0x1D18  // Data queue 6 base
#define FH_MEM_CBBC_QUEUE7     0x1D1C  // Data queue 7 base

// TX 读/写指针寄存器
#define FH_MEM_TFDQ_DB0        0x1F00  // Command queue write pointer
#define FH_MEM_TFDQ_DB1        0x1F04  // Queue 1 write pointer
#define FH_MEM_TFDQ_DB2        0x1F08  // Queue 2 write pointer
#define FH_MEM_TFDQ_DB3        0x1F0C  // Queue 3 write pointer
#define FH_MEM_TFDQ_DB4        0x1F10  // Queue 4 write pointer
#define FH_MEM_TFDQ_DB5        0x1F14  // Queue 5 write pointer
#define FH_MEM_TFDQ_DB6        0x1F18  // Queue 6 write pointer
#define FH_MEM_TFDQ_DB7        0x1F1C  // Queue 7 write pointer

// RX 队列寄存器
#define FH_MEM_RSCSR0_CHNL0    0x1C00  // RX read pointer
#define FH_MEM_RSCSR1_CHNL0    0x1C04  // RX write pointer

// ==================== 函数声明 ====================

// TX 队列管理
int intel_tx_queue_init(uint32_t mem_base, intel_tx_queue_t *q,
                         intel_tx_queue_type_t type, uint16_t size);
void intel_tx_queue_stop(uint32_t mem_base, intel_tx_queue_t *q);
int intel_tx_send(uint32_t mem_base, intel_tx_queue_t *q,
                   const uint8_t *data, uint32_t len);
int intel_tx_complete(uint32_t mem_base, intel_tx_queue_t *q);

// RX 队列管理
int intel_rx_queue_init(uint32_t mem_base, intel_rx_queue_t *q, uint16_t size);
int intel_rx_recv(uint32_t mem_base, intel_rx_queue_t *q,
                   uint8_t *data, uint32_t *len);
void intel_rx_replenish(uint32_t mem_base, intel_rx_queue_t *q);

#endif // INTEL_DMA_H
