// 显示 OK 的版本
extern void sys_putchar(char c);
extern void sys_exit(int code);
extern int sys_getchar(void);
extern int sys_get_mem_stats(void *stats);

// 全局命令缓冲区
char cmdline[128];
int cmdlen;

// 内存统计结构
struct mem_stats {
    int total_pages;
    int free_pages;
    int used_pages;
};

// 简单的数字转字符串（带单位）
void print_num(int pages) {
    if (pages == 0) {
        sys_putchar('0');
        sys_putchar(' ');
        sys_putchar('K');
        sys_putchar('B');
        return;
    }

    // 页数转换为字节 (1页 = 4KB)
    int bytes = pages * 4096;

    // 选择合适的单位
    char unit[3];
    int value;

    if (bytes >= 1073741824) {
        // GB
        value = bytes / 1073741824;
        unit[0] = 'G';
        unit[1] = 'B';
        unit[2] = 0;
    } else if (bytes >= 1048576) {
        // MB
        value = bytes / 1048576;
        unit[0] = 'M';
        unit[1] = 'B';
        unit[2] = 0;
    } else if (bytes >= 1024) {
        // KB
        value = bytes / 1024;
        unit[0] = 'K';
        unit[1] = 'B';
        unit[2] = 0;
    } else {
        // B
        value = bytes;
        unit[0] = 'B';
        unit[1] = 0;
    }

    // 转换数字为字符串
    char buf[16];
    int i = 0;
    if (value == 0) {
        buf[0] = '0';
        i = 1;
    } else {
        while (value > 0) {
            buf[i] = '0' + (value % 10);
            value = value / 10;
            i = i + 1;
        }
    }

    // 输出数字
    while (i > 0) {
        i = i - 1;
        sys_putchar(buf[i]);
    }

    // 输出单位
    i = 0;
    while (unit[i] != 0) {
        sys_putchar(unit[i]);
        i = i + 1;
    }
}

void _start() {
    while (1) {
        sys_putchar('>');
        sys_putchar(' ');

        // 读取命令
        cmdlen = 0;
        char c;
        while (cmdlen < 127) {
            c = sys_getchar();
            if (c == '\n') {
                sys_putchar('\n');
                break;
            }
            if (c >= 32 && c <= 126) {
                cmdline[cmdlen] = c;
                cmdlen = cmdlen + 1;
                sys_putchar(c);
            }
        }
        cmdline[cmdlen] = 0;

        // 解析命令 - 直接内联，避免函数调用

        // help 命令
        if (cmdlen == 4) {
            if (cmdline[0] == 'h' && cmdline[1] == 'e' && cmdline[2] == 'l' && cmdline[3] == 'p') {
                sys_putchar('h'); sys_putchar('e'); sys_putchar('l'); sys_putchar('p');
                sys_putchar('\n');
                continue;
            }
            // exit 命令
            if (cmdline[0] == 'e' && cmdline[1] == 'x' && cmdline[2] == 'i' && cmdline[3] == 't') {
                sys_exit(0);
            }
        }

        // mem 命令
        if (cmdlen == 3) {
            if (cmdline[0] == 'm' && cmdline[1] == 'e' && cmdline[2] == 'm') {
                // 调用内存统计系统调用
                struct mem_stats stats;
                sys_get_mem_stats(&stats);

                // 显示统计信息
                sys_putchar('T'); sys_putchar('o'); sys_putchar('t');
                sys_putchar('a'); sys_putchar('l'); sys_putchar(':');
                sys_putchar(' ');
                print_num(stats.total_pages);
                sys_putchar(' ');

                sys_putchar('F'); sys_putchar('r'); sys_putchar('e');
                sys_putchar('e'); sys_putchar(':');
                sys_putchar(' ');
                print_num(stats.free_pages);
                sys_putchar(' ');

                sys_putchar('U'); sys_putchar('s'); sys_putchar('e');
                sys_putchar('d'); sys_putchar(':');
                sys_putchar(' ');
                print_num(stats.used_pages);
                sys_putchar('\n');
                continue;
            }
        }

        // clear 命令 (清屏 - 换行25次)
        if (cmdlen == 5) {
            if (cmdline[0] == 'c' && cmdline[1] == 'l' && cmdline[2] == 'e' && cmdline[3] == 'a' && cmdline[4] == 'r') {
                sys_putchar('\n'); sys_putchar('\n'); sys_putchar('\n');
                sys_putchar('\n'); sys_putchar('\n'); sys_putchar('\n');
                sys_putchar('\n'); sys_putchar('\n'); sys_putchar('\n');
                sys_putchar('\n'); sys_putchar('\n'); sys_putchar('\n');
                sys_putchar('\n'); sys_putchar('\n'); sys_putchar('\n');
                sys_putchar('\n'); sys_putchar('\n'); sys_putchar('\n');
                sys_putchar('\n'); sys_putchar('\n'); sys_putchar('\n');
                sys_putchar('\n'); sys_putchar('\n'); sys_putchar('\n');
                sys_putchar('\n');
                continue;
            }
        }

        // echo 命令 - 简单回显
        if (cmdlen >= 5) {
            if (cmdline[0] == 'e' && cmdline[1] == 'c' && cmdline[2] == 'h' && cmdline[3] == 'o' && cmdline[4] == ' ') {
                sys_putchar('e'); sys_putchar('c'); sys_putchar('h'); sys_putchar('o'); sys_putchar(':');
                sys_putchar(' ');
                int i = 5;
                while (i < cmdlen) {
                    sys_putchar(cmdline[i]);
                    i = i + 1;
                }
                sys_putchar('\n');
                continue;
            }
        }

        // ver 命令 - 显示版本
        if (cmdlen == 3) {
            if (cmdline[0] == 'v' && cmdline[1] == 'e' && cmdline[2] == 'r') {
                sys_putchar('v'); sys_putchar('e'); sys_putchar('r'); sys_putchar(':');
                sys_putchar(' '); sys_putchar('1'); sys_putchar('.'); sys_putchar('0');
                sys_putchar('\n');
                continue;
            }
        }

        // 未知命令或空命令显示 OK
        if (cmdlen > 0) {
            sys_putchar('O');
            sys_putchar('K');
            sys_putchar('\n');
        }
    }
    sys_exit(0);
}
