/**
 * @file dma.h
 * @brief Atheros WiFi DMA 管理
 */

#ifndef DMA_H
#define DMA_H

#include "types.h"

// DMA 描述符结构
typedef struct {
    uint32_t addr;        // 缓冲区物理地址
    uint32_t ctrl;        // 控制字
    uint32_t status;      // 状态字
    uint32_t len;         // 数据长度
    uint32_t next;        // 下一个描述符
} dma_desc_t;

// DMA 传输方向
typedef enum {
    DMA_DIR_TX = 0,       // 发送
    DMA_DIR_RX = 1        // 接收
} dma_dir_t;

// DMA 通道
typedef struct {
    dma_desc_t *desc;     // 描述符虚拟地址
    uint32_t desc_phys;   // 描述符物理地址
    uint8_t *buffers;     // 缓冲区虚拟地址（连续内存）
    uint32_t buf_phys;    // 缓冲区物理地址（连续内存）
    uint16_t num_desc;    // 描述符数量
    uint16_t buf_size;    // 缓冲区大小
    uint16_t head;        // 头指针（可用描述符）
    uint16_t tail;        // 尾指针（已完成描述符）
    dma_dir_t dir;        // 方向
} dma_channel_t;

// DMA 统计
typedef struct {
    uint32_t tx_packets;     // 发送包数
    uint32_t tx_bytes;       // 发送字节数
    uint32_t tx_errors;      // 发送错误
    uint32_t rx_packets;     // 接收包数
    uint32_t rx_bytes;       // 接收字节数
    uint32_t rx_errors;      // 接收错误
    uint32_t rx_dropped;     // 丢弃包数
} dma_stats_t;

// 函数声明
int atheros_dma_init(uint32_t mem_base, dma_channel_t *tx_ch, dma_channel_t *rx_ch);
void atheros_dma_cleanup(dma_channel_t *channel);

int atheros_dma_tx_send(dma_channel_t *ch, const uint8_t *data, uint32_t len);
int atheros_dma_rx_recv(dma_channel_t *ch, uint8_t *data, uint32_t *len);

int atheros_dma_tx_complete(dma_channel_t *ch);
int atheros_dma_rx_avail(dma_channel_t *ch);

void atheros_dma_get_stats(const dma_channel_t *ch, dma_stats_t *stats);
void atheros_dma_reset_stats(dma_channel_t *ch);

#endif // DMA_H
