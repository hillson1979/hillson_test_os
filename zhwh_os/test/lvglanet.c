/**
 * @file lvglanet.c
 * @brief LVGL + Network 集成测试程序 - 简化版：直接键盘输入
 *
 * 创建一个简单的界面，集成网络功能
 * 左下：黑色虚拟终端，可直接键盘输入命令
 * 右下：系统日志窗口，显示输出
 */

#include "libuser_minimal.h"
#include "lvgl_os.h"
#include "video_player.h"

#include <lvgl.h>
#include <string.h>
#include <stdarg.h>

// 声明 LVGL 端口函数
extern int lvgl_display_init(void);

// 网络函数声明
extern int rtl8139_init_user(void);
extern int e1000_init_user(const char *name);
extern int wifi_init(void);
extern int wifi_scan(void);
extern int net_ping(const char *ip_str);
extern int net_ping_dev(const char *ip_str, const char *dev_name);
extern int net_udp_send(const char *ip_str, int port, const char *msg, int len);

// 系统调用号
#define SYS_READ_INPUT 72
#define SYS_USB_MOUSE_POLL 73

// 输入事件结构
typedef struct {
    uint32_t type;      // 事件类型: 1=键盘, 2=鼠标
    int x;             // 鼠标 X 或 键码
    int y;             // 鼠标 Y 或 保留
    uint32_t pressed;  // 按键状态或保留
} input_event_t;

// USB 鼠标报告结构
typedef struct {
    uint8_t buttons;    // Bit 0: Left, Bit 1: Right, Bit 2: Middle
    int8_t x;          // X 位移
    int8_t y;          // Y 位移
} usb_mouse_report_t;

// 键码定义
#define KEY_TAB     0x0F
#define KEY_ENTER   0x1C
#define KEY_ESCAPE  0x01
#define KEY_UP      0x48  // 上箭头
#define KEY_DOWN    0x50  // 下箭头
#define KEY_LSHIFT  0x2A
#define KEY_RSHIFT  0x36
#define KEY_RELEASED 0x80

// 全局变量
static int mouse_x = 512;  // 当前鼠标 X 位置
static int mouse_y = 384;  // 当前鼠标 Y 位置
static int mouse_pressed = 0;  // 鼠标按钮状态（持久保存）
static lv_obj_t *log_label = NULL;  // 日志窗口标签
static lv_obj_t *term_label = NULL;  // 终端标签
static lv_obj_t *term_textarea = NULL;  // 终端输入框
static lv_group_t *input_group = NULL;  // 焦点组 - 用于Tab键切换
static int shift_pressed = 0;  // Shift键状态

// ====== 视频播放器状态 ======
static int video_enabled = 0;   // 视频播放器是否启用
static lv_obj_t *video_canvas = NULL;  // 视频显示区域

// 命令历史 - 支持上下箭头翻动
static char cmd_history[16][256];  // 最多保存16条历史命令
static int cmd_history_count = 0;   // 当前历史命令数量
static int cmd_history_index = -1;  // 当前浏览的历史索引（-1表示在最新位置）

// US QWERTY 键盘扫描码到 ASCII 的转换表
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

// 终端历史
static char term_history[4096];
static int term_history_len = 0;

// 日志文件
static int log_fd = -1;  // 日志文件描述符
static int log_to_file_enabled = 0;  // 是否启用日志到文件

/**
 * @brief 添加命令到历史记录
 */
static void add_cmd_to_history(const char *cmd) {
    // 如果历史记录已满，删除最旧的（移动数组）
    if (cmd_history_count >= 16) {
        for (int i = 0; i < 15; i++) {
            strcpy(cmd_history[i], cmd_history[i + 1]);
        }
        cmd_history_count = 15;
    }

    // 添加新命令到历史记录
    strcpy(cmd_history[cmd_history_count], cmd);
    cmd_history_count++;

    // 重置浏览索引到最新位置
    cmd_history_index = -1;
}

/**
 * @brief 从历史记录加载命令（上箭头）
 */
static void load_prev_cmd(void) {
    if (cmd_history_count == 0) return;

    // 如果是第一次按上箭头，从最新命令开始
    if (cmd_history_index == -1) {
        cmd_history_index = cmd_history_count - 1;
    } else if (cmd_history_index > 0) {
        // 否则向前翻动
        cmd_history_index--;
    }

    // 加载历史命令到输入框
    lv_textarea_set_text(term_textarea, cmd_history[cmd_history_index]);
}

/**
 * @brief 从历史记录加载命令（下箭头）
 */
static void load_next_cmd(void) {
    if (cmd_history_count == 0) return;

    if (cmd_history_index == -1) {
        // 已经在最新位置，清空输入框
        lv_textarea_set_text(term_textarea, "");
        return;
    }

    // 向后翻动
    cmd_history_index++;

    if (cmd_history_index >= cmd_history_count) {
        // 超过最新命令，清空输入框并重置索引
        cmd_history_index = -1;
        lv_textarea_set_text(term_textarea, "");
    } else {
        // 加载历史命令
        lv_textarea_set_text(term_textarea, cmd_history[cmd_history_index]);
    }
}

/**
 * @brief 添加日志到日志窗口
 * ⚠️ 改为非 static，以便 lvgl_port.c 可以调用
 */
void log_append(const char *text) {
    // 🔥 如果 log_label 还没初始化，直接返回（避免崩溃）
    if (!log_label) {
        // 但是仍然写入文件
        if (log_to_file_enabled && log_fd >= 0) {
            write(log_fd, text, strlen(text));
            write(log_fd, "\n", 1);
        }
        return;
    }

    static char buffer[2048];
    const char *cur = lv_label_get_text(log_label);

    // 复制当前内容
    int cur_len = 0;
    if (cur) {
        while (*cur && cur_len < 1000) {
            buffer[cur_len++] = *cur++;
        }
    }

    // 添加换行符（如果不是第一行）
    if (cur_len > 0) {
        buffer[cur_len++] = '\n';
    }

    // 添加新文本
    const char *add = text;
    while (*add && cur_len < 2000) {
        buffer[cur_len++] = *add++;
    }
    buffer[cur_len] = '\0';

    lv_label_set_text(log_label, buffer);

    // 🔥 写入日志文件
    if (log_to_file_enabled && log_fd >= 0) {
        // 添加换行符
        write(log_fd, text, strlen(text));
        write(log_fd, "\n", 1);
    }
}

/**
 * @brief 将扫描码转换为 ASCII 字符（用于命令解析等非输入设备场景）
 */
static char scancode_to_ascii(uint32_t scancode) {
    uint32_t key_code = scancode & 0x7F;

    // 处理特殊键
    if (key_code == KEY_LSHIFT || key_code == KEY_RSHIFT) {
        return 0;
    }

    // 忽略按键释放事件
    if (scancode & KEY_RELEASED) {
        return 0;
    }

    // 检查扫描码是否在有效范围内
    if (key_code >= sizeof(scancode_to_ascii_table)) {
        return 0;
    }

    // 选择适当的转换表
    const char *table = shift_pressed ? scancode_to_ascii_shift_table : scancode_to_ascii_table;
    return table[key_code];
}

/**
 * @brief 将scan code转换为LVGL键值
 */
static uint32_t scancode_to_lvgl_key(uint32_t scancode) {
    // 去除按键释放标志
    uint32_t key_code = scancode & 0x7F;

    switch (key_code) {
        case 0x01: return 27;  // ESC -> LV_KEY_ESC
        case 0x0F: return 9;   // TAB -> LV_KEY_NEXT
        case 0x1C: return 10;  // ENTER -> LV_KEY_ENTER
        case 0x48: return 17;  // Up -> LV_KEY_UP
        case 0x50: return 18;  // Down -> LV_KEY_DOWN
        case 0x4B: return 20;  // Left -> LV_KEY_LEFT
        case 0x4D: return 19;  // Right -> LV_KEY_RIGHT
        default:   return 0;   // 其他键不映射，LVGL会忽略
    }
}

static int keyboard_read_call_count = 0;
static void keyboard_read_bak1(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    // 🔥 调试：只在每1000次调用时打印一次
    if (++keyboard_read_call_count % 1000 == 0) {
        log_append("[KBD] keyboard_read called 1000 times");
    }

    // 🔥 新增：每次调用都打印（限制次数）
    static int debug_count = 0;
    if (debug_count < 10) {
        char buf[128];
        lv_snprintf(buf, sizeof(buf), "[KBD] keyboard_read call #%d", keyboard_read_call_count);
        log_append(buf);
        debug_count++;
    }

    input_event_t event;
    int ret;

    // 通过系统调用读取键盘输入
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_READ_INPUT), "b"(&event), "c"(1)
        : "memory", "cc"
    );

    // 🔥 新增：打印系统调用返回值
    if (debug_count <= 10) {
        char buf[128];
        lv_snprintf(buf, sizeof(buf), "[KBD] syscall returned %d, event.x=%d", ret, event.x);
        log_append(buf);
    }

    if (ret != 1) {
        // 没有输入
        if (debug_count <= 10) {
            log_append("[KBD] No input data available");
        }
        data->state = LV_INDEV_STATE_REL;
        data->key = 0;
        return;
    }

    // 🔥 新增：打印成功获取的扫描码
    if (debug_count <= 10) {
        char buf[128];
        lv_snprintf(buf, sizeof(buf), "[KBD] Got scancode: 0x%x", event.x);
        log_append(buf);
    }

    uint32_t scancode = event.x;
    uint32_t key_code = scancode & 0x7F;
    int is_release = (scancode & 0x80) != 0;

    // 先处理 Shift 键
    if (key_code == KEY_LSHIFT || key_code == KEY_RSHIFT) {
        shift_pressed = !is_release ? 1 : 0;
        data->state = LV_INDEV_STATE_REL;
        data->key = 0;
        return;
    }

    if (is_release) {
        // 按键释放：不输出字符，只更新状态
        data->state = LV_INDEV_STATE_REL;
        data->key = 0;
        return;
    }

    // 按下事件
    data->state = LV_INDEV_STATE_PR;

    // 特殊键映射
    int is_special_key = 1;
    switch (key_code) {
        case 0x0F: data->key = LV_KEY_NEXT;      break; // Tab
        case 0x1C: data->key = LV_KEY_ENTER;     break; // Enter
        case 0x0E: data->key = LV_KEY_BACKSPACE; break; // Backspace
        case 0x48: data->key = LV_KEY_UP;        break; // Up arrow
        case 0x50: data->key = LV_KEY_DOWN;      break; // Down arrow
        case 0x4B: data->key = LV_KEY_LEFT;      break; // Left arrow
        case 0x4D: data->key = LV_KEY_RIGHT;     break; // Right arrow
        default:   is_special_key = 0;            break; // Not a special key
    }

    // 只有非特殊键才做普通字符映射
    if (!is_special_key) {
        const char *table = shift_pressed ? scancode_to_ascii_shift_table : scancode_to_ascii_table;
        if (key_code < sizeof(scancode_to_ascii_table)) {
            data->key = table[key_code];
        } else {
            data->key = 0; // 超表不处理
        }
    }

    // 调试：记录按键到日志窗口
    char debug_buf[128];
    if (is_special_key) {
        lv_snprintf(debug_buf, sizeof(debug_buf), "[KEY] Special: sc=0x%02X, key=%d", key_code, data->key);
    } else {
        lv_snprintf(debug_buf, sizeof(debug_buf), "[KEY] Char: sc=0x%02X, key=%c", key_code, (char)data->key);
    }
    log_append(debug_buf);
}

// 键盘扫描码
#define KEY_LSHIFT    0x2A
#define KEY_RSHIFT    0x36
#define KEY_CAPSLOCK  0x3A

static void keyboard_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    static int shift_pressed = 0;
    static int caps_lock = 0;
    static int debug_count = 0;
    static int key_count = 0;

    input_event_t event;
    int ret;

    // 调用系统调用读取键盘事件
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_READ_INPUT), "b"(&event), "c"(1)
        : "memory", "cc"
    );

    if (ret != 1) {
        // 没有输入，保持释放状态
        data->state = LV_INDEV_STATE_REL;
        data->key = 0;
        return;
    }

    // 🔥 使用 printf 直接输出到串口（前20次）
    if (debug_count < 20) {
        printf("USER_KBD: syscall ret=%d, event.x=0x%x, event.type=%d\n", ret, event.x, event.type);
        debug_count++;
    }

    uint32_t scancode = event.x;
    uint32_t key_code = scancode & 0x7F;
    int is_release = (scancode & 0x80) != 0;

    // 处理 Shift 键
    if (key_code == KEY_LSHIFT || key_code == KEY_RSHIFT) {
        shift_pressed = !is_release ? 1 : 0;
        data->state = LV_INDEV_STATE_REL;
        data->key = 0;
        return;
    }

    // 处理 Caps Lock
    if (!is_release && key_code == KEY_CAPSLOCK) {
        caps_lock = !caps_lock;
        data->state = LV_INDEV_STATE_REL;
        data->key = 0;
        return;
    }

    if (is_release) {
        // 按键释放：不输出字符
        data->state = LV_INDEV_STATE_REL;
        data->key = 0;
        return;
    }

    // 按下事件
    data->state = LV_INDEV_STATE_PR;

    // 特殊键映射
    int is_special_key = 1;
    switch (key_code) {
        case 0x0F: data->key = LV_KEY_NEXT;      break; // Tab
        case 0x1C: data->key = LV_KEY_ENTER;     break; // Enter
        case 0x0E: data->key = LV_KEY_BACKSPACE; break; // Backspace
        case 0x48: data->key = LV_KEY_UP;        break; // Up arrow
        case 0x50: data->key = LV_KEY_DOWN;      break; // Down arrow
        case 0x4B: data->key = LV_KEY_LEFT;      break; // Left arrow
        case 0x4D: data->key = LV_KEY_RIGHT;     break; // Right arrow
        default:   is_special_key = 0;            break;
    }

    // 普通字符映射
    if (!is_special_key) {
        const char *table = shift_pressed ? scancode_to_ascii_shift_table : scancode_to_ascii_table;
        const int table_size = sizeof(scancode_to_ascii_table) / sizeof(scancode_to_ascii_table[0]);
        if (key_code < table_size) {
            char ch = table[key_code];
            // 根据 Caps Lock 和 Shift 调整大小写
            if (ch >= 'a' && ch <= 'z') {
                if ((caps_lock && !shift_pressed) || (!caps_lock && shift_pressed)) {
                    ch = ch - 'a' + 'A';
                }
            }
            data->key = ch;
        } else {
            data->key = 0;
        }
    }

    // 🔥 使用 printf 直接输出到串口（前20次按键）
    if (key_count < 20) {
        if (is_special_key) {
            printf("USER_KEY: Special: sc=0x%02X, key=%d\n", key_code, data->key);
        } else {
            printf("USER_KEY: Char: sc=0x%02X, key=%c (0x%x)\n", key_code, (char)data->key, (int)data->key);
        }
        key_count++;
    }
}
#ifndef LV_KEY_TAB
#define LV_KEY_TAB        9
#endif
static void keyboard_read_bak2(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    input_event_t event;
    int ret;

    // 通过系统调用读取键盘输入
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_READ_INPUT), "b"(&event), "c"(1)
        : "memory", "cc"
    );

    if (ret != 1) {
        // 没有输入
        data->state = LV_INDEV_STATE_REL;
        data->key = 0;
        return;
    }

    uint32_t scancode = event.x;
    uint32_t key_code = scancode & 0x7F;
    int is_release = (scancode & 0x80) != 0;

    // 先处理 Shift 键
    if (key_code == KEY_LSHIFT || key_code == KEY_RSHIFT) {
        shift_pressed = !is_release ? 1 : 0;
        data->state = LV_INDEV_STATE_REL;
        data->key = 0;
        return;
    }

    if (is_release) {
        // 按键释放：不输出字符，只更新状态
        data->state = LV_INDEV_STATE_REL;
        data->key = 0;
        return;
    }

    // 按下事件
    data->state = LV_INDEV_STATE_PR;

    // 特殊键映射
    switch (key_code) {
        case 0x0F: data->key = LV_KEY_NEXT;      return; // Tab
        case 0x1C: data->key = LV_KEY_ENTER;     return; // Enter
        case 0x0E: data->key = LV_KEY_BACKSPACE; return; // Backspace
        case 0x48: data->key = LV_KEY_UP;        return; // Up arrow
        case 0x50: data->key = LV_KEY_DOWN;      return; // Down arrow
        case 0x4B: data->key = LV_KEY_LEFT;      return; // Left arrow
        case 0x4D: data->key = LV_KEY_RIGHT;     return; // Right arrow
    }

    // 普通字符映射表
    const char *table = shift_pressed ? scancode_to_ascii_shift_table : scancode_to_ascii_table;
    if (key_code < sizeof(scancode_to_ascii_table)) {
        data->key = table[key_code];

        // 可选：打印日志调试
        // printf("[KEY] scancode=0x%02X, char=%c\n", scancode, data->key);
    } else {
        data->key = 0; // 超表不处理
    }
}
/**
 * @brief 键盘输入读取回调
 */
static void keyboard_read_bak(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    input_event_t event;
    int ret;

    // 调用系统调用读取键盘输入
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_READ_INPUT), "b"(&event), "c"(1)  // c=1 表示键盘事件
        : "memory", "cc"
    );

    printf("kbd: ret=%d sc=%x \n", ret, event.x);

    if (ret == 1) {
        // 有键盘输入
        uint32_t scancode = (uint32_t)(event.x & 0xFF); // 只取低8位，避免符号扩展
        uint32_t key_code = scancode & 0x7F;
        int is_release = (scancode & 0x80) != 0;

        // 先更新 Shift 键状态（无论按下还是释放）
        if (key_code == KEY_LSHIFT || key_code == KEY_RSHIFT) {
            if (!is_release) {
                shift_pressed = 1;
            } else {
                shift_pressed = 0;
            }
            data->state = LV_INDEV_STATE_REL;
            data->key = 0;
            return;
        }

        // 处理上下箭头键 - 用于命令历史浏览
        if (!is_release && term_textarea) {
            if (key_code == KEY_UP) {
                load_prev_cmd();
                data->state = LV_INDEV_STATE_REL;
                data->key = 0;
                return;
            } else if (key_code == KEY_DOWN) {
                load_next_cmd();
                data->state = LV_INDEV_STATE_REL;
                data->key = 0;
                return;
            }
        }

        // 映射到LVGL键值
        uint32_t lv_key = scancode_to_lvgl_key(scancode);
        if (lv_key != 0) {
            // 对于特殊键（Tab, Enter, Arrow keys等）
            data->key = lv_key;
            data->state = is_release ? LV_INDEV_STATE_REL : LV_INDEV_STATE_PR;
        } else {
            // 对于普通字符键，转换为ASCII字符
            const char *table = shift_pressed ? scancode_to_ascii_shift_table : scancode_to_ascii_table;
            // 检查键码是否在有效范围内
            if (key_code < sizeof(scancode_to_ascii_table)) {
                char ascii_char = table[key_code];
                if (ascii_char != 0) {
                    data->key = ascii_char;
                    data->state = is_release ? LV_INDEV_STATE_REL : LV_INDEV_STATE_PR;
                } else {
                    data->key = 0;
                    data->state = LV_INDEV_STATE_REL;
                }
            } else {
                data->key = 0;
                data->state = LV_INDEV_STATE_REL;
            }
        }
    } else {
        // 无输入
        data->state = LV_INDEV_STATE_REL;
        data->key = 0;
    }
}

/**
 * @brief 鼠标输入读取回调
 */
static bool mouse_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    input_event_t event;
    int ret;

    // 调用系统调用读取鼠标输入
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_READ_INPUT), "b"(&event), "c"(2)  // c=2 表示鼠标事件
        : "memory", "cc"
    );


    if (ret == 1) {
        // 有鼠标输入
        mouse_x = event.x;
        mouse_y = event.y;
        mouse_pressed = event.pressed;  // 保存按钮状态
    }

    // 填充 LVGL 输入数据结构
    data->point.x = mouse_x;
    data->point.y = mouse_y;
    data->state = (mouse_pressed & 0x01) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;

    // 必须返回 false，告诉 LVGL 没有更多缓存数据
    return false;
}

/**
 * @brief 终端输入回调 - 处理命令执行
 */
static void terminal_ready_callback(lv_event_t *e) {
    lv_obj_t *ta = lv_event_get_target(e);

    // 用户按下回车键
    const char *text = lv_textarea_get_text(ta);

    // 调试：显示原始输入
    if (text) {
        char debug_buf[256];
        lv_snprintf(debug_buf, sizeof(debug_buf), "[DEBUG] Raw input: '%s' (len=%d)", text, strlen(text));
        log_append(debug_buf);

        // 显示每个字符的十六进制值
        char hex_buf[128];
        int hex_pos = 0;
        for (int i = 0; i < strlen(text) && i < 20; i++) {
            lv_snprintf(hex_buf + hex_pos, sizeof(hex_buf) - hex_pos, "%02X ", (unsigned char)text[i]);
            hex_pos += 3;
        }
        log_append(hex_buf);
    }

    if (text && strlen(text) > 0) {
        // 添加到命令历史记录
        add_cmd_to_history(text);

        // 将命令添加到终端历史
        char cmd_buf[512];

        // 添加到终端显示
        if (term_history_len < sizeof(term_history) - 100) {
            int len = lv_snprintf(term_history + term_history_len,
                                         sizeof(term_history) - term_history_len,
                                         "> %s\n", text);
            term_history_len += len;
            lv_label_set_text(term_label, term_history);
        }

        // 同时将命令添加到系统日志窗口
        lv_snprintf(cmd_buf, sizeof(cmd_buf), "[TERM] %s", text);
        log_append(cmd_buf);

        // 执行命令
        if (strncmp(text, "net ", 4) == 0) {
            // net 命令格式：net init XXX
            const char *subcmd = text + 4;  // 跳过 "net "

            if (strncmp(subcmd, "init ", 5) == 0) {
                // net init XXX - 初始化指定设备
                const char *dev = subcmd + 5;  // 跳过 "init "

                char log_buf[256];
                lv_snprintf(log_buf, sizeof(log_buf), "[TERM] net init %s", dev);
                log_append(log_buf);

                int ret = -1;
                if (strcmp(dev, "rtl") == 0 || strcmp(dev, "rtl8139") == 0) {
                    ret = rtl8139_init_user();
                } else if (strcmp(dev, "e1000") == 0) {
                    ret = e1000_init_user("eth1");
                } else if (strcmp(dev, "wifi") == 0) {
                    ret = wifi_init();
                }

                if (ret == 0) {
                    log_append("[TERM] Initialization OK");
                    if (term_history_len < sizeof(term_history) - 100) {
                        term_history_len += lv_snprintf(term_history + term_history_len,
                                                             sizeof(term_history) - term_history_len,
                                                             "%s: OK\n", dev);
                        lv_label_set_text(term_label, term_history);
                    }
                } else {
                    log_append("[TERM] Initialization FAILED");
                    if (term_history_len < sizeof(term_history) - 100) {
                        term_history_len += lv_snprintf(term_history + term_history_len,
                                                             sizeof(term_history) - term_history_len,
                                                             "%s: FAILED\n", dev);
                        lv_label_set_text(term_label, term_history);
                    }
                }
            }
        }
        else if (strcmp(text, "init") == 0 || strcmp(text, "init all") == 0) {
            // 初始化所有网卡
            log_append("[TERM] Initializing all cards...");
            int ret1 = rtl8139_init_user();
            if (ret1 == 0) log_append("[TERM] RTL8139 OK");
            else log_append("[TERM] RTL8139 FAILED");

            int ret2 = e1000_init_user("eth1");
            if (ret2 == 0) log_append("[TERM] E1000 OK");
            else log_append("[TERM] E1000 FAILED");

            if (term_history_len < sizeof(term_history) - 200) {
                term_history_len += lv_snprintf(term_history + term_history_len,
                                             sizeof(term_history) - term_history_len,
                                             "All cards initialized\n");
                lv_label_set_text(term_label, term_history);
            }
        }
        else if (strncmp(text, "ping", 4) == 0) {
            // Ping测试 - 支持格式: ping [IP]
            const char *ip_str = "10.0.2.2";  // 默认IP
            // 检查是否指定了IP地址
            if (strlen(text) > 5 && text[4] == ' ') {
                ip_str = text + 5;
            }

            char log_buf[256];
            lv_snprintf(log_buf, sizeof(log_buf), "[TERM] Pinging %s...", ip_str);
            log_append(log_buf);

            int ret = net_ping(ip_str);
            if (ret == 0) {
                log_append("[TERM] Ping success!");
                if (term_history_len < sizeof(term_history) - 50) {
                    term_history_len += lv_snprintf(term_history + term_history_len,
                                                         sizeof(term_history) - term_history_len,
                                                         "Ping: OK\n");
                    lv_label_set_text(term_label, term_history);
                }
            } else {
                log_append("[TERM] Ping failed");
                if (term_history_len < sizeof(term_history) - 50) {
                    term_history_len += lv_snprintf(term_history + term_history_len,
                                                         sizeof(term_history) - term_history_len,
                                                         "Ping: FAILED\n");
                    lv_label_set_text(term_label, term_history);
                }
            }
        }
        else if (strncmp(text, "udp", 3) == 0) {
            // UDP发送测试 - 支持格式: udp IP PORT MESSAGE
            const char *ip_str = "10.0.2.2";  // 默认IP
            int port = 8080;  // 默认端口
            const char *msg = "Hello from LVGL!";  // 默认消息

            // 简单解析参数
            if (strlen(text) > 3) {
                const char *p = text + 3;  // 跳过 "udp"

                // 跳过空格（如果有）
                while (*p == ' ') p++;

                // 查找IP地址后的空格（端口号）
                const char *ip_end = strchr(p, ' ');

                if (ip_end && ip_end > p) {
                    char ip_buf[64];
                    int ip_len = ip_end - p;
                    if (ip_len > 0 && ip_len < sizeof(ip_buf)) {
                        memcpy(ip_buf, p, ip_len);
                        ip_buf[ip_len] = '\0';
                        ip_str = ip_buf;

                        p = ip_end + 1;
                        port = atoi(p);

                        // 查找端口号后面的消息
                        const char *port_end = strchr(p, ' ');
                        if (port_end) {
                            msg = port_end + 1;
                        }

                        // 调试输出
                        char debug_buf[128];
                        lv_snprintf(debug_buf, sizeof(debug_buf), "[DEBUG] Parsed: IP='%s' port=%d", ip_str, port);
                        log_append(debug_buf);
                    }
                } else {
                    // 没有空格，可能只有IP地址
                    ip_str = p;
                    char debug_buf[128];
                    lv_snprintf(debug_buf, sizeof(debug_buf), "[DEBUG] No space found, IP='%s'", ip_str);
                    log_append(debug_buf);
                }
            }

            char log_buf[256];
            lv_snprintf(log_buf, sizeof(log_buf), "[TERM] UDP send %s:%d \"%s\"", ip_str, port, msg);
            log_append(log_buf);

            int ret = net_send_udp(ip_str, port, msg, strlen(msg));
            if (ret == 0) {
                log_append("[TERM] UDP send OK!");
                if (term_history_len < sizeof(term_history) - 50) {
                    term_history_len += lv_snprintf(term_history + term_history_len,
                                                         sizeof(term_history) - term_history_len,
                                                         "UDP: OK\n");
                    lv_label_set_text(term_label, term_history);
                }
            } else {
                log_append("[TERM] UDP send FAILED");
                if (term_history_len < sizeof(term_history) - 50) {
                    term_history_len += lv_snprintf(term_history + term_history_len,
                                                         sizeof(term_history) - term_history_len,
                                                         "UDP: FAILED\n");
                    lv_label_set_text(term_label, term_history);
                }
            }
        }
        else if (strcmp(text, "help") == 0 || strcmp(text, "?") == 0) {
            // 显示帮助
            const char *help_text =
                "Available commands:\n"
                "  init           - Initialize all network cards\n"
                "  net init rtl   - Initialize RTL8139\n"
                "  net init e1000 - Initialize E1000\n"
                "  net init wifi  - Initialize WiFi\n"
                "  ping [IP]      - Ping test (default: 10.0.2.2)\n"
                "  udp IP PORT [MSG] - UDP send test\n"
                "  wifi           - Initialize WiFi\n"
                "  scan           - WiFi scan\n"
                "  vstart [PORT]  - Start video receiver (def:1234)\n"
                "  vstop          - Stop video receiver\n"
                "  vstat          - Show video statistics\n"
                "  clear          - Clear terminal\n"
                "  help/?         - Show this help\n"
                "> ";

            if (term_history_len < sizeof(term_history) - 400) {
                term_history_len += lv_snprintf(term_history + term_history_len,
                                             sizeof(term_history) - term_history_len,
                                             "%s", help_text);
                lv_label_set_text(term_label, term_history);
            }
            log_append("[TERM] Help displayed");
        }
        else if (strcmp(text, "wifi") == 0) {
            // WiFi初始化
            log_append("[TERM] Initializing WiFi...");
            int ret = wifi_init();
            if (ret == 0) {
                log_append("[TERM] WiFi OK");
                if (term_history_len < sizeof(term_history) - 50) {
                    term_history_len += lv_snprintf(term_history + term_history_len,
                                                         sizeof(term_history) - term_history_len,
                                                         "WiFi initialized\n");
                    lv_label_set_text(term_label, term_history);
                }
            } else {
                log_append("[TERM] WiFi FAILED");
                if (term_history_len < sizeof(term_history) - 50) {
                    term_history_len += lv_snprintf(term_history + term_history_len,
                                                         sizeof(term_history) - term_history_len,
                                                         "WiFi init failed\n");
                    lv_label_set_text(term_label, term_history);
                }
            }
        }
        else if (strcmp(text, "scan") == 0) {
            // WiFi扫描
            log_append("[TERM] Scanning WiFi...");
            wifi_scan();
            if (term_history_len < sizeof(term_history) - 50) {
                term_history_len += lv_snprintf(term_history + term_history_len,
                                                     sizeof(term_history) - term_history_len,
                                                     "WiFi scan started\n");
                lv_label_set_text(term_label, term_history);
            }
        }
        else if (strcmp(text, "clear") == 0) {
            // 清空终端历史
            term_history_len = 0;
            term_history[0] = '\0';
            lv_label_set_text(term_label, "Terminal cleared.\n> ");
            log_append("[TERM] Terminal cleared");
        }
        else if (strncmp(text, "vstart", 6) == 0) {
            // 启动视频播放器 - 支持格式: vstart [PORT]
            int port = 1234;  // 默认端口
            if (strlen(text) > 7 && text[6] == ' ') {
                port = atoi(text + 7);
            }

            char log_buf[256];
            lv_snprintf(log_buf, sizeof(log_buf), "[TERM] Starting video receiver on port %d...", port);
            log_append(log_buf);

            // 初始化网络接收器
            int ret = net_recv_init("0.0.0.0", port, 1);  // UDP
            if (ret == 0) {
                // 初始化视频播放器（如果还没初始化）
                if (!video_canvas) {
                    video_canvas = lv_img_create(lv_scr_act());
                    lv_obj_set_size(video_canvas, VIDEO_W, VIDEO_H);
                    lv_obj_set_pos(video_canvas, 40, 180);  // 放在界面上方
                    lv_obj_set_style_border_width(video_canvas, 2, 0);
                    lv_obj_set_style_border_color(video_canvas, lv_palette_main(LV_PALETTE_CYAN), 0);

                    // 🔥 使用静态分配（在 BSS 中），150KB
                    static uint16_t black_buf[VIDEO_W * VIDEO_H];
                    static lv_img_dsc_t black_dsc;
                    memset(black_buf, 0, sizeof(black_buf));
                    black_dsc.header.always_zero = 0;
                    black_dsc.header.w = VIDEO_W;
                    black_dsc.header.h = VIDEO_H;
                    black_dsc.data_size = VIDEO_W * VIDEO_H * sizeof(uint16_t);
                    black_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
                    black_dsc.data = (uint8_t *)black_buf;
                    lv_img_set_src(video_canvas, &black_dsc);

                    log_append("[VIDEO] Canvas created");
                }

                video_enabled = 1;
                log_append("[VIDEO] Receiver started");
                if (term_history_len < sizeof(term_history) - 50) {
                    term_history_len += lv_snprintf(term_history + term_history_len,
                                                         sizeof(term_history) - term_history_len,
                                                         "Video: started on port %d\n", port);
                    lv_label_set_text(term_label, term_history);
                }
            } else {
                log_append("[VIDEO] Failed to start receiver");
                if (term_history_len < sizeof(term_history) - 50) {
                    term_history_len += lv_snprintf(term_history + term_history_len,
                                                         sizeof(term_history) - term_history_len,
                                                         "Video: failed (ret=%d)\n", ret);
                    lv_label_set_text(term_label, term_history);
                }
            }
        }
        else if (strcmp(text, "vstop") == 0) {
            // 停止视频播放器
            video_enabled = 0;
            net_recv_close();
            log_append("[VIDEO] Receiver stopped");
            if (term_history_len < sizeof(term_history) - 50) {
                term_history_len += lv_snprintf(term_history + term_history_len,
                                                     sizeof(term_history) - term_history_len,
                                                     "Video: stopped\n");
                lv_label_set_text(term_label, term_history);
            }
        }
        else if (strcmp(text, "vstat") == 0) {
            // 显示视频统计信息
            const video_stats_t *stats = video_player_get_stats();
            if (stats) {
                char stat_buf[512];
                int len = lv_snprintf(stat_buf, sizeof(stat_buf),
                    "Video Stats:\n"
                    "  Frames: %u\n"
                    "  FPS: %u\n"
                    "  Bytes: %u\n"
                    "  Packets: %u\n"
                    "  Bandwidth: %u B/s\n"
                    "  Decode avg: %u ms\n"
                    "  Decode max: %u ms\n"
                    "  Recv errors: %u\n"
                    "  Decode errors: %u\n",
                    stats->frames_displayed,
                    stats->fps,
                    stats->bytes_received,
                    stats->total_packets,
                    stats->last_bandwidth,
                    stats->avg_decode_time,
                    stats->max_decode_time,
                    stats->recv_errors,
                    stats->decode_errors);
                log_append(stat_buf);

                if (term_history_len < sizeof(term_history) - len - 50) {
                    term_history_len += lv_snprintf(term_history + term_history_len,
                                                         sizeof(term_history) - term_history_len,
                                                         "%s", stat_buf);
                    lv_label_set_text(term_label, term_history);
                }
            } else {
                log_append("[VIDEO] No stats available");
            }
        }
        else {
            // 未知命令
            char err_buf[256];
            lv_snprintf(err_buf, sizeof(err_buf), "Unknown command: %s", text);
            log_append(err_buf);

            if (term_history_len < sizeof(term_history) - 100) {
                term_history_len += lv_snprintf(term_history + term_history_len,
                                                     sizeof(term_history) - term_history_len,
                                                     "Unknown: %s (type 'help')\n", text);
                lv_label_set_text(term_label, term_history);
            }
        }

        // 清空输入框
        lv_textarea_set_text(ta, "");
    }
}

/**
 * @brief 按钮回调 - 初始化所有网卡
 */
void button1_callback(lv_event_t *e) {
    log_append("=== Initializing All Cards ===");
    int ret1 = rtl8139_init_user();
    if (ret1 == 0) log_append("[OK] RTL8139 initialized");
    else log_append("[FAIL] RTL8139 init failed");

    int ret2 = e1000_init_user("eth1");
    if (ret2 == 0) log_append("[OK] E1000 initialized");
    else log_append("[FAIL] E1000 init failed");

    int ret3 = wifi_init();
    if (ret3 == 0) log_append("[OK] WiFi initialized");
    else log_append("[FAIL] WiFi init failed");

    log_append("Init complete!");
}

/**
 * @brief 按钮回调 - 仅初始化 RTL8139
 */
void button2_callback(lv_event_t *e) {
    log_append("Initializing RTL8139...");
    int ret = rtl8139_init_user();
    if (ret == 0) log_append("[OK] RTL8139 ready!");
    else log_append("[FAIL] RTL8139 init failed");
}

/**
 * @brief 按钮回调 - 仅初始化 E1000
 */
void button3_callback(lv_event_t *e) {
    log_append("Initializing E1000...");
    int ret = e1000_init_user("eth1");
    if (ret == 0) {log_append("[OK] E1000 ready!");
     }
      else {log_append("[FAIL] E1000 init failed");
     }
}

/**
 * @brief 按钮回调 - WiFi 初始化和扫描
 */
void button4_callback(lv_event_t *e) {
    log_append("Initializing WiFi...");
    int ret = wifi_init();
    if (ret == 0) {
        log_append("[OK] WiFi initialized, scanning...");
        wifi_scan();
    } else {
        log_append("[FAIL] WiFi init failed");
    }
}

/**
 * @brief 按钮回调 - Ping 测试
 */
void button5_callback(lv_event_t *e) {
    log_append("Pinging 10.0.2.2...");
    int ret = net_ping("10.0.2.2");
    if (ret == 0) log_append("[OK] Ping success!");
    else log_append("[FAIL] Ping failed");
}

/**
 * @brief 按钮回调 - WiFi 状态
 */
void button6_callback(lv_event_t *e) {
    log_append("Checking WiFi status...");
    extern void wifi_status(void);
    wifi_status();
}

/**
 * @brief 焦点样式回调 - 当控件获得/失去焦点时调用
 */
static void focus_style_cb(lv_event_t *e) {
    lv_obj_t *obj = lv_event_get_target(e);

    // 对于按钮，事件目标可能是内部的 label，需要找到按钮对象
    lv_obj_t *target = obj;
    if (lv_obj_get_class(obj) == &lv_label_class) {
        target = lv_obj_get_parent(obj);
    }

    if (lv_event_get_code(e) == LV_EVENT_FOCUSED) {
        // 获得焦点：非常明显的反馈
        if (lv_obj_get_class(target) == &lv_btn_class) {
            // 按钮：加粗边框 + 高亮背景
            lv_obj_set_style_border_width(target, 5, 0);
            lv_obj_set_style_border_color(target, lv_palette_main(LV_PALETTE_YELLOW), 0);
            lv_obj_set_style_border_opa(target, LV_OPA_COVER, 0);
            lv_obj_set_style_outline_width(target, 3, 0);
            lv_obj_set_style_outline_color(target, lv_palette_main(LV_PALETTE_RED), 0);
            lv_obj_set_style_outline_opa(target, LV_OPA_COVER, 0);
            lv_obj_set_style_outline_pad(target, 2, 0);
        } else if (lv_obj_get_class(target) == &lv_textarea_class) {
            // 输入框：加粗边框 + 白色背景
            lv_obj_set_style_border_width(target, 5, 0);
            lv_obj_set_style_border_color(target, lv_palette_main(LV_PALETTE_YELLOW), 0);
            lv_obj_set_style_border_opa(target, LV_OPA_COVER, 0);
            lv_obj_set_style_outline_width(target, 3, 0);
            lv_obj_set_style_outline_color(target, lv_palette_main(LV_PALETTE_RED), 0);
            lv_obj_set_style_outline_opa(target, LV_OPA_COVER, 0);
            lv_obj_set_style_outline_pad(target, 2, 0);
        }
    } else if (lv_event_get_code(e) == LV_EVENT_DEFOCUSED) {
        // 失去焦点：移除所有焦点样式
        if (lv_obj_get_class(target) == &lv_btn_class) {
            // 按钮恢复
            lv_obj_set_style_border_width(target, 0, 0);
            lv_obj_set_style_outline_width(target, 0, 0);
        } else if (lv_obj_get_class(target) == &lv_textarea_class) {
            // 输入框恢复
            lv_obj_set_style_border_width(target, 2, 0);
            lv_obj_set_style_border_color(target, lv_palette_main(LV_PALETTE_GREEN), 0);
            lv_obj_set_style_outline_width(target, 0, 0);
        }
    }
}

/**
 * @brief 创建测试界面
 */
void create_keyboard_ui(void) {
    // 🔥 暂时禁用 printf
    // printf("[LVGL] Creating LVGL + Network UI...\n");

    // 获取默认屏幕
    lv_obj_t *scr = lv_scr_act();

    // 声明按钮变量 - 需要在焦点组中使用
    lv_obj_t *btn1, *btn2, *btn3, *btn4, *btn5, *btn6;

    // ===== 上部分：标题 + 6个功能按钮 =====

    // 创建标题
    lv_obj_t *title_label = lv_label_create(scr);
    lv_label_set_text(title_label, "LVGL + Network Test");
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_14, 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 10);

    // === 第一行按钮 ===

    // 按钮1: 初始化所有网卡
    btn1 = lv_btn_create(scr);
    lv_obj_set_size(btn1, 180, 45);
    lv_obj_set_pos(btn1, 50, 50);
    lv_obj_set_style_bg_color(btn1, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_bg_opa(btn1, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn1, 10, 0);  // 圆角
    lv_obj_add_event_cb(btn1, button1_callback, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn1, focus_style_cb, LV_EVENT_FOCUSED | LV_EVENT_DEFOCUSED, NULL);

    lv_obj_t *btn1_label = lv_label_create(btn1);
    lv_label_set_text(btn1_label, "Init All");
    lv_obj_center(btn1_label);

    // 按钮2: 初始化 RTL8139
    btn2 = lv_btn_create(scr);
    lv_obj_set_size(btn2, 180, 45);
    lv_obj_set_pos(btn2, 260, 50);
    lv_obj_set_style_bg_color(btn2, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_bg_opa(btn2, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn2, 10, 0);  // 圆角
    lv_obj_add_event_cb(btn2, button2_callback, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn2, focus_style_cb, LV_EVENT_FOCUSED | LV_EVENT_DEFOCUSED, NULL);

    lv_obj_t *btn2_label = lv_label_create(btn2);
    lv_label_set_text(btn2_label, "Init RTL");
    lv_obj_center(btn2_label);

    // 按钮3: 初始化 E1000
    btn3 = lv_btn_create(scr);
    lv_obj_set_size(btn3, 180, 45);
    lv_obj_set_pos(btn3, 470, 50);
    lv_obj_set_style_bg_color(btn3, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_bg_opa(btn3, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn3, 10, 0);  // 圆角
    lv_obj_add_event_cb(btn3, button3_callback, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn3, focus_style_cb, LV_EVENT_FOCUSED | LV_EVENT_DEFOCUSED, NULL);

    lv_obj_t *btn3_label = lv_label_create(btn3);
    lv_label_set_text(btn3_label, "Init E1000");
    lv_obj_center(btn3_label);

    // === 第二行按钮 ===

    // 按钮4: WiFi Init
    btn4 = lv_btn_create(scr);
    lv_obj_set_size(btn4, 180, 45);
    lv_obj_set_pos(btn4, 50, 115);
    lv_obj_set_style_bg_color(btn4, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_set_style_bg_opa(btn4, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn4, 10, 0);  // 圆角
    lv_obj_add_event_cb(btn4, button4_callback, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn4, focus_style_cb, LV_EVENT_FOCUSED | LV_EVENT_DEFOCUSED, NULL);

    lv_obj_t *btn4_label = lv_label_create(btn4);
    lv_label_set_text(btn4_label, "WiFi Init");
    lv_obj_center(btn4_label);

    // 按钮5: Ping Test
    btn5 = lv_btn_create(scr);
    lv_obj_set_size(btn5, 180, 45);
    lv_obj_set_pos(btn5, 260, 115);
    lv_obj_set_style_bg_color(btn5, lv_palette_main(LV_PALETTE_PURPLE), 0);
    lv_obj_set_style_bg_opa(btn5, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn5, 10, 0);  // 圆角
    lv_obj_add_event_cb(btn5, button5_callback, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn5, focus_style_cb, LV_EVENT_FOCUSED | LV_EVENT_DEFOCUSED, NULL);

    lv_obj_t *btn5_label = lv_label_create(btn5);
    lv_label_set_text(btn5_label, "Ping Test");
    lv_obj_center(btn5_label);

    // 按钮6: WiFi Status
    btn6 = lv_btn_create(scr);
    lv_obj_set_size(btn6, 180, 45);
    lv_obj_set_pos(btn6, 470, 115);
    lv_obj_set_style_bg_color(btn6, lv_palette_main(LV_PALETTE_TEAL), 0);
    lv_obj_set_style_bg_opa(btn6, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn6, 10, 0);  // 圆角
    lv_obj_add_event_cb(btn6, button6_callback, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn6, focus_style_cb, LV_EVENT_FOCUSED | LV_EVENT_DEFOCUSED, NULL);

    lv_obj_t *btn6_label = lv_label_create(btn6);
    lv_label_set_text(btn6_label, "WiFi Stat");
    lv_obj_center(btn6_label);

    // ===== 下部分：左虚拟终端 + 右日志窗口 =====

    // 创建虚拟终端窗口（左下）- 黑色终端样式
    lv_obj_t *term_win = lv_obj_create(scr);
    lv_obj_set_size(term_win, 450, 280);
    lv_obj_set_pos(term_win, 40, 190);

    // 设置虚拟终端样式 - 黑色背景，白色边框
    lv_obj_set_style_bg_color(term_win, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(term_win, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(term_win, 2, 0);
    lv_obj_set_style_border_color(term_win, lv_color_white(), 0);
    lv_obj_set_style_radius(term_win, 5, 0);
    lv_obj_set_style_pad_all(term_win, 10, 0);

    // 创建虚拟终端标题
    lv_obj_t *term_title = lv_label_create(term_win);
    lv_label_set_text(term_title, "Virtual Terminal");
    lv_obj_set_style_text_color(term_title, lv_palette_main(LV_PALETTE_LIME), 0);
    lv_obj_set_style_text_font(term_title, &lv_font_montserrat_14, 0);
    lv_obj_align(term_title, LV_ALIGN_TOP_MID, 0, 5);

    // 初始化终端历史
    term_history[0] = '\0';
    term_history_len = lv_snprintf(term_history, sizeof(term_history),
                                 "> ");

    // 创建文本输入框用于键盘输入 - 黑色终端样式
    term_textarea = lv_textarea_create(term_win);
    lv_textarea_set_one_line(term_textarea, true);
    lv_textarea_set_placeholder_text(term_textarea, "input data...");
    lv_obj_set_size(term_textarea, 430, 40);
    lv_obj_set_pos(term_textarea, 10, 30);

    // 设置输入框样式 - 黑色背景，绿色文字
    lv_obj_set_style_bg_color(term_textarea, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(term_textarea, LV_OPA_50, 0);
    lv_obj_set_style_text_color(term_textarea, lv_palette_main(LV_PALETTE_LIME), 0);
    lv_obj_set_style_border_width(term_textarea, 2, 0);
    lv_obj_set_style_border_color(term_textarea, lv_palette_main(LV_PALETTE_GREEN), 0);

    lv_textarea_set_cursor_click_pos(term_textarea, true);   // 允许点击定位光标

    // 设置光标样式 - 明显的黄色光标，便于观察
    lv_obj_set_style_bg_color(term_textarea, lv_palette_main(LV_PALETTE_YELLOW), LV_PART_CURSOR);
    lv_obj_set_style_bg_opa(term_textarea, LV_OPA_COVER, LV_PART_CURSOR);

    lv_obj_add_event_cb(term_textarea, terminal_ready_callback, LV_EVENT_READY, NULL);
    //lv_obj_add_event_cb(term_textarea, textarea_key_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(term_textarea, focus_style_cb, LV_EVENT_FOCUSED | LV_EVENT_DEFOCUSED, NULL);

    // 创建终端输出显示区域 - 黑色终端样式
    term_label = lv_label_create(term_win);
    lv_label_set_text(term_label, term_history);
    lv_obj_set_style_text_color(term_label, lv_palette_main(LV_PALETTE_LIME), 0);  // 绿色文字
    lv_obj_set_style_text_font(term_label, &lv_font_montserrat_14, 0);
    lv_label_set_long_mode(term_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(term_label, 430);
    lv_obj_set_pos(term_label, 10, 80);

    // 创建日志窗口（右下）
    lv_obj_t *log_win = lv_obj_create(scr);
    lv_obj_set_size(log_win, 450, 280);
    lv_obj_set_pos(log_win, 510, 190);

    // 设置日志窗口样式
    lv_obj_set_style_bg_color(log_win, lv_palette_main(LV_PALETTE_INDIGO), 0);
    lv_obj_set_style_bg_opa(log_win, LV_OPA_70, 0);
    lv_obj_set_style_border_width(log_win, 2, 0);
    lv_obj_set_style_border_color(log_win, lv_color_white(), 0);
    lv_obj_set_style_radius(log_win, 10, 0);
    lv_obj_set_style_pad_all(log_win, 10, 0);

    // 创建日志窗口标题
    lv_obj_t *log_title = lv_label_create(log_win);
    lv_label_set_text(log_title, "System Log");
    lv_obj_set_style_text_color(log_title, lv_palette_main(LV_PALETTE_LIME), 0);
    lv_obj_set_style_text_font(log_title, &lv_font_montserrat_14, 0);
    lv_obj_align(log_title, LV_ALIGN_TOP_MID, 0, 5);

    // 创建日志标签
    log_label = lv_label_create(log_win);
    lv_label_set_text(log_label, "System ready.\nClick buttons to test network functions...");
    lv_obj_set_style_text_color(log_label, lv_palette_main(LV_PALETTE_LIME), 0);
    lv_obj_set_style_text_font(log_label, &lv_font_montserrat_14, 0);
    lv_label_set_long_mode(log_label, LV_LABEL_LONG_SCROLL);  // 启用滚动模式
    lv_obj_set_scrollbar_mode(log_label, LV_SCROLLBAR_MODE_AUTO);  // 自动显示滚动条
    lv_obj_set_width(log_label, 430);
    lv_obj_set_height(log_label, 230);  // 设置高度以启用滚动
    lv_obj_set_pos(log_label, 10, 30);
    lv_obj_align(log_label, LV_ALIGN_TOP_MID, 0, 0);  // 顶部居中对齐

    // 强制标记整个屏幕为"脏"，触发渲染
    // printf("[LVGL] Forcing screen invalidation...\n");
    lv_obj_invalidate(scr);
    lv_refr_now(NULL);  // 立即触发刷新

    // 创建焦点组 - 用于Tab键切换
    input_group = lv_group_create();
    if (input_group) {
        // printf("[LVGL] Focus group created\n");

        // 添加控件到焦点组（按Tab切换顺序）
        // 终端输入框放在第一个，这样焦点默认在这里
       
        lv_group_add_obj(input_group, btn1);  // Init All
        lv_group_add_obj(input_group, btn2);  // Init RTL
        lv_group_add_obj(input_group, btn3);  // Init E1000
        lv_group_add_obj(input_group, btn4);  // WiFi Init
        lv_group_add_obj(input_group, btn5);  // Ping Test
        lv_group_add_obj(input_group, btn6);  // WiFi Stat

        // printf("[LVGL] Added 7 objects to focus group\n");
        // lv_obj_add_flag(term_textarea, LV_OBJ_FLAG_FOCUSABLE);
        // lv_obj_add_flag(btn1, LV_OBJ_FLAG_FOCUSABLE);
        // lv_obj_add_flag(btn2, LV_OBJ_FLAG_FOCUSABLE);
        // lv_obj_add_flag(btn3, LV_OBJ_FLAG_FOCUSABLE);
        // lv_obj_add_flag(btn4, LV_OBJ_FLAG_FOCUSABLE);
        // lv_obj_add_flag(btn5, LV_OBJ_FLAG_FOCUSABLE);
        // lv_obj_add_flag(btn6, LV_OBJ_FLAG_FOCUSABLE);

        lv_group_set_wrap(input_group, true);
    }

    // 创建键盘输入设备
    static lv_indev_drv_t keyboard_drv;
    lv_indev_drv_init(&keyboard_drv);
    keyboard_drv.type = LV_INDEV_TYPE_KEYPAD;
    keyboard_drv.read_cb = keyboard_read;
    lv_indev_t *kb_indev = lv_indev_drv_register(&keyboard_drv);

    // 将键盘输入设备关联到焦点组
    if (input_group && kb_indev) {
        log_append("[UI] keyboard_ Created !");
        
        lv_group_add_obj(input_group, term_textarea);  // 终端输入框（默认焦点）

        lv_indev_set_group(kb_indev, input_group);

        // 设置焦点默认在终端输入框
        lv_group_focus_obj(term_textarea);
        lv_obj_add_state(term_textarea, LV_STATE_FOCUSED);  // 强制设置焦点状态
    }

    // 最后再刷新一次屏幕
    //lv_obj_invalidate(lv_scr_act());
    //lv_refr_now(NULL);

    // 创建鼠标输入设备
    static lv_indev_drv_t mouse_drv;
    lv_indev_drv_init(&mouse_drv);
    mouse_drv.type = LV_INDEV_TYPE_POINTER;
    mouse_drv.read_cb = mouse_read;
    lv_indev_drv_register(&mouse_drv);

    log_append("[UI] Created successfully!");

    // printf("[LVGL] UI created with keyboard terminal and mouse control!\n");
    // printf("[LVGL] About to return from create_keyboard_ui()...\n");
}

/**
 * @brief 主循环 - LVGL自动处理输入
 */
// 声明 yield 函数
extern void yield(void);

// 用于 track 函数
static uint32_t tick_count = 0;

void lvgl_main_loop(void) {
    //LV_LOG("Entering main loop");

    uint32_t loop_count = 0;
    log_append("[MAIN] Entering main loop");

    while (1) {
        // 只在需要时 increase tick - 大约每 5 次循环增加 5ms
        // 这样避免时间流逝相对合理
        tick_count++;
        if (tick_count % 5 == 0) {
            lv_tick_inc(1);  // 每次增加 1ms，而不是 5ms
        }

        lv_timer_handler();

        // 🔥 调试：每10000次循环打印一次（使用静态字符串避免 sprintf）
        if (loop_count % 10000 == 0) {
            if (loop_count == 0) log_append("[MAIN] Loop started");
            else if (loop_count == 10000) log_append("[MAIN] Loop count: 10000");
            else if (loop_count == 50000) log_append("[MAIN] Loop count: 50000");
            else if (loop_count == 100000) log_append("[MAIN] Loop count: 100000");
        }

        // 处理视频播放器任务
        if (video_enabled) {
            video_player_task();
        }

        // 🔥 关键：让出 CPU，给中断和调度器有机会运行！
        //yield();

        loop_count++;
    }

    LV_LOG_ERROR("Main loop exited unexpectedly!");
}

// 程序入口点

extern int main(void);
extern char __bss_start, __bss_end;

// ⚠️ 强制 _start 放在 .text.start 段，确保它位于程序最前面
//__attribute__((section(".text.start")))
void _start() {
    // 清 BSS
    // for (char *p = &__bss_start; p < &__bss_end; p++)
    //     *p = 0;

    // 🔥 暂时禁用ESP设置，使用内核设置的栈
    // // 🔥 关键修复：设置栈顶地址为链接脚本中定义的 _stack_top
    // // 确保用户程序使用足够大的栈空间（128KB）
    // extern char _stack_top;
    // __asm__ volatile(
    //     "movl %0, %%esp"
    //     :
    //     : "r"(&_stack_top)
    //     : "esp"
    // );

    
    int ret2 = main();
    exit(ret2);
    //while (1);
}
// void _start() {
//     // 🔥 暂时禁用所有 printf
//     /*
//     const char *msg = "[_start] Reached!";
//     int ret;
//     __asm__ volatile(
//         "int $0x80"
//         : "=a"(ret)
//         : "a"(4), "b"(msg), "d"(__builtin_strlen(msg))
//         : "memory", "cc"
//     );

//     printf("[_start] Entry point\n");
//     */
//     int ret2 = main();
//     // printf("[_start] main() returned=%d, calling exit...\n", ret2);
//     exit(ret2);
// }

/**
 * @brief 主函数
 */
int main(void) {

    if (lvgl_display_init() != 0) {
        // 初始化失败 - 无法继续
        return -1;
    }

    
    create_keyboard_ui();

    
    lvgl_main_loop();

    return 0;
}

/**
 * @brief 屏幕日志函数 - 简化版格式化输出
 * 支持: %d, %u, %x, %s, %c
 *
 * @param fmt 格式化字符串
 * @param ... 可变参数
 */
void screen_log(const char *fmt, ...)
{
    static char buffer[512];
    char *dst = buffer;
    va_list args;

    va_start(args, fmt);

    while (*fmt && dst - buffer < sizeof(buffer) - 1) {
        if (*fmt == '%') {
            fmt++;
            if (*fmt == '\0') break;

            // 解析格式说明符
            switch (*fmt) {
                case 'd': {
                    // 有符号十进制整数
                    int val = va_arg(args, int);
                    if (val < 0) {
                        *dst++ = '-';
                        val = -val;
                    }
                    // 转换为字符串
                    char tmp[16];
                    int i = 0;
                    if (val == 0) {
                        tmp[i++] = '0';
                    } else {
                        while (val > 0 && i < 15) {
                            tmp[i++] = '0' + (val % 10);
                            val /= 10;
                        }
                    }
                    // 反转并复制
                    while (i > 0 && dst - buffer < sizeof(buffer) - 1) {
                        *dst++ = tmp[--i];
                    }
                    break;
                }
                case 'u': {
                    // 无符号十进制整数
                    unsigned int val = va_arg(args, unsigned int);
                    char tmp[16];
                    int i = 0;
                    if (val == 0) {
                        tmp[i++] = '0';
                    } else {
                        while (val > 0 && i < 15) {
                            tmp[i++] = '0' + (val % 10);
                            val /= 10;
                        }
                    }
                    while (i > 0 && dst - buffer < sizeof(buffer) - 1) {
                        *dst++ = tmp[--i];
                    }
                    break;
                }
                case 'x': {
                    // 十六进制整数 (小写)
                    unsigned int val = va_arg(args, unsigned int);
                    char tmp[16];
                    int i = 0;
                    if (val == 0) {
                        tmp[i++] = '0';
                    } else {
                        while (val > 0 && i < 15) {
                            int digit = val % 16;
                            tmp[i++] = (digit < 10) ? '0' + digit : 'a' + digit - 10;
                            val /= 16;
                        }
                    }
                    while (i > 0 && dst - buffer < sizeof(buffer) - 1) {
                        *dst++ = tmp[--i];
                    }
                    break;
                }
                case 's': {
                    // 字符串
                    const char *str = va_arg(args, const char*);
                    if (str) {
                        while (*str && dst - buffer < sizeof(buffer) - 1) {
                            *dst++ = *str++;
                        }
                    } else {
                        const char *null_str = "(null)";
                        while (*null_str && dst - buffer < sizeof(buffer) - 1) {
                            *dst++ = *null_str++;
                        }
                    }
                    break;
                }
                case 'c': {
                    // 字符
                    char c = (char)va_arg(args, int);
                    if (dst - buffer < sizeof(buffer) - 1) {
                        *dst++ = c;
                    }
                    break;
                }
                case '%': {
                    // 转义的 %
                    if (dst - buffer < sizeof(buffer) - 1) {
                        *dst++ = '%';
                    }
                    break;
                }
                default: {
                    // 未知格式，直接输出
                    if (dst - buffer < sizeof(buffer) - 2) {
                        *dst++ = '%';
                        *dst++ = *fmt++;
                    }
                    break;
                }
            }
            fmt++;
        } else {
            // 普通字符
            *dst++ = *fmt++;
        }
    }

    *dst = '\0';
    va_end(args);

    // 输出到标准输出（串口 + VGA）
    int len = dst - buffer;
    int ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_WRITE), "b"(1), "c"(buffer), "d"(len)
        : "memory", "cc"
    );
}
