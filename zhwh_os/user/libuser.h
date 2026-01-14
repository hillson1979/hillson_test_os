// libuser.h - 用户库头文件
#ifndef LIBUSER_H
#define LIBUSER_H

#include "stdint.h"

// 系统调用号
#define SYS_PRINTF 1
#define SYS_EXIT 2
#define SYS_YIELD 3
#define SYS_WRITE 10
#define SYS_FORK 11

// 系统调用包装宏
#define syscall1(num, arg1) \
    __asm__ volatile( \
        "movl $" #num ", %%eax\n" \
        "movl %0, %%ebx\n" \
        "int $0x80\n" \
        : \
        : "r"((uint32_t)(arg1)) \
        : "eax", "ebx", "memory" \
    )

#define syscall3(num, arg1, arg2, arg3) \
    __asm__ volatile( \
        "movl $" #num ", %%eax\n" \
        "movl %0, %%ebx\n" \
        "movl %1, %%ecx\n" \
        "movl %2, %%edx\n" \
        "int $0x80\n" \
        : \
        : "r"((uint32_t)(arg1)), "r"((uint32_t)(arg2)), "r"((uint32_t)(arg3)) \
        : "eax", "ebx", "ecx", "edx", "memory" \
    )

// printf 函数
int printf(const char *fmt, ...);

// write 系统调用
int write(int fd, const char *buf, int len);

// fork 系统调用
int fork(void);

// exit 系统调用
void exit(int code) __attribute__((noreturn));

// yield - 让出CPU
void yield(void);

#endif // LIBUSER_H
