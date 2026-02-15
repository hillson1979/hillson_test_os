/**
 * @file lvgl_disp.c
 * @brief LVGL 显示驱动 - 直接连接 Multiboot2 framebuffer
 *
 * 这个文件实现了 LVGL 的显示驱动
 * 直接使用 VBE 驱动映射的 framebuffer
 */

#include "types.h"
#include "vbe.h"

// LVGL 头文件
#include "../lvgl/lv_conf.h"
#include "../lvgl/src/display/lv_display.h"

// 定义 NULL
#ifndef NULL
#define NULL ((void *)0)
#endif

// Framebuffer 映射信息
// VBE 驱动将物理地址映射到固定的虚拟地址 0xF0000000
#define FB_VIRT_ADDR 0xF0000000
static volatile uint32_t *fb_virt = NULL;
static uint16_t fb_width = 0;
static uint16_t fb_height = 0;
static uint16_t fb_pitch = 0;

/**
 * @brief 初始化 LVGL 显示驱动
 */
void lv_port_disp_init(void) {
    // 获取 framebuffer 信息
    uint32_t fb_phys = vbe_get_framebuffer();
    vbe_get_resolution(&fb_width, &fb_height);
    fb_pitch = vbe_get_pitch();
    uint8_t fb_bpp = vbe_get_bpp();

    printf("[LVGL] Display port initialization:\n");
    printf("[LVGL]   Physical address: 0x%x\n", fb_phys);
    printf("[LVGL]   Virtual address:  0x%x\n", FB_VIRT_ADDR);
    printf("[LVGL]   Resolution: %dx%d\n", fb_width, fb_height);
    printf("[LVGL]   BPP: %d, Pitch: %d\n", fb_bpp, fb_pitch);

    // 设置虚拟地址
    fb_virt = (volatile uint32_t *)FB_VIRT_ADDR;

    if (fb_bpp != 32) {
        printf("[LVGL] ERROR: Only 32-bit BPP is supported, got %d\n", fb_bpp);
        return;
    }

    // 清空屏幕 (黑色)
    for (uint32_t i = 0; i < (fb_pitch / 4) * fb_height; i++) {
        fb_virt[i] = 0xFF000000;  // XRGB8888 黑色
    }

    // 创建显示驱动
    static lv_display_t *disp;
    disp = lv_display_create(fb_width, fb_height);

    // 设置刷新回调 - LVGL 渲染完成后调用
    lv_display_set_flush_cb(disp, lv_port_disp_flush);

    // 设置默认渲染模式为直接渲染
    lv_display_set_default(disp);

    printf("[LVGL] ✓ Display driver initialized\n");
}

/**
 * @brief 刷新显示回调 - LVGL 调用这个来更新屏幕
 * @param disp 显示对象
 * @param area 要刷新的区域
 * @param px_map 像素数据
 */
// Flush 调用计数器
static uint32_t flush_count = 0;

void lv_port_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    flush_count++;

    // 每10次flush打印一次，避免输出太多
    if (flush_count % 10 == 0 || flush_count <= 3) {
        printf("[LVGL FLUSH] #%d - Area: (%d,%d) to (%d,%d)\n",
               flush_count, area->x1, area->y1, area->x2, area->y2);
    }

    if (fb_virt == NULL) {
        printf("[LVGL] ERROR: Framebuffer not mapped!\n");
        lv_display_flush_ready(disp);
        return;
    }

    // 计算区域参数
    int32_t x1 = area->x1;
    int32_t y1 = area->y1;
    int32_t x2 = area->x2;
    int32_t y2 = area->y2;
    int32_t w = x2 - x1 + 1;
    int32_t h = y2 - y1 + 1;

    // 确保坐标在有效范围内
    if (x1 < 0 || y1 < 0 || x2 >= fb_width || y2 >= fb_height) {
        printf("[LVGL FLUSH] WARNING: Area out of bounds: (%d,%d)-(%d,%d), screen: %dx%d\n",
               x1, y1, x2, y2, fb_width, fb_height);
        lv_display_flush_ready(disp);
        return;
    }

    // 将 LVGL 像素数据复制到 framebuffer
    // LVGL 使用 XRGB8888 格式 (32位)
    uint32_t *src = (uint32_t *)px_map;

    for (int32_t y = y1; y <= y2; y++) {
        // 计算 framebuffer 中的起始位置
        volatile uint32_t *dst = fb_virt + y * (fb_pitch / 4) + x1;

        // 复制一行像素
        for (int32_t x = 0; x < w; x++) {
            dst[x] = src[x];
        }

        src += w;  // 移动到下一行源数据
    }

    // 通知 LVGL 刷新完成
    lv_display_flush_ready(disp);
}

/**
 * @brief LVGL 日志输出
 */
void lv_log_print(lv_log_level_t level, const char *buf) {
    printf("[LVGL] %s\n", buf);
}

/**
 * @brief 获取当前滴答数（毫秒）
 */
uint32_t lv_tick_get(void) {
    // TODO: 从系统获取当前时间（毫秒）
    static uint32_t tick = 0;
    return tick;
}

/**
 * @brief 增加滴答数
 */
void lv_tick_inc(uint32_t tick_period) {
    (void)tick_period;
}
