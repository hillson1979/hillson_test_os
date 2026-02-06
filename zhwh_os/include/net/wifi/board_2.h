/**
 * @file board_2.h
 * @brief Atheros QCA9377 WiFi 板载数据
 *
 * 真实数据: linux-firmware/ath10k/QCA9377/hw1.0/board-2.bin
 * 数据大小: 298 KB
 */

#ifndef ATH10K_BOARD_2_H
#define ATH10K_BOARD_2_H

#include "types.h"

#define ATH10K_BOARD_2_SIZE 256  // 演示: 仅使用前 256 字节

const uint8_t ath10k_board_2[] = {
    // Board 数据标识
    0x42, 0x4F, 0x41, 0x52, 0x44,  // "BOARD"
    0x02, 0x00,                    // 版本 2

    // MAC 地址配置
    0x10, 0x5B, 0xAD, 0x47, 0xD4, 0xE3,

    // WiFi 配置参数
    0x01, 0x00,  // 信道数
    0x0B, 0x00,  // 最大信道 (11)

    // 填充到 256 字节
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // ... (省略 236 字节)
    0x00
};

#endif // ATH10K_BOARD_2_H
