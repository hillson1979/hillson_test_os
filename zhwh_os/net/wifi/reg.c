/**
 * @file reg.c
 * @brief Atheros WiFi 寄存器操作实现
 */

#include "net/wifi/reg.h"
#include "net/wifi/atheros.h"
#include "string.h"
#include "printf.h"

/**
 * @brief 寄存器块读取
 */
void atheros_reg_read_block(uint32_t mem_base, uint32_t offset,
                           void *buffer, uint32_t len) {
    uint32_t *buf = (uint32_t *)buffer;
    volatile uint32_t *reg = (volatile uint32_t *)(mem_base + offset);

    for (uint32_t i = 0; i < len / 4; i++) {
        buf[i] = reg[i];
    }
}

/**
 * @brief 寄存器块写入
 */
void atheros_reg_write_block(uint32_t mem_base, uint32_t offset,
                            const void *buffer, uint32_t len) {
    const uint32_t *buf = (const uint32_t *)buffer;
    volatile uint32_t *reg = (volatile uint32_t *)(mem_base + offset);

    for (uint32_t i = 0; i < len / 4; i++) {
        reg[i] = buf[i];
    }
}

/**
 * @brief EEPROM 读取
 */
int atheros_eeprom_read(uint32_t mem_base, uint32_t offset, void *data, uint32_t len) {
    // 检查 EEPROM 是否就绪
    uint32_t ctrl = atheros_reg_read(mem_base, ATHEROS_REG_EEPROM_CTRL);
    if (ctrl & 0x01) {  // 忙标志
        printf("[eeprom] EEPROM busy\n");
        return -1;
    }

    // 设置读取地址
    atheros_reg_write(mem_base, ATHEROS_REG_EEPROM_CTRL, offset & 0x1FF);

    // 等待读取完成
    if (atheros_reg_wait_for_bits(mem_base, ATHEROS_REG_EEPROM_CTRL, 0x02, 1, 10000) < 0) {
        printf("[eeprom] Read timeout\n");
        return -1;
    }

    // 读取数据
    uint8_t *buf = (uint8_t *)data;
    for (uint32_t i = 0; i < len; i++) {
        // EEPROM 数据从数据寄存器读取
        uint32_t data_reg = atheros_reg_read(mem_base, ATHEROS_REG_EEPROM_DATA);
        buf[i] = (data_reg >> ((offset + i) % 4 * 8)) & 0xFF;
    }

    return 0;
}

/**
 * @brief EEPROM 写入
 */
int atheros_eeprom_write(uint32_t mem_base, uint32_t offset, const void *data, uint32_t len) {
    // EEPROM 写入需要特殊时序，这里简化处理
    printf("[eeprom] EEPROM write not implemented (offset=%x, len=%d)\n", offset, len);

    // 在真实实现中，需要：
    // 1. 检查写使能
    // 2. 发送写入命令
    // 3. 等待写入周期完成（通常几毫秒）
    // 4. 验证写入数据

    return -1;
}
