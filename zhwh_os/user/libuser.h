// libuser.h - 用户库头文件
#ifndef LIBUSER_H
#define LIBUSER_H

#include "stdint.h"

// 系统调用号
#define SYS_PRINTF 1
#define SYS_EXIT 2
#define SYS_YIELD 3
#define SYS_GET_MEM_STATS 4
#define SYS_READ_MEM 5
#define SYS_GET_MEM_USAGE 6
#define SYS_GETCHAR 7
#define SYS_PUTCHAR 8
#define SYS_GETCWD 9
#define SYS_GET_FRAMEBUFFER 10
#define SYS_WRITE 11
#define SYS_FORK 12
#define SYS_OPEN 20
#define SYS_CLOSE 21
#define SYS_READ 22
#define SYS_LSEEK 23
#define SYS_SBRK 24      // sbrk — dynamic memory for user programs
#define SYS_NET_PING 30  // 新增：网络 ping 系统调用
#define SYS_NET_IFCONFIG 31  // 新增：网卡接口配置
#define SYS_WIFI_SCAN 32    // WiFi 扫描
#define SYS_WIFI_CONNECT 33 // WiFi 连接
#define SYS_WIFI_DISCONNECT 34 // WiFi 断开
#define SYS_WIFI_STATUS 35   // WiFi 状态
#define SYS_WIFI_INIT 36   // WiFi 初始化
#define SYS_WIFI_FW_BEGIN 37   // WiFi 固件加载：开始
#define SYS_WIFI_FW_CHUNK 38   // WiFi 固件加载：传输块
#define SYS_WIFI_FW_END 39   // WiFi 固件加载：结束
#define SYS_WIFI_LOAD_FIRMWARE 40 // WiFi 固件加载（完整）
#define SYS_NET_RECV_UDP 53    // 🔥 接收 UDP 数据
//#define SYS_EXECV 41     // execv 系统调用（暂时禁用）
#define SYS_LSPCI 42      // 🔥 新增：列出 PCI 设备
#define SYS_NET_INIT_RTL8139 43  // 🔥 新增：初始化 RTL8139
#define SYS_NET_INIT_E1000 44   // 🔥 新增：初始化 E1000
#define SYS_NET_SEND_UDP 45     // 🔥 新增：发送 UDP 包
#define SYS_NET_SET_DEVICE 46   // 🔥 设置当前使用的网卡
#define SYS_NET_POLL_RX 47      // 🔥 轮询RX（通用）
#define SYS_NET_DUMP_REGS 48
#define SYS_GUI_INPUT_READ 72    // 🔥 转储网卡寄存器状态
#define SYS_NET_ARP 49          // 🔥 ARP 命令（显示/扫描 ARP 缓存）
#define SYS_NET_DUMP_RX_REGS 50 // 🔥 转储 RX 寄存器（详细）
#define SYS_NET_IFUP 51        // 🔥 启动网络接口
// #define SYS_NET_RAW_DUMP_RX_DESC 52  // 🔥 暂时注释掉
#define SYS_NET_RECV_UDP 53    // 🔥 接收 UDP 数据
#define SYS_MSI_TEST 60        // 🔥 MSI 测试
#define SYS_NET_LOOPBACK_TEST 61  // 🔥 E1000 硬件 loopback 测试（轮询）
#define SYS_NET_LOOPBACK_TEST_INT 62  // 🔥 E1000 硬件 loopback 测试（中断）
#define SYS_USB_MOUSE_INFO 76         // 获取 USB 鼠标端点信息

// GUI 系统调用
#define SYS_NET_BIND 52       // 🔥 绑定 UDP 端口
#define SYS_GUI_FB_INFO 70      // 获取帧缓冲区信息
#define SYS_GUI_FB_BLIT 71      // 位图传输到帧缓冲区
#define SYS_GUI_INPUT_READ 72   // 读取输入设备事件
#define SYS_USB_MOUSE_POLL 73   // 轮询 USB 鼠标事件

// WiFi 固件加载常量
#define FW_CHUNK_SIZE   4096                // 每块大小（一页）
#define FW_MAX_SIZE     (2 * 1024 * 1024)   // 最大固件大小 2MB（支持Intel 677KB等）

// 文件打开标志
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 0100
#define O_TRUNC 01000
#define O_APPEND 02000

// 系统调用包装宏
#define syscall1(num, arg1) \
    __asm__ volatile( \
        "movl $" #num ", %%eax\n" \
        "movl %0, %%ebx\n" \
        "int $0x80\n" \
        : \
        : "r"((uint32_t)(arg1)) \
        : "eax", "ebx", "memory" \
    )

#define syscall3(num, arg1, arg2, arg3) \
    __asm__ volatile( \
        "movl $" #num ", %%eax\n" \
        "movl %0, %%ebx\n" \
        "movl %1, %%ecx\n" \
        "movl %2, %%edx\n" \
        "int $0x80\n" \
        : \
        : "r"((uint32_t)(arg1)), "r"((uint32_t)(arg2)), "r"((uint32_t)(arg3)) \
        : "eax", "ebx", "ecx", "edx", "memory" \
    )

// printf 函数
int printf(const char *fmt, ...);

// write 系统调用
int write(int fd, const char *buf, int len);

// fork 系统调用
int fork(void);

// exit 系统调用
void exit(int code) __attribute__((noreturn));

// yield - 让出CPU
void yield(void);

// 文件系统系统调用
int open(const char *pathname, int flags);
int close(int fd);
int read(int fd, char *buf, int len);
int lseek(int fd, int offset, int whence);

// 工作目录系统调用
int getcwd(char *buf, int size);

// 字符 I/O 系统调用
int sys_getchar(void);
void sys_putchar(char c);

// 网络系统调用
int net_ping(const char *ip_addr);  // Ping 指定的 IP 地址（使用默认设备）
int net_ping_dev(const char *ip_addr, const char *dev_name);  // Ping 指定 IP 和设备
int net_ifconfig(void);  // 显示网卡接口配置
int lspci(void);  // 列出 PCI 设备
int lsdisk(const char *path, char *buf, int max);  // 列出目录 (syscall 79)

// 🔥 网卡初始化系统调用
int rtl8139_init_user(void);  // 初始化 RTL8139 网卡
int e1000_init_user(const char *dev_name);  // 🔥 初始化 E1000 网卡（指定设备名称）
int net_send_udp(const char *ip, int port, const char *data, int len);  // 发送 UDP 包
int net_set_device(const char *dev_name);  // 设置当前使用的网卡
int net_poll_rx(void);  // 🔥 轮询RX（调试用）
int net_dump_regs(const char *dev_name);  // 🔥 转储网卡寄存器状态（指定设备）
int net_arp(const char *dev_name, int scan);  // 🔥 ARP 命令（指定设备，scan=1 扫描并更新缓存，scan=0 仅显示）
int net_dump_rx_regs(const char *dev_name);  // 🔥 转储 RX 寄存器（指定设备）
int net_ifup(const char *dev_name);  // 🔥 启动网络接口
int net_bind_udp(int port);  // 🔥 绑定 UDP 端口
int net_recv_udp(char *buf, int len);  // 🔥 接收 UDP 数据（阻塞）
// int net_raw_dump_rx_desc(void);  // 🔥 暂时注释掉

// 用户缓冲区描述符（用于安全传递大块数据）
struct user_buf {
    const void *ptr;   // 用户态地址
    uint32_t len;      // 数据长度
};

// WiFi 系统调用
int wifi_init(void);                      // WiFi 初始化
int wifi_scan(void);                     // WiFi 扫描
int wifi_connect(const char *ssid, const char *password);  // WiFi 连接
int wifi_disconnect(void);               // WiFi 断开
void wifi_status(void);                  // WiFi 状态

// WiFi 固件加载（分块安全版本）
int wifi_fw_load_begin(uint32_t size);   // 开始加载
int wifi_fw_load_chunk(const void *ptr, uint32_t len, uint32_t offset);  // 传输一块
int wifi_fw_load_end(void);              // 结束加载并校验

// 进程控制系统调用
int execv(const char *path, char *const argv[]);  // 在当前进程中执行新程序
int spawn(const char *path, const char *arg); // 创建新进程运行 ELF

// 🔥 MSI 测试
int msi_test(void);  // 手动触发 MSI 测试（包括 LAPIC 检查、软中断测试、MSI 写入测试）
int e1000_loopback_test(void);  // 🔥 E1000 硬件 loopback 测试（轮询版本）
int e1000_loopback_test_interrupt(void);  // 🔥 E1000 硬件 loopback 测试（中断版本，测试 MSI）

// 🔥 GUI 系统调用
typedef struct {
    void *fb_addr;     // 帧缓冲区物理地址
    uint32_t width;    // 屏幕宽度
    uint32_t height;   // 屏幕高度
    uint32_t pitch;    // 每行字节数
    uint32_t bpp;      // 每像素位数
} fb_info_t;

typedef struct {
    uint32_t type;      // 1=键盘, 2=鼠标
    int x;             // 鼠标 X 或 键码
    int y;             // 鼠标 Y 或 保留
    uint32_t pressed;  // 按键状态或保留
} input_event_t;

int gui_get_fb_info(fb_info_t *info);           // 获取帧缓冲区信息
int gui_fb_blit(int x, int y, int width, int height, const void *data);  // 位图传输
int gui_read_input(input_event_t *event);      // 读取输入事件
int usb_mouse_poll(void *report);               // 轮询 USB 鼠标事件
int usb_mouse_info(uint8_t *ep, uint8_t *maxpkt, uint8_t *interval);  // 获取鼠标端点信息

// 字符串和内存工具函数
#include "stddef.h"  // for size_t
size_t strlen(const char *s);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
int atoi(const char *str);
void *memcpy(void *dst, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
char *strcpy(char *dest, const char *src);

#endif // LIBUSER_H
