/**
 * @file lvgl_port.c
 * @brief LVGL 显示驱动移植层
 *
 * 这个文件实现了 LVGL 的显示驱动接口
 * 连接到 VBE framebuffer
 */

#include "libuser_minimal.h"
#include "lvgl_os.h"

extern int gui_get_fb_info(fb_info_t *info);           // 获取帧缓冲区信息
// 声明 log_append 函数（在 lvglanet.c 中定义）
extern void log_append(const char *text);

// Framebuffer 信息
static volatile uint32_t *fb_virt = NULL;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint32_t fb_pitch = 0;

// Flush 调用计数器
static uint32_t flush_count = 0;

/**
 * @brief 显示刷新回调
 */
void lv_display_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    flush_count++;

    // 🔥 调试：每100次刷新打印一次（使用静态字符串避免 sprintf）
    if (flush_count % 100 == 0) {
        if (flush_count == 100) log_append("[LVGL] Flush count: 100");
        else if (flush_count == 200) log_append("[LVGL] Flush count: 200");
        else if (flush_count == 500) log_append("[LVGL] Flush count: 500");
        else if (flush_count == 1000) log_append("[LVGL] Flush count: 1000");
    }

    if (fb_virt == NULL) {
        lv_disp_flush_ready(disp_drv);
        return;
    }

    int32_t x1 = area->x1;
    int32_t y1 = area->y1;
    int32_t x2 = area->x2;
    int32_t y2 = area->y2;
    int32_t w = x2 - x1 + 1;
    int32_t h = y2 - y1 + 1;

    // 边界检查
    if (x1 < 0 || y1 < 0 || x2 >= (int32_t)fb_width || y2 >= (int32_t)fb_height) {
        lv_disp_flush_ready(disp_drv);
        return;
    }

    // 复制像素数据到 framebuffer (XRGB8888 格式)
    uint32_t *src = (uint32_t *)color_p;
    uint32_t pitch_pixels = fb_pitch / 4;

    for (int32_t y = y1; y <= y2; y++) {
        volatile uint32_t *dst = fb_virt + y * pitch_pixels + x1;
        for (int32_t x = 0; x < w; x++) {
            dst[x] = src[x];
        }
        src += w;
    }

    // 🔥 必须调用！通知 LVGL 刷新完成
    lv_disp_flush_ready(disp_drv);
}

/**
 * @brief 初始化 LVGL 显示驱动
 */
int lvgl_display_init(void) {
    log_append("[LVGL] Starting init...");

    // 获取 framebuffer 信息
    log_append("[LVGL] Setting FB info...");
    // 获取 framebuffer 信息
    // fb_info_t fb_info;
    // int ret = gui_get_fb_info(&fb_info);
    // if (ret != 0) {
    //     //printf("[LVGL] ERROR: Failed to get framebuffer info\n");
    //     return -1;
    // }

    // fb_width = fb_info.width;
    // fb_height = fb_info.height;
    // fb_pitch = fb_info.pitch;
    fb_width = 1024;
    fb_height = 768;
    fb_pitch = 4096;

    // 使用虚拟地址（VBE framebuffer 映射地址）
    fb_virt = (volatile uint32_t *)0xF0000000;

    log_append("[LVGL] Calling lv_init...");
    // 初始化 LVGL
    lv_init();

    log_append("[LVGL] Creating draw buffer...");
    // 创建显示缓冲区
    static lv_disp_draw_buf_t draw_buf;
    // 🔥 使用更小的缓冲区：10 行 = 40KB
    #define DRAW_BUF_SIZE (1024 * 10)
    static lv_color_t buf[DRAW_BUF_SIZE];  // 静态分配，避免栈溢出
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, DRAW_BUF_SIZE);

    log_append("[LVGL] Creating display driver...");
    // 创建并注册显示驱动
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    // 设置显示分辨率
    disp_drv.hor_res = fb_width;
    disp_drv.ver_res = fb_height;

    // 设置刷新回调
    disp_drv.flush_cb = lv_display_flush_cb;

    // 设置绘制缓冲区
    disp_drv.draw_buf = &draw_buf;

    log_append("[LVGL] Registering display driver...");
    // 注册显示驱动
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    if (disp == NULL) {
        log_append("[LVGL] ERROR: Failed to register display driver!");
        return -1;
    }

    log_append("[LVGL] Display init complete!");
    return 0;
}

// printf 的 va_list 包装函数（必须在 lv_log 之前定义）
static void vprintf_wrapper(const char *fmt, va_list args) {
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    // 使用 log_append 替代 printf
    log_append(buffer);
}

/**
 * @brief LVGL 日志函数 - 连接到用户空间的 log_append
 *
 * 这个函数由 LV_LOG 宏调用，用于输出 LVGL 的日志信息
 */
void lv_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf_wrapper(fmt, args);
    va_end(args);
}
