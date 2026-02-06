/**
 * @file iwlwifi-6000g2a.h
 * @brief Intel Centrino Advanced-N 6205 WiFi 固件接口
 *
 * 固件文件: iwlwifi-6000g2a-6.ucode (真正的 Intel 固件，677KB)
 * 硬件: Intel Centrino Advanced-N 6205 AG
 * Vendor ID: 8086
 * Device ID: 0085 / 0082
 */

#ifndef IWLWIFI_6000G2A_H
#define IWLWIFI_6000G2A_H

#include "types.h"

// 获取 Intel WiFi 固件数据
const uint8_t* intel_get_fw_data(void);
uint32_t intel_get_fw_size(void);
uint32_t intel_get_fw_entry(void);

#endif // IWLWIFI_6000G2A_H
