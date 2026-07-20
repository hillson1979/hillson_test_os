/**
 * @file lvglanet.c
 * @brief LVGL 按钮测试程序
 */

#include "libuser_minimal.h"
#include "lvgl_os.h"

#include <string.h>
#include <stdarg.h>

// 系统调用号
#define SYS_GUI_INPUT_READ 72

// 输入事件结构
typedef struct {
    uint32_t type;      // 事件类型: 1=键盘, 2=鼠标
    int x;             // 鼠标 X 或 键码
    int y;             // 鼠标 Y 或 保留
    uint32_t pressed;  // 按键状态或保留
} input_event_t;

// 全局变量
static int mouse_x = 512;  // 当前鼠标 X 位置
static int mouse_y = 384;  // 当前鼠标 Y 位置
static int mouse_pressed = 0;  // 鼠标按钮状态

extern void yield(void);

// 读取鼠标事件
static void read_mouse(void) {
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
}

// LVGL 输入设备读取回调
static void lvgl_mouse_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    read_mouse();

    data->point.x = mouse_x;
    data->point.y = mouse_y;
    data->state = (mouse_pressed & 0x01) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;

    // 调试信息
    static int debug_count = 0;
    if (debug_count % 100 == 0) {
        char buf[128];
        sprintf(buf, "[LVGL MOUSE] x=%d y=%d btn=%d", mouse_x, mouse_y, mouse_pressed);
        log_append(buf);
    }
    debug_count++;
}

// LVGL 按钮点击事件回调
static void btn_event_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        // 按钮被点击，切换背景颜色
        static int btn_clicked = 0;
        btn_clicked = !btn_clicked;

        if (btn_clicked) {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF00FF), 0); // 紫色
            log_append("[LVGL] Button clicked - turned purple");
        } else {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x00FF00), 0); // 绿色
            log_append("[LVGL] Button released - turned green");
        }
    } else if (code == LV_EVENT_PRESSED) {
        log_append("[LVGL] Button pressed down");
    } else if (code == LV_EVENT_RELEASED) {
        log_append("[LVGL] Button released");
    }
}

__attribute__((section(".text.start")))
void _start() {
    int ret2 = main();
    while (1);
}

int main(void) {
    // 初始化 LVGL 显示驱动
    if (lvgl_display_init() != 0) {
        log_append("[LVGL] ERROR: Failed to initialize display driver!");
        return -1;
    }
    log_append("[LVGL] Display initialized successfully");

    // 初始化鼠标输入设备驱动
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lvgl_mouse_read_cb;
    lv_indev_drv_register(&indev_drv);
    log_append("[LVGL] Mouse input driver registered");

    // 创建主屏幕
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0); // 黑色背景

    // 创建 LVGL 按钮
    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(btn, 200, 100);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x00FF00), 0); // 初始绿色

    // 按钮文字
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "Click Me!");
    lv_obj_center(label);

    // 添加事件回调
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);

    // 调试信息
    log_append("[LVGL] Button created and centered on screen");

    // 主循环
    while (1) {
        // 调用 LVGL 任务处理
        lv_timer_handler();

        // 让出 CPU，避免占用所有时间片
        yield();
    }

    return 0;
}
