/**
 * @file netdebug.c
 * @brief 以太网调试接口 - 通过 UDP 发送内核调试信息
 *
 * 功能：
 * - 通过 UDP 广播调试消息到网络
 * - 支持远程日志查看
 * - 可选的过滤和级别控制
 * - 循环缓冲区避免消息丢失
 */

#include "types.h"
#include <stdarg.h>
#include "net.h"
#include "../include/printf.h"
#include "../include/string.h"
#include "../include/kmalloc.h"

// ==================== 配置 ====================

// 网络调试端口
#define NETDEBUG_PORT  9999    // UDP 端口

// 缓冲区配置
#define NETDEBUG_BUF_SIZE  2048    // 单条消息最大长度
#define NETDEBUG_MSG_QUEUE_SIZE 32  // 消息队列大小

// 调试级别
typedef enum {
    NETDEBUG_LEVEL_ERROR = 0,
    NETDEBUG_LEVEL_WARN  = 1,
    NETDEBUG_LEVEL_INFO  = 2,
    NETDEBUG_LEVEL_DEBUG = 3,
} netdebug_level_t;

// ==================== 数据结构 ====================

/**
 * @brief 调试消息结构
 */
typedef struct {
    int level;                  // 消息级别
    char message[NETDEBUG_BUF_SIZE];  // 消息内容
    uint32_t len;               // 消息长度
    uint32_t timestamp;         // 时间戳（可选）
} netdebug_msg_t;

/**
 * @brief 网络调试上下文
 */
typedef struct {
    int enabled;                // 是否启用
    int min_level;              // 最低输出级别
    uint32_t dest_ip;           // 目标 IP 地址（0 = 广播）
    uint16_t dest_port;         // 目标端口

    // 消息队列
    netdebug_msg_t msg_queue[NETDEBUG_MSG_QUEUE_SIZE];
    int queue_head;             // 队列头
    int queue_tail;             // 队列尾
    int queue_count;            // 队列中消息数

    // 统计信息
    uint32_t total_sent;        // 总发送消息数
    uint32_t total_dropped;     // 丢弃消息数
    uint32_t total_bytes;       // 总发送字节数
} netdebug_context_t;

// ==================== 全局变量 ====================

static netdebug_context_t netdebug_ctx = {
    .enabled = 0,               // 默认禁用
    .min_level = NETDEBUG_LEVEL_INFO,
    .dest_ip = 0xFFFFFFFF,      // 默认广播到 255.255.255.255
    .dest_port = NETDEBUG_PORT,
    .queue_head = 0,
    .queue_tail = 0,
    .queue_count = 0,
    .total_sent = 0,
    .total_dropped = 0,
    .total_bytes = 0,
};

// 网络设备指针（延迟初始化）
static net_device_t *netdebug_dev = NULL;

// ==================== 内部辅助函数 ====================

/**
 * @brief 格式化输出到缓冲区（简化的 vsnprintf）
 */
static int netdebug_vsnprintf(char *buf, int size, const char *fmt, va_list args) {
    int len = 0;

    while (*fmt && len < size - 1) {
        if (*fmt == '%') {
            fmt++;
            if (*fmt == '\0') break;

            // 简单的格式化支持
            if (*fmt == 's') {
                // 字符串
                const char *s = va_arg(args, const char*);
                if (s) {
                    while (*s && len < size - 1) {
                        buf[len++] = *s++;
                    }
                }
            } else if (*fmt == 'd' || *fmt == 'u') {
                // 十进制整数
                int val = va_arg(args, int);
                char tmp[32];
                int i = 0;

                if (val == 0) {
                    buf[len++] = '0';
                } else {
                    if (val < 0) {
                        buf[len++] = '-';
                        val = -val;
                    }
                    while (val > 0 && i < 31) {
                        tmp[i++] = '0' + (val % 10);
                        val /= 10;
                    }
                    while (i > 0 && len < size - 1) {
                        buf[len++] = tmp[--i];
                    }
                }
            } else if (*fmt == 'x' || *fmt == 'X') {
                // 十六进制整数
                uint32_t val = va_arg(args, uint32_t);
                char tmp[16];
                int i = 0;

                if (val == 0) {
                    buf[len++] = '0';
                } else {
                    while (val > 0 && i < 8) {
                        int digit = val & 0xF;
                        tmp[i++] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
                        val >>= 4;
                    }
                    while (i > 0 && len < size - 1) {
                        buf[len++] = tmp[--i];
                    }
                }
            } else if (*fmt == 'c') {
                // 字符
                char c = (char)va_arg(args, int);
                buf[len++] = c;
            } else if (*fmt == '%') {
                // %% 输出 %
                buf[len++] = '%';
            }

            fmt++;
        } else {
            buf[len++] = *fmt++;
        }
    }

    buf[len] = '\0';
    return len;
}

/**
 * @brief 添加消息到队列
 */
static int netdebug_enqueue(int level, const char *message, uint32_t len) {
    if (!netdebug_ctx.enabled) {
        return -1;
    }

    // 检查级别过滤
    if (level < netdebug_ctx.min_level) {
        return -1;
    }

    // 检查队列是否已满
    if (netdebug_ctx.queue_count >= NETDEBUG_MSG_QUEUE_SIZE) {
        netdebug_ctx.total_dropped++;
        return -1;
    }

    // 添加消息到队列
    netdebug_msg_t *msg = &netdebug_ctx.msg_queue[netdebug_ctx.queue_tail];
    msg->level = level;
    msg->len = (len > NETDEBUG_BUF_SIZE - 1) ? NETDEBUG_BUF_SIZE - 1 : len;
    memcpy(msg->message, message, msg->len);
    msg->message[msg->len] = '\0';

    // 更新队列指针
    netdebug_ctx.queue_tail = (netdebug_ctx.queue_tail + 1) % NETDEBUG_MSG_QUEUE_SIZE;
    netdebug_ctx.queue_count++;

    return 0;
}

/**
 * @brief 发送队列中的一条消息
 */
static int netdebug_send_one(netdebug_msg_t *msg) {
    if (!netdebug_dev) {
        // 尝试获取默认网络设备
        netdebug_dev = net_device_get_default();
        if (!netdebug_dev) {
            return -1;
        }
    }

    // 发送 UDP 数据包
    uint32_t dest_ip = (netdebug_ctx.dest_ip == 0xFFFFFFFF) ?
                       0xFFFFFFFF :  // 广播
                       netdebug_ctx.dest_ip;

    int ret = udp_output(netdebug_dev, dest_ip,
                         NETDEBUG_PORT,  // 源端口
                         netdebug_ctx.dest_port,  // 目标端口
                         (uint8_t*)msg->message,
                         msg->len);

    if (ret >= 0) {
        netdebug_ctx.total_sent++;
        netdebug_ctx.total_bytes += msg->len;
    }

    return ret;
}

/**
 * @brief 刷新消息队列（发送所有挂起的消息）
 */
static void netdebug_flush_queue(void) {
    while (netdebug_ctx.queue_count > 0) {
        netdebug_msg_t *msg = &netdebug_ctx.msg_queue[netdebug_ctx.queue_head];
        netdebug_send_one(msg);

        // 移动队列头
        netdebug_ctx.queue_head = (netdebug_ctx.queue_head + 1) % NETDEBUG_MSG_QUEUE_SIZE;
        netdebug_ctx.queue_count--;
    }
}

// ==================== 公共 API ====================

/**
 * @brief 初始化网络调试接口
 */
int netdebug_init(void) {
    printf("[netdebug] Initializing network debug interface...\n");

    // 初始化上下文
    memset(&netdebug_ctx, 0, sizeof(netdebug_ctx));
    netdebug_ctx.enabled = 0;  // 默认禁用，需要手动启用
    netdebug_ctx.min_level = NETDEBUG_LEVEL_INFO;
    netdebug_ctx.dest_ip = 0xFFFFFFFF;  // 默认广播
    netdebug_ctx.dest_port = NETDEBUG_PORT;

    printf("[netdebug] Network debug interface initialized\n");
    printf("[netdebug]   Default destination: %d.%d.%d.%d:%d\n",
           (netdebug_ctx.dest_ip >> 24) & 0xFF,
           (netdebug_ctx.dest_ip >> 16) & 0xFF,
           (netdebug_ctx.dest_ip >> 8) & 0xFF,
           netdebug_ctx.dest_ip & 0xFF,
           netdebug_ctx.dest_port);

    return 0;
}

/**
 * @brief 启用/禁用网络调试
 */
void netdebug_enable(int enable) {
    netdebug_ctx.enabled = enable;
    printf("[netdebug] Debug output %s\n", enable ? "enabled" : "disabled");
}

/**
 * @brief 设置调试级别
 */
void netdebug_set_level(int level) {
    if (level >= NETDEBUG_LEVEL_ERROR && level <= NETDEBUG_LEVEL_DEBUG) {
        netdebug_ctx.min_level = level;
    }
}

/**
 * @brief 设置目标 IP 地址
 */
void netdebug_set_dest(uint32_t ip, uint16_t port) {
    netdebug_ctx.dest_ip = ip;
    netdebug_ctx.dest_port = port;
}

/**
 * @brief 核心调试输出函数（类似 printf）
 */
void netdebug_printf(int level, const char *fmt, ...) {
    if (!netdebug_ctx.enabled) {
        return;
    }

    // 级别过滤
    if (level < netdebug_ctx.min_level) {
        return;
    }

    // 格式化消息
    char buffer[NETDEBUG_BUF_SIZE];
    va_list args;
    va_start(args, fmt);
    int len = netdebug_vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    // 添加到队列
    netdebug_enqueue(level, buffer, len);

    // 立即刷新队列
    netdebug_flush_queue();
}

/**
 * @brief 打印调试统计信息
 */
void netdebug_stats(void) {
    printf("[netdebug] === Statistics ===\n");
    printf("[netdebug]   Enabled: %s\n", netdebug_ctx.enabled ? "Yes" : "No");
    printf("[netdebug]   Level: %d\n", netdebug_ctx.min_level);
    printf("[netdebug]   Destination: %d.%d.%d.%d:%d\n",
           (netdebug_ctx.dest_ip >> 24) & 0xFF,
           (netdebug_ctx.dest_ip >> 16) & 0xFF,
           (netdebug_ctx.dest_ip >> 8) & 0xFF,
           netdebug_ctx.dest_ip & 0xFF,
           netdebug_ctx.dest_port);
    printf("[netdebug]   Total sent: %u\n", netdebug_ctx.total_sent);
    printf("[netdebug]   Total dropped: %u\n", netdebug_ctx.total_dropped);
    printf("[netdebug]   Total bytes: %u\n", netdebug_ctx.total_bytes);
    printf("[netdebug]   Queue depth: %d/%d\n",
           netdebug_ctx.queue_count, NETDEBUG_MSG_QUEUE_SIZE);
}

// ==================== 便捷 API ====================

/**
 * @brief ERROR 级别输出
 */
void netdebug_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buffer[NETDEBUG_BUF_SIZE];
    int len = netdebug_vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    netdebug_enqueue(NETDEBUG_LEVEL_ERROR, buffer, len);
    netdebug_flush_queue();
}

/**
 * @brief WARN 级别输出
 */
void netdebug_warn(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buffer[NETDEBUG_BUF_SIZE];
    int len = netdebug_vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    netdebug_enqueue(NETDEBUG_LEVEL_WARN, buffer, len);
    netdebug_flush_queue();
}

/**
 * @brief INFO 级别输出
 */
void netdebug_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buffer[NETDEBUG_BUF_SIZE];
    int len = netdebug_vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    netdebug_enqueue(NETDEBUG_LEVEL_INFO, buffer, len);
    netdebug_flush_queue();
}

/**
 * @brief DEBUG 级别输出
 */
void netdebug_debug(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buffer[NETDEBUG_BUF_SIZE];
    int len = netdebug_vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    netdebug_enqueue(NETDEBUG_LEVEL_DEBUG, buffer, len);
    netdebug_flush_queue();
}

/**
 * @brief 内存转储（十六进制）
 */
void netdebug_dump(const void *ptr, uint32_t len) {
    if (!netdebug_ctx.enabled) {
        return;
    }

    const uint8_t *data = (const uint8_t*)ptr;
    const char *hex_chars = "0123456789ABCDEF";
    char line[128];

    for (uint32_t i = 0; i < len; i += 16) {
        int offset = 0;

        // 地址
        uint32_t addr = i;
        for (int shift = 28; shift >= 0; shift -= 4) {
            line[offset++] = hex_chars[(addr >> shift) & 0xF];
        }
        line[offset++] = ':';
        line[offset++] = ' ';

        // 十六进制
        for (uint32_t j = 0; j < 16; j++) {
            if (i + j < len) {
                uint8_t b = data[i + j];
                line[offset++] = hex_chars[(b >> 4) & 0xF];
                line[offset++] = hex_chars[b & 0xF];
                line[offset++] = ' ';
            } else {
                line[offset++] = ' ';
                line[offset++] = ' ';
                line[offset++] = ' ';
            }
        }

        // ASCII
        line[offset++] = ' ';
        line[offset++] = '|';
        line[offset++] = ' ';
        for (uint32_t j = 0; j < 16 && i + j < len; j++) {
            char c = data[i + j];
            line[offset++] = (c >= 32 && c < 127) ? c : '.';
        }
        line[offset] = '\0';

        // 发送这一行
        netdebug_enqueue(NETDEBUG_LEVEL_DEBUG, line, offset);
    }

    netdebug_flush_queue();
}

/**
 * @brief 致命错误输出并停机
 */
void netdebug_panic(const char *msg) {
    netdebug_error("[PANIC] %s", msg);

    // 刷新队列
    netdebug_flush_queue();

    // 停机
    printf("[netdebug] PANIC: %s\n", msg);
    __asm__ volatile("cli");
    while (1) {
        __asm__ volatile("hlt");
    }
}
