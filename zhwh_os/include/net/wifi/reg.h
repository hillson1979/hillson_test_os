/**
 * @file reg.h
 * @brief Atheros WiFi 寄存器操作接口
 */

#ifndef REG_H
#define REG_H

#include "types.h"

// 寄存器读写操作
static inline uint32_t atheros_reg_read(uint32_t mem_base, uint32_t offset) {
    return *(volatile uint32_t *)(mem_base + offset);
}

static inline void atheros_reg_write(uint32_t mem_base, uint32_t offset, uint32_t value) {
    // 🔥 关键修复：PCIe MMIO 写入需要内存屏障确保写入顺序
    // 使用 Intel 的内存屏障指令确保写入完成后才继续
    *(volatile uint32_t *)(mem_base + offset) = value;

    // 🔥 内存屏障：确保写入完成后再继续
    // 对于 PCIe 设备，这是关键！否则写入可能被缓存或重排序
    __asm__ volatile("sfence" ::: "memory");  // Store Fence

    // 🔥 可选：读回强制刷新（某些硬件需要）
    // volatile uint32_t dummy = *(volatile uint32_t *)(mem_base + offset);
    // (void)dummy;
}

// 位操作
static inline uint32_t atheros_reg_set_bits(uint32_t mem_base, uint32_t offset, uint32_t bits) {
    uint32_t val = atheros_reg_read(mem_base, offset);
    val |= bits;
    atheros_reg_write(mem_base, offset, val);
    return val;
}

static inline uint32_t atheros_reg_clear_bits(uint32_t mem_base, uint32_t offset, uint32_t bits) {
    uint32_t val = atheros_reg_read(mem_base, offset);
    val &= ~bits;
    atheros_reg_write(mem_base, offset, val);
    return val;
}

static inline uint32_t atheros_reg_modify_bits(uint32_t mem_base, uint32_t offset,
                                               uint32_t mask, uint32_t value) {
    uint32_t val = atheros_reg_read(mem_base, offset);
    val = (val & ~mask) | (value & mask);
    atheros_reg_write(mem_base, offset, val);
    return val;
}

// 等待位设置/清除
static inline int atheros_reg_wait_for_bits(uint32_t mem_base, uint32_t offset,
                                           uint32_t bits, int set, int timeout_us) {
    volatile int i;
    for (i = 0; i < timeout_us; i++) {
        uint32_t val = atheros_reg_read(mem_base, offset);
        if (set ? (val & bits) : !(val & bits)) {
            return 0;
        }
        // 延迟 1 微秒
        __asm__ volatile("nop");
    }
    return -1;  // 超时
}

// 寄存器块读写
void atheros_reg_read_block(uint32_t mem_base, uint32_t offset,
                           void *buffer, uint32_t len);
void atheros_reg_write_block(uint32_t mem_base, uint32_t offset,
                            const void *buffer, uint32_t len);

// EEPROM 操作
int atheros_eeprom_read(uint32_t mem_base, uint32_t offset, void *data, uint32_t len);
int atheros_eeprom_write(uint32_t mem_base, uint32_t offset, const void *data, uint32_t len);

#endif // REG_H
