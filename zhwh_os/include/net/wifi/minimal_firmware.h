/**
 * @file minimal_firmware.h
 * @brief WiFi 固件接口（使用最小化固件，避免内核过大）
 */

#ifndef MINIMAL_FIRMWARE_H
#define MINIMAL_FIRMWARE_H

#include "types.h"

// Intel WiFi 固件接口（使用最小化固件）
const uint8_t* intel_get_fw_data(void);
uint32_t intel_get_fw_size(void);
uint32_t intel_get_fw_entry(void);

// Atheros WiFi 固件接口（使用最小化固件）
const uint8_t* atheros_get_fw_data(void);
uint32_t atheros_get_fw_size(void);
uint32_t atheros_get_fw_entry(void);

// 固件状态枚举
typedef enum {
    FW_STATUS_STUB = 0,      // Stub 固件（仅用于测试）
    FW_STATUS_REAL = 1,      // 真实固件
    FW_STATUS_EXTERNAL = 2   // 外部加载（暂未实现）
} fw_status_t;

// 获取固件状态
fw_status_t intel_get_fw_status(void);
fw_status_t atheros_get_fw_status(void);

#endif // MINIMAL_FIRMWARE_H
