/**
 * @file intel.h
 * @brief Intel WiFi 6000 系列寄存器定义
 *
 * 基于 Linux iwlwifi 驱动的寄存器定义
 */

#ifndef INTEL_WIFI_H
#define INTEL_WIFI_H

#include "types.h"
#include "net/wifi/reg.h"  // 需要 atheros_reg_read/write

// ==================== CSR 寄存器 (直接 PCI 映射，0x000-0x3FF) ====================

#define CSR_BASE                    0x000

// 关键 CSR 寄存器
#define CSR_HW_IF_CONFIG_REG         (CSR_BASE + 0x000)
#define CSR_INT                      (CSR_BASE + 0x008)
#define CSR_INT_MASK                 (CSR_BASE + 0x00c)
#define CSR_RESET                   (CSR_BASE + 0x020)
#define CSR_GP_CNTRL                (CSR_BASE + 0x024)
#define CSR_HW_REV                  (CSR_BASE + 0x028)
#define CSR_GP_DRIVER_REG           (CSR_BASE + 0x050)
#define CSR_UCODE_DRV_GP1           (CSR_BASE + 0x054)
#define CSR_UCODE_DRV_GP1_SET        (CSR_BASE + 0x058)
#define CSR_UCODE_DRV_GP1_CLR        (CSR_BASE + 0x05c)
#define CSR_UCODE_DRV_GP2           (CSR_BASE + 0x060)

// 🔥 UCODE GP1 寄存器位（RF-Kill 控制）
#define CSR_UCODE_SW_BIT_RFKILL      0x00000001
#define CSR_UCODE_DRV_GP1_BIT_CMD_BLOCKED  0x00000002

#define CSR_LED_REG                  (CSR_BASE + 0x094)
#define CSR_GPIO_IN                 (CSR_BASE + 0x018)
#define CSR_FUNC_SCRATCH             (CSR_BASE + 0x02c)
#define CSR_EEPROM_REG              (CSR_BASE + 0x02c)
#define CSR_EEPROM_GP               (CSR_BASE + 0x030)
#define CSR_OTP_GP_REG             (CSR_BASE + 0x034)
#define CSR_GIO_REG                  (CSR_BASE + 0x03C)

// GP_CNTRL 寄存器关键位
#define CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY     0x00000001
#define CSR_GP_CNTRL_REG_FLAG_INIT_DONE         0x00000004
#define CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ     0x00000008
#define CSR_GP_CNTRL_REG_FLAG_GOING_TO_SLEEP   0x00000010
#define CSR_GP_CNTRL_REG_FLAG_XTAL_ON         0x00000400

#define CSR_GP_CNTRL_REG_VAL_MAC_ACCESS_EN     0x00000001
#define CSR_GP_CNTRL_REG_FLAG_INIT_DONE_MSK     0x00000004

// INT 寄存器中断位
#define CSR_INT_BIT_ALIVE                0x00000001
#define CSR_INT_BIT_WAKEUP               0x00000002
#define CSR_INT_BIT_RESET_DONE           0x00000004
#define CSR_INT_BIT_SW_RX                0x00000008
#define CSR_INT_BIT_RF_KILL              0x00000080
#define CSR_INT_BIT_SW_ERR               0x02000000
#define CSR_INT_BIT_FH_TX                0x08000000
#define CSR_INT_BIT_FH_RX                0x80000000
#define CSR_INT_BIT_RX_PERIODIC         0x10000000
#define CSR_INT_BIT_HW_ERR              0x20000000

#define CSR_INI_SET_MASK  (CSR_INT_BIT_FH_RX | CSR_INT_BIT_HW_ERR | \
                             CSR_INT_BIT_FH_TX | CSR_INT_BIT_SW_ERR | \
                             CSR_INT_BIT_RF_KILL | CSR_INT_BIT_SW_RX | \
                             CSR_INT_BIT_WAKEUP | CSR_INT_BIT_RESET_DONE | \
                             CSR_INT_BIT_ALIVE | CSR_INT_BIT_RX_PERIODIC)

// RESET 寄存器标志
#define CSR_RESET_REG_FLAG_SW_RESET           0x00000080
#define CSR_RESET_REG_FLAG_MASTER_DISABLED    0x00000100
#define CSR_RESET_REG_FLAG_NEVO_RESET         0x00000100

// uCode 相关寄存器
#define CSR_UCODE_LOAD_STATUS         (CSR_BASE + 0x0a0)
#define CSR_UCODE_SYSTERO             (CSR_BASE + 0x0bc)
#define CSR_LMPM_SECURE_HID_CFG       (CSR_BASE + 0x0a8)


// 🔥 LMPM (Link Manager and Power Management) 寄存器 - 固件加载关键
#define LMPM_SECURE_UCODE_LOAD_CPU1_HDR_ADDR		0xA05C  // 🔥 修复：CPU1 PRPH 地址
#define LMPM_SECURE_UCODE_LOAD_CPU2_HDR_ADDR		0xA060  // CPU2 PRPH 地址（如果需要）

// 🔥 SRAM 空间定义（固件运行内存）
#define LMPM_SECURE_CPU1_HDR_MEM_SPACE		0x2000  // CPU1 固件在 SRAM 中的基址

// BSM (Boot State Machine) 寄存器 - 用于启动 firmware
#define CSR_BSM_WR_CTRL_REG          (CSR_BASE + 0x140)
#define CSR_BSM_WR_MEM_SRC_REG       (CSR_BASE + 0x144)
#define CSR_BSM_WR_MEM_DST_REG       (CSR_BASE + 0x148)
#define CSR_BSM_WR_MEM_COUNT_REG     (CSR_BASE + 0x14c)
#define CSR_BSM_DRAM_INST_PTR        (CSR_BASE + 0x170)
#define CSR_BSM_DRAM_DATA_PTR        (CSR_BASE + 0x174)

// BSM 控制位
#define CSR_BSM_WR_CTRL_REG_BIT_START 0x00000001  // 🔥 修复：START bit 是 bit 0，不是 bit 31
#define CSR_BSM_WR_CTRL_REG_BIT_WRITE 0x2
#define CSR_BSM_WR_CTRL_REG_BIT_CMD   0x00000002

// Shared memory (SRAM) 配置
#define CSR_FW_MEM_BOUNDARY           (CSR_BASE + 0x168)

// 🔥 UCode FIFO 寄存器（用于 firmware 加载）
#define CSR_UCODE_LOAD_STATUS         (CSR_BASE + 0x0a0)
#define CSR_UCODE_CLASS_INST_SIZE     (CSR_BASE + 0x0a4)
#define CSR_UCODE RTP_DATA_INST_SIZE   (CSR_BASE + 0x0a8)
#define CSR_UCODE_DATA_SIZE           (CSR_BASE + 0x0ac)
#define CSR_UCODE_INST_ADDR           (CSR_BASE + 0x0b0)
#define CSR_UCODE_DATA_ADDR           (CSR_BASE + 0x0b4)

// UCode load 状态位
#define UCODE_VALID_STATUS               0x00000001
#define UCODE_INIT_COMPLETE             0x00000002

// 🔥 BSM WR_DATA 寄存器（用于直接写 firmware）
#define CSR_BSM_WR_DATA               (CSR_BASE + 0x144)

// ==================== HBUS 寄存器 (0x400-0x4FF) ====================

#define HBUS_BASE                   0x400

// 共享内存访问（需要 MAC 访问权限）
#define HBUS_TARG_PRPH_WADDR         (HBUS_BASE + 0x044)
#define HBUS_TARG_PRPH_RADDR         (HBUS_BASE + 0x048)
#define HBUS_TARG_PRPH_WDAT          (HBUS_BASE + 0x04c)
#define HBUS_TARG_PRPH_RDAT          (HBUS_BASE + 0x050)

// ==================== PRPH 寄存器 (内部寄存器，需要通过 HBUS 间接访问) ====================

#define PRPH_BASE                   0x00000

// APMG (电源管理) - 关键！
#define APMG_BASE                   (PRPH_BASE + 0x3000)
#define APMG_CLK_CTRL_REG           (APMG_BASE + 0x0000)
#define APMG_CLK_EN_REG             (APMG_BASE + 0x0004)
#define APMG_CLK_DIS_REG           (APMG_BASE + 0x0008)
#define APMG_PS_CTRL_REG           (APMG_BASE + 0x000c)
#define APMG_RTC_INT_STT_REG       (APMG_BASE + 0x001c)
#define APMG_RTC_INT_MSK_REG       (APMG_BASE + 0x0020)

// 🔥 APMG 时钟控制位 (Linux iwlwifi 定义)
#define APMG_CLK_CTRL_REG_MSK_DMA_CLK_RQT    0x00000100  // DMA 时钟请求
#define APMG_CLK_CTRL_REG_MSK_BSM_CLK_RQT    0x00000800  // BSM 时钟请求

// 🔥 APMG 时钟使能位
#define APMG_CLK_EN_REG_MSK_DMA_CLK_INIT     0x00000001  // DMA 时钟初始化
#define APMG_CLK_EN_REG_MSK_BSM_CLK_INIT     0x00000008  // BSM 时钟初始化

// 寄存器操作（通过 HBUS 间接访问）
static inline uint32_t intel_read_prph(uint32_t mem_base, uint32_t offset) {
    // 写地址寄存器
    atheros_reg_write(mem_base, HBUS_TARG_PRPH_WADDR, offset);
    // 🔥 关键：需要等待一小段时间让硬件准备好
    // atheros_delay_us(1);  // 可选：如果硬件需要延迟
    // 读数据寄存器
    return atheros_reg_read(mem_base, HBUS_TARG_PRPH_RDAT);
}

static inline void intel_write_prph(uint32_t mem_base, uint32_t offset, uint32_t value) {
    // 🔥 根据 Linux iwlwifi 驱动的 PRPH 写入序列
    // 必须严格按照以下顺序，否则写入会失败

    // 步骤 1: 写 PRPH 地址到 WADDR
    atheros_reg_write(mem_base, HBUS_TARG_PRPH_WADDR, offset);

    // 步骤 2: 写数据到 WDAT
    atheros_reg_write(mem_base, HBUS_TARG_PRPH_WDAT, value);

    // 🔥 关键：某些设备需要等待一小段时间让写入生效
    // 但不能太长，否则会影响性能
    // atheros_delay_us(1);

    // 🔥 关键修复：写入后需要**读回 WADDR** 来确保写入完成！
    // 这是 Intel 6000 系列的硬件特性
    volatile uint32_t dummy = atheros_reg_read(mem_base, HBUS_TARG_PRPH_WADDR);
    (void)dummy;  // 避免编译器警告
}

// 🔥 设置/清除 PRPH 位
static inline void intel_set_bits_prph(uint32_t mem_base, uint32_t offset, uint32_t mask) {
    uint32_t val = intel_read_prph(mem_base, offset);
    intel_write_prph(mem_base, offset, val | mask);
}

static inline void intel_clear_bits_prph(uint32_t mem_base, uint32_t offset, uint32_t mask) {
    uint32_t val = intel_read_prph(mem_base, offset);
    intel_write_prph(mem_base, offset, val & ~mask);
}

// ==================== FH (FIFO Hardware) DMA 寄存器 ====================
// 用于固件加载的 DMA 引擎（Linux iwlwifi 使用的方式）

#define FH_BASE                     0x0000  // FH 在 CSR 空间内的偏移
#define FH_SRVC_CHNL                9       // 服务通道（用于固件加载）

// TX 配置寄存器
#define FH_TCSR_CHNL_TX_CONFIG_REG(ch)    (FH_BASE + 0x020 + ((ch) * 0x40))
#define FH_TCSR_TX_CONFIG_REG_VAL_DMA_CHNL_PAUSE       0x00000000
#define FH_TCSR_TX_CONFIG_REG_VAL_DMA_CHNL_ENABLE      0x80000000
#define FH_TCSR_TX_CONFIG_REG_VAL_DMA_CREDIT_DISABLE   0x40000000
#define FH_TCSR_TX_CONFIG_REG_VAL_CIRQ_HOST_ENDTFD     0x00001000

// SRAM 地址寄存器（目标地址）
#define FH_SRVC_CHNL_SRAM_ADDR_REG(ch)     (FH_BASE + 0x02C + ((ch) * 0x40))

// TFDIB（传输帧描述符信息块）寄存器
#define FH_TFDIB_CTRL0_REG(ch)             (FH_BASE + 0x070 + ((ch) * 0x40))
#define FH_TFDIB_CTRL1_REG(ch)             (FH_BASE + 0x074 + ((ch) * 0x40))

#define FH_MEM_TFDIB_DRAM_ADDR_LSB_MSK     0x0FFFFFFF
#define FH_MEM_TFDIB_REG1_ADDR_BITSHIFT    28
#define FH_MEM_TFDIB_REG1_LEN_MSK          0x0FFF

// TX 缓冲区状态寄存器
#define FH_TCSR_CHNL_TX_BUF_STS_REG(ch)    (FH_BASE + 0x024 + ((ch) * 0x40))
#define FH_TCSR_CHNL_TX_BUF_STS_REG_POS_TB_NUM     0
#define FH_TCSR_CHNL_TX_BUF_STS_REG_POS_TB_IDX     16
#define FH_TCSR_CHNL_TX_BUF_STS_REG_VAL_TFDB_VALID 0x00000001

// FH DMA 最大传输长度
#define FH_MEM_TB_MAX_LENGTH        0x2000  // 8KB

// 扩展地址空间（用于大地址 SRAM）
#define LMPM_CHICK                  0xA01E8  // PRPH 寄存器
#define LMPM_CHICK_EXTENDED_ADDR_SPACE  0x80000000

#define IWL_FW_MEM_EXTENDED_START   0x40000
#define IWL_FW_MEM_EXTENDED_END     0x50000

// 常量定义
#define IWL_HOST_INT_TIMEOUT_DEF   0x40

// ==================== 函数声明 ====================

// 硬件初始化和控制
int intel_hw_reset(uint32_t mem_base);
int intel_hw_grant_mac_access(uint32_t mem_base);
int intel_hw_init_clocks(uint32_t mem_base);
void intel_hw_stop_device(uint32_t mem_base);
int intel_hw_init(uint32_t mem_base);

// 中断控制
int intel_hw_enable_interrupts(uint32_t mem_base);
void intel_hw_disable_interrupts(uint32_t mem_base);
uint32_t intel_hw_get_int_status(uint32_t mem_base);
void intel_hw_ack_interrupts(uint32_t mem_base, uint32_t ints);

// MAC 地址
int intel_hw_read_mac_addr(uint32_t mem_base, uint8_t *mac);

// 硬件状态
int intel_hw_is_alive(uint32_t mem_base);

// Firmware 启动
// 注意：这些函数需要包含 "net/wifi/intel_fw_parser.h" 才能使用
struct intel_fw_parsed;  // Forward declaration
int intel_fw_start_parsed(uint32_t mem_base, struct intel_fw_parsed *parsed);

// 动态固件加载接口（从用户空间指针加载）
int intel_fw_load_from_buffer(uint32_t mem_base, const uint8_t *fw_data, uint32_t fw_size);

// Legacy interface（向后兼容）
int intel_fw_start(uint32_t mem_base, uint32_t fw_addr, uint32_t fw_size);

#endif // INTEL_WIFI_H
