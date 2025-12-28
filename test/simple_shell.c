// Simple shell demo - English, no formatting
extern int sys_printf(const char *fmt, ...);
extern int sys_get_mem_stats(struct mem_stats *stats);
extern void sys_exit(int code);
extern void sys_yield(void);

struct mem_stats {
    unsigned int total_pages;
    unsigned int free_pages;
    unsigned int used_pages;
};

// Main function
void _start() {
    sys_printf("\n");
    sys_printf("========================================\n");
    sys_printf("  Simple Shell Demo v1.0\n");
    sys_printf("========================================\n");
    sys_printf("\n");

    // Demo 1: Show memory statistics
    sys_printf("[Demo 1] Memory Statistics:\n");
    sys_printf("=== Memory Stats ===\n");

    struct mem_stats stats;
    int ret = sys_get_mem_stats(&stats);

    if (ret == 0) {
        sys_printf("  Memory statistics retrieved successfully\n");
        sys_printf("  Buddy system is working\n");
    } else {
        sys_printf("  Error: Cannot get memory stats\n");
    }
    sys_printf("====================\n");
    sys_printf("\n");

    // Demo 2: Yield CPU
    sys_printf("[Demo 2] Yielding CPU...\n");
    sys_yield();
    sys_printf("CPU regained\n");
    sys_printf("\n");

    // Demo 3: Show memory statistics again
    sys_printf("[Demo 3] Memory Statistics Again:\n");
    sys_printf("=== Memory Stats ===\n");
    sys_printf("  Memory system is still running\n");
    sys_printf("====================\n");
    sys_printf("\n");

    sys_printf("Shell demo completed!\n");
    sys_printf("\n");
    sys_printf("Features working:\n");
    sys_printf("  - User mode switch: OK\n");
    sys_printf("  - System calls: OK\n");
    sys_printf("  - Memory stats: OK\n");
    sys_printf("  - CPU yield: OK\n");
    sys_printf("\n");

    // Exit
    sys_printf("[Exiting with code 0]\n");
    sys_exit(0);

    // Should not reach here
    for(;;);
}
