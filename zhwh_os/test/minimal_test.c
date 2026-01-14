// minimal_test.c - 最小化测试程序
#include "libuser.h"

void _start() {
    // 最简单的测试：直接调用 exit，不调用 printf
    exit(42);  // 使用特殊值 42，方便识别
}
