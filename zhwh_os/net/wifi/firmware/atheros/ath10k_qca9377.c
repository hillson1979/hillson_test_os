/**
 * @file ath10k_qca9377.c
 * @brief Qualcomm Atheros QCA9377 WiFi 固件接口
 *
 * 固件文件: ath10k/QCA9377/hw1.0/firmware-5.bin (真正的 Atheros 固件)
 * 硬件: Qualcomm Atheros QCA9377 802.11ac Wireless Adapter
 * Vendor ID: 168C
 * Device ID: 003E
 *
 * 精简版：使用小型模拟固件，避免内核过大
 */

#include "types.h"

// 小型模拟固件（用于测试）
static const uint8_t ath10k_qca9377_fw5_bin[] = {
    // 固件头
    0x42, 0x4F, 0x4F, 0x54,  // "BOOT" 魔术字
    0x00, 0x00, 0x01, 0x00,  // 固件大小: 256KB
    0x01, 0x00, 0x00, 0x00,  // 版本: 1.0
    0x01, 0x00, 0x00, 0x00,  // HW 目标: QCA9377
    // 固件数据（最小化）
    0x51, 0x43, 0x41, 0x2D, 0x41, 0x54, 0x48, 0x31,
    0x30, 0x4B, 0x00, 0x77, 0x01, 0x00, 0x00, 0x00
};

const uint32_t ath10k_qca9377_fw5_bin_size = sizeof(ath10k_qca9377_fw5_bin);

#define ATH10K_FW_ENTRY 0x0000

// 导出固件数据接口
const uint8_t* atheros_get_fw_data(void) {
    return ath10k_qca9377_fw5_bin;
}

uint32_t atheros_get_fw_size(void) {
    return ath10k_qca9377_fw5_bin_size;
}

uint32_t atheros_get_fw_entry(void) {
    return ATH10K_FW_ENTRY;
}
