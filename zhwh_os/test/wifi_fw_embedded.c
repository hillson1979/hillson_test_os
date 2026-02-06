/**
 * @file wifi_fw_embedded.c
 * @brief 嵌入式固件加载器（固件直接编译进用户程序）
 *
 * 这个版本将真实固件编译到用户程序中，避免文件系统依赖
 */

#include "libuser.h"

// 嵌入真实固件（从 iwlwifi-6000g2a-6-real.c 复制）
extern const uint8_t iwlwifi_6000g2a_fw[];
extern const uint32_t IWLWIFI_6000G2A_FW_SIZE;

// 打印字符串
void print_str(const char *s) {
    while (*s) {
        sys_putchar(*s);
        s++;
    }
}

void print_dec(uint32_t value) {
    if (value == 0) {
        sys_putchar('0');
        return;
    }

    char buf[16];
    int pos = 0;
    while (value > 0) {
        buf[pos++] = '0' + (value % 10);
        value /= 10;
    }

    while (pos > 0) {
        sys_putchar(buf[--pos]);
    }
}

int main() {
    print_str("\n======== WiFi Firmware Loader (Embedded) ========\n\n");

    print_str("Firmware size: ");
    print_dec(IWLWIFI_6000G2A_FW_SIZE);
    print_str(" bytes (");
    print_dec(IWLWIFI_6000G2A_FW_SIZE / 1024);
    print_str(" KB)\n");

    print_str("Firmware address: 0x");
    // 打印地址（简化版）
    print_str("\n\n");

    print_str("Status: Embedded firmware ready\n");
    print_str("\nNext steps:\n");
    print_str("  1. Implement kernel firmware buffer allocation\n");
    print_str("  2. Copy firmware from userspace to kernel\n");
    print_str("  3. Parse TLV format and load INIT/RUNTIME\n");
    print_str("  4. Enable DMA protection before loading\n");
    print_str("\n=====================================\n");

    return 0;
}

void _start() {
    int ret = main();
    sys_exit(ret);
}
