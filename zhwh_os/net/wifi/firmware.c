/**
 * @file firmware.c
 * @brief Atheros WiFi 固件加载实现
 */

#include "net/wifi/firmware.h"
#include "net/wifi/atheros.h"
#include "net/wifi/reg.h"
#include "net/wifi/fw_data.h"
#include "mm.h"
#include "string.h"
#include "printf.h"

static fw_manager_t fw_mgr = {0};

/**
 * @brief 固件管理器初始化
 */
int atheros_fw_init(void) {
    memset(&fw_mgr, 0, sizeof(fw_mgr));
    fw_mgr.state = FW_STATE_IDLE;
    printf("[atheros-fw] Firmware manager initialized\n");
    return 0;
}

/**
 * @brief 验证固件
 */
int atheros_fw_verify(const uint8_t *fw_data, uint32_t fw_size) {
    if (!fw_data || fw_size < 16) {
        printf("[atheros-fw] Invalid firmware data\n");
        return -1;
    }

    // 🔥 调试：打印固件前 16 字节（已屏蔽）
    // printf("[atheros-fw] Debug: fw_data=%p, fw_size=%d\n", fw_data, fw_size);
    // printf("[atheros-fw] Debug: First 16 bytes: ");
    // for (int i = 0; i < 16 && i < fw_size; i++) {
    //     printf("%02X ", fw_data[i]);
    // }
    // printf("\n");

    // 检查魔术字（支持 Atheros "BOOT" 和 Intel "IWL\n"）
    uint32_t magic = *(uint32_t *)fw_data;
    // printf("[atheros-fw] Debug: magic=0x%x, INTEL_FW_MAGIC=0x%x\n", magic, INTEL_FW_MAGIC);

    // Intel 固件格式检查 (TLV 格式: "IWL\n" 在偏移 0)
    if (magic == INTEL_FW_MAGIC) {
        // Intel TLV 固件 - 魔术字在开头
        printf("[atheros-fw] Intel TLV firmware verified:\n");

        // 提取人类可读版本字符串 (偏移 12)
        if (fw_size >= 76) {  // 12 (header) + 64 (human_readable)
            char version_str[64];
            memcpy(version_str, fw_data + 12, 64);
            // 找到第一个 null 终止符
            for (int i = 0; i < 63; i++) {
                if (version_str[i] == '\0') {
                    version_str[i] = ' ';  // 替换 null 为空格
                }
            }
            version_str[63] = '\0';
            printf("  Firmware: %s\n", version_str);
        }

        printf("  Size: %d bytes\n", fw_size);
        return 0;
    }

    // Intel 固件格式检查 (旧格式: 偏移 4 字节后是 "IWL\n")
    if (fw_size >= 8) {
        uint32_t intel_magic = *(uint32_t *)(fw_data + 4);
        if (intel_magic == INTEL_FW_MAGIC) {
            // Intel 固件 - 不使用 fw_header_t 结构
            printf("[atheros-fw] Intel firmware verified:\n");

            // 提取固件名称 (偏移 8)
            char fw_name[16];
            memcpy(fw_name, fw_data + 8, 12);
            fw_name[12] = '\0';
            printf("  Firmware: %s\n", fw_name);

            // 提取版本信息 (偏移 20 左右)
            if (fw_size >= 64) {
                char version_str[32];
                memcpy(version_str, fw_data + 20, 24);
                version_str[24] = '\0';
                printf("  %s\n", version_str);
            }

            printf("  Size: %d bytes\n", fw_size);
            return 0;
        }
    }

    // Atheros 固件格式检查 (使用 fw_header_t)
    if (magic == ATHEROS_FW_MAGIC) {
        fw_header_t *header = (fw_header_t *)fw_data;

        printf("[atheros-fw] Atheros firmware verified:\n");
        printf("  Version: %d.%d\n",
               (header->fw_version >> 16) & 0xFF,
               header->fw_version & 0xFFFF);
        printf("  Size: %d bytes\n", fw_size);
        printf("  HW Target: 0x%x\n", header->hw_target);

        return 0;
    }

    // 都不匹配
    printf("[atheros-fw] Invalid firmware magic: 0x%x\n", magic);
    printf("[atheros-fw] Expected: 0x%x (Atheros) or 0x%x (Intel)\n",
           ATHEROS_FW_MAGIC, INTEL_FW_MAGIC);
    printf("[atheros-fw] First 16 bytes: ");
    for (int i = 0; i < 16 && i < fw_size; i++) {
        printf("%02X ", fw_data[i]);
    }
    printf("\n");
    return -1;
}

/**
 * @brief 加载固件
 */
int atheros_fw_load(const uint8_t *fw_data, uint32_t fw_size) {
    if (fw_mgr.state != FW_STATE_IDLE) {
        printf("[atheros-fw] Firmware not in IDLE state\n");
        return -1;
    }

    // 验证固件
    if (atheros_fw_verify(fw_data, fw_size) < 0) {
        fw_mgr.state = FW_STATE_ERROR;
        return -1;
    }

    fw_mgr.state = FW_STATE_LOADING;

    // 检查是否是 Intel 固件（支持两种格式）
    uint32_t magic = *(uint32_t *)fw_data;
    uint32_t intel_magic_offset4 = *(uint32_t *)(fw_data + 4);
    int is_intel = (magic == INTEL_FW_MAGIC) || (intel_magic_offset4 == INTEL_FW_MAGIC);

    if (is_intel) {
        // Intel 固件：直接加载整个固件到内存
        printf("[atheros-fw] Loading Intel firmware...\n");

        // 计算需要的页数
        uint32_t num_pages = (fw_size + 4095) / 4096;
        uint32_t fw_mem_phys = pmm_alloc_pages(num_pages);

        if (!fw_mem_phys) {
            printf("[atheros-fw] Failed to allocate memory for Intel firmware\n");
            fw_mgr.state = FW_STATE_ERROR;
            return -1;
        }

        // 映射物理内存到虚拟地址
        uint32_t fw_mem_virt = map_highmem_physical(fw_mem_phys, num_pages * 4096, 0);

        if (!fw_mem_virt) {
            printf("[atheros-fw] Failed to map firmware memory\n");
            fw_mgr.state = FW_STATE_ERROR;
            return -1;
        }

        // 复制整个固件到虚拟地址
        memcpy((void *)fw_mem_virt, fw_data, fw_size);

        fw_mgr.fw_addr = fw_mem_phys;  // 保存物理地址
        fw_mgr.fw_virt = fw_mem_virt;  // 保存虚拟地址
        fw_mgr.fw_size = fw_size;
        fw_mgr.version = 0;  // Intel 固件版本在字符串中
        fw_mgr.entry_point = 0;  // Intel 固件有自己的入口点
        fw_mgr.state = FW_STATE_READY;

        printf("[atheros-fw] Intel firmware loaded at phys=0x%x virt=0x%x\n",
               fw_mem_phys, fw_mem_virt);
    } else {
        // Atheros 固件：使用 fw_header_t 结构解析
        fw_header_t *header = (fw_header_t *)fw_data;

        // 跳过固件头，指向固件段
        uint8_t *fw_ptr = (uint8_t *)(fw_data + sizeof(fw_header_t));
        uint32_t remaining = fw_size - sizeof(fw_header_t);

        // 解析并加载固件段
        while (remaining >= sizeof(fw_segment_t)) {
            fw_segment_t *seg = (fw_segment_t *)fw_ptr;

            // 分配内存
            uint32_t seg_mem = pmm_alloc_pages((seg->len + 4095) / 4096);
            if (!seg_mem) {
                printf("[atheros-fw] Failed to allocate memory for segment\n");
                fw_mgr.state = FW_STATE_ERROR;
                return -1;
            }

            // 映射到虚拟地址
            uint32_t seg_virt = seg_mem;  // 简化：直接映射

            // 复制段数据
            if (seg->type == FW_SEG_TEXT || seg->type == FW_SEG_DATA) {
                memcpy((void *)seg_virt, fw_ptr + sizeof(fw_segment_t), seg->len);
            } else if (seg->type == FW_SEG_BSS) {
                memset((void *)seg_virt, 0, seg->len);
            }

            printf("[atheros-fw] Loaded segment: type=%d, addr=0x%x, len=%d\n",
                   seg->type, seg->addr, seg->len);

            // 移动到下一个段
            fw_ptr += sizeof(fw_segment_t) + seg->len;
            remaining -= sizeof(fw_segment_t) + seg->len;
        }

        fw_mgr.fw_size = fw_size;
        fw_mgr.version = header->fw_version;
        fw_mgr.entry_point = header->hw_target;  // 简化：入口点
        fw_mgr.state = FW_STATE_READY;
    }

    printf("[atheros-fw] Firmware loaded successfully\n");
    return 0;
}

/**
 * @brief 启动固件
 */
int atheros_fw_start(void) {
    if (fw_mgr.state != FW_STATE_READY) {
        printf("[atheros-fw] Firmware not ready\n");
        return -1;
    }

    printf("[atheros-fw] Starting firmware...\n");

    // 检查是否是 Intel 固件（通过检查 firmware 加载时是否检测到 Intel 格式）
    // Intel firmware 在 atheros_fw_load() 时会设置 fw_mgr.version = 0
    int is_intel = (fw_mgr.version == 0 && fw_mgr.fw_virt != 0);

    if (is_intel) {
        // Intel WiFi 固件启动
        printf("[atheros-fw] Intel WiFi firmware detected, using Intel startup sequence\n");

        // 获取 mem_base（从 wifi.c 传递过来）
        extern uint32_t atheros_wifi_mem_base;
        uint32_t mem_base = atheros_wifi_mem_base;

        // 调用 Intel 特定的 firmware 启动函数
        extern int intel_fw_start(uint32_t mem_base, uint32_t fw_addr, uint32_t fw_size);
        if (intel_fw_start(mem_base, fw_mgr.fw_addr, fw_mgr.fw_size) < 0) {
            printf("[atheros-fw] Intel firmware start failed\n");
            fw_mgr.state = FW_STATE_ERROR;
            return -1;
        }

        printf("[atheros-fw] Intel firmware started successfully\n");
        fw_mgr.state = FW_STATE_RUNNING;
        return 0;
    }

    // Atheros 固件启动
    printf("[atheros-fw] Atheros firmware startup not implemented yet\n");

    fw_mgr.state = FW_STATE_RUNNING;
    printf("[atheros-fw] Firmware running\n");

    return 0;
}

/**
 * @brief 停止固件
 */
int atheros_fw_stop(void) {
    if (fw_mgr.state != FW_STATE_RUNNING) {
        printf("[atheros-fw] Firmware not running\n");
        return -1;
    }

    printf("[atheros-fw] Stopping firmware...\n");

    // 复位固件状态
    fw_mgr.state = FW_STATE_IDLE;

    return 0;
}

/**
 * @brief 获取固件状态
 */
fw_state_t atheros_fw_get_state(void) {
    return fw_mgr.state;
}
