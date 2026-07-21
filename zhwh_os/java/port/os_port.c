/**
 * os_port.c — HillsonOS 移植层实现
 *
 * 将 os_port.h 接口映射到 HillsonOS libuser 提供的函数。
 *
 * ⚠️ 函数名映射（libuser 中的实际名称）:
 *   open / close / read / write / lseek  — 文件 I/O
 *   sys_getchar / sys_putchar             — 控制台 I/O
 *   exit / yield                          — 进程控制
 *   net_recv_udp                          — 网络
 *
 * ⚠️ libuser 不提供 malloc/free，这里使用内置的简单 bump 分配器。
 */
#include "os_port.h"
#include "stdint.h"

/* ================================================================
 * libuser 函数声明（这些在 libuser.c 中实现）
 * ================================================================ */
extern int open(const char *pathname, int flags);
extern int close(int fd);
extern int read(int fd, char *buf, int len);
extern int write(int fd, const char *buf, int len);
extern int lseek(int fd, int offset, int whence);
extern void exit(int code) __attribute__((noreturn));
extern void yield(void);
extern int sys_getchar(void);
extern void sys_putchar(char c);
extern int net_recv_udp(char *buf, int len);
extern int getcwd(char *buf, int size);

/* ================================================================
 * 内置堆分配器（简易 bump allocator）
 *
 * libuser 没有 malloc/free，这里使用静态缓冲区实现。
 * 对于 JVM，后续可以扩展为从内核获取更大的堆区域。
 * ================================================================ */

#define HEAP_SIZE  (4 * 1024 * 1024)  /* 4MB 静态堆 */
static char heap[HEAP_SIZE];
static uint32_t heap_offset = 0;

/* 分配块头部（用于 free 和 realloc） */
typedef struct {
    uint32_t size;    /* 块大小（含头部） */
    uint32_t magic;   /* 魔数：0x4A564D = "JVM" */
} heap_header_t;

#define HEAP_MAGIC  0x4A564D

void *os_malloc(uint32_t size) {
    if (size == 0) return NULL;
    /* 对齐到 8 字节 */
    size = (size + 7) & ~7;
    uint32_t total = size + sizeof(heap_header_t);

    if (heap_offset + total > HEAP_SIZE) {
        /* 堆耗尽 */
        return NULL;
    }

    heap_header_t *hdr = (heap_header_t *)(heap + heap_offset);
    hdr->size = total;
    hdr->magic = HEAP_MAGIC;
    heap_offset += total;

    return (void *)(hdr + 1);
}

void os_free(void *ptr) {
    /* bump allocator 不支持释放单个块 */
    /* TODO: 后续改用空闲链表或从内核分配 */
    (void)ptr;
}

void *os_realloc(void *ptr, uint32_t new_size) {
    if (!ptr) return os_malloc(new_size);
    if (new_size == 0) { os_free(ptr); return NULL; }

    new_size = (new_size + 7) & ~7;
    uint32_t new_total = new_size + sizeof(heap_header_t);

    /* 检查是否在堆末尾（可原地扩展） */
    heap_header_t *hdr = ((heap_header_t *)ptr) - 1;
    if (hdr->magic != HEAP_MAGIC) return NULL;

    char *block_end = ((char *)ptr) + hdr->size - sizeof(heap_header_t);
    if (block_end == heap + heap_offset) {
        /* 是最后一个块，可以扩展 */
        uint32_t extra = new_total - hdr->size;
        if (heap_offset + extra > HEAP_SIZE) return NULL;
        hdr->size = new_total;
        heap_offset += extra;
        return ptr;
    }

    /* 非末尾块，分配新块并拷贝 */
    void *new_ptr = os_malloc(new_size);
    if (!new_ptr) return NULL;

    uint32_t copy_size = hdr->size - sizeof(heap_header_t);
    if (copy_size > new_size) copy_size = new_size;
    char *src = (char *)ptr;
    char *dst = (char *)new_ptr;
    for (uint32_t i = 0; i < copy_size; i++) {
        dst[i] = src[i];
    }
    return new_ptr;
}

uint32_t os_heap_used(void) {
    return heap_offset;
}

/* ================================================================
 * 文件 I/O — 映射到 libuser 的 open/close/read/write/lseek
 * ================================================================ */

os_fd_t os_file_open(const char *path, int flags) {
    int fd = open(path, flags);
    return (os_fd_t)fd;
}

int os_file_close(os_fd_t fd) {
    return close((int)fd);
}

int32_t os_file_read(os_fd_t fd, void *buf, uint32_t count) {
    return (int32_t)read((int)fd, (char *)buf, (int)count);
}

int32_t os_file_write(os_fd_t fd, const void *buf, uint32_t count) {
    return (int32_t)write((int)fd, (const char *)buf, (int)count);
}

int32_t os_file_seek(os_fd_t fd, int32_t offset, int whence) {
    return (int32_t)lseek((int)fd, (int)offset, whence);
}

uint32_t os_file_size(os_fd_t fd) {
    int32_t cur = lseek((int)fd, 0, 1);   /* SEEK_CUR */
    if (cur < 0) return 0;
    int32_t end = lseek((int)fd, 0, 2);   /* SEEK_END */
    lseek((int)fd, cur, 0);               /* 恢复 */
    return (end > 0) ? (uint32_t)end : 0;
}

/* ================================================================
 * 线程 / 任务
 * ================================================================ */

os_thread_t os_thread_create(os_thread_entry_t entry, void *arg,
                             uint32_t stack_size) {
    /* TODO: 需要内核 task 创建接口 */
    (void)entry; (void)arg; (void)stack_size;
    return -1;
}

void os_thread_yield(void) {
    yield();
}

void os_thread_sleep(uint32_t ms) {
    /* 忙等待 */
    volatile uint32_t count = ms * 5000;
    while (count--) {
        __asm__ volatile("nop");
    }
}

int os_thread_join(os_thread_t tid) {
    (void)tid;
    return -1;
}

/* ================================================================
 * 同步原语（单核环境 — 自旋锁 + cli）
 * ================================================================ */

void os_mutex_init(os_mutex_t *mutex) {
    mutex->locked = 0;
}

void os_mutex_lock(os_mutex_t *mutex) {
    while (1) {
        int expected = 0;
        int result;
        __asm__ volatile(
            "lock cmpxchgl %2, %1\n"
            "movl %%eax, %0\n"
            : "=r"(result), "+m"(mutex->locked)
            : "r"(1), "a"(expected)
            : "memory"
        );
        if (result == 0) break;
        os_thread_yield();
    }
}

void os_mutex_unlock(os_mutex_t *mutex) {
    __asm__ volatile(
        "movl $0, %0\n"
        : "=m"(mutex->locked)
        :
        : "memory"
    );
}

void os_cond_init(os_cond_t *cond) {
    cond->signaled = 0;
}

void os_cond_wait(os_cond_t *cond, os_mutex_t *mutex) {
    os_mutex_unlock(mutex);
    while (!cond->signaled) {
        os_thread_yield();
    }
    cond->signaled = 0;
    os_mutex_lock(mutex);
}

void os_cond_signal(os_cond_t *cond) {
    cond->signaled = 1;
}

void os_cond_broadcast(os_cond_t *cond) {
    cond->signaled = 1;
}

/* ================================================================
 * 控制台 I/O
 * ================================================================ */

void os_putchar(char c) {
    sys_putchar(c);
}

void os_print(const char *str) {
    while (*str) {
        os_putchar(*str++);
    }
}

char os_getchar(void) {
    return (char)sys_getchar();
}

/* ================================================================
 * 时间
 * ================================================================ */

uint64_t os_time_ms(void) {
    /* TODO: 需要内核时间系统调用 */
    return 0;
}

uint32_t os_time_sec(void) {
    /* 避免 64 位除法 (会引入 __udivdi3 依赖) */
    uint64_t ms = os_time_ms();
    uint32_t sec = 0;
    while (ms >= 1000) {
        ms -= 1000;
        sec++;
    }
    return sec;
}

/* ================================================================
 * 网络
 * ================================================================ */

os_socket_t os_socket_udp(void) {
    return OS_SOCKET_INVALID;
}

int os_socket_bind(os_socket_t sock, uint16_t port) {
    (void)sock; (void)port;
    return -1;
}

int32_t os_socket_recv(os_socket_t sock, void *buf, uint32_t len) {
    (void)sock;
    return (int32_t)net_recv_udp((char *)buf, (int)len);
}

int os_socket_sendto(os_socket_t sock, const void *buf, uint32_t len,
                     uint32_t ip, uint16_t port) {
    (void)sock; (void)buf; (void)len; (void)ip; (void)port;
    return -1;
}

int os_socket_close(os_socket_t sock) {
    (void)sock;
    return 0;
}

/* ================================================================
 * 图形
 * ================================================================ */

int os_fb_get_info(os_fb_info_t *info) {
    (void)info;
    return -1;
}

int os_fb_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
               const void *pixels) {
    (void)x; (void)y; (void)w; (void)h; (void)pixels;
    return -1;
}

/* ================================================================
 * 系统
 * ================================================================ */

void os_exit(int code) {
    exit(code);
}

const char *os_getenv(const char *name) {
    (void)name;
    return NULL;
}
