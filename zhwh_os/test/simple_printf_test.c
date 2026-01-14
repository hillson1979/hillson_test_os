// simple_printf_test.c - 最简单的 printf 测试
#include "libuser.h"

void _start() {
    printf("Test 1\n");
    printf("Test 2: %d\n", 42);
    printf("Test 3: %s %d\n", "value", 123);
    printf("All tests passed!\n");
    exit(0);
}
