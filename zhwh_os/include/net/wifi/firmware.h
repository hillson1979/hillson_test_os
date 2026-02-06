/**
 * @file firmware.h
 * @brief Atheros WiFi 固件加载接口
 */

#ifndef FIRMWARE_H
#define FIRMWARE_H

#include "types.h"

// 固件头结构
typedef struct {
    uint32_t magic;           // 魔术字 "BOOT"
    uint32_t fw_size;         // 固件大小
    uint32_t fw_version;      // 固件版本
    uint32_t hw_target;       // 目标硬件
    uint32_t crc32;           // CRC32 校验和
} fw_header_t;

// 固件段类型
typedef enum {
    FW_SEG_TEXT    = 0x01,  // 代码段
    FW_SEG_DATA    = 0x02,  // 数据段
    FW_SEG_BSS     = 0x03,  // BSS 段
    FW_SEG_RAM     = 0x04   // RAM 段
} fw_seg_type_t;

// 固件段描述符
typedef struct {
    uint32_t addr;            // 目标地址
    uint32_t len;             // 长度
    uint32_t type;            // 段类型
    uint32_t checksum;        // 段校验和
} fw_segment_t;

// 固件加载状态
typedef enum {
    FW_STATE_IDLE      = 0,
    FW_STATE_LOADING   = 1,
    FW_STATE_READY     = 2,
    FW_STATE_RUNNING   = 3,
    FW_STATE_ERROR     = 4
} fw_state_t;

// 固件管理结构
typedef struct {
    fw_state_t state;         // 当前状态
    uint32_t fw_addr;         // 固件物理地址
    uint32_t fw_virt;         // 固件虚拟地址
    uint32_t fw_size;         // 固件大小
    uint32_t entry_point;     // 入口点
    uint32_t version;         // 版本号
} fw_manager_t;

// 函数声明
int atheros_fw_init(void);
int atheros_fw_load(const uint8_t *fw_data, uint32_t fw_size);
int atheros_fw_start(void);
int atheros_fw_stop(void);
int atheros_fw_verify(const uint8_t *fw_data, uint32_t fw_size);
fw_state_t atheros_fw_get_state(void);

#endif // FIRMWARE_H
