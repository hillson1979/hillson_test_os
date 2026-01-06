// driver/keyboard.c - PS/2 Keyboard Driver Implementation
// 参考: https://wiki.osdev.org/PS/2_Keyboard
// 参考: https://github.com/dreamportdev/Osdev-Notes/blob/master/02_Architecture/10_Keyboard_Interrupt_Handling.md

#include "keyboard.h"
#include "x86/io.h"

// US QWERTY 键盘扫描码到 ASCII 的转换表
// 索引是扫描码，值是对应的 ASCII 字符
static const char scancode_to_ascii_table[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

// Shift 状态下的扫描码转换表
static const char scancode_to_ascii_shift_table[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

// 全局键盘状态
static keyboard_state_t kbd_state = {
    .shift_pressed = 0,
    .caps_lock = 0,
    .buffer_head = 0,
    .buffer_tail = 0
};

// 将扫描码转换为 ASCII 字符
static char scancode_to_ascii(uint8_t scancode) {
    // 处理扩展扫描码 (0xE0 前缀)
    if (scancode == 0xE0) {
        return 0;  // 暂不处理扩展键
    }

    // 处理特殊键
    switch (scancode) {
        case KEY_LSHIFT:
        case KEY_RSHIFT:
            kbd_state.shift_pressed = 1;
            return 0;
        case KEY_LSHIFT | KEY_RELEASED:
        case KEY_RSHIFT | KEY_RELEASED:
            kbd_state.shift_pressed = 0;
            return 0;
        case KEY_CAPSLOCK:
            kbd_state.caps_lock = !kbd_state.caps_lock;
            return 0;
    }

    // 检查是否是按键释放
    if (scancode & KEY_RELEASED) {
        return 0;  // 忽略释放事件
    }

    // 检查扫描码是否在有效范围内
    if (scancode >= sizeof(scancode_to_ascii_table)) {
        return 0;
    }

    // 选择适当的转换表
    const char *table = kbd_state.shift_pressed ?
                        scancode_to_ascii_shift_table :
                        scancode_to_ascii_table;

    char c = table[scancode];

    // 处理 Caps Lock (只影响字母)
    if (kbd_state.caps_lock && c >= 'a' && c <= 'z') {
        c = c - 'a' + 'A';
    } else if (kbd_state.caps_lock && c >= 'A' && c <= 'Z') {
        c = c - 'A' + 'a';
    }

    return c;
}

// 向键盘缓冲区写入字符
static void keyboard_buffer_put(char c) {
    int next_tail = (kbd_state.buffer_tail + 1) % KBD_BUFFER_SIZE;

    // 如果缓冲区满，丢弃最旧的字符
    if (next_tail == kbd_state.buffer_head) {
        kbd_state.buffer_head = (kbd_state.buffer_head + 1) % KBD_BUFFER_SIZE;
    }

    kbd_state.buffer[kbd_state.buffer_tail] = c;
    kbd_state.buffer_tail = next_tail;
}

// 初始化键盘驱动
void keyboard_init(void) {
    kbd_state.shift_pressed = 0;
    kbd_state.caps_lock = 0;
    kbd_state.buffer_head = 0;
    kbd_state.buffer_tail = 0;

    // 简化的PS/2键盘控制器初始化
    // 直接启用键盘中断
    outb(KBD_CMD_PORT, 0xAE);  // 启用键盘设备

    // 读取并修改配置字节
    outb(KBD_CMD_PORT, 0x20);  // 读取配置命令
    unsigned char config = inb(KBD_DATA_PORT);
    config |= 0x01;  // 设置bit 0，启用键盘中断
    outb(KBD_CMD_PORT, 0x60);  // 写配置命令
    outb(KBD_DATA_PORT, config);

    extern int printf(const char*, ...);
    printf("[KBD] Controller initialized, config=0x%x\n", config);
}

// 简单的十六进制转字符辅助函数
static char hex_char(uint8_t nibble) {
    if (nibble < 10) return '0' + nibble;
    return 'A' + nibble - 10;
}

// 简化的输出函数（不使用printf）
static void keyboard_debug_print(uint8_t scancode) {
    // 直接写到VGA显存
    volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
    static int pos = 160; // 从第二行开始（避免覆盖启动信息）

    // 输出 "[KBD: XX] "
    const char msg[] = "[KBD: 0x";
    for (int i = 0; i < 8 && pos < 2000; i++, pos++) {
        vga[pos] = (0x0F << 8) | msg[i];
    }

    // 输出扫描码的高位
    vga[pos++] = (0x0F << 8) | hex_char(scancode >> 4);

    // 输出扫描码的低位
    vga[pos++] = (0x0F << 8) | hex_char(scancode & 0x0F);

    // 输出 "] "
    vga[pos++] = (0x0F << 8) | ']';
    vga[pos++] = (0x0F << 8) | ' ';
}

// 键盘中断处理程序
void keyboard_handler(void) {
    // 读取扫描码
    uint8_t scancode = inb(KBD_DATA_PORT);

    // 转换为 ASCII
    char c = scancode_to_ascii(scancode);

    // 如果是有效字符，放入缓冲区
    if (c != 0) {
        keyboard_buffer_put(c);
    }
}

// 从键盘缓冲区读取一个字符
int keyboard_getchar(void) {
    // 等待直到有字符可用
    while (kbd_state.buffer_head == kbd_state.buffer_tail) {
        // 轮询键盘状态端口
        unsigned char status = inb(KBD_CMD_PORT);
        if (status & 0x01) {  // 输出缓冲区有数据
            // 直接读取扫描码并处理
            uint8_t scancode = inb(KBD_DATA_PORT);

            // 忽略按键释放事件（bit 7 = 1）
            if (scancode & 0x80) {
                continue;  // 跳过释放事件，继续等待下一个按下
            }

            // 转换为ASCII（此时bit 7 = 0，是按下事件）
            char c = scancode_to_ascii(scancode);

            if (c != 0) {
                keyboard_buffer_put(c);
            }
        }

        // 短暂延迟，避免CPU占用过高
        for(volatile int i=0; i<1000; i++);
    }

    char c = kbd_state.buffer[kbd_state.buffer_head];
    kbd_state.buffer_head = (kbd_state.buffer_head + 1) % KBD_BUFFER_SIZE;

    return c;
}

// 检查是否有按键可用
int keyboard_kbhit(void) {
    return (kbd_state.buffer_head != kbd_state.buffer_tail);
}

// 清空键盘缓冲区
void keyboard_flush(void) {
    kbd_state.buffer_head = 0;
    kbd_state.buffer_tail = 0;
}
