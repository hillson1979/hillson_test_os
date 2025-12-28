// 简单的Shell演示程序
extern int sys_printf(const char *fmt, ...);
extern void sys_exit(int code);
extern void sys_yield(void);

struct mem_stats {
    unsigned int total_pages;
    unsigned int free_pages;
    unsigned int used_pages;
};

extern int sys_get_mem_stats(struct mem_stats *stats);
extern int sys_read_mem(unsigned int addr, unsigned int *value);

// 辅助函数
void print_mem_stats() {
    struct mem_stats stats;
    int ret = sys_get_mem_stats(&stats);

    if (ret == 0) {
        sys_printf("=== 内存统计 ===\n");
        sys_printf("  总页数:   %u\n", stats.total_pages);
        sys_printf("  空闲页: %u\n", stats.free_pages);
        sys_printf("  已用页: %u\n", stats.used_pages);
        sys_printf("  总内存:   %u MB\n", (stats.total_pages * 4) / 1024);
        sys_printf("  空闲:    %u MB\n", (stats.free_pages * 4) / 1024);
        sys_printf("  已用:    %u MB\n", (stats.used_pages * 4) / 1024);
        sys_printf("================\n");
    } else {
        sys_printf("错误: 无法获取内存统计\n");
    }
}

void demo_read_memory() {
    unsigned int value;
    unsigned int test_addrs[] = {
        0xC0000000,  // 内核起始地址
        0xC0200000,  // 页表区域
        0xC1000000,  // Buddy数据区域
        0
    };

    sys_printf("=== 内存读取演示 ===\n");
    for (int i = 0; test_addrs[i] != 0; i++) {
        unsigned int addr = test_addrs[i];
        if (sys_read_mem(addr, &value) == 0) {
            sys_printf("[0x%x] = 0x%x\n", addr, value);
        }
    }
    sys_printf("=====================\n");
}

// 主函数 - Shell演示
void _start() {
    sys_printf("\n");
    sys_printf("========================================\n");
    sys_printf("  Simple Shell Demo v1.0\n");
    sys_printf("========================================\n");
    sys_printf("\n");

    // 演示1: 显示内存统计
    sys_printf("[演示1] 显示内存统计:\n");
    print_mem_stats();
    sys_printf("\n");

    // 演示2: 读取内存
    sys_printf("[演示2] 读取关键内存地址:\n");
    demo_read_memory();
    sys_printf("\n");

    // 演示3: 让出CPU
    sys_printf("[演示3] 让出CPU...\n");
    sys_yield();
    sys_printf("CPU已重新获得\n");
    sys_printf("\n");

    // 演示4: 再次查看内存状态
    sys_printf("[演示4] 再次查看内存统计:\n");
    print_mem_stats();
    sys_printf("\n");

    sys_printf("Shell演示完成!\n");
    sys_printf("\n");
    sys_printf("注意: 这是一个演示版本,实际shell需要键盘输入支持\n");
    sys_printf("未来可以添加的功能:\n");
    sys_printf("  - 键盘输入处理\n");
    sys_printf("  - 命令解析器\n");
    sys_printf("  - 更多命令(ls, cat, ps等)\n");
    sys_printf("  - 管道和重定向\n");
    sys_printf("\n");

    // 永不退出
    for(;;) {
        sys_yield();
    }
}
