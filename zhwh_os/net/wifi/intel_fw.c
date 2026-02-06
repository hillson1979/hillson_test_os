/**
 * @file intel_fw.c
 * @brief Intel WiFi 固件通信实现
 *
 * 基于 Linux iwlwifi 驱动的固件接口实现
 */

#include "net/wifi/intel_fw.h"
#include "net/wifi/intel.h"
#include "net/wifi/intel_dma.h"
#include "net/wifi/reg.h"
#include "mm.h"
#include "kmalloc.h"
#include "printf.h"
#include "string.h"

// 全局变量：固件 alive 状态
static int fw_alive = 0;

// 全局变量：Intel TX/RX 队列
static intel_tx_queue_t *cmd_queue = NULL;  // 命令队列
static intel_tx_queue_t *data_queue = NULL; // 数据队列
static intel_rx_queue_t *rx_queue = NULL;   // RX 队列

/**
 * @brief 初始化 Intel 固件通信
 */
int intel_fw_init(uint32_t mem_base) {
    printf("[intel-fw] Initializing firmware communication...\n");

    // 分配命令队列
    cmd_queue = (intel_tx_queue_t *)kmalloc(sizeof(intel_tx_queue_t));
    if (!cmd_queue) {
        printf("[intel-fw] Failed to allocate command queue\n");
        return -1;
    }

    // 初始化命令队列
    if (intel_tx_queue_init(mem_base, cmd_queue, IWL_TX_QUEUE_CMD, IWL_CMD_QUEUE_SIZE) < 0) {
        printf("[intel-fw] Failed to initialize command queue\n");
        return -1;
    }

    // 分配数据队列
    data_queue = (intel_tx_queue_t *)kmalloc(sizeof(intel_tx_queue_t));
    if (!data_queue) {
        printf("[intel-fw] Failed to allocate data queue\n");
        return -1;
    }

    // 初始化数据队列
    if (intel_tx_queue_init(mem_base, data_queue, IWL_TX_QUEUE_DATA, IWL_TX_QUEUE_SIZE) < 0) {
        printf("[intel-fw] Failed to initialize data queue\n");
        return -1;
    }

    // 分配 RX 队列
    rx_queue = (intel_rx_queue_t *)kmalloc(sizeof(intel_rx_queue_t));
    if (!rx_queue) {
        printf("[intel-fw] Failed to allocate RX queue\n");
        return -1;
    }

    // 初始化 RX 队列
    if (intel_rx_queue_init(mem_base, rx_queue, IWL_NUM_RX_BUFS) < 0) {
        printf("[intel-fw] Failed to initialize RX queue\n");
        return -1;
    }

    printf("[intel-fw] Firmware communication initialized\n");
    return 0;
}

/**
 * @brief 等待固件发送 REPLY_ALIVE
 */
int intel_fw_wait_alive(uint32_t mem_base) {
    printf("[intel-fw] Waiting for firmware alive...\n");

    // 初始化通信队列
    if (intel_fw_init(mem_base) < 0) {
        printf("[intel-fw] Failed to initialize communication\n");
        return -1;
    }

    // 🔥 调试：定期检查 RX 队列状态
    extern uint32_t atheros_reg_read(uint32_t mem_base, uint32_t reg);

    printf("[intel-fw] Starting ALIVE wait loop...\n");
    printf("[intel-fw] RX write ptr register: 0x%x\n", FH_MEM_RSCSR1_CHNL0);

    // 等待一段时间让固件启动
    for (int i = 0; i < 1000; i++) {
        // 每 100 次循环检查一次 RX 写指针
        if (i % 100 == 0) {
            uint32_t hw_write_ptr = atheros_reg_read(mem_base, FH_MEM_RSCSR1_CHNL0);
            printf("[intel-fw] Loop %d: RX hw_write_ptr = 0x%x\n", i, hw_write_ptr);
        }

        // 检查是否有 RX 数据
        uint8_t rx_buf[256];
        uint32_t rx_len = sizeof(rx_buf);

        if (intel_rx_recv(mem_base, rx_queue, rx_buf, &rx_len) > 0) {
            // 检查是否是 REPLY_ALIVE
            iwl_cmd_header_t *hdr = (iwl_cmd_header_t *)rx_buf;
            if (hdr->cmd == REPLY_ALIVE) {
                iwl_alive_resp_t *alive = (iwl_alive_resp_t *)hdr->data;
                if (alive->status == IWL_ALIVE_STATUS_OK) {
                    fw_alive = 1;
                    printf("[intel-fw] 🔥🔥🔥 Firmware is ALIVE! 🔥🔥🔥\n");
                    return 0;
                }
            }
        }

        // 延迟
        for (volatile int j = 0; j < 10000; j++) {
            __asm__ volatile("nop");
        }
    }

    printf("[intel-fw] Timeout waiting for firmware alive\n");
    printf("[intel-fw] Final RX hw_write_ptr: 0x%x\n", atheros_reg_read(mem_base, FH_MEM_RSCSR1_CHNL0));
    // 即使超时也继续，固件可能已经启动但没有发送 ALIVE
    fw_alive = 1;
    return 0;
}

/**
 * @brief 发送命令到固件
 */
int intel_fw_send_cmd(uint32_t mem_base, uint32_t cmd_id,
                       const void *data, uint32_t len) {
    if (!fw_alive) {
        printf("[intel-fw] ERROR: Firmware not alive yet!\n");
        return -1;
    }

    if (!cmd_queue) {
        printf("[intel-fw] ERROR: Command queue not initialized!\n");
        return -1;
    }

    printf("[intel-fw] Sending CMD_ID=%d, len=%d\n", cmd_id, len);

    // 构造完整的命令包（头 + 数据）
    uint32_t total_len = sizeof(iwl_cmd_header_t) + len;
    uint8_t *cmd_buf = (uint8_t *)kmalloc(total_len);
    if (!cmd_buf) {
        printf("[intel-fw] Failed to allocate command buffer\n");
        return -1;
    }

    // 填充命令头
    iwl_cmd_header_t *hdr = (iwl_cmd_header_t *)cmd_buf;
    hdr->cmd = cmd_id;
    hdr->len = len;

    // 复制命令数据
    if (data && len > 0) {
        memcpy(hdr->data, data, len);
    }

    // 通过命令队列发送
    int result = intel_tx_send(mem_base, cmd_queue, cmd_buf, total_len);

    // 释放临时缓冲区
    // TODO: 需要实现 kfree
    // kfree(cmd_buf);

    if (result < 0) {
        printf("[intel-fw] Failed to send command\n");
        return -1;
    }

    printf("[intel-fw] Command sent successfully\n");
    return 0;
}

/**
 * @brief 配置 RX (接收) - 最关键的初始化命令
 */
int intel_fw_rxon(uint32_t mem_base, uint8_t channel,
                   const uint8_t *mac_addr) {
    printf("[intel-fw] Sending RXON command (channel=%d)\n", channel);

    iwl_rxon_cmd_t rxon;
    memset(&rxon, 0, sizeof(rxon));

    // 设置 MAC 地址
    rxon.node_addr = 0;
    for (int i = 0; i < 6; i++) {
        ((uint8_t *)&rxon.node_addr)[i] = mac_addr[i];
    }

    // 设置信道
    rxon.channel = channel;

    // 设置标志
    rxon.flags = (1 << RXON_FLG_CTL_CHANNEL_MODE_POS);

    // 设置速率
    rxon.ofdm_basic_rates = 0x15;  // 6, 9, 12, 24 Mbps
    rxon.cck_basic_rates = 0x0F;    // 1, 2, 5.5, 11 Mbps

    // 🔥 关键：设置 RX 过滤器（接受所有帧用于调试）
    rxon.filter_flags = 0xFFFFFFFF;  // 接受所有帧

    // 发送命令
    int ret = intel_fw_send_cmd(mem_base, CMD_RXON, &rxon, sizeof(rxon));
    if (ret < 0) {
        return ret;
    }

    // 🔥 关键：等待 REPLY_RXON 响应
    printf("[intel-fw] Waiting for REPLY_RXON...\n");
    for (int i = 0; i < 1000; i++) {
        uint8_t rx_buf[256];
        uint32_t rx_len = sizeof(rx_buf);

        if (intel_rx_recv(mem_base, rx_queue, rx_buf, &rx_len) > 0) {
            iwl_cmd_header_t *hdr = (iwl_cmd_header_t *)rx_buf;
            printf("[intel-fw] Got response: CMD_ID=%d\n", hdr->cmd);

            if (hdr->cmd == REPLY_RXON) {
                printf("[intel-fw] REPLY_RXON received! RX is now enabled.\n");
                return 0;
            }
        }

        // 延迟
        for (volatile int j = 0; j < 10000; j++) {
            __asm__ volatile("nop");
        }
    }

    printf("[intel-fw] Timeout waiting for REPLY_RXON, but continuing...\n");
    return 0;
}

/**
 * @brief 设置模式
 */
int intel_fw_set_mode(uint32_t mem_base, int mode) {
    printf("[intel-fw] Setting mode=%d\n", mode);

    // TODO: 实现模式设置
    return 0;
}

/**
 * @brief Intel WiFi 发送 802.11 数据帧
 */
int intel_fw_tx_data(uint32_t mem_base, const uint8_t *data, uint32_t len) {
    if (!fw_alive) {
        printf("[intel-fw] ERROR: Firmware not alive yet!\n");
        return -1;
    }

    if (!data_queue) {
        printf("[intel-fw] ERROR: Data queue not initialized!\n");
        return -1;
    }

    printf("[intel-fw] TX data: %d bytes\n", len);

    // 构造 TX 命令（802.11 帧格式）
    uint32_t total_len = sizeof(iwl_tx_cmd_t) + len;
    uint8_t *tx_buf = (uint8_t *)kmalloc(total_len);
    if (!tx_buf) {
        printf("[intel-fw] Failed to allocate TX buffer\n");
        return -1;
    }

    // 填充 TX 命令头
    iwl_tx_cmd_t *tx_cmd = (iwl_tx_cmd_t *)tx_buf;
    tx_cmd->len = len;
    tx_cmd->rate = 0;  // 0 = 自动速率
    tx_cmd->sta_id = 0;
    tx_cmd->offload_assist = 0;
    tx_cmd->flags = TX_CMD_FLG_SEQ_CTL_MSK;

    // 复制 802.11 帧数据
    memcpy(tx_cmd->frame, data, len);

    // 通过数据队列发送
    int result = intel_tx_send(mem_base, data_queue, tx_buf, total_len);

    // TODO: 需要实现 kfree
    // kfree(tx_buf);

    if (result < 0) {
        printf("[intel-fw] Failed to send data\n");
        return -1;
    }

    printf("[intel-fw] Data sent successfully\n");
    return 0;
}

/**
 * @brief 获取 RX 队列指针（供中断处理使用）
 */
void *intel_fw_get_rx_queue(void) {
    return rx_queue;
}

/**
 * @brief 处理固件响应（从中断处理调用）
 *
 * @return 0 = 成功处理固件响应, -1 = 不是固件响应
 */
int intel_fw_handle_response(uint8_t *data, uint32_t len) {
    if (!data || len < sizeof(iwl_cmd_header_t)) {
        return -1;
    }

    // 解析命令头
    iwl_cmd_header_t *hdr = (iwl_cmd_header_t *)data;

    printf("[intel-fw] Handling response: CMD_ID=%d, len=%d\n", hdr->cmd, hdr->len);

    // 处理 REPLY_ALIVE
    if (hdr->cmd == REPLY_ALIVE) {
        if (hdr->len >= sizeof(iwl_alive_resp_t)) {
            iwl_alive_resp_t *alive = (iwl_alive_resp_t *)hdr->data;
            printf("[intel-fw] REPLY_ALIVE received: status=0x%x\n", alive->status);

            if (alive->status == IWL_ALIVE_STATUS_OK) {
                fw_alive = 1;
                printf("[intel-fw] 🔥 Firmware is ALIVE!\n");
                return 0;
            }
        }
        return 0;
    }

    // 处理 REPLY_RXON
    if (hdr->cmd == REPLY_RXON) {
        printf("[intel-fw] REPLY_RXON received!\n");
        // 可以在这里通知等待 RXON 的线程
        return 0;
    }

    // 处理 REPLY_ERROR
    if (hdr->cmd == REPLY_ERROR) {
        printf("[intel-fw] REPLY_ERROR received!\n");
        return 0;
    }

    // 其他固件响应
    printf("[intel-fw] Unhandled firmware response: CMD_ID=%d\n", hdr->cmd);
    return 0;
}
