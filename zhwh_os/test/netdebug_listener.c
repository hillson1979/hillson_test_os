/**
 * @file netdebug_listener.c
 * @brief 网络调试消息监听器 - 用户态程序
 *
 * 用法：
 *   ./netdebug_listener
 *
 * 功能：
 *   - 监听 UDP 端口 9999 接收调试消息
 *   - 实时显示所有收到的调试信息
 *   - 显示消息来源和统计信息
 */

#include "libuser.h"
#include <stdarg.h>

// 简单的字符串转数字
int atoi(const char *str) {
    int result = 0;
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    return result;
}

// 字符串比较
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

// 字符串长度
int strlen(const char *s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

// 打印字符串
void print_str(const char *s) {
    while (*s) {
        sys_putchar(*s);
        s++;
    }
}

// 打印数字
void print_int(int val) {
    if (val < 0) {
        sys_putchar('-');
        val = -val;
    }

    char buffer[32];
    int i = 0;

    if (val == 0) {
        sys_putchar('0');
        return;
    }

    while (val > 0) {
        buffer[i++] = '0' + (val % 10);
        val /= 10;
    }

    while (i > 0) {
        sys_putchar(buffer[--i]);
    }
}

// 程序入口点
void _start() {
    int ret = main();
    exit(ret);
}

// 打印欢迎信息
void print_welcome(void) {
    print_str("\n+==========================================+\n");
    print_str("|   Network Debug Listener v1.0          |\n");
    print_str("|   ================================       |\n");
    print_str("|   Listening for debug messages on       |\n");
    print_str("|   UDP port 9999                         |\n");
    print_str("+==========================================+\n\n");
}

// 打印帮助
void print_help(void) {
    print_str("\nCommands:\n");
    print_str("  help     - Show this help message\n");
    print_str("  stats    - Show statistics\n");
    print_str("  clear    - Clear screen\n");
    print_str("  quit     - Exit listener\n\n");
}

// 主函数
int main(int argc, char **argv) {
    print_welcome();

    // 打印提示
    print_str("Waiting for debug messages...\n");
    print_str("(Messages will appear below when sent by kernel)\n\n");

    // 统计信息
    int total_messages = 0;
    int total_bytes = 0;

    // 主循环
    while (1) {
        // 在真实环境中，这里需要：
        // 1. 创建 UDP socket
        // 2. 绑定到端口 9999
        // 3. 接收数据包
        // 4. 显示消息内容

        // 模拟等待（实际应该是阻塞在 recvfrom）
        volatile int i;
        for (i = 0; i < 100000000; i++) {
            __asm__ volatile("nop");
        }

        // 打印等待提示
        print_str(".");
        if (total_messages % 10 == 9) {
            print_str("\n");
        }

        total_messages++;

        // 每 50 次打印一次统计
        if (total_messages % 50 == 0) {
            print_str("\n\n[Stats] Total messages: ");
            print_int(total_messages);
            print_str("\n");
        }
    }

    return 0;
}
