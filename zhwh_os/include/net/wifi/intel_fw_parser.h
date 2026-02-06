/**
 * @file intel_fw_parser.h
 * @brief Intel WiFi 固件文件解析器
 *
 * 基于 Linux iwlwifi 驱动的固件格式解析
 * 支持从单个 .ucode 文件中提取 INIT 和 RUNTIME 固件镜像
 */

#ifndef INTEL_FW_PARSER_H
#define INTEL_FW_PARSER_H

#include "types.h"

// ==================== TLV 固件格式 ====================

/**
 * @brief Intel TLV 固件文件头
 */
typedef struct {
    uint32_t magic;        // 0x0A4C5749 ("IWL\n" in little-endian)
    uint32_t ver;          // 固件版本
    uint32_t build;        // 构建号
    uint8_t  human_readable[64];  // 人类可读版本信息
} __attribute__((packed)) intel_tlv_header_t;

// TLV 魔术字 (little-endian "IWL\n")
#define IWL_TLV_UCODE_MAGIC  0x0A4C5749

/**
 * @brief TLV (Type-Length-Value) 结构
 */
typedef struct {
    uint32_t type;         // TLV 类型
    uint32_t length;       // 数据长度
    uint8_t  data[0];      // 可变长度数据
} __attribute__((packed)) intel_tlv_t;

// ==================== TLV 类型定义 ====================

/**
 * @brief Intel 固件 TLV 类型
 */
enum intel_ucode_tlv_type {
    IWL_UCODE_TLV_INST        = 1,    // Runtime instructions
    IWL_UCODE_TLV_DATA        = 2,    // Runtime data
    IWL_UCODE_TLV_INIT        = 3,    // Init instructions
    IWL_UCODE_TLV_INIT_DATA   = 4,    // Init data
    IWL_UCODE_TLV_BOOT        = 5,    // Boot (deprecated)
    IWL_UCODE_TLV_PROBE_MAX_LEN = 6,  // Max probe length
    IWL_UCODE_TLV_PAN         = 7,    // PAN capabilities
    IWL_UCODE_TLV_API_CHANGES_SET = 0x1, // API changes
    IWL_UCODE_TLV_ENABLED_CAPABILITIES = 0x2, // Capabilities
    IWL_UCODE_TLV_INIT_EVTLOG_PTR = 0x3, // Init event log pointer
    IWL_UCODE_TLV_INIT_EVTLOG_SIZE = 0x4, // Init event log size
    IWL_UCODE_TLV_INIT_ERRLOG_PTR = 0x5, // Init error log pointer
    IWL_UCODE_TLV_RUNT_EVTLOG_PTR = 0x6, // Runtime event log pointer
    IWL_UCODE_TLV_RUNT_EVTLOG_SIZE = 0x7, // Runtime event log size
    IWL_UCODE_TLV_RUNT_ERRLOG_PTR = 0x8, // Runtime error log pointer
    IWL_UCODE_TLV_WOWLAN_INST  = 0xA,  // WoWLAN instructions
    IWL_UCODE_TLV_WOWLAN_DATA  = 0xB,  // WoWLAN data
    IWL_UCODE_TLV_PHY_CALIBRATION_SIZE = 0xC, // PHY calibration size
    IWL_UCODE_TLV_SEC_RT       = 0x11, // Runtime section (new format)
    IWL_UCODE_TLV_SEC_INIT     = 0x12, // Init section (new format)
    IWL_UCODE_TLV_SEC_WOWLAN   = 0x13, // WoWLAN section (new format)
    IWL_UCODE_TLV_DEF_CALIB    = 0x14, // Default calibration
    IWL_UCODE_TLV_PHY_SKU      = 0x15, // PHY SKU
    IWL_UCODE_TLV_SECURE_SEC_RT   = 0x16, // Secure runtime section
    IWL_UCODE_TLV_SECURE_SEC_INIT = 0x17, // Secure init section
    IWL_UCODE_TLV_SECURE_SEC_WOWLAN = 0x18, // Secure WoWLAN section
    IWL_UCODE_TLV_NUM_OF_CPU    = 0x1B, // Number of CPUs
    IWL_UCODE_TLV_N_SCAN_CHANNELS = 0x1C, // Number of scan channels
    IWL_UCODE_TLV_FW_VERSION    = 0x1D, // Firmware version string
    IWL_UCODE_TLV_FW_DBG_DEST   = 0x1E, // Debug destination
    IWL_UCODE_TLV_FW_DBG_CONF   = 0x1F, // Debug configuration
};

/**
 * @brief 特殊段偏移值（用于分隔符）
 *
 * 这些值出现在段的 offset 字段中，表示 CPU 分隔符
 * 对应 Linux iwlwifi 的 CPU1_CPU2_SEPARATOR_SECTION 和 PAGING_SEPARATOR_SECTION
 */
#define CPU1_CPU2_SEPARATOR_SECTION  0xFFFFFFF0  // CPU1 和 CPU2 段之间的分隔符
#define PAGING_SEPARATOR_SECTION     0xFFFFFFF1  // 分页段分隔符

/**
 * @brief TLV 段解析结构
 */
typedef struct {
    uint32_t offset;        // 段在 SRAM 中的偏移
    const uint8_t *data;    // 段数据指针
    uint32_t size;          // 段大小
} intel_fw_chunk_t;

/**
 * @brief TLV 段（可能由多个 chunk 组成）
 */
#define MAX_CHUNKS_PER_SECTION  16

typedef struct {
    intel_fw_chunk_t chunks[MAX_CHUNKS_PER_SECTION];  // 所有 chunk
    uint32_t num_chunks;                               // chunk 数量
    uint32_t total_size;                               // 总大小（所有chunk之和）
} intel_fw_section_t;

/**
 * @brief Intel 固件镜像类型
 */
enum intel_ucode_type {
    IWL_UCODE_REGULAR = 0,  // Runtime firmware
    IWL_UCODE_INIT    = 1,  // Init firmware
    IWL_UCODE_WOWLAN  = 2,  // WoWLAN firmware
    IWL_UCODE_TYPE_MAX = 3,
};

/**
 * @brief Intel 固件镜像
 */
typedef struct {
    intel_fw_section_t inst;  // 指令段
    intel_fw_section_t data;  // 数据段
    uint8_t has_inst;         // 是否有指令段
    uint8_t has_data;         // 是否有数据段
    uint8_t first_chunk_loaded; // 🔥 第一个 chunk 是否已加载（用于强制放 SRAM 0x0）
} intel_fw_image_t;

/**
 * @brief 解析后的 Intel 固件
 */
struct intel_fw_parsed {
    intel_fw_image_t images[IWL_UCODE_TYPE_MAX];  // INIT, REGULAR, WOWLAN
    uint32_t ucode_ver;          // 固件版本
    char fw_version[128];        // 固件版本字符串
    uint8_t valid;               // 解析是否成功
};

// Typedef for convenience
typedef struct intel_fw_parsed intel_fw_parsed_t;

// ==================== 函数声明 ====================

/**
 * @brief 解析 Intel 固件文件
 *
 * @param fw_data 固件数据指针
 * @param fw_size 固件大小
 * @param parsed 输出：解析后的固件结构
 * @return 0 = 成功, -1 = 失败
 */
int intel_fw_parse(const uint8_t *fw_data, uint32_t fw_size,
                   struct intel_fw_parsed *parsed);

/**
 * @brief 获取 INIT 固件镜像
 *
 * @param parsed 解析后的固件
 * @return INIT 镜像指针，如果不存在返回 NULL
 */
static inline struct intel_fw_image *intel_fw_get_init(struct intel_fw_parsed *parsed) {
    return &parsed->images[IWL_UCODE_INIT];
}

/**
 * @brief 获取 RUNTIME 固件镜像
 *
 * @param parsed 解析后的固件
 * @return RUNTIME 镜像指针，如果不存在返回 NULL
 */
static inline struct intel_fw_image *intel_fw_get_runtime(struct intel_fw_parsed *parsed) {
    return &parsed->images[IWL_UCODE_REGULAR];
}

#endif // INTEL_FW_PARSER_H
