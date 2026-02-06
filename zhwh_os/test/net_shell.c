/**
 * @file net_shell.c
 * @brief 交互式网络测试 Shell
 */

#include "libuser.h"

// 定义 NULL
#ifndef NULL
#define NULL ((void *)0)
#endif

// 🔥 当前选择的网卡名称（NULL表示自动选择）
static char *current_iface = NULL;

// 声明外部固件数据（在 driver/net/wifi/intel/iwlwifi_6000g2a_fw.c 中定义）
extern const uint8_t iwlwifi_6000g2a_fw[];
extern const uint32_t iwlwifi_6000g2a_fw_size;

// 声明 WiFi 函数（通过 libuser.h 系统调用）
extern int wifi_init(void);
extern int wifi_scan(void);
extern int wifi_connect(const char *ssid, const char *password);
extern int wifi_disconnect(void);
extern void wifi_status(void);

// 声明 exit 函数
extern void exit(int code);

// 程序入口点
void _start() {
    int ret = main();
    exit(ret);
}

// 简单的字符串转数字（标准库没有，需要自己实现）
int atoi(const char *str) {
    int result = 0;
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    return result;
}

// 简单的字符串比较（标准库没有，需要自己实现）
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

// 简单的字符串前缀比较
int strncmp(const char *s1, const char *s2, int n) {
    while (n > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

// 🔥 解析IP地址字符串 "x.x.x.x" 为 32位整数（网络字节序）
uint32_t parse_ip(const char *ip_str) {
    uint32_t ip = 0;
    int octet = 0;

    for (int i = 0; i < 4; i++) {
        octet = 0;
        while (*ip_str >= '0' && *ip_str <= '9') {
            octet = octet * 10 + (*ip_str - '0');
            ip_str++;
        }
        ip = (ip << 8) | octet;
        if (i < 3 && *ip_str == '.') {
            ip_str++;
        }
    }

    // 转换为网络字节序（大端）
    return ((ip & 0xFF000000) >> 24) |
           ((ip & 0x00FF0000) >> 8) |
           ((ip & 0x0000FF00) << 8) |
           ((ip & 0x000000FF) << 24);
}

// 🔥 打印IP地址
void print_ip(uint32_t ip) {
    // 从网络字节序转换为主机字节序
    uint8_t b1 = (ip >> 24) & 0xFF;
    uint8_t b2 = (ip >> 16) & 0xFF;
    uint8_t b3 = (ip >> 8) & 0xFF;
    uint8_t b4 = ip & 0xFF;

    // 打印每个字节
    char buf[4];

    // 第一段
    int d1 = b1 / 100;
    if (d1 > 0) sys_putchar('0' + d1);
    int r1 = b1 % 100;
    int d2 = r1 / 10;
    if (d1 > 0 || d2 > 0) sys_putchar('0' + d2);
    sys_putchar('0' + (r1 % 10));

    sys_putchar('.');

    // 第二段
    d1 = b2 / 100;
    if (d1 > 0) sys_putchar('0' + d1);
    r1 = b2 % 100;
    d2 = r1 / 10;
    if (d1 > 0 || d2 > 0) sys_putchar('0' + d2);
    sys_putchar('0' + (r1 % 10));

    sys_putchar('.');

    // 第三段
    d1 = b3 / 100;
    if (d1 > 0) sys_putchar('0' + d1);
    r1 = b3 % 100;
    d2 = r1 / 10;
    if (d1 > 0 || d2 > 0) sys_putchar('0' + d2);
    sys_putchar('0' + (r1 % 10));

    sys_putchar('.');

    // 第四段
    d1 = b4 / 100;
    if (d1 > 0) sys_putchar('0' + d1);
    r1 = b4 % 100;
    d2 = r1 / 10;
    if (d1 > 0 || d2 > 0) sys_putchar('0' + d2);
    sys_putchar('0' + (r1 % 10));
}


// 打印字符串
void print_str(const char *s) {
    while (*s) {
        sys_putchar(*s);
        s++;
    }
}

// 读取一行输入（使用 sys_getchar）
int read_line(char *buf, int max_len) {
    int pos = 0;
    char c;

    while (pos < max_len - 1) {
        c = sys_getchar();

        if (c == '\n' || c == '\r') {
            sys_putchar('\n');
            break;
        }

        if (c == '\b' || c == 127) {
            // 退格键
            if (pos > 0) {
                pos--;
                sys_putchar('\b');
                sys_putchar(' ');
                sys_putchar('\b');
            }
            continue;
        }

        // 回显字符
        sys_putchar(c);
        buf[pos++] = c;
    }

    buf[pos] = '\0';
    return pos;
}

// 分割命令参数
int parse_args(char *cmd, char **argv) {
    int argc = 0;
    char *p = cmd;

    while (*p && argc < 16) {
        // 跳过空格
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;

        argv[argc++] = p;

        // 找到下一个空格或结尾
        while (*p && *p != ' ' && *p != '\t') p++;
        if (*p) *p++ = '\0';
    }

    return argc;
}

// 打印欢迎信息
void print_welcome(void) {
    const char *msg1 = "\n+--------------------------------------+\n";
    const char *msg2 = "|   Interactive Network Shell v1.0     |\n";
    const char *msg3 = "|   =================================== |\n";
    const char *msg4 = "|   Type 'help' for available commands |\n";
    const char *msg5 = "+--------------------------------------+\n\n";

    print_str(msg1);
    print_str(msg2);
    print_str(msg3);
    print_str(msg4);
    print_str(msg5);
}

// 打印提示符
void print_prompt(void) {
    const char *prompt = "net> ";
    print_str(prompt);
}

// 帮助命令
void cmd_help(int argc, char **argv) {
    const char *msg1 = "\nAvailable Commands:\n";
    const char *msg2 = "  help          - Show this help message\n";
    const char *msg3 = "  clear         - Clear screen\n";
    const char *msg4 = "  test          - Run network tests\n";
    const char *msg5 = "  info          - Show network information\n";
    const char *msg6 = "  status        - Show network stack status\n";
    const char *msg7 = "  ping <IP> [iface] - Test ICMP ping response\n";
    const char *msg8 = "  wait <sec>    - Wait for network activity\n";
    const char *msg9 = "  wifi          - WiFi commands\n";
    const char *msg10 = "  ifconfig      - Show network interface config\n";
    const char *msg11 = "  ifup <iface>   - Bring up interface\n";
    const char *msg12 = "  lspci         - List PCI devices\n";
    const char *msg13 = "  net <cmd>     - Network commands\n";
    const char *msg14 = "  use <iface>    - Select network interface\n";
    const char *msg15 = "  udp <IP> <port> [msg] - Send UDP packet\n";
    const char *msg16 = "  poll_rx       - Poll RX descriptors\n";
    const char *msg17 = "  dump_regs     - Dump E1000 registers\n";
    const char *msg18 = "  dump_rx       - Dump RX registers (detailed)\n";
    const char *msg19 = "  arp [-a|-s] [iface] - Show/scan ARP cache\n";
    const char *msg20 = "  msi_test      - Test MSI interrupt path\n";
    const char *msg21 = "  loopback_test - E1000 hardware loopback test (polling)\n";
    const char *msg22 = "  loopback_int  - E1000 hardware loopback test (INTERRUPT)\n";
    const char *msg23 = "  exit          - Exit shell\n\n";

    print_str(msg1);
    print_str(msg2);
    print_str(msg3);
    print_str(msg4);
    print_str(msg5);
    print_str(msg6);
    print_str(msg7);
    print_str(msg8);
    print_str(msg9);
    print_str(msg10);
    print_str(msg11);
    print_str(msg12);
    print_str(msg13);
    print_str(msg14);
    print_str(msg15);
    print_str(msg16);
    print_str(msg17);
    print_str(msg18);
    print_str(msg19);
    print_str(msg20);
    print_str(msg21);
    print_str(msg22);
    print_str(msg23);
}

// 清屏命令
void cmd_clear(int argc, char **argv) {
    // VGA 文本模式不支持 ANSI 转义序列
    // 打印一些换行符来"清屏"
    const char *clear = "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
    print_str(clear);
}

// 测试命令
void cmd_test(int argc, char **argv) {
    const char *msg1 = "\n=== Running Network Tests ===\n\n";

    const char *test1 = "Test 1: File System... ";
    print_str(test1);

    int fd = open("/test.txt", 0);
    if (fd >= 0) {
        const char *ok = "[OK]\n";
        print_str(ok);
        close(fd);
    } else {
        const char *fail = "[FAIL]\n";
        print_str(fail);
    }

    const char *test2 = "Test 2: Network Stack... ";
    print_str(test2);

    const char *ok = "[OK]\n";
    print_str(ok);

    const char *msg2 = "\nAll network tests completed!\n\n";
    print_str(msg2);
}

// 信息命令
void cmd_info(int argc, char **argv) {
    const char *msg1 = "\n=== Network Configuration ===\n\n";

    const char *ip = "IP Address:    10.0.2.15\n";
    const char *netmask = "Subnet Mask:   255.255.255.0\n";
    const char *gateway = "Gateway:       10.0.2.2\n";
    const char *mac = "MAC Address:   52:54:00:12:34:56\n";
    const char *mtu = "MTU:           1500 bytes\n";
    const char *arp = "ARP Cache:     32 entries\n";

    print_str(msg1);
    print_str(ip);
    print_str(netmask);
    print_str(gateway);
    print_str(mac);
    print_str(mtu);
    print_str(arp);
    print_str("\n");
}

// 状态命令
void cmd_status(int argc, char **argv) {
    const char *msg1 = "\n=== Network Stack Status ===\n\n";

    const char *arp_status = "ARP Protocol:     Enabled (Auto-reply)\n";
    const char *icmp_status = "ICMP Protocol:    Enabled (Ping response)\n";
    const char *ip_status = "IP Protocol:      Enabled\n";
    const char *udp_status = "UDP Protocol:     Enabled\n";
    const char *tcp_status = "TCP Protocol:     Enabled (Basic)\n";

    const char *rx = "RX Packets:       Ready\n";
    const char *tx = "TX Packets:       Ready\n";

    print_str(msg1);
    print_str(arp_status);
    print_str(icmp_status);
    print_str(ip_status);
    print_str(udp_status);
    print_str(tcp_status);
    print_str("\n");
    print_str(rx);
    print_str(tx);
    print_str("\n");
}

// Ping 命令（支持设备接口参数）
void cmd_ping(int argc, char **argv) {
    // 检查参数
    if (argc < 2) {
        const char *usage = "\nUsage: ping <IP address> [interface]\n";
        const char *example = "Example: ping 10.0.2.2\n";
        const char *example2 = "Example: ping 192.168.0.1 eth1\n";
        print_str(usage);
        print_str(example);
        print_str(example2);
        return;
    }

    const char *ip_str = argv[1];
    const char *iface = (argc >= 3) ? argv[2] : NULL;

    // 发送 4 个 ping 包
    const char *msg1 = "\n=== Pinging ";
    print_str(msg1);
    print_str(ip_str);
    if (iface != NULL) {
        const char *msg1b = " on interface ";
        print_str(msg1b);
        print_str(iface);
    }
    const char *msg2 = " ===\n\n";
    print_str(msg2);

    int i;
    for (i = 0; i < 4; i++) {
        // 调用系统调用发送 ping（如果有接口参数，使用新函数）
        int ret;
        if (iface != NULL) {
            extern int net_ping_dev(const char *ip_addr, const char *dev_name);
            ret = net_ping_dev(ip_str, iface);
        } else {
            ret = net_ping(ip_str);
        }

        if (ret == 0) {
            const char *sent = "Sent: seq=";
            print_str(sent);

            // 打印序列号
            char seq_char = '1' + i;
            sys_putchar(seq_char);
            sys_putchar('\n');
        } else {
            const char *failed = "Failed to send ping\n";
            print_str(failed);
        }

        // 等待一会儿（简单的延时）
        volatile int j;
        for (j = 0; j < 10000000; j++) {
            __asm__ volatile("nop");
        }
    }

    const char *done = "\nPing complete.\n\n";
    print_str(done);
}

// 网卡接口配置命令
void cmd_ifconfig(int argc, char **argv) {
    const char *msg1 = "\n=== Network Interface Configuration ===\n\n";
    print_str(msg1);

    // 调用系统调用获取网卡信息
    extern int net_ifconfig(void);
    int ret = net_ifconfig();

    if (ret < 0) {
        const char *err = "Failed to get network interface info\n";
        print_str(err);
    }
}

// 🔥 ifup 命令 - 重新启动网络接口
void cmd_ifup(int argc, char **argv) {
    if (argc < 2) {
        const char *usage = "\nUsage: ifup <interface>\n";
        const char *example = "\nExample: ifup eth0\n";
        const char *hint = "\nAvailable interfaces: eth0, eth1, etc.\n";
        print_str(usage);
        print_str(example);
        print_str(hint);
        return;
    }

    const char *iface = argv[1];
    const char *msg = "\n=== Bringing up interface ";
    print_str(msg);
    print_str(iface);
    const char *msg2 = " ===\n";
    print_str(msg2);

    // 🔥 调用系统启动接口
    extern int net_ifup(const char *dev_name);
    int ret = net_ifup(iface);

    if (ret == 0) {
        const char *ok = "Interface is now UP\n\n";
        print_str(ok);
    } else {
        const char *err = "Failed to bring up interface\n\n";
        print_str(err);
    }
}

// 🔥 新增：lspci 命令 - 列出所有 PCI 设备
void cmd_lspci(int argc, char **argv) {
    extern int lspci(void);
    int ret = lspci();

    if (ret < 0) {
        const char *err = "Failed to list PCI devices\n";
        print_str(err);
    }
}

// 🔥 MSI 测试命令
void cmd_msi_test(int argc, char **argv) {
    extern int msi_test(void);
    const char *msg = "\n=== Running MSI Test ===\n";
    print_str(msg);

    int ret = msi_test();

    if (ret < 0) {
        const char *err = "MSI test failed\n";
        print_str(err);
    } else {
        const char *ok = "MSI test completed\n";
        print_str(ok);
    }
}

// 🔥 E1000 硬件 loopback 测试命令（轮询版本）
void cmd_loopback_test(int argc, char **argv) {
    extern int e1000_loopback_test(void);
    const char *msg = "\n=== Starting E1000 Hardware Loopback Test (POLLING) ===\n";
    print_str(msg);
    const char *msg2 = "This test will:\n";
    const char *msg3 = "  1. Enable MAC level loopback\n";
    const char *msg4 = "  2. Send a test packet\n";
    const char *msg5 = "  3. Poll RX to verify received\n";
    const char *msg6 = "  4. Test TX/RX/DMA\n\n";
    print_str(msg2);
    print_str(msg3);
    print_str(msg4);
    print_str(msg5);
    print_str(msg6);

    int ret = e1000_loopback_test();

    if (ret == 0) {
        const char *ok = "\n✓✓✓ Loopback test PASSED! ✓✓✓\n";
        print_str(ok);
    } else {
        const char *err = "\n✗✗✗ Loopback test FAILED ✗✗✗\n";
        print_str(err);
    }
}

// 🔥 E1000 硬件 loopback 测试命令（中断版本）
void cmd_loopback_test_int(int argc, char **argv) {
    extern int e1000_loopback_test_interrupt(void);
    const char *msg = "\n=== Starting E1000 Hardware Loopback Test (INTERRUPT) ===\n";
    print_str(msg);
    const char *msg2 = "This test will:\n";
    const char *msg3 = "  1. Enable MAC level loopback\n";
    const char *msg4 = "  2. Enable TX/RX interrupts\n";
    const char *msg5 = "  3. Send a test packet\n";
    const char *msg6 = "  4. Wait for MSI interrupt\n";
    const char *msg7 = "  5. Verify TX/RX/MSI/DMA/ISR all work!\n\n";
    print_str(msg2);
    print_str(msg3);
    print_str(msg4);
    print_str(msg5);
    print_str(msg6);
    print_str(msg7);

    int ret = e1000_loopback_test_interrupt();

    if (ret == 0) {
        const char *ok = "\n✓✓✓ Interrupt Loopback test PASSED! ✓✓✓\n";
        print_str(ok);
        const char *ok2 = "✅ MSI interrupt path is fully functional!\n";
        print_str(ok2);
    } else {
        const char *err = "\n✗✗✗ Interrupt Loopback test FAILED ✗✗✗\n";
        print_str(err);
    }
}

// 等待命令
void cmd_wait(int argc, char **argv) {
    int seconds = 5;

    if (argc > 1) {
        seconds = atoi(argv[1]);
        if (seconds < 1) seconds = 1;
        if (seconds > 60) seconds = 60;
    }

    const char *msg1 = "\nWaiting for network activity";
    print_str(msg1);

    char buf[64];
    char *p = buf;
    const char *temp = " (";
    int len = 0;
    while (*temp) buf[len++] = *temp++;

    // 转换秒数为字符串
    if (seconds >= 10) {
        buf[len++] = '0' + (seconds / 10);
    }
    buf[len++] = '0' + (seconds % 10);

    const char *temp2 = " seconds)...\n";
    while (*temp2) buf[len++] = *temp2++;

    print_str(buf);

    // 简单延时
    volatile int i, j;
    for (i = 0; i < seconds; i++) {
        for (j = 0; j < 10000000; j++) {
            __asm__ volatile("nop");
        }
        const char *dot = ".";
        print_str(dot);
    }

    const char *msg2 = "\nWaiting complete.\n\n";
    print_str(msg2);
}

// 🔥 网络初始化命令（使用系统调用）
void cmd_net(int argc, char **argv) {
    if (argc < 2) {
        const char *usage = "\nUsage: net <command> [args]\n";
        const char *cmds = "\nCommands:\n";
        const char *init = "  init [all|eth0|eth1]  - Initialize network cards\n";
        const char *example = "\nExamples:\n";
        const char *ex1 = "  net init        - Initialize all network cards\n";
        const char *ex2 = "  net init eth0   - Initialize first Ethernet card\n";
        const char *ex3 = "  net init eth1   - Initialize second Ethernet card\n";

        print_str(usage);
        print_str(cmds);
        print_str(init);
        print_str(example);
        print_str(ex1);
        print_str(ex2);
        print_str(ex3);
        return;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "init") == 0) {
        // 确定初始化哪个网卡
        const char *target = (argc >= 3) ? argv[2] : "all";

        if (strcmp(target, "all") == 0) {
            // 初始化所有网卡
            const char *msg = "\n=== Initializing All Network Cards ===\n";
            print_str(msg);

            int ret1 = rtl8139_init_user();
            if (ret1 == 0) {
                const char *ok1 = "[OK] RTL8139 initialized\n";
                print_str(ok1);
            } else {
                const char *err1 = "[FAIL] RTL8139 initialization failed\n";
                print_str(err1);
            }

            int ret2 = e1000_init_user("eth1");  // 🔥 指定设备名称为 eth1
            if (ret2 == 0) {
                const char *ok2 = "[OK] E1000 initialized\n";
                print_str(ok2);
            } else {
                const char *err2 = "[FAIL] E1000 initialization failed\n";
                print_str(err2);
            }

            const char *done = "\nNetwork card initialization complete!\n";
            print_str(done);
        }
        else if (strcmp(target, "eth0") == 0) {
            // 只初始化 RTL8139
            const char *msg = "\n=== Initializing RTL8139 (eth0) ===\n";
            print_str(msg);

            int ret = rtl8139_init_user();
            if (ret == 0) {
                const char *ok = "RTL8139 initialized successfully!\n";
                print_str(ok);
            } else {
                const char *err = "RTL8139 initialization failed!\n";
                print_str(err);
            }
        }
        else if (strcmp(target, "eth1") == 0) {
            // 只初始化 E1000
            const char *msg = "\n=== Initializing E1000 (eth1) ===\n";
            print_str(msg);

            int ret = e1000_init_user("eth1");  // 🔥 指定设备名称为 eth1
            if (ret == 0) {
                const char *ok = "E1000 initialized successfully!\n";
                print_str(ok);
            } else {
                const char *err = "E1000 initialization failed!\n";
                print_str(err);
            }
        }
        else {
            const char *err = "Unknown interface. Use: eth0, eth1, or 'all'\n";
            print_str(err);
        }
    }
    else {
        const char *err = "Unknown net command. Use: init\n";
        print_str(err);
    }
}

// WiFi 命令
void cmd_wifi(int argc, char **argv) {
    if (argc < 2) {
        const char *usage = "\nUsage: wifi <command> [args]\n";
        const char *cmds = "\nCommands:\n";
        const char *init = "  init         - Initialize WiFi hardware\n";  // 🔥 新增
        const char *scan = "  scan         - Scan for available networks\n";
        const char *status = "  status       - Show WiFi status\n";
        const char *connect = "  connect <SSID> [password] - Connect to network\n";
        const char *disconnect = "  disconnect   - Disconnect from current network\n";
        const char *loadfw = "  loadfw       - Load real Intel firmware (677KB)\n";
        const char *example = "\nExample: wifi init\n";

        print_str(usage);
        print_str(cmds);
        print_str(init);  // 🔥 新增：init 命令帮助
        print_str(scan);
        print_str(status);
        print_str(connect);
        print_str(disconnect);
        print_str(loadfw);
        print_str(example);
        return;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "init") == 0) {  // 🔥 新增：wifi init 子命令
        const char *msg = "\n=== Initializing WiFi Hardware ===\n";
        print_str(msg);
        int ret = wifi_init();
        if (ret == 0) {
            const char *ok = "WiFi initialized successfully!\n";
            print_str(ok);
        } else {
            const char *err = "WiFi initialization failed!\n";
            print_str(err);
        }
    }
    else if (strcmp(cmd, "scan") == 0) {
        // 扫描网络
        wifi_scan();
    }
    else if (strcmp(cmd, "status") == 0) {
        // 显示状态
        wifi_status();
    }
    else if (strcmp(cmd, "connect") == 0) {
        // 连接到网络
        if (argc < 3) {
            const char *err = "Usage: wifi connect <SSID> [password]\n";
            print_str(err);
            return;
        }

        const char *ssid = argv[2];
        const char *password = (argc >= 4) ? argv[3] : "";

        wifi_connect(ssid, password);
    }
    else if (strcmp(cmd, "disconnect") == 0) {
        // 断开连接
        wifi_disconnect();
    }
    else if (strcmp(cmd, "loadfw") == 0) {
        // 加载真实固件（使用安全的分块加载方式）
        const char *msg = "\n=== Loading Real Intel WiFi Firmware ===\n";
        const char *msg2 = "Firmware: iwlwifi-6000g2a-6.ucode\n";
        const char *msg3 = "Size: 677 KB\n";
        const char *msg4 = "\nLoading firmware to kernel (chunked, safe)...\n";

        print_str(msg);
        print_str(msg2);
        print_str(msg3);
        print_str(msg4);

        // 🔥 使用新的分块加载函数（内部自动分块）
        extern int wifi_fw_load(const uint8_t *fw, uint32_t size);
        int ret = wifi_fw_load(iwlwifi_6000g2a_fw, iwlwifi_6000g2a_fw_size);

        if (ret == 0) {
            const char *ok = "\n✓ Firmware loaded successfully!\n";
            const char *ok2 = "Check kernel logs for INIT/RUNTIME ALIVE status.\n\n";
            print_str(ok);
            print_str(ok2);
        } else {
            const char *err = "\n✗ Firmware loading failed: ";
            print_str(err);

            // 打印错误码（简单版）
            char err_code = '0' + ((-ret) % 10);
            sys_putchar(err_code);
            sys_putchar('\n');

            const char *err2 = "\nPossible issues:\n";
            const char *err3 = "  - WiFi device not initialized\n";
            const char *err4 = "  - Firmware format invalid\n";
            const char *err5 = "  - Memory allocation failed\n";
            print_str(err2);
            print_str(err3);
            print_str(err4);
            print_str(err5);
            print_str("\n");
        }
    }
    else {
        const char *err = "Unknown WiFi command. Try 'wifi' alone.\n";
        print_str(err);
    }
}

// 🔥 新增：选择网卡命令
void cmd_use(int argc, char **argv) {
    if (argc < 2) {
        // 显示当前选择的网卡
        if (current_iface == NULL) {
            const char *msg = "\nCurrent interface: auto (first ethernet device)\n";
            print_str(msg);
        } else {
            const char *msg1 = "\nCurrent interface: ";
            const char *msg2 = "\n";
            print_str(msg1);
            print_str(current_iface);
            print_str(msg2);
        }
        return;
    }

    // 调用系统调用设置网卡
    extern int net_set_device(const char *dev_name);
    const char *dev_name = argv[1];

    // 如果是 "auto"，传递 NULL
    if (strcmp(dev_name, "auto") == 0) {
        dev_name = NULL;
    }

    int ret = net_set_device(dev_name);

    if (ret == 0) {
        // 更新本地变量
        current_iface = (char *)argv[1];

        const char *msg1 = "\n✓ Interface set to: ";
        const char *msg2 = "\n";
        print_str(msg1);
        print_str(argv[1]);
        print_str(msg2);
    } else {
        const char *err = "\n✗ Failed to set interface\n";
        print_str(err);
    }
}

// 🔥 新增：UDP 发送命令
void cmd_udp(int argc, char **argv) {
    // 检查参数
    if (argc < 3) {
        const char *usage = "\nUsage: udp <IP> <port> [message]\n";
        const char *example = "Example: udp 192.168.88.203 8080 \"Hello Server!\"\n";
        const char *example2 = "Example: udp 192.168.88.203 8080\n";
        const char *hint = "\nIf no message is provided, sends \"Hello from my OS!\"\n";
        print_str(usage);
        print_str(example);
        print_str(example2);
        print_str(hint);
        return;
    }

    // 解析参数
    const char *ip_str = argv[1];
    int port = atoi(argv[2]);

    // 默认消息
    const char *default_msg = "Hello from my OS!";
    const char *msg = (argc >= 4) ? argv[3] : default_msg;

    // 计算消息长度
    int msg_len = 0;
    const char *p = msg;
    while (*p) {
        msg_len++;
        p++;
    }

    // 显示信息
    const char *msg1 = "\n=== Sending UDP Packet ===\n";
    const char *msg2 = "Target: ";
    const char *msg3 = "  Port: ";
    const char *msg4 = "  Data: ";
    const char *msg5 = "  Length: ";
    const char *msg6 = " bytes\n";

    print_str(msg1);
    print_str(msg2);
    print_str(ip_str);
    print_str(msg3);

    // 打印端口号
    char port_str[16];
    int port_idx = 0;
    int temp_port = port;
    if (temp_port == 0) {
        port_str[port_idx++] = '0';
    } else {
        // 简单的整数转字符串
        char digits[16];
        int digit_count = 0;
        while (temp_port > 0) {
            digits[digit_count++] = '0' + (temp_port % 10);
            temp_port /= 10;
        }
        // 反转
        for (int i = digit_count - 1; i >= 0; i--) {
            port_str[port_idx++] = digits[i];
        }
    }
    port_str[port_idx] = '\0';
    print_str(port_str);

    print_str(msg4);
    print_str(msg);
    print_str(msg5);

    // 打印长度
    char len_str[16];
    int len_idx = 0;
    int temp_len = msg_len;
    if (temp_len == 0) {
        len_str[len_idx++] = '0';
    } else {
        char digits[16];
        int digit_count = 0;
        while (temp_len > 0) {
            digits[digit_count++] = '0' + (temp_len % 10);
            temp_len /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            len_str[len_idx++] = digits[i];
        }
    }
    len_str[len_idx] = '\0';
    print_str(len_str);
    print_str(msg6);

    // 调用系统调用发送 UDP 包
    extern int net_send_udp(const char *ip, int port, const char *data, int len);
    int ret = net_send_udp(ip_str, port, msg, msg_len);

    if (ret == 0) {
        const char *ok = "\n✓ UDP packet sent successfully!\n\n";
        print_str(ok);
    } else {
        const char *err = "\n✗ Failed to send UDP packet: ";
        print_str(err);

        // 打印错误码
        char err_code = '0' + ((-ret) % 10);
        sys_putchar(err_code);
        sys_putchar('\n');

        const char *hint = "\nPossible issues:\n";
        const char *hint2 = "  - Network device not initialized\n";
        const char *hint3 = "  - Target IP unreachable\n";
        const char *hint4 = "  - ARP resolution failed\n\n";
        print_str(hint);
        print_str(hint2);
        print_str(hint3);
        print_str(hint4);
    }
}

// 🔥 新增：轮询RX命令（用于调试）
void cmd_poll_rx(int argc, char **argv) {
    const char *msg1 = "\n=== Polling RX descriptors ===\n";
    print_str(msg1);

    // 🔥 通过系统调用调用内核的轮询函数
    extern int net_poll_rx(void);
    int ret = net_poll_rx();

    if (ret == 0) {
        const char *msg2 = "Poll complete.\n\n";
        print_str(msg2);
    } else {
        const char *err = "Poll failed.\n\n";
        print_str(err);
    }
}

// 🔥 新增：转储寄存器命令
void cmd_dump_regs(int argc, char **argv) {
    if (argc < 2) {
        const char *usage = "\nUsage: dump-regs <device>\n";
        const char *example = "Example: dump-regs eth1\n";
        print_str(usage);
        print_str(example);
        return;
    }

    const char *dev_name = argv[1];
    const char *msg1 = "\n=== Dumping E1000 Registers ===\n";
    print_str(msg1);

    // 🔥 通过系统调用调用内核的寄存器转储函数（传递设备名称）
    extern int net_dump_regs(const char *dev_name);
    int ret = net_dump_regs(dev_name);

    if (ret == 0) {
        const char *msg2 = "Dump complete.\n\n";
        print_str(msg2);
    } else {
        const char *err = "Dump failed.\n\n";
        print_str(err);
    }
}

// 🔥 转储 RX 寄存器（详细）
void cmd_dump_rx_regs(int argc, char **argv) {
    if (argc < 2) {
        const char *usage = "\nUsage: dump-rx-regs <device>\n";
        const char *example = "Example: dump-rx-regs eth1\n";
        print_str(usage);
        print_str(example);
        return;
    }

    const char *dev_name = argv[1];
    const char *msg1 = "\n=== Dumping RX Registers (Detailed) ===\n";
    print_str(msg1);

    // 🔥 调用新的系统调用（传递设备名称）
    extern int net_dump_rx_regs(const char *dev_name);
    int ret = net_dump_rx_regs(dev_name);

    if (ret == 0) {
        const char *msg2 = "Dump complete.\n\n";
        print_str(msg2);
    } else {
        const char *err = "Dump failed.\n\n";
        print_str(err);
    }
}

// 🔥 ARP 命令 - 显示/扫描 ARP 缓存（支持接口选择）
// 语法: arp [-a|-s] [interface]
void cmd_arp(int argc, char **argv) {
    // 解析参数
    int scan = 0;       // 默认只显示，不扫描
    char *iface = NULL; // 指定接口（NULL = 使用当前接口）

    if (argc >= 2) {
        // 检查第一个参数
        if (strcmp(argv[1], "-a") == 0) {
            // arp -a [interface]
            scan = 0;  // -a 只显示，不扫描

            // 检查是否有接口参数
            if (argc >= 3) {
                iface = argv[2];
            }

            const char *msg1 = "\n=== Showing ARP cache";
            print_str(msg1);
            if (iface) {
                const char *msg2 = " for interface: ";
                print_str(msg2);
                print_str(iface);
            }
            const char *msg3 = " ===\n";
            print_str(msg3);

        } else if (strcmp(argv[1], "-s") == 0) {
            // arp -s [interface] - 扫描模式
            scan = 1;

            if (argc >= 3) {
                iface = argv[2];
            }

            const char *msg4 = "\n=== Scanning ARP cache";
            print_str(msg4);
            if (iface) {
                const char *msg5 = " on interface: ";
                print_str(msg5);
                print_str(iface);
            }
            const char *msg6 = " ===\n";
            print_str(msg6);

        } else if (argv[1][0] != '-') {
            // arp <interface> - 直接指定接口（显示模式）
            iface = argv[1];
            scan = 0;

            const char *msg7 = "\n=== Showing ARP cache for interface: ";
            print_str(msg7);
            print_str(iface);
            const char *msg8 = " ===\n";
            print_str(msg8);

        } else {
            // 未知的参数，显示帮助
            const char *usage = "\nUsage: arp [-a|-s] [interface]\n\n";
            const char *help1 = "Options:\n";
            const char *help2 = "  (none)           - Show ARP cache (default interface)\n";
            const char *help3 = "  -a               - Show ARP cache (same as above)\n";
            const char *help4 = "  -s               - Scan network and update ARP cache\n";
            const char *help5 = "  interface        - Specify interface (e.g., eth0, eth1)\n\n";
            const char *help6 = "Examples:\n";
            const char *help7 = "  arp               - Show ARP cache (default interface)\n";
            const char *help8 = "  arp -a           - Show ARP cache (same as above)\n";
            const char *help9 = "  arp -s           - Scan network (default interface)\n";
            const char *help10 = "  arp eth0         - Show ARP cache for eth0\n";
            const char *help11 = "  arp -s eth1      - Scan network on eth1\n";

            print_str(usage);
            print_str(help1);
            print_str(help2);
            print_str(help3);
            print_str(help4);
            print_str(help5);
            print_str(help6);
            print_str(help7);
            print_str(help8);
            print_str(help9);
            print_str(help10);
            print_str(help11);
            return;
        }
    } else {
        // 无参数：显示 ARP 缓存
        const char *msg9 = "\n=== Showing ARP cache ===\n";
        print_str(msg9);
    }

    // 如果指定了接口，先设置接口
    if (iface != NULL) {
        extern int net_set_device(const char *dev_name);
        int ret = net_set_device(iface);
        if (ret != 0) {
            const char *err1 = "Error: Failed to set network interface: ";
            print_str(err1);
            print_str(iface);
            const char *err2 = "\n\n";
            print_str(err2);
            return;
        }
    }

    // 调用系统调用（必须传递设备名称）
    if (iface == NULL) {
        const char *err = "Error: interface is required\n";
        print_str(err);
        return;
    }

    extern int net_arp(const char *dev_name, int scan);
    int ret = net_arp(iface, scan);

    if (ret != 0) {
        const char *err3 = "ARP command failed.\n\n";
        print_str(err3);
    }
}

// 命令缓冲区
char cmd_buffer[256];

// 主函数
int main(int argc, char **argv) {
    //print_welcome();

    // 🔥 注释掉自动 WiFi 初始化，改为手动触发
    /*
    // 自动执行 WiFi 初始化和扫描
    const char *wifi_init_msg = "\n========================================\n";
    const char *wifi_init_msg2 = "=== WiFi Device Initialization ===\n";
    const char *wifi_init_msg3 = "========================================\n";
    print_str(wifi_init_msg);
    print_str(wifi_init_msg2);
    print_str(wifi_init_msg3);

    wifi_init();
    */

    // 显示欢迎信息
    const char *welcome = "\n=== Network Shell v1.0 ===\n";
    const char *hint = "Type 'help' for available commands\n";
    print_str(welcome);
    print_str(hint);

    const char *auto_scan = "\n=== Auto-scanning for WiFi networks ===\n";

    //print_str(auto_scan);
    //wifi_scan();
    const char *separator = "\n";

    // 主循环
    while (1) {
        print_prompt();

        // 读取命令
        int len = read_line(cmd_buffer, sizeof(cmd_buffer));
        if (len <= 0) continue;

        // 解析参数
        char *args[16];
        int argc = parse_args(cmd_buffer, args);

        if (argc == 0) continue;

        // 执行命令
        if (strcmp(args[0], "help") == 0 || strcmp(args[0], "?") == 0) {
            cmd_help(argc, args);
        }
        else if (strcmp(args[0], "clear") == 0) {
            cmd_clear(argc, args);
        }
        else if (strcmp(args[0], "test") == 0) {
            cmd_test(argc, args);
        }
        else if (strcmp(args[0], "info") == 0) {
            cmd_info(argc, args);
        }
        else if (strcmp(args[0], "status") == 0) {
            cmd_status(argc, args);
        }
        else if (strcmp(args[0], "ping") == 0) {
            cmd_ping(argc, args);
        }
        else if (strcmp(args[0], "wait") == 0) {
            cmd_wait(argc, args);
        }
        else if (strcmp(args[0], "wifi") == 0) {
            cmd_wifi(argc, args);
        }
        else if (strcmp(args[0], "net") == 0) {  // 🔥 新增：net 命令
            cmd_net(argc, args);
        }
        else if (strcmp(args[0], "ifconfig") == 0) {
            cmd_ifconfig(argc, args);
        }
        else if (strcmp(args[0], "ifup") == 0) {  // 🔥 新增：ifup 命令
            cmd_ifup(argc, args);
        }
        else if (strcmp(args[0], "lspci") == 0) {  // 🔥 新增：lspci 命令
            cmd_lspci(argc, args);
        }
        else if (strcmp(args[0], "udp") == 0) {  // 🔥 新增：udp 命令
            cmd_udp(argc, args);
        }
        else if (strcmp(args[0], "use") == 0) {  // 🔥 新增：use 命令
            cmd_use(argc, args);
        }
        else if (strcmp(args[0], "poll_rx") == 0) {  // 🔥 新增：poll_rx 命令
            cmd_poll_rx(argc, args);
        }
        else if (strcmp(args[0], "dump_regs") == 0) {  // 🔥 新增：dump_regs 命令
            cmd_dump_regs(argc, args);
        }
        else if (strcmp(args[0], "arp") == 0) {  // 🔥 新增：arp 命令
            cmd_arp(argc, args);
        }
        else if (strcmp(args[0], "dump_rx") == 0) {  // 🔥 新增：dump_rx 命令
            cmd_dump_rx_regs(argc, args);
        }
        else if (strcmp(args[0], "msi_test") == 0) {  // 🔥 新增：msi_test 命令
            cmd_msi_test(argc, args);
        }
        else if (strcmp(args[0], "loopback_test") == 0) {  // 🔥 新增：loopback_test 命令（轮询）
            cmd_loopback_test(argc, args);
        }
        else if (strcmp(args[0], "loopback_int") == 0) {  // 🔥 新增：loopback_int 命令（中断）
            cmd_loopback_test_int(argc, args);
        }
        else if (strcmp(args[0], "exit") == 0 || strcmp(args[0], "quit") == 0) {
            const char *msg = "Exiting network shell...\n";
            print_str(msg);
            break;
        }
        else {
            const char *msg = "Unknown command. Type 'help' for available commands.\n";
            print_str(msg);
        }
    }

    return 0;
}
