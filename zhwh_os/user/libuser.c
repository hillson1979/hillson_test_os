// libuser.c - 用户库实现（编译成静态库 libuser.a）
#include "libuser.h"
#include "stddef.h"  // for size_t

// 定义 NULL
#ifndef NULL
#define NULL ((void *)0)
#endif

// ⚠️ 所有函数都不再使用 __attribute__((section(...)))
//    因为整个 libuser.a 会在链接时放在主程序之后

// 简单的 strlen 实现
size_t strlen(const char *s) {
    const char *p = s;
    while (*p) p++;
    return p - s;
}

// 简单的 atoi 实现
int atoi(const char *str) {
    int result = 0;
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    return result;
}

// 优化的 memcpy 实现
void *memcpy(void *dst, const void *src, size_t n) {
    char *d = (char*)dst;
    const char *s = (const char*)src;

    // 如果长度小于 16，直接字节拷贝
    if (n < 16) {
        while (n--) *d++ = *s++;
        return dst;
    }

    // 对齐到 4 字节边界
    while (((uint32_t)d & 3) && n) {
        *d++ = *s++;
        n--;
    }

    // 32 位拷贝
    uint32_t *dw = (uint32_t*)d;
    const uint32_t *sw = (const uint32_t*)s;
    while (n >= 4) {
        *dw++ = *sw++;
        n -= 4;
    }

    // 剩余字节
    d = (char*)dw;
    s = (const char*)sw;
    while (n--) *d++ = *s++;

    return dst;
}

// 简单的 memset 实现
void *memset(void *s, int c, size_t n) {
    char *p = (char*)s;
    while (n--) *p++ = c;
    return s;
}

// 字符串比较
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

// 字符串前缀比较
int strncmp(const char *s1, const char *s2, size_t n) {
    while (n > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

// 内存比较
int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

// 字符串拷贝
char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++) != '\0');
    return dest;
}

// 简单的数字转字符串
static char *itoa(int num, char *str, int base) {
    int i = 0;
    int is_negative = 0;

    // 处理 0
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    // 处理负数
    if (num < 0 && base == 10) {
        is_negative = 1;
        num = -num;
    }

    // 转换数字
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }

    // 添加负号
    if (is_negative) {
        str[i++] = '-';
    }

    // ⚠️⚠️⚠️ 关键修复：先设置终止符，再反转！
    str[i] = '\0';

    // 反转字符串
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char tmp = str[start];
        str[start] = str[end];
        str[end] = tmp;
        start++;
        end--;
    }

    return str;
}

// va_list 支持
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

// printf 实现
int printf(const char *fmt, ...) {
    char buf[1024];
    int buf_idx = 0;

    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            if (*fmt == '\0') break;

            if (*fmt == 'd' || *fmt == 'u') {
                // 整数
                int num = va_arg(args, int);
                char num_str[32];
                itoa(num, num_str, 10);
                int len = strlen(num_str);
                memcpy(&buf[buf_idx], num_str, len);
                buf_idx += len;
            } else if (*fmt == 'x' || *fmt == 'X') {
                // 十六进制
                int num = va_arg(args, int);
                char num_str[32];
                itoa(num, num_str, 16);
                int len = strlen(num_str);
                memcpy(&buf[buf_idx], num_str, len);
                buf_idx += len;
            } else if (*fmt == 's') {
                // 字符串
                char *str = va_arg(args, char*);
                int len = strlen(str);
                memcpy(&buf[buf_idx], str, len);
                buf_idx += len;
            } else if (*fmt == 'c') {
                // 字符
                char c = (char)va_arg(args, int);
                buf[buf_idx++] = c;
            } else if (*fmt == '%') {
                // %% 输出 %
                buf[buf_idx++] = '%';
            }
            fmt++;
        } else {
            buf[buf_idx++] = *fmt++;
        }

        // 防止缓冲区溢出
        if (buf_idx >= 1010) break;
    }

    va_end(args);

    buf[buf_idx] = '\0';

    // 使用 write 系统调用输出
    write(1, buf, buf_idx);

    return buf_idx;
}

// 系统调用号定义
#define SYS_WRITE 11
#define SYS_FORK 12
#define SYS_EXIT 2
#define SYS_YIELD 3
#define SYS_OPEN 20
#define SYS_CLOSE 21
#define SYS_READ 22
#define SYS_LSEEK 23
#define SYS_NET_PING 30
#define SYS_NET_IFCONFIG 31
#define SYS_WIFI_SCAN 32
#define SYS_WIFI_CONNECT 33
#define SYS_WIFI_DISCONNECT 34
#define SYS_WIFI_STATUS 35
#define SYS_WIFI_INIT 36
#define SYS_WIFI_FW_BEGIN 37
#define SYS_WIFI_FW_CHUNK 38
#define SYS_WIFI_FW_END 39
#define SYS_EXECV 41

// write 系统调用
int write(int fd, const char *buf, int len) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_WRITE), "b"(fd), "c"(buf), "d"(len)
        : "memory", "cc"
    );
    return ret;
}

// fork 系统调用
int fork(void) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_FORK)
        : "memory", "cc"
    );
    return ret;
}

// exit 系统调用
void exit(int code) {
    __asm__ volatile (
        "int $0x80"
        :
        : "a"(SYS_EXIT), "b"(code)
        : "memory", "cc"
    );
    // 永远不会到达这里
    while (1) {
        __asm__ volatile("hlt");
    }
}

// yield - 让出CPU
void yield(void) {
    __asm__ volatile (
        "int $0x80"
        :
        : "a"(SYS_YIELD)
        : "memory", "cc"
    );
}

// 文件系统系统调用
int open(const char *pathname, int flags) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_OPEN), "b"(pathname), "c"(flags)
        : "memory", "cc"
    );
    return ret;
}

int close(int fd) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_CLOSE), "b"(fd)
        : "memory", "cc"
    );
    return ret;
}

int read(int fd, char *buf, int len) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_READ), "b"(fd), "c"(buf), "d"(len)
        : "memory", "cc"
    );
    return ret;
}

int lseek(int fd, int offset, int whence) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_LSEEK), "b"(fd), "c"(offset), "d"(whence)
        : "memory", "cc"
    );
    return ret;
}

// getcwd 系统调用
extern int getcwd(char *buf, int size);

// net_ping - 发送 ping（可选指定设备）
int net_ping(const char *ip_addr) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_NET_PING), "b"(ip_addr), "c"(0)
        : "memory", "cc"
    );
    return ret;
}

// net_ping_dev - 发送 ping（指定设备）
int net_ping_dev(const char *ip_addr, const char *dev_name) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_NET_PING), "b"(ip_addr), "c"(dev_name)
        : "memory", "cc"
    );
    return ret;
}

int net_ifconfig(void) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_NET_IFCONFIG)
        : "memory", "cc"
    );
    return ret;
}

// 🔥 lspci - 直接使用内联汇编实现系统调用
int lspci(void) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(42)  // SYS_LSPCI = 42
        : "memory", "cc"
    );
    return ret;
}

// 🔥 网卡初始化系统调用
int rtl8139_init_user(void) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(43)  // SYS_NET_INIT_RTL8139 = 43
        : "memory", "cc"
    );
    return ret;
}

// 🔥 初始化 E1000 网卡（用户态包装）
int e1000_init_user(const char *dev_name) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(44),        // SYS_NET_INIT_E1000 = 44
          "b"(dev_name)  // 设备名称
        : "memory", "cc"
    );
    return ret;
}

// 🔥 UDP 发送系统调用
int net_send_udp(const char *ip, int port, const char *data, int len) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(45),  // SYS_NET_SEND_UDP = 45
          "b"(ip),
          "c"(port),
          "d"(data),
          "S"(len)
        : "memory", "cc"
    );
    return ret;
}

// 🔥 设置当前使用的网络设备
int net_set_device(const char *dev_name) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(46),  // SYS_NET_SET_DEVICE = 46
          "b"(dev_name)
        : "memory", "cc"
    );
    return ret;
}

// 🔥 轮询RX系统调用
int net_poll_rx(void) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(47)  // SYS_NET_POLL_RX = 47
        : "memory", "cc"
    );
    return ret;
}

// 🔥 转储寄存器状态系统调用
int net_dump_regs(const char *dev_name) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(48),        // SYS_NET_DUMP_REGS = 48
          "b"(dev_name)  // 🔥 设备名称
        : "memory", "cc"
    );
    return ret;
}

// 🔥 ARP 命令系统调用
int net_arp(const char *dev_name, int scan) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(49),        // SYS_NET_ARP = 49
          "b"(dev_name),  // 🔥 设备名称
          "c"(scan)       // scan 参数
        : "memory", "cc"
    );
    return ret;
}

// 🔥 转储 RX 寄存器（详细）系统调用
int net_dump_rx_regs(const char *dev_name) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(50),        // SYS_NET_DUMP_RX_REGS = 50
          "b"(dev_name)  // 🔥 设备名称
        : "memory", "cc"
    );
    return ret;
}

// 🔥 启动网络接口系统调用
int net_ifup(const char *dev_name) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(51),         // SYS_NET_IFUP = 51
          "b"(dev_name)    // 设备名称
        : "memory", "cc"
    );
    return ret;
}

// 🔥 绑定 UDP 端口系统调用
int net_bind_udp(int port) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(52),         // SYS_NET_BIND = 52
          "b"(port)        // 端口号
        : "memory", "cc"
    );
    return ret;
}

// 🔥 接收 UDP 数据系统调用
int net_recv_udp(char *buf, int len) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(53),         // SYS_NET_RECV_UDP = 53
          "b"(buf),        // 缓冲区
          "c"(len)         // 最大长度
        : "memory", "cc"
    );
    return ret;
}

// 🔥 Raw dump RX descriptor (暂时注释掉)
// int net_raw_dump_rx_desc(void) {
//     int ret;
//     __asm__ volatile (
//         "int $0x80"
//         : "=a"(ret)
//         : "a"(52)         // SYS_NET_RAW_DUMP_RX_DESC = 52
//         : "memory", "cc"
//     );
//     return ret;
// }

// WiFi 系统调用包装函数
int wifi_init(void) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_WIFI_INIT)
        : "memory", "cc"
    );
    return ret;
}

int wifi_scan(void) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_WIFI_SCAN)
        : "memory", "cc"
    );
    return ret;
}

int wifi_connect(const char *ssid, const char *password) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_WIFI_CONNECT), "b"(ssid), "c"(password)
        : "memory", "cc"
    );
    return ret;
}

int wifi_disconnect(void) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_WIFI_DISCONNECT)
        : "memory", "cc"
    );
    return ret;
}

void wifi_status(void) {
    __asm__ volatile (
        "int $0x80"
        :
        : "a"(SYS_WIFI_STATUS)
        : "memory", "cc"
    );
}

// WiFi 固件加载（分块安全版本）
int wifi_fw_load_begin(uint32_t size) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_WIFI_FW_BEGIN), "b"(size)
        : "memory", "cc"
    );
    return ret;
}

int wifi_fw_load_chunk(const void *ptr, uint32_t len, uint32_t offset) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_WIFI_FW_CHUNK), "b"(ptr), "c"(len), "d"(offset)
        : "memory", "cc"
    );
    return ret;
}

int wifi_fw_load_end(void) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_WIFI_FW_END)
        : "memory", "cc"
    );
    return ret;
}

// 便捷函数：一次性加载整个固件（内部使用分块）
int wifi_fw_load(const uint8_t *fw, uint32_t size) {
    if (size == 0 || size > FW_MAX_SIZE)
        return -1;

    // 1️⃣ 通知内核开始
    if (wifi_fw_load_begin(size) < 0)
        return -1;

    // 2️⃣ 分块传输
    for (uint32_t off = 0; off < size; off += FW_CHUNK_SIZE) {
        uint32_t len = size - off;
        if (len > FW_CHUNK_SIZE)
            len = FW_CHUNK_SIZE;

        int ret = wifi_fw_load_chunk(fw + off, len, off);
        if (ret < 0)
            return -1;
    }

    // 3️⃣ 通知结束 + 校验
    return wifi_fw_load_end();
}

// 进程控制系统调用包装函数
int execv(const char *path, char *const argv[]) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_EXECV), "b"(path), "c"(argv)
        : "memory", "cc"
    );
    return ret;
}

// 🔥 MSI 测试系统调用包装
int msi_test(void) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(60)  // SYS_MSI_TEST = 60
        : "memory", "cc"
    );
    return ret;
}

// 🔥 E1000 硬件 loopback 测试（轮询版本）
int e1000_loopback_test(void) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(61)  // SYS_NET_LOOPBACK_TEST = 61
        : "memory", "cc"
    );
    return ret;
}

// 🔥 E1000 硬件 loopback 测试（中断版本）
int e1000_loopback_test_interrupt(void) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(62)  // SYS_NET_LOOPBACK_TEST_INT = 62
        : "memory", "cc"
    );
    return ret;
}

// 🔥 GUI 系统调用实现

/**
 * @brief 获取帧缓冲区信息
 */
int gui_get_fb_info(fb_info_t *info) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(70), "b"(info)  // SYS_GUI_FB_INFO = 70
        : "memory", "cc"
    );
    return ret;
}

/**
 * @brief 位图传输到帧缓冲区
 */
int gui_fb_blit(int x, int y, int width, int height, const void *data) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(71), "b"(x), "c"(y), "d"(width), "S"(height), "D"(data)  // SYS_GUI_FB_BLIT = 71
        : "memory", "cc"
    );
    return ret;
}

/**
 * @brief 读取输入事件
 */
int gui_read_input(input_event_t *event) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(72), "b"(event), "c"(0)  // SYS_GUI_INPUT_READ = 72
        : "memory", "cc"
    );
    return ret;
}

/**
 * @brief 轮询 USB 鼠标事件
 */
int usb_mouse_poll(void *report) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(73), "b"(report)  // SYS_USB_MOUSE_POLL = 73
        : "memory", "cc"
    );
    return ret;
}

int usb_mouse_info(uint8_t *ep, uint8_t *maxpkt, uint8_t *interval) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(76), "b"(0), "c"(0), "d"(0)
        : "memory", "cc"
    );
    if (ep) *ep = (uint8_t)(ret & 0xFF);
    if (maxpkt) *maxpkt = (uint8_t)((ret>>8) & 0xFF);
    if (interval) *interval = (uint8_t)((ret>>16) & 0xFF);
    return ret;
}

