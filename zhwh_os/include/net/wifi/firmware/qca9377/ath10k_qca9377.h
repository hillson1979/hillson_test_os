/**
 * @file ath10k_qca9377.h
 * @brief Qualcomm Atheros QCA9377 WiFi 固件接口
 */

#ifndef ATH10K_QCA9377_H
#define ATH10K_QCA9377_H

#include "types.h"

// 获取 Atheros WiFi 固件数据
const uint8_t* atheros_get_fw_data(void);
uint32_t atheros_get_fw_size(void);
uint32_t atheros_get_fw_entry(void);

#endif // ATH10K_QCA9377_H
