/**
 * @file lvgl_port.c
 * @brief LVGL 显示驱动移植层
 *
 * 这个文件实现了 LVGL 的显示驱动接口
 * 连接到 VBE framebuffer
 */

#include "libuser_minimal.h"
#include "lvgl_os.h"

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

    // 🔥 每次都在左上角画红色方块（无条件）
    for (int y = 0; y < 50 && y < (int32_t)fb_height; y++) {
        for (int x = 0; x < 50 && x < (int32_t)fb_width; x++) {
            fb_virt[y * (fb_pitch / 4) + x] = 0xFFFF0000;  // 红色
        }
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

    // 🔥 调试：打印前几个像素的颜色
    if (flush_count == 1) {
        // 直接写 framebuffer 显示调试信息
        // 在屏幕顶部显示：第一个像素的颜色值
        uint32_t first_color = src[0];
        for (int y = 0; y < 20; y++) {
            for (int x = 0; x < 20; x++) {
                fb_virt[y * (fb_pitch / 4) + x] = first_color;
            }
        }
    }

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
    printf("[LVGL] Initializing display...\n");

    // 获取 framebuffer 信息
    fb_info_t fb_info;
    int ret = gui_get_fb_info(&fb_info);
    if (ret != 0) {
        printf("[LVGL] ERROR: Failed to get framebuffer info\n");
        return -1;
    }

    fb_width = fb_info.width;
    fb_height = fb_info.height;
    fb_pitch = fb_info.pitch;

    printf("[LVGL] Framebuffer: %dx%d, pitch=%d\n", fb_width, fb_height, fb_pitch);

    // 使用虚拟地址（已经由系统调用映射好了）
    fb_virt = (volatile uint32_t *)0xF0000000;

    // 初始化 LVGL
    lv_init();

    // 创建显示缓冲区
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf[1024 * 100];  // 100 行缓冲区
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, sizeof(buf) / sizeof(lv_color_t));

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

    // 注册显示驱动
    printf("[LVGL] Registering display driver...\n");
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    if (disp == NULL) {
        printf("[LVGL] ERROR: Failed to register display driver!\n");
        return -1;
    }

    printf("[LVGL] Display driver registered successfully!\n");
    printf("[LVGL]   Registered resolution: %dx%d\n",
           lv_disp_get_hor_res(disp), lv_disp_get_ver_res(disp));

    // 创建一个定时器来触发LVGL处理
    lv_timer_t *timer = lv_timer_create(lv_timer_handler, 5, NULL);
    if (!timer) {
        printf("[LVGL] WARNING: Failed to create timer\n");
    } else {
        printf("[LVGL] Timer created successfully\n");
    }

    printf("[LVGL] Display initialized successfully!\n");
    return 0;
}
