/**
 * @file text_editor.c
 * @brief 轻量级文本编辑器（Sublime 风格）
 *
 * 一个基于 LVGL 的轻量级文本编辑器，支持：
 * - 基本文本编辑
 * - 文件加载/保存
 * - 查找/替换
 * - 简单的代码高亮
 * - 键盘/鼠标操作
 */

#include "libuser_minimal.h"
#include "lvgl_os.h"

#include <lvgl.h>
#include <string.h>
#include <stdarg.h>

// 系统调用号
#define SYS_GUI_INPUT_READ 72
#define SYS_USB_MOUSE_POLL 73

// 输入事件结构
typedef struct {
    uint32_t type;      // 事件类型: 1=键盘, 2=鼠标
    int x;             // 鼠标 X 或 键码
    int y;             // 鼠标 Y 或 保留
    uint32_t pressed;  // 按键状态或保留
} input_event_t;

// 键码定义
#define KEY_TAB         0x0F
#define KEY_ENTER       0x1C
#define KEY_ESCAPE      0x01
#define KEY_UP          0x48  // 上箭头
#define KEY_DOWN        0x50  // 下箭头
#define KEY_LEFT        0x4B  // 左箭头
#define KEY_RIGHT       0x4D  // 右箭头
#define KEY_LSHIFT      0x2A
#define KEY_RSHIFT      0x36
#define KEY_RELEASED    0x80
#define KEY_BACKSPACE   0x0E
#define KEY_DELETE      0x53

// 全局变量
static int mouse_x = 512;  // 当前鼠标 X 位置
static int mouse_y = 384;  // 当前鼠标 Y 位置
static int mouse_pressed = 0;  // 鼠标按钮状态

static lv_obj_t *text_area = NULL;      // 主编辑区域
static lv_obj_t *status_label = NULL;   // 状态栏
static lv_obj_t *file_label = NULL;     // 文件名标签
static lv_obj_t *find_panel = NULL;     // 查找面板
static lv_obj_t *find_input = NULL;    // 查找输入框
static lv_obj_t *replace_input = NULL; // 替换输入框

static char current_file[256] = "";     // 当前打开的文件名
static int file_modified = 0;          // 文件是否已修改

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

static int shift_pressed = 0;  // Shift键状态

/**
 * @brief 将扫描码转换为 ASCII 字符
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

    const char *table = shift_pressed ? scancode_to_ascii_shift_table : scancode_to_ascii_table;
    return table[key_code];
}

/**
 * @brief 将扫描码转换为 LVGL 键值
 */
static uint32_t scancode_to_lvgl_key(uint32_t scancode) {
    uint32_t key_code = scancode & 0x7F;

    switch (key_code) {
        case 0x01: return LV_KEY_ESC;    // ESC
        case 0x0F: return LV_KEY_TAB;    // TAB
        case 0x1C: return LV_KEY_ENTER;  // ENTER
        case 0x0E: return LV_KEY_BACKSPACE;  // BACKSPACE
        case 0x53: return LV_KEY_DEL;    // DELETE
        case 0x48: return LV_KEY_UP;     // Up
        case 0x50: return LV_KEY_DOWN;   // Down
        case 0x4B: return LV_KEY_LEFT;   // Left
        case 0x4D: return LV_KEY_RIGHT;  // Right
        default:   return 0;
    }
}

/**
 * @brief 键盘输入读取回调
 */
static void keyboard_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    input_event_t event;
    int ret;

    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_GUI_INPUT_READ), "b"(&event), "c"(1)
        : "memory", "cc"
    );

    if (ret == 1) {
        uint32_t scancode = event.x;
        if (scancode == 0x3B && !(scancode & 0x80)) { on_f1_pressed(); data->state = LV_INDEV_STATE_REL; data->key = 0; return; }
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
            data->state = LV_INDEV_STATE_REL;
            data->key = 0;
            return;
        }

        data->state = LV_INDEV_STATE_PR;

        // 映射到LVGL键值
        uint32_t lv_key = scancode_to_lvgl_key(scancode);
        if (lv_key != 0) {
            data->key = lv_key;
        } else {
            char ascii_char = scancode_to_ascii(scancode);
            data->key = ascii_char;
        }
    } else {
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

    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_GUI_INPUT_READ), "b"(&event), "c"(2)
        : "memory", "cc"
    );

    if (ret == 1) {
        mouse_x = event.x;
        mouse_y = event.y;
        mouse_pressed = event.pressed;
    }

    data->point.x = mouse_x;
    data->point.y = mouse_y;
    data->state = (mouse_pressed & 0x01) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;

    return false;
}

/**
 * @brief 打开文件
 */
static void open_file(const char *filename) {
    int fd = open(filename, 0);  // 只读模式
    if (fd < 0) {
        lv_label_set_text(status_label, "Error: Could not open file");
        return;
    }

    // 读取文件内容
    char buffer[8192];
    int bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (bytes_read < 0) {
        lv_label_set_text(status_label, "Error: Could not read file");
        return;
    }

    buffer[bytes_read] = '\0';
    lv_textarea_set_text(text_area, buffer);

    strncpy(current_file, filename, sizeof(current_file) - 1);
    current_file[sizeof(current_file) - 1] = '\0';

    lv_label_set_text(file_label, filename);
    lv_label_set_text(status_label, "File loaded successfully");
    file_modified = 0;
}

/**
 * @brief 保存文件
 */
static void save_file(const char *filename) {
    int fd = open(filename, 0x40 | 0x02);  // 读写模式，如果文件不存在则创建
    if (fd < 0) {
        lv_label_set_text(status_label, "Error: Could not open file for writing");
        return;
    }

    const char *text = lv_textarea_get_text(text_area);
    int len = strlen(text);

    if (write(fd, text, len) != len) {
        lv_label_set_text(status_label, "Error: Could not write file");
        close(fd);
        return;
    }

    close(fd);

    strncpy(current_file, filename, sizeof(current_file) - 1);
    current_file[sizeof(current_file) - 1] = '\0';

    lv_label_set_text(file_label, filename);
    lv_label_set_text(status_label, "File saved successfully");
    file_modified = 0;
}

/**
 * @brief 查找文本
 */
static void find_text(const char *text) {
    if (!text || *text == '\0') {
        lv_label_set_text(status_label, "Error: Search text is empty");
        return;
    }

    const char *content = lv_textarea_get_text(text_area);
    const char *pos = strstr(content, text);

    if (pos) {
        int index = pos - content;
        int len = strlen(text);

        // 简单的高亮显示（这里可以用更高级的方法）
        lv_label_set_text(status_label, "Found text");
    } else {
        lv_label_set_text(status_label, "Text not found");
    }
}

/**
 * @brief 替换文本
 */
static void replace_text(const char *find, const char *replace) {
    if (!find || *find == '\0') {
        lv_label_set_text(status_label, "Error: Search text is empty");
        return;
    }

    const char *content = lv_textarea_get_text(text_area);
    char *new_content = video_kmalloc(strlen(content) * 2 + 1);
    if (!new_content) {
        lv_label_set_text(status_label, "Error: Out of memory");
        return;
    }

    new_content[0] = '\0';
    const char *pos = content;

    while ((pos = strstr(pos, find)) != NULL) {
        strncat(new_content, content, pos - content);
        strcat(new_content, replace);
        pos += strlen(find);
        content = pos;
    }

    strcat(new_content, content);
    lv_textarea_set_text(text_area, new_content);
    video_kfree(new_content);

    lv_label_set_text(status_label, "Text replaced");
    file_modified = 1;
}

/**
 * @brief 文件打开按钮回调
 */
static void btn_open_callback(lv_event_t *e) {
    // 简单实现：打开固定路径的文件
    open_file("/test.txt");
}

/**
 * @brief F1 key handler: open kernel log
 */
static void on_f1_pressed(void) {
    open_file("/kern.log");
}

/**
 * @brief 文件保存按钮回调
 */
static void btn_save_callback(lv_event_t *e) {
    if (current_file[0]) {
        save_file(current_file);
    } else {
        save_file("/test.txt");
    }
}

/**
 * @brief 查找按钮回调
 */
static void btn_find_callback(lv_event_t *e) {
    if (find_panel) {
        lv_obj_toggle_flag(find_panel, LV_OBJ_FLAG_HIDDEN);
    }
}

/**
 * @brief 查找执行按钮回调
 */
static void btn_find_execute_callback(lv_event_t *e) {
    const char *text = lv_textarea_get_text(find_input);
    find_text(text);
}

/**
 * @brief 替换执行按钮回调
 */
static void btn_replace_execute_callback(lv_event_t *e) {
    const char *find = lv_textarea_get_text(find_input);
    const char *replace = lv_textarea_get_text(replace_input);
    replace_text(find, replace);
}

/**
 * @brief 文本区域内容变化回调
 */
static void textarea_changed_callback(lv_event_t *e) {
    file_modified = 1;
}

/**
 * @brief 创建查找面板
 */
static void create_find_panel(lv_obj_t *parent) {
    find_panel = lv_obj_create(parent);
    lv_obj_set_size(find_panel, 400, 100);
    lv_obj_align(find_panel, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_add_flag(find_panel, LV_OBJ_FLAG_HIDDEN);

    // 查找标签
    lv_obj_t *find_label = lv_label_create(find_panel);
    lv_label_set_text(find_label, "Find:");
    lv_obj_set_pos(find_label, 10, 10);

    // 查找输入框
    find_input = lv_textarea_create(find_panel);
    lv_obj_set_size(find_input, 180, 30);
    lv_obj_set_pos(find_input, 50, 10);

    // 替换标签
    lv_obj_t *replace_label = lv_label_create(find_panel);
    lv_label_set_text(replace_label, "Replace:");
    lv_obj_set_pos(replace_label, 10, 50);

    // 替换输入框
    replace_input = lv_textarea_create(find_panel);
    lv_obj_set_size(replace_input, 180, 30);
    lv_obj_set_pos(replace_input, 70, 50);

    // 查找按钮
    lv_obj_t *btn_find_exec = lv_btn_create(find_panel);
    lv_obj_set_size(btn_find_exec, 70, 30);
    lv_obj_set_pos(btn_find_exec, 250, 10);
    lv_obj_t *btn_find_label = lv_label_create(btn_find_exec);
    lv_label_set_text(btn_find_label, "Find");
    lv_obj_center(btn_find_label);
    lv_obj_add_event_cb(btn_find_exec, btn_find_execute_callback, LV_EVENT_CLICKED, NULL);

    // 替换按钮
    lv_obj_t *btn_replace_exec = lv_btn_create(find_panel);
    lv_obj_set_size(btn_replace_exec, 70, 30);
    lv_obj_set_pos(btn_replace_exec, 250, 50);
    lv_obj_t *btn_replace_label = lv_label_create(btn_replace_exec);
    lv_label_set_text(btn_replace_label, "Replace");
    lv_obj_center(btn_replace_label);
    lv_obj_add_event_cb(btn_replace_exec, btn_replace_execute_callback, LV_EVENT_CLICKED, NULL);
}

/**
 * @brief 创建编辑器界面
 */
static void create_editor_ui(void) {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_palette_main(LV_PALETTE_GRAY), 0);

    // 菜单栏
    lv_obj_t *menu_bar = lv_obj_create(scr);
    lv_obj_set_size(menu_bar, lv_disp_get_hor_res(NULL), 40);
    lv_obj_align(menu_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(menu_bar, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_border_width(menu_bar, 0, 0);

    // 文件打开按钮
    lv_obj_t *btn_open = lv_btn_create(menu_bar);
    lv_obj_set_size(btn_open, 80, 30);
    lv_obj_set_pos(btn_open, 10, 5);
    lv_obj_t *btn_open_label = lv_label_create(btn_open);
    lv_label_set_text(btn_open_label, "Open");
    lv_obj_center(btn_open_label);
    lv_obj_add_event_cb(btn_open, btn_open_callback, LV_EVENT_CLICKED, NULL);

    // 文件保存按钮
    lv_obj_t *btn_save = lv_btn_create(menu_bar);
    lv_obj_set_size(btn_save, 80, 30);
    lv_obj_set_pos(btn_save, 100, 5);
    lv_obj_t *btn_save_label = lv_label_create(btn_save);
    lv_label_set_text(btn_save_label, "Save");
    lv_obj_center(btn_save_label);
    lv_obj_add_event_cb(btn_save, btn_save_callback, LV_EVENT_CLICKED, NULL);

    // 查找按钮
    lv_obj_t *btn_find = lv_btn_create(menu_bar);
    lv_obj_set_size(btn_find, 80, 30);
    lv_obj_set_pos(btn_find, 190, 5);
    lv_obj_t *btn_find_label = lv_label_create(btn_find);
    lv_label_set_text(btn_find_label, "Find");
    lv_obj_center(btn_find_label);
    lv_obj_add_event_cb(btn_find, btn_find_callback, LV_EVENT_CLICKED, NULL);

    // 文件名标签
    file_label = lv_label_create(menu_bar);
    lv_label_set_text(file_label, "Untitled");
    lv_obj_set_style_text_color(file_label, lv_color_white(), 0);
    lv_obj_align(file_label, LV_ALIGN_CENTER, 0, 0);

    // 主编辑区域
    text_area = lv_textarea_create(scr);
    lv_obj_set_size(text_area, lv_disp_get_hor_res(NULL) - 20, lv_disp_get_ver_res(NULL) - 100);
    lv_obj_align(text_area, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_color(text_area, lv_color_white(), 0);
    lv_obj_set_style_text_color(text_area, lv_color_black(), 0);
    lv_obj_set_style_border_width(text_area, 1, 0);
    lv_obj_set_style_border_color(text_area, lv_palette_main(LV_PALETTE_GRAY), 0);

    lv_textarea_set_placeholder_text(text_area, "Type here...");
    lv_textarea_set_one_line(text_area, false);
    lv_obj_add_event_cb(text_area, textarea_changed_callback, LV_EVENT_VALUE_CHANGED, NULL);

    // 状态栏
    status_label = lv_label_create(scr);
    lv_label_set_text(status_label, "Ready");
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_text_color(status_label, lv_color_white(), 0);

    // 创建查找面板
    create_find_panel(scr);
}

/**
 * @brief 主循环
 */
extern void yield(void);
static uint32_t tick_count = 0;

void text_editor_main_loop(void) {
    while (1) {
        tick_count++;
        lv_tick_inc(5);
        lv_timer_handler();

        yield();
    }
}

/**
 * @brief 程序入口点
 */
__attribute__((section(".text.start")))
void _start() {
    int ret2 = main();
    while (1);
}

int main(void) {
    fb_info_t fb_info;
    int ret = gui_get_fb_info(&fb_info);
    if (ret != 0) {
        return -1;
    }

    // 初始化 LVGL
    if (lvgl_display_init() != 0) {
        return -1;
    }

    // 创建界面
    create_editor_ui();

    // 创建键盘输入设备
    static lv_indev_drv_t keyboard_drv;
    lv_indev_drv_init(&keyboard_drv);
    keyboard_drv.type = LV_INDEV_TYPE_KEYPAD;
    keyboard_drv.read_cb = keyboard_read;
    lv_indev_drv_register(&keyboard_drv);

    // 创建鼠标输入设备
    static lv_indev_drv_t mouse_drv;
    lv_indev_drv_init(&mouse_drv);
    mouse_drv.type = LV_INDEV_TYPE_POINTER;
    mouse_drv.read_cb = mouse_read;
    lv_indev_drv_register(&mouse_drv);

    // Load kernel log on startup
    open_file("/kern.log");

    // 主循环
    text_editor_main_loop();

    return 0;
}
