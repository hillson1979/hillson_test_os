/**
 * @file gui.c
 * @brief LVGL 图形界面初始化和主循环
 *
 * 这个文件实现了 LVGL 图形库的核心功能：
 * - 显示接口驱动
 * - 输入设备驱动（鼠标、键盘）
 * - LVGL 初始化和任务处理
 */

#include "libuser.h"

// 系统调用包装宏
#define syscall1(num, arg1) \
    __asm__ volatile( \
        "movl $" #num ", %%eax\n" \
        "movl %0, %%ebx\n" \
        "int $0x80\n" \
        : \
        : "r"((uint32_t)(arg1)) \
        : "eax", "ebx", "memory" \
    )

#define syscall5(num, arg1, arg2, arg3, arg4, arg5) \
    __asm__ volatile( \
        "movl $" #num ", %%eax\n" \
        "movl %0, %%ebx\n" \
        "movl %1, %%ecx\n" \
        "movl %2, %%edx\n" \
        "movl %3, %%esi\n" \
        "movl %4, %%edi\n" \
        "int $0x80\n" \
        : \
        : "r"((uint32_t)(arg1)), "r"((uint32_t)(arg2)), "r"((uint32_t)(arg3)), \
          "r"((uint32_t)(arg4)), "r"((uint32_t)(arg5)) \
        : "eax", "ebx", "ecx", "edx", "esi", "edi", "memory" \
    )

// 定义 NULL
#ifndef NULL
#define NULL ((void *)0)
#endif

// LVGL 配置
#define LV_HOR_RES_MAX 1024      // 屏幕水平分辨率
#define LV_VER_RES_MAX 768       // 屏幕垂直分辨率
#define LV_COLOR_DEPTH 16        // 颜色深度（RGB565）

// 显存缓冲区（在 VESA/VGA 模式下的帧缓冲区）
static uint16_t *fb_ptr = (uint16_t *)0xE0000000;  // 默认帧缓冲区地址
static uint32_t fb_width = 1024;
static uint32_t fb_height = 768;
static uint32_t fb_pitch = 2048;  // 每行字节数

// 鼠标状态
typedef struct {
    int x;           // 当前 X 坐标
    int y;           // 当前 Y 坐标
    int left_btn;    // 左键状态 (0=释放, 1=按下)
    int right_btn;   // 右键状态
} mouse_state_t;

static mouse_state_t mouse = {0};

// GUI 系统调用包装函数
int gui_get_fb_info(fb_info_t *info) {
    int ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_GUI_FB_INFO), "b"(info)
        : "memory", "cc"
    );
    return ret;
}

int gui_fb_blit(int x, int y, int width, int height, const void *data) {
    int ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_GUI_FB_BLIT), "b"(x), "c"(y), "d"(width), "S"(height), "D"(data)
        : "memory", "cc"
    );
    return ret;
}

int gui_read_input(input_event_t *event) {
    int ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_GUI_INPUT_READ), "b"(event)
        : "memory", "cc"
    );
    return ret;
}

// 注意: strcmp, memcpy, memset 已经在 libuser.c 中定义，这里不再重复定义

/**
 * @brief 初始化图形显示
 *
 * 设置帧缓冲区地址和分辨率
 * 通过系统调用从内核获取 VBE 帧缓冲区信息
 */
int gui_init_display(void) {
    // 使用系统调用获取帧缓冲区信息
    fb_info_t info;
    int ret = gui_get_fb_info(&info);
    if (ret != 0) {
        printf("GUI: Failed to get framebuffer info\n");
        return -1;
    }

    // 保存帧缓冲区信息
    fb_ptr = (uint16_t *)info.fb_addr;
    fb_width = info.width;
    fb_height = info.height;
    fb_pitch = info.pitch;

    printf("GUI: Display initialized %dx%d at %p (pitch=%d, bpp=%d)\n",
           fb_width, fb_height, fb_ptr, fb_pitch, info.bpp);

    return 0;
}

/**
 * @brief 初始化输入设备
 *
 * 初始化鼠标和键盘驱动
 */
int gui_init_input(void) {
    // TODO: 初始化 PS/2 鼠标和键盘驱动
    printf("GUI: Input devices initialized\n");
    return 0;
}

/**
 * @brief 刷新显示区域
 *
 * @param x1 左上角 X 坐标
 * @param y1 左上角 Y 坐标
 * @param x2 右下角 X 坐标
 * @param y2 右下角 Y 坐标
 */
void gui_flush(int x1, int y1, int x2, int y2) {
    // 在真实实现中，这里会标记脏区域，由显卡驱动刷新
    // 对于直接帧缓冲区访问，写入内存即可自动显示
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
}

/**
 * @brief 读取鼠标状态
 *
 * @return 鼠标状态结构
 */
mouse_state_t gui_read_mouse(void) {
    // TODO: 从 PS/2 鼠标驱动读取真实的鼠标状态
    // 目前返回固定值（屏幕中心）
    mouse.x = 512;
    mouse.y = 384;
    mouse.left_btn = 0;
    mouse.right_btn = 0;
    return mouse;
}

/**
 * @brief GUI 主任务处理
 *
 * 处理 LVGL 的任务，包括：
 * - 动画更新
 * - 事件处理
 * - 刷新显示
 */
void gui_task_handler(void) {
    // TODO: 调用 LVGL 的 lv_task_handler()
    // lv_task_handler();
}

/**
 * @brief GUI 主循环
 *
 * 持续处理 GUI 任务和输入事件
 */
void gui_main_loop(void) {
    printf("GUI: Entering main loop...\n");

    while (1) {
        // 处理 GUI 任务
        gui_task_handler();

        // 读取输入设备
        mouse_state_t m = gui_read_mouse();
        if (m.left_btn) {
            printf("Mouse: x=%d y=%d clicked\n", m.x, m.y);
        }

        // 让出 CPU，避免占用所有时间片
        yield();
    }
}

/**
 * @brief 简单的图形演示
 *
 * 在屏幕上绘制一些简单的图形，用于验证显示功能
 */
void gui_demo(void) {
    printf("GUI: Running simple demo...\n");

    // 检查帧缓冲区是否已初始化
    if (fb_ptr == (uint16_t *)0xE0000000 && fb_width == 1024) {
        printf("GUI: Warning: Using default framebuffer values, may not be correct\n");
        printf("GUI: Please call gui_init() first\n");
    }

    printf("GUI: Clearing screen to black...\n");

    // 清屏为黑色
    for (uint32_t i = 0; i < fb_width * fb_height; i++) {
        fb_ptr[i] = 0x0000;  // RGB565 黑色
    }

    printf("GUI: Drawing red rectangle...\n");

    // 在屏幕中心绘制一个红色矩形
    int rect_x = fb_width / 2 - 100;
    int rect_y = fb_height / 2 - 75;
    int rect_w = 200;
    int rect_h = 150;

    for (int y = rect_y; y < rect_y + rect_h && y < fb_height; y++) {
        for (int x = rect_x; x < rect_x + rect_w && x < fb_width; x++) {
            fb_ptr[y * fb_width + x] = 0xF800;  // RGB565 红色
        }
    }

    printf("GUI: Drawing green rectangle...\n");

    // 在红色矩形内部绘制一个绿色矩形
    int green_x = fb_width / 2 - 50;
    int green_y = fb_height / 2 - 50;
    int green_w = 100;
    int green_h = 100;

    for (int y = green_y; y < green_y + green_h && y < fb_height; y++) {
        for (int x = green_x; x < green_x + green_w && x < fb_width; x++) {
            fb_ptr[y * fb_width + x] = 0x07E0;  // RGB565 绿色
        }
    }

    printf("GUI: Demo complete: rectangles drawn at (%d, %d) size %dx%d\n",
           rect_x, rect_y, rect_w, rect_h);
}

/**
 * @brief 初始化 GUI 系统
 *
 * @return 0 成功，负数失败
 */
int gui_init(void) {
    printf("GUI: Initializing...\n");

    // 初始化显示
    if (gui_init_display() != 0) {
        printf("GUI: Failed to initialize display\n");
        return -1;
    }

    // 初始化输入设备
    if (gui_init_input() != 0) {
        printf("GUI: Failed to initialize input devices\n");
        return -2;
    }

    // TODO: 初始化 LVGL
    // lv_init();

    printf("GUI: Initialization complete\n");
    return 0;
}
