/**
 * @file wifi_fw_loader.c
 * @brief WiFi 固件加载工具
 *
 * 用法：
 *   wifi_fw_loader.elf               # 使用默认路径
 *   wifi_fw_loader.elf /path/to/fw   # 指定固件路径
 *
 * 这个程序会：
 * 1. 打开固件文件
 * 2. 读取固件内容
 * 3. 通过 syscall 传递给内核
 * 4. 内核解析 TLV 格式并加载到设备
 */

#include "libuser.h"

// 打印字符串
void print_str(const char *s) {
    while (*s) {
        sys_putchar(*s);
        s++;
    }
}

// 打印十六进制数
void print_hex(uint32_t value) {
    char hex_chars[] = "0123456789ABCDEF";
    print_str("0x");
    for (int i = 28; i >= 0; i -= 4) {
        sys_putchar(hex_chars[(value >> i) & 0xF]);
    }
}

// 打印十进制数
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

// 简单的字符串比较
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

// 简单的字符串长度
int strlen(const char *s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

// 简单的 memcpy
void* memcpy(void *dest, const void *src, int n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

int main(int argc, char **argv) {
    const char *fw_path = "/lib/firmware/iwlwifi-6000g2a-6.ucode";  // 默认路径

    print_str("\n======== WiFi Firmware Loader ========\n\n");

    // 如果提供了参数，使用参数作为路径
    if (argc > 1) {
        fw_path = argv[1];
    }

    print_str("Firmware path: ");
    print_str(fw_path);
    print_str("\n\n");

    // 打开固件文件
    print_str("Opening firmware file...\n");
    int fd = open(fw_path, O_RDONLY);
    if (fd < 0) {
        print_str("ERROR: Failed to open firmware file\n");
        print_str("File: ");
        print_str(fw_path);
        print_str("\n");
        return 1;
    }

    print_str("File opened: fd=");
    print_dec(fd);
    print_str("\n");

    // 调用 WiFi 固件加载 syscall
    print_str("Loading firmware to kernel...\n");
    int ret = wifi_load_firmware_from_fd(fd);

    if (ret == 0) {
        print_str("\n✓ Firmware loaded successfully!\n");
        print_str("Check kernel logs for INIT/RUNTIME ALIVE status\n");
    } else {
        print_str("\n✗ Firmware loading failed: ");
        print_dec(ret);
        print_str("\n");
        print_str("Possible reasons:\n");
        print_str("  - Firmware file not found\n");
        print_str("  - Invalid firmware format\n");
        print_str("  - WiFi device not initialized\n");
        print_str("  - DMA protection not enabled\n");
    }

    // 关闭文件
    close(fd);

    print_str("\n====================================\n");
    return ret;
}

void _start() {
    int ret = main();

    print_str("Exit code: ");
    print_dec(ret);
    print_str("\n");

    sys_exit(ret);
}
