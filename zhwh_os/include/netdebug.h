/**
 * @file netdebug.h
 * @brief 以太网调试接口 - 通过 UDP 发送内核调试信息
 */

#ifndef NETDEBUG_H
#define NETDEBUG_H

#include "types.h"

// ==================== 核心接口 ====================

// 调试级别常量
#define NETDEBUG_LEVEL_ERROR  0
#define NETDEBUG_LEVEL_WARN   1
#define NETDEBUG_LEVEL_INFO   2
#define NETDEBUG_LEVEL_DEBUG  3

// 初始化
int netdebug_init(void);

// 控制接口
void netdebug_enable(int enable);          // 启用/禁用
void netdebug_set_level(int level);        // 设置最低输出级别
void netdebug_set_dest(uint32_t ip, uint16_t port);  // 设置目标 IP:Port

// 核心输出函数（类似 printf）
void netdebug_printf(int level, const char *fmt, ...);

// 统计信息
void netdebug_stats(void);

// ==================== 便捷输出函数 ====================

// ERROR 级别
void netdebug_error(const char *fmt, ...);

// WARN 级别
void netdebug_warn(const char *fmt, ...);

// INFO 级别
void netdebug_info(const char *fmt, ...);

// DEBUG 级别
void netdebug_debug(const char *fmt, ...);

// 内存转储
void netdebug_dump(const void *ptr, uint32_t len);

// 致命错误输出并停机
void netdebug_panic(const char *msg);

// ==================== 调试宏 ====================

#ifdef NETDEBUG_ENABLED
    #define NETDEBUG_ERROR(fmt, ...) netdebug_error(fmt, ##__VA_ARGS__)
    #define NETDEBUG_WARN(fmt, ...)  netdebug_warn(fmt, ##__VA_ARGS__)
    #define NETDEBUG_INFO(fmt, ...)  netdebug_info(fmt, ##__VA_ARGS__)
    #define NETDEBUG_DEBUG(fmt, ...) netdebug_debug(fmt, ##__VA_ARGS__)
#else
    #define NETDEBUG_ERROR(fmt, ...) do {} while(0)
    #define NETDEBUG_WARN(fmt, ...)  do {} while(0)
    #define NETDEBUG_INFO(fmt, ...)  do {} while(0)
    #define NETDEBUG_DEBUG(fmt, ...) do {} while(0)
#endif

#endif // NETDEBUG_H
