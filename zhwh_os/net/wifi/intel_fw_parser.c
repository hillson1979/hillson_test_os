/**
 * @file intel_fw_parser.c
 * @brief Intel WiFi 固件文件解析器实现
 *
 * 解析 Intel .ucode 格式的固件文件
 * 从单个固件文件中提取 INIT 和 RUNTIME 两个独立的固件镜像
 */

#include "net/wifi/intel_fw_parser.h"
#include "printf.h"
#include "string.h"

/**
 * @brief 添加指令段 chunk
 */
static void add_inst_chunk(intel_fw_image_t *img, const uint8_t *data, uint32_t size, uint32_t offset) {
    // 过滤掉空的 chunk
    if (size == 0) {
        printf("[intel-fw-parser] Skipping empty instruction chunk (offset=0x%x)\n", offset);
        return;
    }

    if (img->inst.num_chunks >= MAX_CHUNKS_PER_SECTION) {
        printf("[intel-fw-parser] ERROR: Too many instruction chunks!\n");
        return;
    }

    intel_fw_chunk_t *chunk = &img->inst.chunks[img->inst.num_chunks];
    chunk->data = data;
    chunk->size = size;
    chunk->offset = offset;

    img->inst.total_size += size;
    img->inst.num_chunks++;
    img->has_inst = 1;
}

/**
 * @brief 添加数据段 chunk
 */
static void add_data_chunk(intel_fw_image_t *img, const uint8_t *data, uint32_t size, uint32_t offset) {
    // 过滤掉空的 chunk
    if (size == 0) {
        printf("[intel-fw-parser] Skipping empty data chunk (offset=0x%x)\n", offset);
        return;
    }

    if (img->data.num_chunks >= MAX_CHUNKS_PER_SECTION) {
        printf("[intel-fw-parser] ERROR: Too many data chunks!\n");
        return;
    }

    intel_fw_chunk_t *chunk = &img->data.chunks[img->data.num_chunks];
    chunk->data = data;
    chunk->size = size;
    chunk->offset = offset;

    img->data.total_size += size;
    img->data.num_chunks++;
    img->has_data = 1;
}

/**
 * @brief 解析 Intel TLV 格式固件
 *
 * 固件文件格式：
 * - TLV Header (magic + version + build + human_readable)
 * - TLV entries (type + length + data)
 *
 * TLV 类型：
 * - IWL_UCODE_TLV_INST: Runtime instructions
 * - IWL_UCODE_TLV_DATA: Runtime data
 * - IWL_UCODE_TLV_INIT: Init instructions
 * - IWL_UCODE_TLV_INIT_DATA: Init data
 */
int intel_fw_parse(const uint8_t *fw_data, uint32_t fw_size,
                   struct intel_fw_parsed *parsed) {
    printf("[intel-fw-parser] Parsing Intel firmware file (size=%d)...\n", fw_size);

    // 清零输出结构
    memset(parsed, 0, sizeof(struct intel_fw_parsed));

    // 检查最小文件大小
    if (fw_size < sizeof(intel_tlv_header_t)) {
        printf("[intel-fw-parser] ERROR: File too small!\n");
        return -1;
    }

    // 读取固件头
    // 注意：真实固件文件有4字节前导填充 (0x00 0x00 0x00 0x00)
    // 魔术字在偏移4的位置
    const uint8_t *fw_start = fw_data;

    // 检查是否有前导填充（魔术字在偏移0或偏移4）
    uint32_t magic_at_0 = *((uint32_t *)(fw_start));
    uint32_t magic_at_4 = *((uint32_t *)(fw_start + 4));

    const intel_tlv_header_t *header;
    uint32_t expected_magic = 0x0A4C5749;  // "IWL\n" little-endian

    if (magic_at_0 == expected_magic) {
        // 魔术字在偏移0（模拟固件）
        header = (const intel_tlv_header_t *)fw_start;
    } else if (magic_at_4 == expected_magic) {
        // 魔术字在偏移4（真实固件，有4字节前导）
        header = (const intel_tlv_header_t *)(fw_start + 4);
        printf("[intel-fw-parser] Note: Firmware has 4-byte padding, magic at offset 4\n");
    } else {
        // 都不是，报错
        printf("[intel-fw-parser] ERROR: Invalid magic 0x%x at offset 0, 0x%x at offset 4 (expected 0x%x)\n",
               magic_at_0, magic_at_4, expected_magic);
        return -1;
    }

    printf("[intel-fw-parser] Magic: OK (0x%x)\n", header->magic);
    printf("[intel-fw-parser] Version: 0x%x\n", header->ver);
    printf("[intel-fw-parser] Build: %d\n", header->build);

    // 保存固件版本
    parsed->ucode_ver = header->ver;

    // 复制人类可读版本字符串
    uint32_t copy_len = sizeof(parsed->fw_version) - 1;
    if (copy_len > sizeof(header->human_readable)) {
        copy_len = sizeof(header->human_readable);
    }
    memcpy(parsed->fw_version, header->human_readable, copy_len);
    parsed->fw_version[copy_len] = '\0';

    printf("[intel-fw-parser] Firmware: %s\n", parsed->fw_version);

    // 解析 TLV 条目
    // 需要从header之后开始（如果有4字节填充，header已经在fw_data+4）
    const uint8_t *data = (const uint8_t *)header + sizeof(intel_tlv_header_t);
    uint32_t len = fw_size - (data - fw_start);

    printf("[intel-fw-parser] Scanning TLV entries (remaining %d bytes)...\n", len);

    while (len >= sizeof(intel_tlv_t)) {
        const intel_tlv_t *tlv = (const intel_tlv_t *)data;
        uint32_t tlv_type = tlv->type;
        uint32_t tlv_len = tlv->length;
        const uint8_t *tlv_data = tlv->data;

        // 检查长度
        if (len < sizeof(intel_tlv_t) + tlv_len) {
            printf("[intel-fw-parser] ERROR: TLV len %d exceeds remaining %d\n",
                   tlv_len, len);
            break;
        }

        // 对齐到 4 字节边界
        uint32_t tlv_total = sizeof(intel_tlv_t) + ((tlv_len + 3) & ~3);
        if (len < tlv_total) {
            printf("[intel-fw-parser] ERROR: TLV total %d exceeds remaining %d\n",
                   tlv_total, len);
            break;
        }

        // Debug: 打印所有遇到的 TLV 类型
        printf("[intel-fw-parser] Processing TLV: type=%d, len=%d, offset=%d\n",
               tlv_type, tlv_len, (uint32_t)(data - fw_data));

        // 处理不同的 TLV 类型
        switch (tlv_type) {
        case IWL_UCODE_TLV_INST:
            // Runtime instructions（旧格式，需要手动计算 offset）
            {
                intel_fw_section_t *inst = &parsed->images[IWL_UCODE_REGULAR].inst;
                uint32_t offset = inst->total_size;  // 累加到当前位置
                printf("[intel-fw-parser] Found RUNTIME INST: %d bytes, calculated offset=0x%x\n",
                       tlv_len, offset);
                add_inst_chunk(&parsed->images[IWL_UCODE_REGULAR], tlv_data, tlv_len, offset);
            }
            break;

        case IWL_UCODE_TLV_DATA:
            // Runtime data（旧格式，需要手动计算 offset）
            {
                intel_fw_section_t *data = &parsed->images[IWL_UCODE_REGULAR].data;
                uint32_t offset = 0x400000 + data->total_size;  // data 段从 0x400000 开始
                printf("[intel-fw-parser] Found RUNTIME DATA: %d bytes, calculated offset=0x%x\n",
                       tlv_len, offset);
                add_data_chunk(&parsed->images[IWL_UCODE_REGULAR], tlv_data, tlv_len, offset);
            }
            break;

        case IWL_UCODE_TLV_INIT:
            // Init instructions（旧格式，需要手动计算 offset）
            {
                intel_fw_section_t *inst = &parsed->images[IWL_UCODE_INIT].inst;
                uint32_t offset = inst->total_size;  // 累加到当前位置
                printf("[intel-fw-parser] Found INIT INST: %d bytes, calculated offset=0x%x\n",
                       tlv_len, offset);
                add_inst_chunk(&parsed->images[IWL_UCODE_INIT], tlv_data, tlv_len, offset);
            }
            break;

        case IWL_UCODE_TLV_INIT_DATA:
            // Init data（旧格式，需要手动计算 offset）
            {
                intel_fw_section_t *data = &parsed->images[IWL_UCODE_INIT].data;
                uint32_t offset = 0x400000 + data->total_size;  // data 段从 0x400000 开始
                printf("[intel-fw-parser] Found INIT DATA: %d bytes, calculated offset=0x%x\n",
                       tlv_len, offset);
                add_data_chunk(&parsed->images[IWL_UCODE_INIT], tlv_data, tlv_len, offset);
            }
            break;

        case IWL_UCODE_TLV_WOWLAN_INST:
            // WoWLAN instructions
            printf("[intel-fw-parser] Found WOWLAN INST: %d bytes (ignored)\n", tlv_len);
            break;

        case IWL_UCODE_TLV_WOWLAN_DATA:
            // WoWLAN data
            printf("[intel-fw-parser] Found WOWLAN DATA: %d bytes (ignored)\n", tlv_len);
            break;

        case IWL_UCODE_TLV_SEC_RT:
        case IWL_UCODE_TLV_SECURE_SEC_RT:
            // Runtime section (新格式) - 每个TLV都有独立的offset
            printf("[intel-fw-parser] Found RUNTIME SECTION (new format): %d bytes\n", tlv_len);
            if (tlv_len >= 4) {
                uint32_t offset = *((uint32_t *)tlv_data);
                printf("[intel-fw-parser]   Offset: 0x%x\n", offset);
                add_inst_chunk(&parsed->images[IWL_UCODE_REGULAR], tlv_data + 4, tlv_len - 4, offset);
            }
            break;

        case IWL_UCODE_TLV_SEC_INIT:
        case IWL_UCODE_TLV_SECURE_SEC_INIT:
        {
            // Init section (新格式) - 每个 TLV 前 4 字节是 SRAM 加载地址
            // 🔥 关键修复：第一个 INIT chunk 强制放 SRAM 0x0 (CPU boot vector)
            intel_fw_image_t *init_img = &parsed->images[IWL_UCODE_INIT];
            uint32_t chunk_offset = 0;
            const uint8_t *payload;
            uint32_t payload_len;

            printf("[intel-fw-parser] Found INIT SECTION (new format): %d bytes\n", tlv_len);

            if (!init_img->first_chunk_loaded) {
                // 🔹 第一个 INIT chunk 必须放在 SRAM 0x0（CPU boot vector）
                chunk_offset = 0x0;
                init_img->first_chunk_loaded = 1;
                printf("[intel-fw-parser]   First INIT chunk: forcing to SRAM 0x0 (boot vector)\n");

                // 跳过 TLV 中的 offset 字段（前4字节）
                if (tlv_len >= 4) {
                    payload = tlv_data + 4;
                    payload_len = tlv_len - 4;
                } else {
                    payload = tlv_data;
                    payload_len = tlv_len;
                }
            } else {
                // 🔹 后续 chunk 按 TLV 指定 offset 加载
                if (tlv_len >= 4) {
                    chunk_offset = *((uint32_t *)tlv_data);
                    payload = tlv_data + 4;
                    payload_len = tlv_len - 4;
                    printf("[intel-fw-parser]   Subsequent INIT chunk: loading at SRAM 0x%x (from TLV offset)\n", chunk_offset);

                    // 🔥 关键诊断：如果 offset 是 0x2000，这是 header！
                    if (chunk_offset == 0x2000) {
                        printf("[intel-fw-parser]   🔥🔥🔥 This is the HEADER section! 🔥🔥🔥\n");
                    }
                } else {
                    chunk_offset = 0;
                    payload = tlv_data;
                    payload_len = tlv_len;
                }
            }

            printf("[intel-fw-parser]   Size: %d bytes @ 0x%x\n", payload_len, chunk_offset);
            add_inst_chunk(init_img, payload, payload_len, chunk_offset);
            break;
        }

        default:
            // 忽略未知的 TLV 类型
            if (tlv_type < 0x100) {  // 只打印小的 TLV 类型（避免太多日志）
                printf("[intel-fw-parser] Unknown TLV type 0x%x, len=%d (skipped)\n",
                       tlv_type, tlv_len);
            }
            break;
        }

        // 移动到下一个 TLV
        data += tlv_total;
        len -= tlv_total;
    }

    // 验证解析结果
    printf("\n[intel-fw-parser] === Parsing Results ===\n");

    intel_fw_image_t *init_img = &parsed->images[IWL_UCODE_INIT];
    intel_fw_image_t *rt_img = &parsed->images[IWL_UCODE_REGULAR];

    printf("[intel-fw-parser] INIT firmware:\n");
    printf("[intel-fw-parser]   Instructions: %s (%d bytes in %d chunks)\n",
           init_img->has_inst ? "YES" : "NO",
           init_img->has_inst ? init_img->inst.total_size : 0,
           init_img->inst.num_chunks);
    printf("[intel-fw-parser]   Data: %s (%d bytes in %d chunks)\n",
           init_img->has_data ? "YES" : "NO",
           init_img->has_data ? init_img->data.total_size : 0,
           init_img->data.num_chunks);

    printf("[intel-fw-parser] RUNTIME firmware:\n");
    printf("[intel-fw-parser]   Instructions: %s (%d bytes in %d chunks)\n",
           rt_img->has_inst ? "YES" : "NO",
           rt_img->has_inst ? rt_img->inst.total_size : 0,
           rt_img->inst.num_chunks);
    printf("[intel-fw-parser]   Data: %s (%d bytes in %d chunks)\n",
           rt_img->has_data ? "YES" : "NO",
           rt_img->has_data ? rt_img->data.total_size : 0,
           rt_img->data.num_chunks);

    // 检查是否有必要的段
    if (!init_img->has_inst || !rt_img->has_inst) {
        printf("[intel-fw-parser] ERROR: Missing required firmware sections!\n");
        printf("[intel-fw-parser] This firmware file may not be supported.\n");
        return -1;
    }

    parsed->valid = 1;
    printf("[intel-fw-parser] ✓ Firmware parsing successful!\n");

    return 0;
}
