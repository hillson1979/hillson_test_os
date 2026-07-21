/**
 * os_port.h — HillsonOS 移植层接口
 *
 * 封装所有 OS 相关的系统调用，为 JVM 提供统一的平台抽象。
 * JVM 核心代码通过此接口访问 OS 功能，不直接调用 syscall。
 */
#ifndef OS_PORT_H
#define OS_PORT_H

#include "stdint.h"

/* ================================================================
 * 内存管理
 * ================================================================ */

/** 分配 size 字节内存，返回指针（失败返回 NULL） */
void *os_malloc(uint32_t size);

/** 释放内存 */
void os_free(void *ptr);

/** 重新分配内存（类似 realloc） */
void *os_realloc(void *ptr, uint32_t new_size);

/** 获取当前已分配的堆内存总量 */
uint32_t os_heap_used(void);

/* ================================================================
 * 文件 I/O
 * ================================================================ */

/** 文件描述符类型 */
typedef uint32_t os_fd_t;

#define OS_FD_INVALID  0
#define OS_FD_STDIN    (0)
#define OS_FD_STDOUT   (1)
#define OS_FD_STDERR   (2)

/** 打开文件，返回文件描述符 */
os_fd_t os_file_open(const char *path, int flags);

/** 关闭文件 */
int os_file_close(os_fd_t fd);

/** 读取文件 */
int32_t os_file_read(os_fd_t fd, void *buf, uint32_t count);

/** 写入文件 */
int32_t os_file_write(os_fd_t fd, const void *buf, uint32_t count);

/** 移动文件读写位置 */
int32_t os_file_seek(os_fd_t fd, int32_t offset, int whence);

/** 获取文件大小 */
uint32_t os_file_size(os_fd_t fd);

/* ================================================================
 * 线程 / 任务
 * ================================================================ */

/** 线程 ID 类型 */
typedef int32_t os_thread_t;

/** 线程入口函数 */
typedef void (*os_thread_entry_t)(void *arg);

/** 创建新线程，返回线程 ID（失败返回 -1） */
os_thread_t os_thread_create(os_thread_entry_t entry, void *arg,
                             uint32_t stack_size);

/** 让出 CPU */
void os_thread_yield(void);

/** 当前线程休眠（毫秒） */
void os_thread_sleep(uint32_t ms);

/** 等待线程结束 */
int os_thread_join(os_thread_t tid);

/* ================================================================
 * 同步原语
 * ================================================================ */

/** 互斥锁（基于关闭中断或自旋锁实现） */
typedef struct {
    volatile int locked;
} os_mutex_t;

/** 初始化互斥锁 */
void os_mutex_init(os_mutex_t *mutex);

/** 加锁 */
void os_mutex_lock(os_mutex_t *mutex);

/** 解锁 */
void os_mutex_unlock(os_mutex_t *mutex);

/** 条件变量（简化实现） */
typedef struct {
    volatile int signaled;
} os_cond_t;

void os_cond_init(os_cond_t *cond);
void os_cond_wait(os_cond_t *cond, os_mutex_t *mutex);
void os_cond_signal(os_cond_t *cond);
void os_cond_broadcast(os_cond_t *cond);

/* ================================================================
 * 控制台 I/O
 * ================================================================ */

/** 输出一个字符 */
void os_putchar(char c);

/** 输出字符串 */
void os_print(const char *str);

/** 读取一个字符（阻塞） */
char os_getchar(void);

/* ================================================================
 * 时间和定时器
 * ================================================================ */

/** 获取系统启动以来的毫秒数 */
uint64_t os_time_ms(void);

/** 获取系统启动以来的秒数 */
uint32_t os_time_sec(void);

/* ================================================================
 * 网络（用于 java.net.*）
 * ================================================================ */

/** Socket 类型（简化） */
typedef int32_t os_socket_t;

#define OS_SOCKET_INVALID  (-1)

os_socket_t os_socket_udp(void);
int os_socket_bind(os_socket_t sock, uint16_t port);
int32_t os_socket_recv(os_socket_t sock, void *buf, uint32_t len);
int os_socket_sendto(os_socket_t sock, const void *buf, uint32_t len,
                     uint32_t ip, uint16_t port);
int os_socket_close(os_socket_t sock);

/* ================================================================
 * 图形帧缓冲（用于 java.awt.* / 图形输出）
 * ================================================================ */

typedef struct {
    void    *fb_base;       /* 帧缓冲区基地址 */
    uint32_t fb_size;       /* 缓冲区字节数 */
    uint32_t width;         /* 屏幕宽度 */
    uint32_t height;        /* 屏幕高度 */
    uint32_t bpp;           /* 每像素位数 */
    uint32_t pitch;         /* 每行字节数 */
} os_fb_info_t;

int os_fb_get_info(os_fb_info_t *info);

/** 将一个矩形区域的像素数据写入帧缓冲区 */
int os_fb_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
               const void *pixels);

/* ================================================================
 * 系统
 * ================================================================ */

/** 退出进程 */
void os_exit(int code);

/** 获取环境变量（返回 NULL 表示未设置） */
const char *os_getenv(const char *name);

#endif /* OS_PORT_H */
