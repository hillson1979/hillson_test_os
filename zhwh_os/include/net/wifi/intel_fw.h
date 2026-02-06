/**
 * @file intel_fw.h
 * @brief Intel WiFi 固件通信协议
 *
 * 基于 Linux iwlwifi 驱动的固件接口
 */

#ifndef INTEL_FW_H
#define INTEL_FW_H

#include "types.h"

// ==================== 固件命令定义 ====================

/**
 * @brief 命令 ID (CMD ID)
 */
enum iwl_cmd_id {
    // 系统状态和配置
    REPLY_ALIVE = 1,           // 固件启动响应
    REPLY_ERROR = 2,           // 错误响应

    // RX/TX 配置
    CMD_RXON = 8,              // RX (接收) 配置
    REPLY_RXON = 9,            // RXON 响应
    CMD_RXON_ASSOC = 10,       // 关联配置
    REPLY_RXON_ASSOC = 11,     // 关联响应

    // 扫描
    CMD_SCAN = 12,             // 扫描命令
    REPLY_SCAN = 13,           // 扫描响应

    // 功率管理
    CMD_SET_POWER = 24,        // 设置电源管理

    // TX 相关
    CMD_TX_DATA = 27,          // 发送数据

    // LED 控制
    CMD_LEDS = 36,             // LED 控制

    // 其他
    REPLY_STATISTICS = 42,     // 统计信息
};

/**
 * @brief 命令头结构
 */
typedef struct {
    uint32_t cmd;              // 命令 ID
    uint32_t len;              // 数据长度
    uint8_t data[0];           // 可变长度数据
} __attribute__((packed)) iwl_cmd_header_t;

/**
 * @brief REPLY_ALIVE 响应
 * 固件启动后会发送此响应
 */
typedef struct {
    uint16_t status;           // 状态码
    uint8_t config;            // 配置
    uint8_t subtype;           // 子类型
    uint32_t log_event;        // 日志事件
} __attribute__((packed)) iwl_alive_resp_t;

// ALIVE 状态码
#define IWL_ALIVE_STATUS_OK 0x01

/**
 * @brief CMD_RXON - RX 配置命令
 */
typedef struct {
    uint64_t node_addr;        // MAC 地址（低 48 位）
    uint32_t reserved1;
    uint16_t flags;            // 配置标志
    uint16_t filter_flags;     // 过滤标志
    uint8_t channel;           // 信道
    uint8_t ofdm_basic_rates;  // OFDM 基本速率
    uint8_t cck_basic_rates;   // CCK 基本速率
    uint8_t assoc_id;          // 关联 ID
    uint32_t reserved3;
    uint32_t reserved4;
    uint32_t reserved5;
    uint32_t reserved6;
} __attribute__((packed)) iwl_rxon_cmd_t;

// RXON 标志
#define RXON_FLG_TSF2HOST_MSK          0x00100000
#define RXON_FLG_CTL_CHANNEL_MODE_POS  22
#define RXON_FLG_CTL_CHANNEL_MODE_MSK  0x00C00000
#define RXON_FLG_RX_CHANNEL_DRIVER_POS  23

/**
 * @brief CMD_TX_DATA - TX 数据格式
 */
typedef struct {
    uint16_t len;              // 帧长度
    uint8_t rate;              // 传输速率
    uint8_t sta_id;            // 站点 ID
    uint32_t offload_assist;   // 卸载辅助
    uint32_t flags;            // 标志
    uint8_t frame[0];          // 802.11 帧数据
} __attribute__((packed)) iwl_tx_cmd_t;

// TX 标志
#define TX_CMD_FLG_SEQ_CTL_MSK   0x0080
#define TX_CMD_FLG_BT_DIS_MSK    0x0100

/**
 * @brief CMD_SCAN - 扫描命令
 */
typedef struct {
    uint16_t len;              // 命令长度
    uint8_t type;              // 扫描类型
    uint8_t flags;             // 标志
    uint32_t status;           // 状态
    uint8_t channel;           // 起始信道
    uint8_t active_dwell;      // 主动停留时间
    uint8_t passive_dwell;     // 被动停留时间
    uint8_t reserved;
    uint16_t quiet_time;       // 安静时间
    uint16_t quiet_plcp_th;    // 安静 PLCP 阈值
    uint16_t flags_mask;       // 标志掩码
    uint16_t reserved2;
} __attribute__((packed)) iwl_scan_cmd_t;

// ==================== 函数声明 ====================

// 固件通信初始化
int intel_fw_init(uint32_t mem_base);
int intel_fw_wait_alive(uint32_t mem_base);
int intel_fw_send_cmd(uint32_t mem_base, uint32_t cmd_id,
                       const void *data, uint32_t len);
int intel_fw_rxon(uint32_t mem_base, uint8_t channel,
                   const uint8_t *mac_addr);
int intel_fw_set_mode(uint32_t mem_base, int mode);

// 数据发送
int intel_fw_tx_data(uint32_t mem_base, const uint8_t *data, uint32_t len);

// 获取队列指针
void *intel_fw_get_rx_queue(void);

// 固件响应处理（从中断处理调用）
int intel_fw_handle_response(uint8_t *data, uint32_t len);

#endif // INTEL_FW_H
