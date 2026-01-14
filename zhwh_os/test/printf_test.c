// printf_test.c - 测试用户态 printf 和系统调用
#include "libuser.h"

void _start() {
    // 测试 1: 简单字符串输出
    printf("[printf_test] Test 1: Simple string output\n");

    // 测试 2: 整数输出
    printf("[printf_test] Test 2: Integer output: %d %d %d\n", 10, 20, 30);

    // 测试 3: 字符串格式化
    printf("[printf_test] Test 3: String formatting: %s %s\n", "Hello", "World");

    // 测试 4: 十六进制输出
    printf("[printf_test] Test 4: Hex output: 0x%x 0x%x 0x%x\n", 0x1234, 0xABCD, 0xDEAD);

    // 测试 5: 字符输出
    printf("[printf_test] Test 5: Char output: %c %c %c\n", 'A', 'B', 'C');

    // 测试 6: 混合格式
    printf("[printf_test] Test 6: Mixed format: PID=%d, value=0x%x, msg=%s\n", 1, 0x1234, "OK");

    // 测试 7: 负数
    printf("[printf_test] Test 7: Negative numbers: %d %d %d\n", -10, -20, -30);

    // 测试 8: 无符号整数
    printf("[printf_test] Test 8: Unsigned: %u %u %u\n", 10, 20, 30);

    // 测试 9: fork 测试
    printf("[printf_test] Test 9: Starting fork test...\n");

    int pid = fork();
    if (pid == 0) {
        // 子进程
        printf("[printf_test] Child process: PID=%d (should be 0)\n", pid);
        printf("[printf_test] Child: Exiting...\n");
        exit(0);
    } else {
        // 父进程
        printf("[printf_test] Parent process: Child PID=%d\n", pid);
        printf("[printf_test] Parent: Waiting...\n");
    }

    printf("[printf_test] All tests completed!\n");
    exit(0);
}
