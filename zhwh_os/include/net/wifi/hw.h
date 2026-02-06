/**
 * @file hw.h
 * @brief Atheros WiFi 硬件初始化和控制
 */

#ifndef HW_H
#define HW_H

#include "types.h"

// 硬件初始化状态
typedef enum {
    HW_STATE_RESET      = 0,
    HW_STATE_INIT       = 1,
    HW_STATE_READY      = 2,
    HW_STATE_RUNNING    = 3,
    HW_STATE_ERROR      = 4
} hw_state_t;

// 硬件统计信息
typedef struct {
    uint32_t tx_frames;      // 发送帧数
    uint32_t rx_frames;      // 接收帧数
    uint32_t tx_bytes;       // 发送字节数
    uint32_t rx_bytes;       // 接收字节数
    uint32_t tx_errors;      // 发送错误
    uint32_t rx_errors;      // 接收错误
    uint32_t tx_dropped;     // 丢弃的发送包
    uint32_t rx_dropped;     // 丢弃的接收包
    uint32_t interrupts;     // 中断次数
} hw_stats_t;

// 函数声明
int atheros_hw_init(uint32_t mem_base);
int atheros_hw_reset(uint32_t mem_base);
int atheros_hw_enable_wlan(uint32_t mem_base);
int atheros_hw_disable_wlan(uint32_t mem_base);
int atheros_hw_enable_interrupts(uint32_t mem_base);
int atheros_hw_disable_interrupts(uint32_t mem_base);
int atheros_hw_read_mac_addr(uint32_t mem_base, uint8_t *mac);
uint32_t atheros_hw_get_int_status(uint32_t mem_base);
void atheros_hw_ack_interrupts(uint32_t mem_base, uint32_t ints);
void atheros_hw_get_stats(uint32_t mem_base, hw_stats_t *stats);

// RTC 和电源管理
int atheros_hw_wakeup(uint32_t mem_base);
int atheros_hw_sleep(uint32_t mem_base);
int atheros_hw_wait_for_wakeup(uint32_t mem_base);

// PCIe 配置
int atheros_hw_pcie_init(uint32_t mem_base);
int atheros_hw_pcie_enable_msi(uint32_t mem_base);

#endif // HW_H
