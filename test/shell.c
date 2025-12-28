// 简单的Shell实现
extern int sys_printf(const char *fmt, ...);
extern int sys_scanf(const char *fmt, ...);
extern void sys_exit(int code);
extern void sys_yield(void);

// 系统调用: 读取内存
extern int sys_read_mem(unsigned int addr, unsigned int *value);

// 系统调用: 获取内存统计
struct mem_stats {
    unsigned int total_pages;
    unsigned int free_pages;
    unsigned int used_pages;
};
extern int sys_get_mem_stats(struct mem_stats *stats);

// 缓冲区大小
#define BUF_SIZE 128
#define MAX_ARGS 10

// 辅助函数: 字符串长度
int strlen(const char *s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

// 辅助函数: 字符串比较
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

// 辅助函数: 字符串转整数
int atoi(const char *s) {
    int result = 0;
    int sign = 1;

    if (*s == '-') {
        sign = -1;
        s++;
    }

    while (*s >= '0' && *s <= '9') {
        result = result * 10 + (*s - '0');
        s++;
    }

    return sign * result;
}

// 辅助函数: 内存转十六进制字符串
void to_hex_str(unsigned int value, char *buf) {
    const char *hex = "0123456789ABCDEF";
    int i;

    buf[0] = '0';
    buf[1] = 'x';

    for (i = 0; i < 8; i++) {
        buf[9 - i] = hex[value & 0xF];
        value >>= 4;
    }
    buf[10] = '\0';
}

// Shell命令: 显示帮助
void cmd_help() {
    sys_printf("可用命令:\n");
    sys_printf("  help          - 显示此帮助信息\n");
    sys_printf("  echo <text>   - 回显文本\n");
    sys_printf("  mem           - 显示内存统计\n");
    sys_printf("  read <addr>   - 读取内存地址(十六进制)\n");
    sys_printf("  cls           - 清屏\n");
    sys_printf("  exit          - 退出shell\n");
    sys_printf("  yield         - 让出CPU\n");
}

// Shell命令: 回显文本
void cmd_echo(int argc, char *argv[]) {
    if (argc < 2) {
        sys_printf("\n");
        return;
    }

    for (int i = 1; i < argc; i++) {
        sys_printf("%s", argv[i]);
        if (i < argc - 1) sys_printf(" ");
    }
    sys_printf("\n");
}

// Shell命令: 显示内存统计
void cmd_mem() {
    struct mem_stats stats;
    int ret = sys_get_mem_stats(&stats);

    if (ret == 0) {
        sys_printf("=== 内存统计 ===\n");
        sys_printf("  总页数:   %u (%u MB)\n", stats.total_pages,
                   (stats.total_pages * 4) / 1024);
        sys_printf("  空闲页: %u (%u MB)\n", stats.free_pages,
                   (stats.free_pages * 4) / 1024);
        sys_printf("  已用页: %u (%u MB)\n", stats.used_pages,
                   (stats.used_pages * 4) / 1024);
        sys_printf("================\n");
    } else {
        sys_printf("错误: 无法获取内存统计\n");
    }
}

// Shell命令: 读取内存
void cmd_read(int argc, char *argv[]) {
    unsigned int addr;
    unsigned int value;
    char hex_buf[16];

    if (argc < 2) {
        sys_printf("用法: read <地址>\n");
        sys_printf("示例: read 0xC0000000\n");
        return;
    }

    // 简单的十六进制解析 (0x前缀)
    addr = 0;
    char *s = argv[1];

    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
        while (*s) {
            addr <<= 4;
            if (*s >= '0' && *s <= '9')
                addr |= *s - '0';
            else if (*s >= 'A' && *s <= 'F')
                addr |= *s - 'A' + 10;
            else if (*s >= 'a' && *s <= 'f')
                addr |= *s - 'a' + 10;
            s++;
        }
    } else {
        addr = atoi(argv[1]);
    }

    ret = sys_read_mem(addr, &value);
    if (ret == 0) {
        to_hex_str(value, hex_buf);
        sys_printf("[0x%x] = %s (%u)\n", addr, hex_buf, value);
    } else {
        sys_printf("错误: 无法读取地址 0x%x\n", addr);
    }
}

// Shell命令: 清屏
void cmd_cls() {
    // 打印30个换行符来清屏
    for (int i = 0; i < 30; i++) {
        sys_printf("\n");
    }
}

// 简单的命令解析
void parse_command(char *cmd_line, int *argc, char *argv[]) {
    *argc = 0;
    char *p = cmd_line;

    while (*p && *p == ' ') p++;  // 跳过前导空格

    while (*p && *argc < MAX_ARGS) {
        argv[(*argc)++] = p;

        while (*p && *p != ' ') p++;  // 找到参数结尾
        if (*p) {
            *p++ = '\0';  // 替换空格为字符串结束符
            while (*p && *p == ' ') p++;  // 跳过空格
        }
    }
}

// 主循环
void shell_main() {
    char buf[BUF_SIZE];
    int argc;
    char *argv[MAX_ARGS];
    int running = 1;

    cmd_cls();
    sys_printf("========================================\n");
    sys_printf("  Simple Shell v1.0\n");
    sys_printf("  输入 'help' 查看可用命令\n");
    sys_printf("========================================\n");
    sys_printf("\n");

    while (running) {
        sys_printf("shell> ");

        // 读取命令 (这里简化处理,实际应该有系统调用)
        int i = 0;
        char ch;
        // 模拟读取输入
        // TODO: 需要实现真正的输入系统调用
        for (i = 0; i < BUF_SIZE - 1; i++) {
            // 简化: 假设已经读取到buf
            // 实际需要从键盘读取
            break;
        }
        buf[i] = '\0';

        // 如果没有输入,继续
        if (i == 0) {
            sys_yield();
            continue;
        }

        // 解析命令
        parse_command(buf, &argc, argv);

        if (argc == 0) continue;

        // 执行命令
        if (strcmp(argv[0], "help") == 0 || strcmp(argv[0], "?") == 0) {
            cmd_help();
        } else if (strcmp(argv[0], "echo") == 0) {
            cmd_echo(argc, argv);
        } else if (strcmp(argv[0], "mem") == 0) {
            cmd_mem();
        } else if (strcmp(argv[0], "read") == 0) {
            cmd_read(argc, argv);
        } else if (strcmp(argv[0], "cls") == 0 || strcmp(argv[0], "clear") == 0) {
            cmd_cls();
        } else if (strcmp(argv[0], "exit") == 0 || strcmp(argv[0], "quit") == 0) {
            sys_printf("退出shell...\n");
            running = 0;
        } else if (strcmp(argv[0], "yield") == 0) {
            sys_printf("让出CPU...\n");
            sys_yield();
        } else {
            sys_printf("未知命令: %s (输入 'help' 查看帮助)\n", argv[0]);
        }

        sys_printf("\n");
    }

    sys_printf("Shell结束.\n");
}

// 入口函数
void _start() {
    shell_main();
    sys_exit(0);
}
