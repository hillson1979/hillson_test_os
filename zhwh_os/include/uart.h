/**
 * @file uart.h
 * @brief 串口驱动接口
 */

#ifndef UART_H
#define UART_H

#include "types.h"

// 串口初始化
int uart_init(void);

// 字符 I/O
void uart_putc(char c);
char uart_getc(void);
int uart_data_available(void);

// 字符串 I/O
void uart_puts(const char *str);

// 格式化输出
void uart_puthex(uint32_t value);
void uart_putdec(uint32_t value);

// 调试接口
void uart_debug(const char *fmt, ...);
void uart_panic(const char *msg) __attribute__((noreturn));

// 内存 dump
void uart_dump(void *ptr, uint32_t len);

// 🔥 调试宏（通过串口输出）
#ifdef UART_DEBUG
    #define UART_DEBUG_PRINTF(fmt, ...) uart_debug(fmt, ##__VA_ARGS__)
#else
    #define UART_DEBUG_PRINTF(fmt, ...) do {} while(0)
#endif

#endif // UART_H
