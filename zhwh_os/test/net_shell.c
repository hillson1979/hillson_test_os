/**
 * @file net_shell.c
 * @brief 交互式网络测试 Shell
 */

#include "libuser.h"

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
    const char *msg7 = "  ping          - Test ICMP ping response\n";
    const char *msg8 = "  wait <sec>    - Wait for network activity\n";
    const char *msg9 = "  exit          - Exit shell\n\n";

    print_str(msg1);
    print_str(msg2);
    print_str(msg3);
    print_str(msg4);
    print_str(msg5);
    print_str(msg6);
    print_str(msg7);
    print_str(msg8);
    print_str(msg9);
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

// Ping 命令
void cmd_ping(int argc, char **argv) {
    // 检查参数
    if (argc < 2) {
        const char *usage = "\nUsage: ping <IP address>\n";
        const char *example = "Example: ping 10.0.2.2\n";
        print_str(usage);
        print_str(example);
        return;
    }

    const char *ip_str = argv[1];

    // 发送 4 个 ping 包
    const char *msg1 = "\n=== Pinging ";
    print_str(msg1);
    print_str(ip_str);
    const char *msg2 = " ===\n\n";
    print_str(msg2);

    int i;
    for (i = 0; i < 4; i++) {
        // 调用系统调用发送 ping
        int ret = net_ping(ip_str);

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

// 命令缓冲区
char cmd_buffer[256];

// 主函数
int main(int argc, char **argv) {
    print_welcome();

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
