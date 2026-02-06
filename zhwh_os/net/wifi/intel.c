/**
 * @file intel.c
 * @brief Intel WiFi 硬件初始化和控制实现
 *
 * 基于 Linux iwlwifi 驱动的初始化序列
 */

// ==================== 固件加载方式选择 ====================
// 定义为 1 使用 FH DMA（Linux iwlwifi 推荐方式）
// 定义为 0 使用 BSM DMA（旧方式，可能不适用于 Intel 6205）
#define USE_FH_DMA_FOR_FW_LOADING  1

#include "types.h"  // uint8_t 等类型定义

// VGA 颜色函数
extern void vga_setcolor(uint8_t fg, uint8_t bg);
#define SET_COLOR_RED()     vga_setcolor(4, 0)   // 红字黑底
#define SET_COLOR_WHITE()   vga_setcolor(15, 0)  // 白字黑底

#include "net/wifi/intel.h"
#include "net/wifi/intel_fw_parser.h"
#include "net/wifi/reg.h"
#include "uart.h"
#include "netdebug.h"
#include "printf.h"
#include "string.h"
#include "page.h"

/**
 * @brief 简单的微秒级延迟
 */
static inline void atheros_delay_us(uint32_t us) {
    // 简单的忙等待延迟
    // 假设 1GHz CPU，每次循环约 1-2 ns
    // 对于粗略延迟，这个实现足够了
    volatile uint32_t count = us * 100;
    while (count--) {
        __asm__ volatile("nop");
    }
}

/**
 * @brief 调试快照结构 - 记录每个检查点的关键数据
 */
typedef struct {
    const char *step_name;
    uint32_t csr_gp_cntrl;
    uint32_t apmg_clk_ctrl;
    uint32_t apmg_clk_en;
    uint32_t cpu1_hdr_addr;
    uint32_t sram_0x0;
    uint32_t sram_0x2000;
    int prph_write_success;
} debug_snapshot_t;

// 🔥 全局调试快照数组
#define MAX_SNAPSHOTS 16
static debug_snapshot_t snapshots[MAX_SNAPSHOTS];
static int snapshot_count = 0;

/**
 * @brief 捕获当前状态的快照
 */
static void capture_snapshot(const char *step_name, uint32_t mem_base) {
    if (snapshot_count < MAX_SNAPSHOTS) {
        debug_snapshot_t *snap = &snapshots[snapshot_count];
        snap->step_name = step_name;
        snap->csr_gp_cntrl = atheros_reg_read(mem_base, CSR_GP_CNTRL);
        snap->apmg_clk_ctrl = intel_read_prph(mem_base, APMG_CLK_CTRL_REG);
        snap->apmg_clk_en = intel_read_prph(mem_base, APMG_CLK_EN_REG);
        snap->cpu1_hdr_addr = intel_read_prph(mem_base, LMPM_SECURE_UCODE_LOAD_CPU1_HDR_ADDR);

        // 读取 SRAM 内容
        uint32_t sram_base = mem_base;
        snap->sram_0x0 = *(volatile uint32_t *)(sram_base);
        snap->sram_0x2000 = *(volatile uint32_t *)(sram_base + 0x2000);

        snap->prph_write_success = -1;  // 未知
        snapshot_count++;
    }
}

/**
 * @brief PRPH 写入方法枚举
 */
typedef enum {
    PRPH_METHOD_STANDARD = 0,     // 标准：先地址后数据
    PRPH_METHOD_RADDR_FIRST,      // 先设置读地址
    PRPH_METHOD_POSTING_WRITE,    // Posting 写 + 轮询
    PRPH_METHOD_DATA_FIRST,       // 先数据后地址（某些新硬件）
    PRPH_METHOD_MAX
} prph_write_method_t;

/**
 * @brief 智能 PRPH 写入函数 - 带多种方法和重试机制
 *
 * @param mem_base   PCI 内存基地址
 * @param prph_addr  PRPH 寄存器地址
 * @param value      要写入的值
 * @param method     尝试的写入方法
 * @param max_retries 最大重试次数
 * @return 0 = 成功, -1 = 失败
 */
static int intel_write_prph_with_retry(uint32_t mem_base, uint32_t prph_addr,
                                       uint32_t value, prph_write_method_t method,
                                       int max_retries)
{
    uint32_t readback;
    int attempt;

    const char *method_names[] = {
        "Standard (addr then data)",
        "RADDR first",
        "Posting write + polling",
        "Data first (reverse order)"
    };

    printf("[intel-prph] Trying method: %s\n", method_names[method]);
    printf("[intel-prph]   Target: PRPH[0x%x] = 0x%x\n", prph_addr, value);

    for (attempt = 0; attempt < max_retries; attempt++) {
        switch (method) {
            case PRPH_METHOD_STANDARD:
                // 方法 1: 标准方式（先地址后数据）
                atheros_reg_write(mem_base, HBUS_TARG_PRPH_WADDR, prph_addr);
                atheros_delay_us(10);
                atheros_reg_write(mem_base, HBUS_TARG_PRPH_WDAT, value);
                atheros_delay_us(50);
                break;

            case PRPH_METHOD_RADDR_FIRST:
                // 方法 2: 先设置读地址
                atheros_reg_write(mem_base, HBUS_TARG_PRPH_RADDR, prph_addr);
                atheros_delay_us(10);
                atheros_reg_write(mem_base, HBUS_TARG_PRPH_WADDR, prph_addr);
                atheros_delay_us(10);
                atheros_reg_write(mem_base, HBUS_TARG_PRPH_WDAT, value);
                atheros_delay_us(50);
                break;

            case PRPH_METHOD_POSTING_WRITE:
                // 方法 3: Posting 写 + 轮询验证
                atheros_reg_write(mem_base, HBUS_TARG_PRPH_WADDR, prph_addr);
                atheros_delay_us(10);
                atheros_reg_write(mem_base, HBUS_TARG_PRPH_WDAT, value);

                // 轮询等待写入完成
                for (int poll = 0; poll < 1000; poll++) {
                    atheros_delay_us(10);
                    readback = intel_read_prph(mem_base, prph_addr);
                    if (readback == value) {
                        printf("[intel-prph]   Posting write confirmed after %d polls\n", poll);
                        return 0;  // 成功
                    }
                }
                // 轮询超时，继续到下一次重试
                break;

            case PRPH_METHOD_DATA_FIRST:
                // 方法 4: 先数据后地址（某些新硬件需要）
                atheros_reg_write(mem_base, HBUS_TARG_PRPH_WDAT, value);
                atheros_delay_us(10);
                atheros_reg_write(mem_base, HBUS_TARG_PRPH_WADDR, prph_addr);
                atheros_delay_us(50);
                break;

            default:
                printf("[intel-prph] ERROR: Unknown method %d\n", method);
                return -1;
        }

        // 立即读回检查
        readback = intel_read_prph(mem_base, prph_addr);

        if (attempt == 0 || (attempt % 2) == 0) {
            printf("[intel-prph]   Attempt %d: Readback = 0x%x (expected 0x%x)\n",
                   attempt + 1, readback, value);
        }

        if (readback == value) {
            printf("[intel-prph] ✓ SUCCESS on attempt %d!\n", attempt + 1);
            return 0;  // 成功
        }

        // 检查是否是 MAC 访问错误
        if (readback == 0xA5A5A5A1 || readback == 0xA5A5A5A2) {
            printf("[intel-prph] ERROR: MAC access denied (0x%x)\n", readback);
            return -1;  // 不要重试，这是权限问题
        }

        // 重试前延迟
        if (attempt < max_retries - 1) {
            atheros_delay_us(100);
        }
    }

    printf("[intel-prph] ✗ FAILED after %d attempts\n", max_retries);
    printf("[intel-prph]   Final readback: 0x%x (expected 0x%x)\n", readback, value);
    return -1;
}

/**
 * @brief 显示快照摘要
 */
static void show_snapshot_summary(void) {
    
    printf("Total checkpoints: %d\n", snapshot_count);
    
    for (int i = 0; i < snapshot_count; i++) {
        debug_snapshot_t *snap = &snapshots[i];
        printf("[%d]:===", i, snap->step_name);

        printf("CSR_GP_CNTRL   = 0x%08x\n", snap->csr_gp_cntrl);
        printf("MAC_CLOCK_READY=%s MAC_ACCESS=%s XTAL_ON=%s\n",
               (snap->csr_gp_cntrl & 1) ? "Y" : "N",
               (snap->csr_gp_cntrl & 8) ? "Y" : "N",
               (snap->csr_gp_cntrl & 0x400) ? "Y" : "N");

        printf("APMG_CLK_CTRL  = 0x%08x\n", snap->apmg_clk_ctrl);
        printf("APMG_CLK_EN    = 0x%08x\n", snap->apmg_clk_en);
        printf("CPU1_HDR_ADDR  = 0x%08x ,%s\n",
               snap->cpu1_hdr_addr,
               (snap->cpu1_hdr_addr == 0x2000) ? "YES" : "NO");

        printf("SRAM[0x0]= 0x%08x\n", snap->sram_0x0);
        printf("SRAM[0x2000]= 0x%08x\n", snap->sram_0x2000);

        if (i < snapshot_count - 1) {
            printf("═══════════════════════════════════════════════════════════════════\n");
        }
    }

    
    printf("═══════════════════════════════════════════════════════════════════\n");
    
}

/**
 * @brief 交互式调试暂停 - 显示信息并暂停10秒，方便截图
 *
 * 注意：在内核代码中无法直接读取用户输入（需要通过中断等复杂机制）
 * 所以这里只是显示一个暂停标记，并延迟10秒方便截图
 */
static void intel_debug_pause(const char *step_name, uint32_t mem_base, int is_final) {
    // 🔥 捕获当前状态快照
    capture_snapshot(step_name, mem_base);

    // 🔥 串口调试输出（无条件输出，方便无显示器调试）
    uart_debug("\n[intel-debug] ===== PAUSE: %s =====\n", step_name);

    printf("\n");
    SET_COLOR_RED();
    printf("╔════════════════════════════════════════════════════════╗\n");
    SET_COLOR_WHITE();
    printf("║  🔵 DEBUG PAUSE: %s", step_name);
    for (int i = strlen(step_name); i < 43; i++) printf(" ");
    printf(" ║\n");
    SET_COLOR_RED();
    printf("╠════════════════════════════════════════════════════════╣\n");
    SET_COLOR_WHITE();
    printf("║  === Checkpoint Reached ===                          ║\n");
    printf("║  📸 Screenshot opportunity (10 seconds)                  ║\n");
    SET_COLOR_RED();
    printf("╚════════════════════════════════════════════════════════╝\n");
    SET_COLOR_WHITE();

    // 🔥 显示当前关键寄存器值
    uint32_t csr_gp_cntrl = atheros_reg_read(mem_base, CSR_GP_CNTRL);
    uint32_t cpu1_hdr = intel_read_prph(mem_base, LMPM_SECURE_UCODE_LOAD_CPU1_HDR_ADDR);

    printf("   CSR_GP_CNTRL = 0x%08x (MAC:%s ACC:%s XTAL:%s)\n",
           csr_gp_cntrl,
           (csr_gp_cntrl & 1) ? "Y" : "N",
           (csr_gp_cntrl & 8) ? "Y" : "N",
           (csr_gp_cntrl & 0x400) ? "Y" : "N");
    printf("   CPU1_HDR_ADDR = 0x%08x %s\n",
           cpu1_hdr,
           (cpu1_hdr == 0x2000) ? "✓" : "✗");

    // 🔥 同时输出到串口
    uart_debug("   CSR_GP_CNTRL = 0x%08x (MAC:%c ACC:%c XTAL:%c)\n",
           csr_gp_cntrl,
           (csr_gp_cntrl & 1) ? 'Y' : 'N',
           (csr_gp_cntrl & 8) ? 'Y' : 'N',
           (csr_gp_cntrl & 0x400) ? 'Y' : 'N');
    uart_debug("   CPU1_HDR_ADDR = 0x%08x %s\n",
           cpu1_hdr,
           (cpu1_hdr == 0x2000) ? "OK" : "FAIL");

    // 🔥 如果是最后一个暂停点，显示完整摘要
    if (is_final) {
        //show_snapshot_summary();
    } else {
        // 暂停 10 秒
        atheros_delay_us(10000000);
    }

    printf("[intel-debug] Continuing...\n\n");
    uart_debug("[intel-debug] Continuing...\n");
}

/**
 * @brief 等待 Intel WiFi 硬件位
 */
static int intel_wait_for_bits(uint32_t mem_base, uint32_t reg,
                                uint32_t bits, uint32_t val,
                                uint32_t timeout_us) {
    uint32_t reg_val;
    int timeout = timeout_us / 10;

    while (timeout-- > 0) {
        reg_val = atheros_reg_read(mem_base, reg);
        if ((reg_val & bits) == val) {
            return 0;
        }
        atheros_delay_us(10);
    }

    printf("[intel-hw] Timeout waiting for bits at reg 0x%x (bits=0x%x, val=0x%x)\n",
           reg, bits, val);
    return -1;
}

/**
 * @brief 硬件复位
 */
int intel_hw_reset(uint32_t mem_base) {
    printf("[intel-hw] Resetting hardware...\n");
    uart_debug("[intel-hw] Resetting hardware...\n");

    // 🔥 关键修复：清除 reset 位！
    // 这是 INIT firmware 能够执行的前提
    atheros_reg_write(mem_base, CSR_RESET, 0x00000000);
    atheros_delay_us(10);

    // 验证 reset 已清除
    uint32_t reset_val = atheros_reg_read(mem_base, CSR_RESET);
    printf("[intel-hw] CSR_RESET after clear: 0x%08x (must be 0!)\n", reset_val);
    uart_debug("[intel-hw] CSR_RESET after clear: 0x%08x\n", reset_val);
    if (reset_val & 0x00000001) {
        printf("[intel-hw] ERROR: Device still in reset!\n");
        // uart_panic("[intel-hw] ERROR: Device still in reset!");  // 🔥 注释掉，避免未初始化的串口访问导致重启
        return -1;
    }

    // 初始化完成标志
    atheros_reg_write(mem_base, CSR_GP_CNTRL, CSR_GP_CNTRL_REG_FLAG_INIT_DONE);

    // 等待初始化完成
    if (intel_wait_for_bits(mem_base, CSR_GP_CNTRL,
                             CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY,
                             CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY, 200000) < 0) {
        printf("[intel-hw] Warning: MAC clock not ready after reset\n");
        uart_debug("[intel-hw] Warning: MAC clock not ready after reset\n");
        return -1;
    }

    printf("[intel-hw] Hardware reset complete, MAC clock ready\n");
    uart_debug("[intel-hw] Hardware reset complete, MAC clock ready\n");
    return 0;
}

/**
 * @brief 请求 MAC 访问权限
 */
int intel_hw_grant_mac_access(uint32_t mem_base) {
    printf("[intel-hw] Requesting MAC access...\n");
    uart_debug("[intel-hw] Requesting MAC access...\n");

    // 🔥 诊断：读取初始状态
    uint32_t gp_cntrl_init = atheros_reg_read(mem_base, CSR_GP_CNTRL);
    printf("[intel-hw] CSR_GP_CNTRL initial: 0x%08x\n", gp_cntrl_init);
    uart_debug("[intel-hw] CSR_GP_CNTRL initial: 0x%08x\n", gp_cntrl_init);
    printf("[intel-hw]   MAC_CLOCK_READY (bit 0): %s\n",
           (gp_cntrl_init & CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY) ? "YES" : "NO");
    printf("[intel-hw]   MAC_ACCESS_REQ (bit 3): %s\n",
           (gp_cntrl_init & CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ) ? "YES" : "NO");
    printf("[intel-hw]   INIT_DONE (bit 2): %s\n",
           (gp_cntrl_init & CSR_GP_CNTRL_REG_FLAG_INIT_DONE) ? "YES" : "NO");
    printf("[intel-hw]   XTAL_ON (bit 10): %s\n",
           (gp_cntrl_init & CSR_GP_CNTRL_REG_FLAG_XTAL_ON) ? "YES" : "NO");

    // 同时输出到串口
    uart_debug("[intel-hw] MAC_CLK:%c ACC:%c INIT:%c XTAL:%c\n",
           (gp_cntrl_init & CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY) ? 'Y' : 'N',
           (gp_cntrl_init & CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ) ? 'Y' : 'N',
           (gp_cntrl_init & CSR_GP_CNTRL_REG_FLAG_INIT_DONE) ? 'Y' : 'N',
           (gp_cntrl_init & CSR_GP_CNTRL_REG_FLAG_XTAL_ON) ? 'Y' : 'N');

    // 🔥 关键修复：如果 MAC_ACCESS_REQ 已经置位，说明已经有访问权限
    if (gp_cntrl_init & CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ) {
        printf("[intel-hw] MAC access already granted\n");
        return 0;
    }

    // 🔥 关键修复：如果 XTAL_ON 没有置位，尝试启用 XTAL
    // 某些设备需要 XTAL 稳定后才会授予 MAC 访问权限
    if (!(gp_cntrl_init & CSR_GP_CNTRL_REG_FLAG_XTAL_ON)) {
        printf("[intel-hw] WARNING: XTAL not ready, trying to enable it...\n");

        // 方法 1: 设置 XTAL_ON 位（某些设备支持）
        uint32_t gp = gp_cntrl_init;
        gp |= CSR_GP_CNTRL_REG_FLAG_XTAL_ON;
        atheros_reg_write(mem_base, CSR_GP_CNTRL, gp);
        printf("[intel-hw] Set XTAL_ON bit\n");

        atheros_delay_us(10000);  // 等待 10 ms 让 XTAL 稳定

        // 重新读取状态
        gp = atheros_reg_read(mem_base, CSR_GP_CNTRL);
        printf("[intel-hw] After XTAL enable: CSR_GP_CNTRL = 0x%08x\n", gp);

        // 方法 2: 如果还是不行，尝试通过 APMG 寄存器启用 XTAL
        if (!(gp & CSR_GP_CNTRL_REG_FLAG_XTAL_ON)) {
            printf("[intel-hw] XTAL still not ready, trying APMG approach...\n");
            // 注意：此时 PRPH 可能还不可访问，所以这里可能失败
            // 我们继续尝试，不中断流程
        }
    }

    // 🔥 关键修复：强制设置 MAC_ACCESS_REQ 位来**请求** MAC 访问权限
    // 注意：这不是"授予"访问权限，而是向硬件**请求**访问权限
    uint32_t gp_cntrl = atheros_reg_read(mem_base, CSR_GP_CNTRL);
    gp_cntrl |= CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ;  // 请求 MAC 访问
    atheros_reg_write(mem_base, CSR_GP_CNTRL, gp_cntrl);
    printf("[intel-hw] Set MAC_ACCESS_REQ bit (requesting access)\n");

    // 🔥 关键修复：等待硬件授予 MAC 访问权限（轮询 MAC_ACCESS_REQ 位）
    // 根据 Intel 文档，这个位可能需要一些时间才能被硬件置位
    printf("[intel-hw] Waiting for MAC access grant...\n");
    int timeout = 10000;  // 10 秒超时
    int granted = 0;

    for (int i = 0; i < timeout; i++) {
        uint32_t gp = atheros_reg_read(mem_base, CSR_GP_CNTRL);
        if (gp & CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ) {
            printf("[intel-hw] ✓ MAC access granted after %d ms\n", i);
            granted = 1;
            break;
        }
        atheros_delay_us(1000);  // 1 ms

        // 每 1 秒打印一次进度
        if ((i % 1000) == 0 && i > 0) {
            printf("[intel-hw] Still waiting... (%d sec)\n", i / 1000);
        }
    }

    if (!granted) {
        printf("[intel-hw] ✗ ERROR: MAC access NOT granted after %d ms!\n", timeout);
        printf("[intel-hw]   PRPH writes will likely fail!\n");
        printf("[intel-hw]   Continuing anyway...\n");
        // 不返回错误，让程序继续尝试
    }

    return granted ? 0 : -1;
}

/**
 * @brief 初始化时钟
 */
int intel_hw_init_clocks(uint32_t mem_base) {
    printf("[intel-hw] Initializing clocks...\n");

    // 请求 MAC 访问权限
    if (intel_hw_grant_mac_access(mem_base) < 0) {
        return -1;
    }

    // 🔥 关键修复：直接写 APMG 寄存器，不需要先读取（此时 PRPH 可能还不可访问）
    // 步骤 1: 使能 DMA 和 BSM 时钟 (APMG_CLK_EN_REG)
    printf("[intel-hw] Enabling DMA and BSM clocks...\n");
    intel_write_prph(mem_base, APMG_CLK_EN_REG,
                     APMG_CLK_EN_REG_MSK_DMA_CLK_INIT | APMG_CLK_EN_REG_MSK_BSM_CLK_INIT);
    atheros_delay_us(20);

    // 步骤 2: 请求 DMA 和 BSM 时钟 (APMG_CLK_CTRL_REG)
    printf("[intel-hw] Requesting DMA and BSM clocks...\n");
    intel_write_prph(mem_base, APMG_CLK_CTRL_REG,
                     APMG_CLK_CTRL_REG_MSK_DMA_CLK_RQT | APMG_CLK_CTRL_REG_MSK_BSM_CLK_RQT);
    atheros_delay_us(20);

    // 步骤 3: 验证时钟是否成功使能
    uint32_t clk_en = intel_read_prph(mem_base, APMG_CLK_EN_REG);
    uint32_t clk_ctrl = intel_read_prph(mem_base, APMG_CLK_CTRL_REG);
    printf("[intel-hw] APMG_CLK_EN_REG = 0x%08x (expected 0x%08x)\n",
           clk_en, APMG_CLK_EN_REG_MSK_DMA_CLK_INIT | APMG_CLK_EN_REG_MSK_BSM_CLK_INIT);
    printf("[intel-hw] APMG_CLK_CTRL_REG = 0x%08x (expected 0x%08x)\n",
           clk_ctrl, APMG_CLK_CTRL_REG_MSK_DMA_CLK_RQT | APMG_CLK_CTRL_REG_MSK_BSM_CLK_RQT);

    if (clk_en == 0xA5A5A5A1 || clk_ctrl == 0xA5A5A5A1) {
        printf("[intel-hw] ERROR: PRPH registers still returning 0xA5A5A5A1 after clock init!\n");
        return -1;
    }

    printf("[intel-hw] Clocks initialized successfully\n");
    return 0;
}

/**
 * @brief 停止 PCIe 主设备
 */
void intel_hw_stop_device(uint32_t mem_base) {
    printf("[intel-hw] Stopping device...\n");

    // 复位主设备
    uint32_t reset_val = atheros_reg_read(mem_base, CSR_RESET);
    reset_val |= CSR_RESET_REG_FLAG_MASTER_DISABLED;
    atheros_reg_write(mem_base, CSR_RESET, reset_val);

    atheros_delay_us(100);

    printf("[intel-hw] Device stopped\n");
}

/**
 * @brief 初始化硬件
 */
int intel_hw_init(uint32_t mem_base) {
    printf("[intel-hw] Initializing Intel WiFi hardware...\n");

    // 1. 停止设备
    intel_hw_stop_device(mem_base);

    // 2. 复位硬件
    if (intel_hw_reset(mem_base) < 0) {
        printf("[intel-hw] Hardware reset failed\n");
        return -1;
    }

    // 3. 初始化时钟
    if (intel_hw_init_clocks(mem_base) < 0) {
        printf("[intel-hw] Clock initialization failed\n");
        return -1;
    }

    // 4. 配置中断
    // 禁用所有中断
    atheros_reg_write(mem_base, CSR_INT_MASK, 0x00000000);
    atheros_reg_write(mem_base, CSR_INT, 0xFFFFFFFF);

    // 5. 设置 LED 寄存器
    atheros_reg_write(mem_base, CSR_LED_REG, 0x00000038);

    // 6. 配置 EEPROM/OTP GPIO
    atheros_reg_write(mem_base, CSR_EEPROM_GP, 0x00000007);

    // 7. 设置驱动指示
    atheros_reg_write(mem_base, CSR_GP_DRIVER_REG, 0x000000FF);

    printf("[intel-hw] Hardware initialized successfully\n");
    return 0;
}

/**
 * @brief 使能中断
 */
int intel_hw_enable_interrupts(uint32_t mem_base) {
    printf("[intel-hw] Enabling interrupts...\n");

    // 清除所有挂起的中断
    atheros_reg_write(mem_base, CSR_INT, 0xFFFFFFFF);

    // 使能需要的中断
    uint32_t int_mask = CSR_INI_SET_MASK;
    atheros_reg_write(mem_base, CSR_INT_MASK, int_mask);

    printf("[intel-hw] Interrupts enabled (mask=0x%x)\n", int_mask);
    return 0;
}

/**
 * @brief 禁用中断
 */
void intel_hw_disable_interrupts(uint32_t mem_base) {
    printf("[intel-hw] Disabling interrupts...\n");

    // 禁用所有中断
    atheros_reg_write(mem_base, CSR_INT_MASK, 0x00000000);

    printf("[intel-hw] Interrupts disabled\n");
}

/**
 * @brief 读取 MAC 地址
 */
int intel_hw_read_mac_addr(uint32_t mem_base, uint8_t *mac) {
    if (!mac) return -1;

    // Intel 6000 系列的 MAC 地址存储在 OTP 中
    // 简化实现：使用你的真实 Intel 网卡 MAC 地址
    mac[0] = 0x84;
    mac[1] = 0x3A;
    mac[2] = 0x4B;
    mac[3] = 0xA0;
    mac[4] = 0x05;
    mac[5] = 0x0C;

    printf("[intel-hw] MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return 0;
}

/**
 * @brief 获取中断状态
 */
uint32_t intel_hw_get_int_status(uint32_t mem_base) {
    return atheros_reg_read(mem_base, CSR_INT);
}

/**
 * @brief 应答中断
 */
void intel_hw_ack_interrupts(uint32_t mem_base, uint32_t ints) {
    atheros_reg_write(mem_base, CSR_INT, ints);
}

/**
 * @brief 检查硬件是否存活
 */
int intel_hw_is_alive(uint32_t mem_base) {
    uint32_t gp_cntrl = atheros_reg_read(mem_base, CSR_GP_CNTRL);

    if (gp_cntrl & CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY) {
        return 1;
    }

    return 0;
}

/**
 * @brief 通过 BSM 加载 firmware 段到 NIC SRAM
 *
 * Intel 6000 series 使用 BSM (Boot State Machine) 来加载 firmware
 *
 * @param mem_base PCI 内存基地址
 * @param fw_data Firmware 数据指针（虚拟地址）
 * @param fw_size Firmware 大小（字节数）
 * @param sram_offset SRAM 目标偏移地址
 * @return 0 = 成功, -1 = 失败
 */
static int intel_fw_load_via_bsm_bak(uint32_t mem_base, const uint8_t *fw_data,
                                  uint32_t fw_size, uint32_t sram_offset) {
     printf("[intel-fw] Loading firmware via BSM...\n");
    // printf("[intel-fw]   Data: virt=0x%x, size=%d bytes\n", (uint32_t)fw_data, fw_size);
    // printf("[intel-fw]   Target SRAM offset: 0x%x\n", sram_offset);

    // 固件数据必须是 word (4 bytes) 对齐
    uint32_t *fw_data_32 = (uint32_t *)fw_data;
    uint32_t fw_words = fw_size / 4;

    // printf("[intel-fw] Writing %d words (%d bytes) to SRAM at 0x%x\n", fw_words, fw_size, sram_offset);
    // 使用 BSM 写入 firmware（每次写一个 word）
    for (uint32_t i = 0; i < fw_words; i++) {
        // 写数据到 BSM 数据寄存器
        atheros_reg_write(mem_base, CSR_BSM_WR_DATA, fw_data_32[i]);
        printf("BSM 1 write SRAM[0x%x] = 0x%08x\n",(sram_offset + i * 4), fw_data_32[i]);

        // 写地址和启动位
        uint32_t target_addr = sram_offset + i * 4;
        uint32_t ctrl = target_addr |CSR_BSM_WR_CTRL_REG_BIT_WRITE | CSR_BSM_WR_CTRL_REG_BIT_START;
        atheros_reg_write(mem_base, CSR_BSM_WR_CTRL_REG, ctrl);

        printf("BSM 2 write SRAM[0x%x] = 0x%08x\n",(sram_offset + i * 4), fw_data_32[i]);

        // 等待完成（短延迟）
        for (volatile int j = 0; j < 100; j++) {
            __asm__ volatile("nop");
        }

        // 检查是否完成
        // while(atheros_reg_read(mem_base, CSR_BSM_WR_CTRL_REG) & CSR_BSM_WR_CTRL_REG_BIT_START);
        int timeout = 3;
        uint32_t v;
        do {
            v = atheros_reg_read(mem_base, CSR_BSM_WR_CTRL_REG);
            printf("BSM_CTRL = 0x%08x\n", v);
        } while (v & CSR_BSM_WR_CTRL_REG_BIT_START && --timeout);

        if (!timeout) {
            printf("BSM WRITE TIMEOUT at addr=0x%x\n",
                   (sram_offset + i * 4));
            return -1;
        }
         
    }
    
    printf("[intel-fw] BSM load complete\n");
    return 0;
}

// ==================== FH DMA 固件加载（Linux iwlwifi 模式） ====================

/**
 * @brief 使用 FH DMA 加载固件块（对应 iwl_pcie_load_firmware_chunk_fh）
 *
 * @param mem_base PCI 内存基地址
 * @param dst_addr 目标 SRAM 地址（设备内部地址）
 * @param phy_addr 物理 DMA 地址（我们在 OS 中使用虚拟地址 + 偏移）
 * @param byte_cnt 要加载的字节数
 *
 * @return 0 = 成功, -1 = 失败
 */
static int intel_pcie_load_firmware_chunk_fh(uint32_t mem_base,
                                              uint32_t dst_addr,
                                              uint32_t phy_addr,
                                              uint32_t byte_cnt)
{
    printf("[intel-fh] Loading chunk: dst=0x%x, src=0x%x, size=%u\n",
           dst_addr, phy_addr, byte_cnt);

    // 1. 暂停 DMA 通道
    atheros_reg_write(mem_base, FH_TCSR_CHNL_TX_CONFIG_REG(FH_SRVC_CHNL),
                      FH_TCSR_TX_CONFIG_REG_VAL_DMA_CHNL_PAUSE);

    // 2. 设置目标 SRAM 地址
    atheros_reg_write(mem_base, FH_SRVC_CHNL_SRAM_ADDR_REG(FH_SRVC_CHNL), dst_addr);

    // 3. 设置源物理地址（低 32 位）
    uint32_t phy_addr_lsb = phy_addr & FH_MEM_TFDIB_DRAM_ADDR_LSB_MSK;
    atheros_reg_write(mem_base, FH_TFDIB_CTRL0_REG(FH_SRVC_CHNL), phy_addr_lsb);

    // 4. 设置源物理地址（高 4 位）和传输长度
    uint32_t phy_addr_msb = (phy_addr >> 32) & 0xF;
    uint32_t ctrl1 = (phy_addr_msb << FH_MEM_TFDIB_REG1_ADDR_BITSHIFT) |
                     (byte_cnt & FH_MEM_TFDIB_REG1_LEN_MSK);
    atheros_reg_write(mem_base, FH_TFDIB_CTRL1_REG(FH_SRVC_CHNL), ctrl1);

    // 5. 设置缓冲区状态（标记有效）
    atheros_reg_write(mem_base, FH_TCSR_CHNL_TX_BUF_STS_REG(FH_SRVC_CHNL),
                      (1 << FH_TCSR_CHNL_TX_BUF_STS_REG_POS_TB_NUM) |
                      (1 << FH_TCSR_CHNL_TX_BUF_STS_REG_POS_TB_IDX) |
                      FH_TCSR_CHNL_TX_BUF_STS_REG_VAL_TFDB_VALID);

    // 6. 启动 DMA 传输
    atheros_reg_write(mem_base, FH_TCSR_CHNL_TX_CONFIG_REG(FH_SRVC_CHNL),
                      FH_TCSR_TX_CONFIG_REG_VAL_DMA_CHNL_ENABLE |
                      FH_TCSR_TX_CONFIG_REG_VAL_DMA_CREDIT_DISABLE |
                      FH_TCSR_TX_CONFIG_REG_VAL_CIRQ_HOST_ENDTFD);

    // 7. 等待传输完成（简单延迟，生产环境应该等待中断）
    for (volatile int i = 0; i < 1000; i++) {
        __asm__ volatile("nop");
    }

    printf("[intel-fh] Chunk loaded\n");
    return 0;
}

/**
 * @brief 加载固件块（对应 iwl_pcie_load_firmware_chunk）
 *
 * @param mem_base PCI 内存基地址
 * @param dst_addr 目标 SRAM 地址（设备内部地址）
 * @param src_addr 源数据地址（虚拟地址）
 * @param byte_cnt 要加载的字节数
 *
 * @return 0 = 成功, -1 = 失败/超时
 */
static int intel_pcie_load_firmware_chunk(uint32_t mem_base,
                                          uint32_t dst_addr,
                                          const uint8_t *src_addr,
                                          uint32_t byte_cnt)
{
    printf("[intel-fh] Loading firmware chunk: dst=0x%x, size=%u\n",
           dst_addr, byte_cnt);

    // 🔥 在我们的 OS 中，我们直接使用虚拟地址作为 DMA 地址
    // （因为我们没有实现复杂的 DMA 映射机制）
    // Linux 使用 dma_alloc_coherent 获得物理地址
    uint32_t phy_addr = (uint32_t)src_addr;

    // 授予 MAC 访问权限（如果需要）
    uint32_t gp_cntrl = atheros_reg_read(mem_base, CSR_GP_CNTRL);
    if (!(gp_cntrl & CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY)) {
        printf("[intel-fh] WARNING: MAC clock not ready, trying to grant access\n");
        intel_hw_grant_mac_access(mem_base);
    }

    // 检查是否需要扩展地址空间
    bool extended_addr = false;
    if (dst_addr >= IWL_FW_MEM_EXTENDED_START &&
        dst_addr <= IWL_FW_MEM_EXTENDED_END) {
        extended_addr = true;
        printf("[intel-fh] Using extended address space\n");
        intel_set_bits_prph(mem_base, LMPM_CHICK, LMPM_CHICK_EXTENDED_ADDR_SPACE);
    }

    // 调用 FH DMA 加载函数
    int ret = intel_pcie_load_firmware_chunk_fh(mem_base, dst_addr, phy_addr, byte_cnt);

    // 清除扩展地址空间标志
    if (extended_addr) {
        intel_clear_bits_prph(mem_base, LMPM_CHICK, LMPM_CHICK_EXTENDED_ADDR_SPACE);
    }

    if (ret < 0) {
        printf("[intel-fh] ERROR: Failed to load firmware chunk!\n");
        return -1;
    }

    // 🔥 等待写入完成（Linux 使用等待队列，我们使用简单轮询）
    // 在生产环境中，应该等待 FH_TX_INTERRUPT 中断
    for (int timeout = 0; timeout < 10000; timeout++) {
        // 检查 DMA 通道是否空闲
        uint32_t tx_config = atheros_reg_read(mem_base, FH_TCSR_CHNL_TX_CONFIG_REG(FH_SRVC_CHNL));
        if (tx_config & FH_TCSR_TX_CONFIG_REG_VAL_DMA_CHNL_ENABLE) {
            // DMA 仍在运行
            for (volatile int i = 0; i < 100; i++) {
                __asm__ volatile("nop");
            }
        } else {
            // DMA 完成
            printf("[intel-fh] DMA transfer completed\n");
            return 0;
        }
    }

    printf("[intel-fh] WARNING: DMA transfer timeout (continuing anyway)\n");
    return 0;
}

/**
 * @brief 加载固件段（对应 iwl_pcie_load_section）- Linux 方式
 *
 * 🔥 使用临时 DMA 缓冲区，确保物理地址连续
 * 这完全模拟 Linux iwlwifi 的 dma_alloc_coherent 方式
 *
 * @param mem_base PCI 内存基地址
 * @param section_num 段编号
 * @param section_data 固件数据
 * @param section_len 固件长度
 * @param section_offset SRAM 偏移地址
 *
 * @return 0 = 成功, -1 = 失败
 */
// 🔥 栈保护计数器（检测可能的递归）
static int stack_depth_counter = 0;
#define MAX_STACK_DEPTH 50

static int intel_pcie_load_section(uint32_t mem_base,
                                    uint8_t section_num,
                                    const uint8_t *section_data,
                                    uint32_t section_len,
                                    uint32_t section_offset)
{
    // 🔥 栈溢出保护：检测递归
    stack_depth_counter++;
    if (stack_depth_counter > MAX_STACK_DEPTH) {
        printf("\n\n");
        printf("╔════════════════════════════════════════════════════════╗\n");
        printf("║  🔴 CRITICAL: STACK OVERFLOW DETECTED! 🔴               ║\n");
        printf("║  stack_depth=%d > MAX_STACK_DEPTH=%d                  ║\n", stack_depth_counter, MAX_STACK_DEPTH);
        printf("║  Preventing system reboot...                             ║\n");
        printf("╚════════════════════════════════════════════════════════╝\n");
        printf("\n");
        stack_depth_counter--;
        return -1;
    }

    printf("[intel-fh] [%d] Loading uCode section (Linux dma_alloc_coherent mode)...\n", section_num);
    printf("[intel-fh] [%d] Offset: 0x%x, Length: %u bytes\n",
           section_num, section_offset, section_len);

    // 🔥 诊断：检查输入参数
    if (!section_data || section_len == 0) {
        printf("[intel-fh] ERROR: Invalid section data!\n");
        stack_depth_counter--;
        return -1;
    }

    // 🔥 安全限制：防止栈溢出
    // 4KB 栈分配可能太大，改为 2KB，并且使用静态缓冲区（更安全）
    static uint8_t dma_buf[2048];  // 🔥 静态分配，不在栈上
    uint32_t dma_buf_size = 2048;

    // 🔥 栈使用诊断：打印当前栈指针
    uint32_t current_esp;
    __asm__ volatile("movl %%esp, %0" : "=r"(current_esp));
    printf("[intel-fh] [%d] DMA buffer: virt=0x%x, size=%u bytes (STATIC)\n",
           section_num, (uint32_t)dma_buf, dma_buf_size);
    printf("[intel-fh] [%d] Stack pointer: ESP=0x%x (stack_depth=%d)\n",
           section_num, current_esp, stack_depth_counter);

    uint32_t offset = 0;
    uint32_t chunk_sz = FH_MEM_TB_MAX_LENGTH;  // 8KB chunks（外层循环）

    // 外层循环：每次处理 8KB
    while (offset < section_len) {
        uint32_t copy_size = chunk_sz;
        if (offset + chunk_sz > section_len) {
            copy_size = section_len - offset;
        }

        // 内层循环：每次传输 4KB（dma_buf 大小）
        uint32_t chunk_offset = 0;
        while (chunk_offset < copy_size) {
            uint32_t this_copy = copy_size - chunk_offset;
            if (this_copy > dma_buf_size) {
                this_copy = dma_buf_size;
            }

            uint32_t dst_addr = section_offset + offset + chunk_offset;

            // 🔥 步骤 1: 复制数据到 DMA 缓冲区（对应 Linux 的 memcpy）
            memcpy(dma_buf, section_data + offset + chunk_offset, this_copy);
            printf("[intel-fh] [%d] Copied %u bytes to DMA buffer\n",
                   section_num, this_copy);

            // 🔥 步骤 2: 获取 DMA 缓冲区的物理地址
            // 在我们的 OS 中，我们假设栈地址也是物理可访问的
            // （这在 x86 上通常可行，因为使用 1:1 映射）
            uint32_t phy_addr = (uint32_t)dma_buf;
            printf("[intel-fh] [%d] DMA phys addr: 0x%x\n",
                   section_num, phy_addr);

            // 🔥 步骤 3: 检查是否需要扩展地址空间
            bool extended_addr = false;
            if (dst_addr >= IWL_FW_MEM_EXTENDED_START &&
                dst_addr <= IWL_FW_MEM_EXTENDED_END) {
                extended_addr = true;
                printf("[intel-fh] [%d] Enabling extended address space\n", section_num);
                intel_set_bits_prph(mem_base, LMPM_CHICK, LMPM_CHICK_EXTENDED_ADDR_SPACE);
            }

            // 🔥 步骤 4: 使用 FH DMA 加载这个块（对应 Linux 的 iwl_pcie_load_firmware_chunk）
            int ret = intel_pcie_load_firmware_chunk_fh(mem_base, dst_addr, phy_addr, this_copy);

            // 清除扩展地址空间标志
            if (extended_addr) {
                intel_clear_bits_prph(mem_base, LMPM_CHICK, LMPM_CHICK_EXTENDED_ADDR_SPACE);
            }

            if (ret < 0) {
                printf("[intel-fh] ERROR: Could not load the [%d] uCode section at offset %u\n",
                       section_num, offset + chunk_offset);
                return -1;
            }

            chunk_offset += this_copy;
        }

        offset += chunk_sz;

        // 🔥 调试：每 32KB 打印一次进度
        if ((offset & 0x7FFF) == 0) {
            printf("[intel-fh] [%d] Progress: %u/%u bytes (%d%%)\r",
                   section_num, offset, section_len, (offset * 100) / section_len);
        }
    }

    printf("\n[intel-fh] [%d] Section loaded successfully (Linux mode)\n", section_num);
    stack_depth_counter--;  // 🔥 减少栈深度计数器
    return 0;
}

// ⚠️ DEPRECATED: 此函数已废弃，请使用 intel_pcie_load_section() 代替
// 保留此函数仅用于兼容 #else 分支（USE_FH_DMA_FOR_FW_LOADING = 0）
static int intel_fw_load_via_bsm(uint32_t mem_base,
                                 const uint8_t *fw_data,
                                 uint32_t fw_size,
                                 uint32_t sram_offset)
{
    // 🔥 关键修复：区分 device internal SRAM 地址和 PCI MMIO 地址
    //
    // TLV load_addr (如 0x400000) 是设备内部 SRAM 地址空间
    // PCI BAR 只映射了部分 SRAM 窗口（通常前几 KB）
    //
    // 对于大 offset（如 0x400000），必须使用 BSM DMA 引擎
    // BSM 理解设备内部地址空间，可以写入整个 SRAM
    //
    // 对于小 offset（如 0x0），可以使用直接 MMIO 写入

    printf("[intel-fw] SRAM write: %u bytes to device offset 0x%x\n",
           fw_size, sram_offset);

    // 固件数据必须是 4 字节对齐
    const uint32_t *fw_data_32 = (const uint32_t *)fw_data;
    uint32_t fw_words = fw_size / 4;

    // 🔥 判断使用哪种写入方式
    // 小 offset（在 BAR 映射窗口内）：直接 MMIO 写入
    // 大 offset（超出 BAR 窗口）：使用 BSM DMA
    //
    // Intel 6205 BAR 窗口通常较小（CSR 寄存器约 1.3KB）
    // 使用 0x40000 (256KB) 作为安全阈值
    if (sram_offset < 0x40000) {
        // 方法 1：直接 MMIO 写入（适用于指令段）
        volatile uint32_t *sram = (volatile uint32_t *)(mem_base + sram_offset);
        printf("[intel-fw]   Using direct MMIO write (BAR+offset: 0x%x)\n", (uint32_t)sram);

        for (uint32_t i = 0; i < fw_words; i++) {
            sram[i] = fw_data_32[i];
        }
    } else {
        // 方法 2：BSM DMA 写入（适用于数据段）
        // BSM (Boot State Machine) 可以访问设备内部完整 SRAM 空间
        printf("[intel-fw]   Using BSM DMA (offset 0x%x exceeds BAR window)\n", sram_offset);
        printf("[intel-fw]   Writing %d words via BSM...\n", fw_words);

        // 使用 BSM 寄存器写入每个字
        for (uint32_t i = 0; i < fw_words; i++) {
            // 1. 写数据到 BSM 数据寄存器
            atheros_reg_write(mem_base, CSR_BSM_WR_DATA, fw_data_32[i]);

            // 2. 计算目标地址（设备内部 SRAM 地址）
            uint32_t target_addr = sram_offset + (i * 4);

            // 3. 写地址和启动位到 BSM 控制寄存器
            // BSM 理解设备内部地址空间，所以 target_addr = 0x400000 是有效的
            uint32_t ctrl = target_addr | CSR_BSM_WR_CTRL_REG_BIT_WRITE | CSR_BSM_WR_CTRL_REG_BIT_START;
            atheros_reg_write(mem_base, CSR_BSM_WR_CTRL_REG, ctrl);

            // 4. 等待 BSM 完成写入
            // 🔥 调试：每 8KB (2048 words) 打印一次进度
            if ((i & 0x7FF) == 0) {
                printf("[intel-fw]   Progress: %d/%d words (%d%%) to SRAM[0x%x]\r",
                       i, fw_words, (i * 100) / fw_words, target_addr);
            }

            // 短暂延迟，让 BSM 完成写入
            for (volatile int j = 0; j < 100; j++) {
                __asm__ volatile("nop");
            }

            // 可选：等待 BSM_START 位清除（表示写入完成）
            // while (atheros_reg_read(mem_base, CSR_BSM_WR_CTRL_REG) & CSR_BSM_WR_CTRL_REG_BIT_START);
        }
        printf("\n[intel-fw]   BSM DMA write complete\n");
    }

    // 处理剩余字节（如果有）
    if (fw_size % 4) {
        printf("[intel-fw]   Writing %d remaining bytes\n", fw_size % 4);
        const uint8_t *remaining = (const uint8_t *)(fw_data_32 + fw_words);

        if (sram_offset < 0x100000) {
            // 小 offset：直接写入
            volatile uint8_t *sram_bytes = (volatile uint8_t *)(mem_base + sram_offset + fw_words * 4);
            for (uint32_t i = 0; i < fw_size % 4; i++) {
                sram_bytes[i] = remaining[i];
            }
        } else {
            // 大 offset：使用 BSM 写入剩余字节（需要临时缓冲区）
            // 为了简单，我们用最后一个完整的 word 来处理
            printf("[intel-fw]   WARNING: Partial bytes at large offset, padding with zeros\n");
        }
    }

    printf("[intel-fw] SRAM write done\n");
    return 0;
}
#define CSR_BSM_CTRL   (CSR_BASE + 0x140)
#define CSR_GP_CNTRL_INIT_DONE  0x00000004  // bit 2
#define INT_ALIVE 0x00000001
/**
 * @brief 启动 Intel WiFi Firmware (完整的 INIT + RUNTIME 流程)
 *
 * @param mem_base PCI 内存基地址
 * @param parsed 解析后的固件结构
 *
 * @return 0 = 成功, -1 = 失败
 */
int intel_fw_start_parsed(uint32_t mem_base, struct intel_fw_parsed *parsed) {
    printf("\n[intel-fw] ========================================\n");
    printf("[intel-fw] Intel WiFi Firmware Loading\n");
    printf("[intel-fw] ========================================\n");
    netdebug_info("[intel-fw] ========================================\n");
    netdebug_info("[intel-fw] Intel WiFi Firmware Loading\n");
    netdebug_info("[intel-fw] ========================================\n");

    if (!parsed || !parsed->valid) {
        printf("[intel-fw] ERROR: Invalid firmware structure!\n");
        netdebug_error("[intel-fw] ERROR: Invalid firmware structure!\n");
        return -1;
    }

    // 🔥 关键修复：在加载固件之前，必须初始化硬件（reset + clocks）
    printf("[intel-fw] Step 0: Hardware initialization\n");
    netdebug_info("[intel-fw] Step 0: Hardware initialization\n");
    intel_debug_pause("Before HW init", mem_base, 0);
    if (intel_hw_init(mem_base) < 0) {
        printf("[intel-fw] ERROR: Hardware initialization failed!\n");
        netdebug_error("[intel-fw] ERROR: Hardware initialization failed!\n");
        return -1;
    }
    intel_debug_pause("After HW init", mem_base, 0);

    intel_fw_image_t *init_img = &parsed->images[IWL_UCODE_INIT];
    intel_fw_image_t *rt_img = &parsed->images[IWL_UCODE_REGULAR];

    if (!init_img->has_inst || !rt_img->has_inst) {
        printf("[intel-fw] ERROR: Missing required firmware sections!\n");
        netdebug_error("[intel-fw] ERROR: Missing required firmware sections!\n");
        return -1;
    }

    printf("[intel-fw] INIT: %d bytes in %d chunks\n",
           init_img->inst.total_size, init_img->inst.num_chunks);
    printf("[intel-fw] RUNTIME: %d bytes in %d chunks\n",
           rt_img->inst.total_size, rt_img->inst.num_chunks);
    netdebug_info("[intel-fw] INIT: %d bytes in %d chunks\n",
           init_img->inst.total_size, init_img->inst.num_chunks);
    netdebug_info("[intel-fw] RUNTIME: %d bytes in %d chunks\n",
           rt_img->inst.total_size, rt_img->inst.num_chunks);

    // 判断是否跳过 INIT
    int skip_init = (init_img->inst.total_size == 0);

    // ========== Step 1: Hardware Initialization ==========

    printf("\n[intel-fw] === Step 1: Hardware Init ===\n");
    netdebug_info("[intel-fw] === Step 1: Hardware Init ===\n");

    // 1.1 确保 MAC 访问权限
    if (intel_hw_grant_mac_access(mem_base) < 0) {
        printf("[intel-fw] ERROR: Failed to get MAC access\n");
        netdebug_error("[intel-fw] ERROR: Failed to get MAC access\n");
        return -1;
    }
    printf("[intel-fw] MAC access granted\n");
    netdebug_info("[intel-fw] MAC access granted\n");

    // 1.2 初始化时钟
    // 🔥 关键修复：使用 read-modify-write，不要直接覆盖！

    // 🔥 诊断：先测试 PRPH 访问是否工作
    printf("[intel-fw] Testing PRPH access...\n");

    // 诊断：检查 HBUS 寄存器（PRPH 访问的底层机制）
    uint32_t hbus_waddr = atheros_reg_read(mem_base, HBUS_TARG_PRPH_WADDR);
    uint32_t hbus_raddr = atheros_reg_read(mem_base, HBUS_TARG_PRPH_RADDR);
    uint32_t hbus_rdat = atheros_reg_read(mem_base, HBUS_TARG_PRPH_RDAT);
    uint32_t hbus_wdat = atheros_reg_read(mem_base, HBUS_TARG_PRPH_WDAT);
    printf("[intel-fw] HBUS registers (before PRPH access):\n");
    printf("[intel-fw]   HBUS_TARG_PRPH_WADDR = 0x%08x\n", hbus_waddr);
    printf("[intel-fw]   HBUS_TARG_PRPH_RADDR = 0x%08x\n", hbus_raddr);
    printf("[intel-fw]   HBUS_TARG_PRPH_RDAT = 0x%08x\n", hbus_rdat);
    printf("[intel-fw]   HBUS_TARG_PRPH_WDAT = 0x%08x\n", hbus_wdat);

    netdebug_debug("[intel-fw] HBUS: WADDR=0x%08x RADDR=0x%08x RDAT=0x%08x WDAT=0x%08x\n",
           hbus_waddr, hbus_raddr, hbus_rdat, hbus_wdat);

    uint32_t test_read = intel_read_prph(mem_base, APMG_CLK_CTRL_REG);
    printf("[intel-fw] APMG_CLK_CTRL_REG initial read: 0x%08x\n", test_read);
    netdebug_debug("[intel-fw] APMG_CLK_CTRL_REG initial: 0x%08x\n", test_read);

    if (test_read == 0xA5A5A5A2) {
        printf("[intel-fw] ERROR: PRPH access not working! All reads return 0xA5A5A5A2\n");
        printf("[intel-fw] This indicates MAC access was not granted!\n");
        netdebug_error("[intel-fw] PRPH access not working! Returns 0xA5A5A5A2\n");

        // 诊断：检查 CSR_GP_CNTRL 的状态
        uint32_t gp_cntrl = atheros_reg_read(mem_base, CSR_GP_CNTRL);
        printf("[intel-fw] CSR_GP_CNTRL = 0x%08x\n", gp_cntrl);
        printf("[intel-fw]   MAC_CLOCK_READY (bit 0): %s\n",
               (gp_cntrl & CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY) ? "YES" : "NO");
        printf("[intel-fw]   MAC_ACCESS_REQ (bit 3): %s\n",
               (gp_cntrl & CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ) ? "YES" : "NO");

        return -1;
    }

    printf("[intel-fw] ✓ PRPH access is working!\n");
    netdebug_info("[intel-fw] PRPH access OK\n");

    uint32_t clk_ctrl = intel_read_prph(mem_base, APMG_CLK_CTRL_REG);
    clk_ctrl |= 0x00000001;
    intel_write_prph(mem_base, APMG_CLK_CTRL_REG, clk_ctrl);
    atheros_delay_us(20);

    // 🔥 关键：APMG_CLK_EN_REG 必须使用 read-modify-write
    uint32_t clk_en = intel_read_prph(mem_base, APMG_CLK_EN_REG);
    clk_en |= 0x00001FFF;
    intel_write_prph(mem_base, APMG_CLK_EN_REG, clk_en);
    atheros_delay_us(20);

    // 验证时钟是否启用
    clk_en = intel_read_prph(mem_base, APMG_CLK_EN_REG);
    printf("[intel-fw] APMG_CLK_EN_REG after write: 0x%08x\n", clk_en);
    netdebug_debug("[intel-fw] APMG_CLK_EN_REG after write: 0x%08x\n", clk_en);
    printf("[intel-fw] Clocks initialized\n");
    netdebug_info("[intel-fw] Clocks initialized\n");

    // 1.3 清除 RF-Kill 位（对应 iwl_enable_rfkill_int）
    // 🔥 关键：Linux iwlwifi 清除 RF-Kill 软件 bit
    atheros_reg_write(mem_base, CSR_UCODE_DRV_GP1_CLR, CSR_UCODE_SW_BIT_RFKILL);
    atheros_reg_write(mem_base, CSR_UCODE_DRV_GP1_CLR, CSR_UCODE_DRV_GP1_BIT_CMD_BLOCKED);
    printf("[intel-fw] RF-Kill bits cleared\n");

    // 1.4 暂时不使能中断 - 等固件加载后再启用
    // Linux iwlwifi 在固件加载后、CSR_RESET=0 前启用中断
    atheros_reg_write(mem_base, CSR_INT, 0xFFFFFFFF);
    atheros_reg_write(mem_base, CSR_INT_MASK, 0x00000000);  // 先禁用
    printf("[intel-fw] Interrupts disabled (will enable after firmware load)\n");
   
    // ========== Step 2: INIT Firmware (if present) ==========

    if (!skip_init) {
        printf("\n[intel-fw] === Step 2: Loading INIT Firmware ===\n");
        netdebug_info("[intel-fw] === Step 2: Loading INIT Firmware ===\n");

        // 2.1 加载 INIT instruction chunks
        printf("[intel-fw] Loading INIT instructions...\n");
        netdebug_info("[intel-fw] Loading INIT instructions...\n");
        for (uint32_t i = 0; i < init_img->inst.num_chunks; i++) {
            intel_fw_chunk_t *chunk = &init_img->inst.chunks[i];
            printf("[intel-fw]   Chunk %d: %d bytes @ 0x%x\n", i, chunk->size, chunk->offset);

#if USE_FH_DMA_FOR_FW_LOADING
            // 🔥 使用 FH DMA 加载（Linux iwlwifi 方式）
            if (intel_pcie_load_section(mem_base, i, chunk->data, chunk->size, chunk->offset) < 0) {
                printf("[intel-fw] ERROR: Failed to load INIT chunk %d\n", i);
                return -1;
            }
#else
            // 使用 BSM DMA 加载（旧方式）
            if (intel_fw_load_via_bsm(mem_base, chunk->data, chunk->size, chunk->offset) < 0) {
                printf("[intel-fw] ERROR: Failed to load INIT chunk %d\n", i);
                return -1;
            }
#endif
        }

        // 🔥 验证：dump SRAM[0x0] 前 16 字节，确认固件已加载
        uint32_t sram_base = mem_base + 0x00000000;  // SRAM 起始地址
        printf("[intel-fw] SRAM[0x0:0x10] after load: ");
        for (int i = 0; i < 4; i++) {  // 4 * 4 bytes = 16 bytes
            uint32_t val = *(volatile uint32_t *)(sram_base + i * 4);
            printf("%08x ", val);
        }
        printf("\n");
        netdebug_dump((void*)sram_base, 16);  // 发送 SRAM dump 到网络

        // 2.2 加载 INIT data chunks
        // 🔥 启用数据段加载，统一使用 FH DMA（带栈保护）
        if (init_img->has_data && 1) {  // 改为 0 来禁用数据段加载
            printf("[intel-fw] Loading INIT data...\n");
            for (uint32_t i = 0; i < init_img->data.num_chunks; i++) {
                intel_fw_chunk_t *chunk = &init_img->data.chunks[i];
                printf("[intel-fw]   Data chunk %d: %d bytes @ 0x%x\n", i, chunk->size, chunk->offset);

                // 🔥 统一使用 FH DMA 加载（带栈保护）
                if (intel_pcie_load_section(mem_base, 100 + i, chunk->data, chunk->size, chunk->offset) < 0) {
                    printf("[intel-fw] ERROR: Failed to load INIT data chunk %d\n", i);
                    return -1;
                }

                // 🔥 关键诊断：每次数据段加载后验证 PRPH 访问
                uint32_t test_prph = intel_read_prph(mem_base, APMG_CLK_CTRL_REG);
                printf("[intel-fw]   PRPH verify after chunk %d: APMG_CLK_CTRL_REG = 0x%08x\n",
                       i, test_prph);
                if (test_prph == 0xA5A5A5A2) {
                    printf("[intel-fw] WARNING: PRPH access lost after data chunk %d!\n", i);
                }
            }
        } else {
            printf("[intel-fw] Skipping INIT data loading (disabled for debugging)\n");
        }

        // 2.3 启动 INIT CPU - 完全按照 Linux iwlwifi 的序列
        printf("[intel-fw] Starting INIT CPU (Linux iwlwifi sequence)...\n");
        netdebug_info("[intel-fw] Starting INIT CPU\n");
        printf("[intel-fw] DEBUG: Reached line 913\n");

        // 🔥 关键诊断：检查 MAC 访问权限
        printf("[intel-fw] DEBUG: About to read CSR_GP_CNTRL at 0x%x\n", CSR_GP_CNTRL);
        uint32_t gp_cntrl_check = atheros_reg_read(mem_base, CSR_GP_CNTRL);
        printf("[intel-fw] DEBUG: Successfully read CSR_GP_CNTRL\n");
        printf("[intel-fw] CSR_GP_CNTRL before PRPH write: 0x%08x\n", gp_cntrl_check);
        printf("[intel-fw]   MAC_CLOCK_READY (bit 0): %s\n",
               (gp_cntrl_check & CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY) ? "YES" : "NO");
        printf("[intel-fw]   MAC_ACCESS_REQ (bit 3): %s\n",
               (gp_cntrl_check & CSR_GP_CNTRL_REG_FLAG_MAC_ACCESS_REQ) ? "YES" : "NO");
        printf("[intel-fw]   INIT_DONE (bit 2): %s\n",
               (gp_cntrl_check & CSR_GP_CNTRL_REG_FLAG_INIT_DONE) ? "YES" : "NO");

        // 🔥 如果 MAC_CLOCK_READY 未置位，尝试重新授予访问权限
        if (!(gp_cntrl_check & CSR_GP_CNTRL_REG_FLAG_MAC_CLOCK_READY)) {
            printf("[intel-fw] WARNING: MAC_CLOCK_READY not set! Retrying MAC access...\n");
            netdebug_warn("[intel-fw] MAC_CLOCK_READY not set! Retrying...\n");
            if (intel_hw_grant_mac_access(mem_base) < 0) {
                printf("[intel-fw] ERROR: Failed to grant MAC access!\n");
                netdebug_error("[intel-fw] Failed to grant MAC access!\n");
            }
        }

        // 🔥 暂时跳过 CPU1_HDR_ADDR 设置，直接测试 CSR_RESET
        printf("[intel-fw] Skipping CPU1_HDR_ADDR setup for testing...\n");

        // 2.3.1 设置 CPU1 header 地址（🔥 关键！告诉 MCU 固件在内存中的位置）
        printf("[intel-fw] Setting CPU1_HDR_ADDR...\n");
        printf("[intel-fw]   PRPH addr: 0x%x\n", LMPM_SECURE_UCODE_LOAD_CPU1_HDR_ADDR);
        printf("[intel-fw]   Value to write: 0x%x\n", LMPM_SECURE_CPU1_HDR_MEM_SPACE);

        intel_debug_pause("Before PRPH write", mem_base, 0);

        // 🔥 诊断：写入前的 HBUS 寄存器状态
        uint32_t hbus_waddr_before = atheros_reg_read(mem_base, HBUS_TARG_PRPH_WADDR);
        uint32_t hbus_wdat_before = atheros_reg_read(mem_base, HBUS_TARG_PRPH_WDAT);
        uint32_t hbus_raddr_before = atheros_reg_read(mem_base, HBUS_TARG_PRPH_RADDR);
        uint32_t hbus_rdat_before = atheros_reg_read(mem_base, HBUS_TARG_PRPH_RDAT);
        printf("[intel-fw]   HBUS_WADDR before: 0x%x\n", hbus_waddr_before);
        printf("[intel-fw]   HBUS_WDAT before: 0x%x\n", hbus_wdat_before);
        printf("[intel-fw]   HBUS_RADDR before: 0x%x\n", hbus_raddr_before);
        printf("[intel-fw]   HBUS_RDAT before: 0x%x\n", hbus_rdat_before);

        // 🔥 测试：验证基本 CSR 寄存器可访问（CSR_GP_CNTRL 是可读写的）
        printf("[intel-fw] Testing basic CSR register access...\n");
        uint32_t gp_cntrl_orig = atheros_reg_read(mem_base, CSR_GP_CNTRL);
        printf("[intel-fw]   CSR_GP_CNTRL original: 0x%08x\n", gp_cntrl_orig);

        // 尝试写入并读回 CSR_GP_CNTRL
        atheros_reg_write(mem_base, CSR_GP_CNTRL, gp_cntrl_orig);
        uint32_t gp_cntrl_verify = atheros_reg_read(mem_base, CSR_GP_CNTRL);
        printf("[intel-fw]   CSR_GP_CNTRL write/read: 0x%08x\n", gp_cntrl_verify);

        if (gp_cntrl_verify == gp_cntrl_orig) {
            printf("[intel-fw] ✓ CSR register access OK\n");
        } else {
            printf("[intel-fw] ✗ CSR register access FAILED!\n");
        }

        // 🔥 关键修复：使用智能 PRPH 写入函数，尝试多种方法和重试机制
        printf("[intel-fw] Attempting PRPH write to CPU1_HDR_ADDR...\n");
        printf("[intel-fw] Target: PRPH[0x%x] = 0x%x\n",
               LMPM_SECURE_UCODE_LOAD_CPU1_HDR_ADDR, LMPM_SECURE_CPU1_HDR_MEM_SPACE);

        int prph_success = 0;

        // 尝试所有方法，直到成功
        for (int method = 0; method < PRPH_METHOD_MAX; method++) {
            intel_debug_pause("PRPH write attempt", mem_base, 0);

            if (intel_write_prph_with_retry(mem_base, LMPM_SECURE_UCODE_LOAD_CPU1_HDR_ADDR,
                                           LMPM_SECURE_CPU1_HDR_MEM_SPACE,
                                           (prph_write_method_t)method, 5) == 0) {
                printf("[intel-fw] ✓✓✓ PRPH write SUCCESS with method %d! ✓✓✓\n", method);
                prph_success = 1;
                break;
            }

            printf("[intel-fw] Method %d failed, trying next...\n", method);
        }

        if (!prph_success) {
            printf("[intel-fw] ✗✗✗ All PRPH write methods FAILED! ✗✗✗\n");
            printf("[intel-fw]\n");
            printf("[intel-fw] ══════════════════════════════════════════════════════\n");
            printf("[intel-fw]  PRPH WRITE FAILED - CONTINUING ANYWAY\n");
            printf("[intel-fw] ══════════════════════════════════════════════════════\n");
            printf("[intel-fw]  Some devices (e.g., 6000 series) boot from SRAM 0x0 by default\n");
            printf("[intel-fw]  Trying to continue without CPU1_HDR_ADDR...\n");
            printf("[intel-fw]  Will check for ALIVE interrupt to see if firmware starts\n");
            printf("[intel-fw] ══════════════════════════════════════════════════════\n");
            printf("[intel-fw]\n");
            netdebug_error("[intel-fw] All PRPH write methods FAILED!\n");
            netdebug_warn("[intel-fw] Continuing anyway, will check for ALIVE...\n");
        } else {
            netdebug_info("[intel-fw] PRPH write SUCCESS!\n");
        }

        // 2.3.2 设置 INIT_DONE flag
        uint32_t gp_cntrl = atheros_reg_read(mem_base, CSR_GP_CNTRL);
        gp_cntrl |= CSR_GP_CNTRL_REG_FLAG_INIT_DONE;
        atheros_reg_write(mem_base, CSR_GP_CNTRL, gp_cntrl);
        printf("[intel-fw] INIT_DONE flag set\n");

        // 🔥 2.3.3 启用中断 - Linux iwlwifi 在 CSR_RESET=0 之前启用中断
        atheros_reg_write(mem_base, CSR_INT, 0xFFFFFFFF);  // 清除所有挂起的中断
        atheros_reg_write(mem_base, CSR_INT_MASK, CSR_INI_SET_MASK);  // 启用必要的中断
        printf("[intel-fw] Interrupts enabled (mask=0x%08x)\n", CSR_INI_SET_MASK);

        // 🔥 关键修复：Linux iwlwifi 不使用 BSM_START！
        // 它只是简单地将 CSR_RESET 写为 0 来释放 CPU
        // Intel 6205 及以后设备直接从 SRAM 0x0 启动

        printf("[intel-fw] Releasing CPU reset (CSR_RESET = 0)...\n");
        atheros_reg_write(mem_base, CSR_RESET, 0);

        // 🔥 诊断：检查关键 CSR 和 PRPH 寄存器
        printf("[intel-fw] === Diagnostic Register Dump ===\n");
        uint32_t gp_cntrl2 = atheros_reg_read(mem_base, CSR_GP_CNTRL);
        uint32_t reset = atheros_reg_read(mem_base, CSR_RESET);
        uint32_t int_mask = atheros_reg_read(mem_base, CSR_INT_MASK);
        printf("[intel-fw] CSR_GP_CNTRL  = 0x%08x\n", gp_cntrl2);
        printf("[intel-fw] CSR_RESET     = 0x%08x\n", reset);
        printf("[intel-fw] CSR_INT_MASK  = 0x%08x\n", int_mask);

        // 检查一些关键 PRPH 寄存器
        uint32_t clk_ctrl = intel_read_prph(mem_base, APMG_CLK_CTRL_REG);
        uint32_t clk_en = intel_read_prph(mem_base, APMG_CLK_EN_REG);
        uint32_t cpu1_hdr = intel_read_prph(mem_base, LMPM_SECURE_UCODE_LOAD_CPU1_HDR_ADDR);
        printf("[intel-fw] APMG_CLK_CTRL = 0x%08x\n", clk_ctrl);
        printf("[intel-fw] APMG_CLK_EN   = 0x%08x\n", clk_en);
        printf("[intel-fw] CPU1_HDR_ADDR = 0x%08x\n", cpu1_hdr);

        intel_debug_pause("Before waiting for ALIVE (FINAL)", mem_base, 1);  // 🔥 is_final=1，显示完整摘要

        // 🔥 验证：dump SRAM 0x2000 (header 位置)
        uint32_t sram_header = mem_base + 0x2000;  // SRAM header 位置
        printf("[intel-fw] SRAM[0x2000:0x2010] (header area): ");
        for (int i = 0; i < 4; i++) {  // 4 * 4 bytes = 16 bytes
            uint32_t val = *(volatile uint32_t *)(sram_header + i * 4);
            printf("%08x ", val);
        }
        printf("\n");
        // 🔥 移除了 if(1){return;} 调试代码，允许继续执行完整的 INIT 启动流程
        atheros_delay_us(100);

        // 2.4 等待 INIT ALIVE
        printf("[intel-fw] Waiting for INIT ALIVE...\n");
        netdebug_info("[intel-fw] Waiting for INIT ALIVE...\n");
        int init_alive = 0;
        for (int timeout = 0; timeout < 12000; timeout++) {
            uint32_t int_status = atheros_reg_read(mem_base, CSR_INT);
            if (int_status & CSR_INT_BIT_ALIVE) {
                atheros_reg_write(mem_base, CSR_INT, CSR_INT_BIT_ALIVE);
                init_alive = 1;
                printf("[intel-fw] ✓ INIT ALIVE received\n");
                netdebug_info("[intel-fw] INIT ALIVE received! Firmware started!\n");
                break;
            }
            for (volatile int i = 0; i < 10000; i++) {
                __asm__ volatile("nop");
            }
            if (timeout % 3000 == 0 && timeout > 0) {
                //printf("[intel-fw] Still waiting... (%d ms)\n", timeout / 10);
            }
        }

        // 最终 SRAM dump
        printf("[intel-fw] SRAM[0x0:0x10] at end: ");
        printf("[intel-fw] CPU1_HDR_ADDR = 0x%08x\n", cpu1_hdr);
        for (int i = 0; i < 4; i++) {
            uint32_t val = *(volatile uint32_t *)(sram_base + i * 4);
            printf("%08x ", val);
        }
        printf("\n");

        // 诊断信息
        if (!init_alive) {
            SET_COLOR_RED();
            printf("[intel-fw] ✗ ERROR: INIT firmware never came alive!\n");
            SET_COLOR_WHITE();
            netdebug_error("[intel-fw] INIT firmware never came alive!\n");

            uint32_t csr_int = atheros_reg_read(mem_base, CSR_INT);
            uint32_t csr_int_mask = atheros_reg_read(mem_base, CSR_INT_MASK);
            uint32_t csr_reset = atheros_reg_read(mem_base, CSR_RESET);
            uint32_t csr_bsm = atheros_reg_read(mem_base, CSR_BSM_WR_CTRL_REG);

            SET_COLOR_RED();
            printf("[intel-fw] ========== DIAGNOSTICS ==========\n");
            SET_COLOR_WHITE();
            printf("[intel-fw] CSR_INT      = 0x%08x\n", csr_int);
            printf("[intel-fw] CSR_INT_MASK = 0x%08x\n", csr_int_mask);
            printf("[intel-fw] CSR_RESET    = 0x%08x\n", csr_reset);
            printf("[intel-fw] CSR_BSM_CTRL = 0x%08x\n", csr_bsm);

            show_snapshot_summary();
            return -1;
        }

        // 2.6 停止 INIT firmware
        printf("[intel-fw] Stopping INIT firmware...\n");
        atheros_reg_write(mem_base, CSR_RESET, CSR_RESET_REG_FLAG_SW_RESET);
        for (volatile int i = 0; i < 10000; i++) {
            __asm__ volatile("nop");
        }
        atheros_reg_write(mem_base, CSR_RESET, 0);
    } else {
        printf("\n[intel-fw] === Skipping INIT (no INIT firmware) ===\n");
    }

    // ========== Step 3: RUNTIME Firmware ==========

runtime_phase:
    printf("\n[intel-fw] === Step 3: Loading RUNTIME Firmware ===\n");
    netdebug_info("[intel-fw] === Step 3: Loading RUNTIME Firmware ===\n");

    uint32_t rt_entry = 0x00000000;  // Intel firmware entry point is always 0x0
    uint32_t rt_data_offset = (rt_img->has_data && rt_img->data.num_chunks > 0) ?
                               rt_img->data.chunks[0].offset : rt_entry;

    // 3.1 加载 RUNTIME instruction chunks
    printf("[intel-fw] Loading RUNTIME instructions...\n");
    for (uint32_t i = 0; i < rt_img->inst.num_chunks; i++) {
        intel_fw_chunk_t *chunk = &rt_img->inst.chunks[i];
        printf("[intel-fw]   Chunk %d: %d bytes @ 0x%x\n", i, chunk->size, chunk->offset);

        // 🔥 统一使用 FH DMA 加载（带栈保护）
        if (intel_pcie_load_section(mem_base, 200 + i, chunk->data, chunk->size, chunk->offset) < 0) {
            printf("[intel-fw] ERROR: Failed to load RUNTIME chunk %d\n", i);
            return -1;
        }
    }

    // 3.2 加载 RUNTIME data chunks
    if (rt_img->has_data) {
        printf("[intel-fw] Loading RUNTIME data...\n");
        for (uint32_t i = 0; i < rt_img->data.num_chunks; i++) {
            intel_fw_chunk_t *chunk = &rt_img->data.chunks[i];
            printf("[intel-fw]   Data chunk %d: %d bytes @ 0x%x\n", i, chunk->size, chunk->offset);

            // 🔥 统一使用 FH DMA 加载（带栈保护）
            if (intel_pcie_load_section(mem_base, 300 + i, chunk->data, chunk->size, chunk->offset) < 0) {
                printf("[intel-fw] ERROR: Failed to load RUNTIME data chunk %d\n", i);
                return -1;
            }
        }
    }

    // 3.3 启动 RUNTIME firmware
    printf("[intel-fw] Kicking RUNTIME (entry=0x%x, data=0x%x)...\n", rt_entry, rt_data_offset);

    atheros_reg_write(mem_base, CSR_BSM_WR_CTRL_REG, 0);
    atheros_reg_write(mem_base, CSR_BSM_DRAM_INST_PTR, rt_entry);
    atheros_reg_write(mem_base, CSR_BSM_DRAM_DATA_PTR, rt_data_offset);
    atheros_reg_write(mem_base, CSR_BSM_WR_CTRL_REG, CSR_BSM_WR_CTRL_REG_BIT_START);

    // 3.4 等待 RUNTIME ALIVE
    printf("[intel-fw] Waiting for RUNTIME ALIVE...\n");
    for (int timeout = 0; timeout < 3000; timeout++) {
        uint32_t int_status = atheros_reg_read(mem_base, CSR_INT);
        if (int_status & CSR_INT_BIT_ALIVE) {
            atheros_reg_write(mem_base, CSR_INT, CSR_INT_BIT_ALIVE);
            printf("[intel-fw] ✓✓✓ RUNTIME ALIVE! ✓✓✓\n");
            printf("[intel-fw] ========================================\n");
            printf("[intel-fw] Firmware Loading SUCCESSFUL\n");
            printf("[intel-fw] ========================================\n");
            return 0;
        }
        for (volatile int i = 0; i < 10000; i++) {
            __asm__ volatile("nop");
        }
        if (timeout % 500 == 0 && timeout > 0) {
            printf("[intel-fw] Still waiting... (%d ms)\n", timeout / 10);
        }
    }

    printf("[intel-fw] ⚠ Timeout waiting for RUNTIME ALIVE\n");
    printf("[intel-fw] Continuing anyway (firmware may still work)\n");
    printf("[intel-fw] ========================================\n");
    return 0;
}

/**
 * @brief Legacy firmware start interface（向后兼容）
 *
 * @param mem_base PCI 内存基地址
 * @param fw_addr Firmware 物理地址
 * @param fw_size Firmware 大小
 *
 * @return 0 = 成功, -1 = 失败
 */
int intel_fw_start(uint32_t mem_base, uint32_t fw_addr, uint32_t fw_size) {
    // 映射 firmware 到虚拟地址
    extern uint32_t map_highmem_physical(uint32_t phys, uint32_t size, uint32_t flags);
    uint32_t fw_virt = map_highmem_physical(fw_addr, (fw_size + 4095) / 4096 * 4096, 0x10);
    if (!fw_virt) {
        // printf("[intel-fw] Failed to map firmware\n");
        return -1;
    }

    // printf("[intel-fw] Firmware mapped: phys=0x%x virt=0x%x\n", fw_addr, fw_virt);

    // 解析固件文件
    struct intel_fw_parsed parsed;
    if (intel_fw_parse((const uint8_t *)fw_virt, fw_size, &parsed) < 0) {
        // printf("[intel-fw] Failed to parse firmware\n");
        return -1;
    }

    // 使用解析后的固件启动
    return intel_fw_start_parsed(mem_base, &parsed);
}

/**
 * @brief 动态固件加载接口（从内存指针加载）
 *
 * 这个函数允许从任意内存位置（包括文件系统缓冲区）加载固件
 *
 * @param mem_base PCI 内存基地址
 * @param fw_data 固件数据指针（虚拟地址）
 * @param fw_size 固件大小
 *
 * @return 0 = 成功, -1 = 失败
 */
int intel_fw_load_from_buffer(uint32_t mem_base, const uint8_t *fw_data, uint32_t fw_size) {
    // printf("[intel-fw] Loading firmware from buffer: virt=0x%x, size=%d\n", (uint32_t)fw_data, fw_size);
    if (!fw_data || fw_size == 0) {
        // printf("[intel-fw] ERROR: Invalid firmware buffer\n");
        return -1;
    }

    // 解析固件文件
    struct intel_fw_parsed parsed;
    if (intel_fw_parse(fw_data, fw_size, &parsed) < 0) {
        // printf("[intel-fw] Failed to parse firmware\n");
        return -1;
    }

    // 使用解析后的固件启动
    return intel_fw_start_parsed(mem_base, &parsed);
}
