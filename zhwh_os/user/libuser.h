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
#define SYS_OPEN 20
#define SYS_CLOSE 21
#define SYS_READ 22
#define SYS_LSEEK 23

// 文件打开标志
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 0100
#define O_TRUNC 01000
#define O_APPEND 02000

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

// 文件系统系统调用
int open(const char *pathname, int flags);
int close(int fd);
int read(int fd, char *buf, int len);
int lseek(int fd, int offset, int whence);

// 工作目录系统调用
int getcwd(char *buf, int size);

#endif // LIBUSER_H
