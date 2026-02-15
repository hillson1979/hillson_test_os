// include/keyboard.h - PS/2 Keyboard Driver
// 基于 OSDev Wiki: https://wiki.osdev.org/PS/2_Keyboard

#ifndef _KEYBOARD_H
#define _KEYBOARD_H

// 使用内核的类型定义
typedef unsigned char uint8_t;

// 键盘缓冲区大小
#define KBD_BUFFER_SIZE 256

// 键盘端口
#define KBD_DATA_PORT   0x60
#define KBD_CMD_PORT    0x64

// 特殊键码
#define KEY_LSHIFT      0x2A
#define KEY_RSHIFT      0x36
#define KEY_CAPSLOCK    0x3A
#define KEY_ENTER       0x1C
#define KEY_BACKSPACE   0x0E
#define KEY_TAB         0x0F
#define KEY_SPACE       0x39
#define KEY_ESC         0x01

// 扫描码释放标志
#define KEY_RELEASED    0x80

// 键盘状态
typedef struct {
    int shift_pressed;
    int caps_lock;
    uint8_t buffer[KBD_BUFFER_SIZE];
    int buffer_head;
    int buffer_tail;
} keyboard_state_t;

// 初始化键盘驱动
void keyboard_init(void);

// 键盘中断处理程序
void keyboard_handler(void);

// 从键盘缓冲区读取一个字符
int keyboard_getchar(void);

// 检查是否有按键可用
int keyboard_kbhit(void);

// 非阻塞版本：从键盘缓冲区读取一个字符
// 如果没有字符，返回 -1
int keyboard_getchar_nonblock(void);

// 清空键盘缓冲区
void keyboard_flush(void);

#endif // _KEYBOARD_H
