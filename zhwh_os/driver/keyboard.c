// driver/keyboard.c - PS/2 Keyboard Driver Implementation
// 参考: https://wiki.osdev.org/PS/2_Keyboard
// 参考: https://github.com/dreamportdev/Osdev-Notes/blob/master/02_Architecture/10_Keyboard_Interrupt_Handling.md

#include "keyboard.h"
#include "x86/io.h"

// 声明 printf 函数
extern int printf(const char*, ...);

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

// 辅助函数：等待输入缓冲区空（IBF=0）
static void kbd_wait_input_clear(void) {
    int timeout = 100000;
    while (timeout-- && (inb(0x64) & 0x02)) {
        // Wait for IBF clear
    }
    if (timeout == 0) {
        printf("[KBD] ⚠️⚠️⚠️ Timeout waiting for IBF clear!\n");
    }
}

// 辅助函数：等待输出缓冲区满（OBF=1）
static void kbd_wait_output_full(void) {
    int timeout = 100000;
    while (timeout-- && !(inb(0x64) & 0x01)) {
        // Wait for OBF set
    }
    if (timeout == 0) {
        printf("[KBD] ⚠️⚠️⚠️ Timeout waiting for OBF set!\n");
    }
}

// 初始化键盘驱动 - 严格版初始化序列
void keyboard_init(void) {
    kbd_state.shift_pressed = 0;
    kbd_state.caps_lock = 0;
    kbd_state.buffer_head = 0;
    kbd_state.buffer_tail = 0;

    // 1. 禁用键盘端口
    kbd_wait_input_clear();
    outb(0x64, 0xAD);

    // 2. 清空可能残留的 OBF
    if (inb(0x64) & 0x01) {
        inb(0x60);  // 丢弃
    }

    // 3. 读 Controller Config
    kbd_wait_input_clear();
    outb(0x64, 0x20);
    kbd_wait_output_full();
    unsigned char cfg = inb(0x60);

    // 4. 启用 IRQ1 (设置 bit 0)
    cfg |= 0x01;

    // 5. 写回 Config
    kbd_wait_input_clear();
    outb(0x64, 0x60);
    kbd_wait_input_clear();
    outb(0x60, cfg);

    // 6. 启用键盘端口
    kbd_wait_input_clear();
    outb(0x64, 0xAE);

    // 7. 发送回送命令测试键盘
    kbd_wait_input_clear();
    outb(0x60, 0xEE);
    kbd_wait_output_full();
    inb(0x60);  // 丢弃回送响应

    // 8. 🔥🔥 启用扫描（关键步骤）
    kbd_wait_input_clear();
    outb(0x60, 0xF4);

    // 9. 等待 ACK
    kbd_wait_output_full();
    inb(0x60);  // 丢弃 ACK

    // 🔥🔥🔥 关键修复：在 IOAPIC 中启用 IRQ1！
    extern void ioapicenable(int irq, int cpunum);
    ioapicenable(1, 0);  // 启用 IRQ1，路由到 CPU 0
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

    // ⚠️⚠️⚠️ 注意：不要在这里发送 EOI！
    // EOI 由 interrupt.c 中的 lapiceoi() 统一发送
}

// 从键盘缓冲区读取一个字符（使用中断驱动的 buffer）
int keyboard_getchar(void) {
    // 等待直到有字符可用
    // 使用 sti + hlt 让 CPU 进入可中断的睡眠状态
    // 只有这样，IRQ1 才有机会被执行！
    while (kbd_state.buffer_head == kbd_state.buffer_tail) {
        // sti: 确保中断标志位 IF=1
        // hlt: CPU 暂停，等待任何中断唤醒
        // 当 IRQ1 到达时，CPU 会立刻唤醒，执行 keyboard_handler()
        __asm__ volatile("sti; hlt");
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
