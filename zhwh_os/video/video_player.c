#include "video_player.h"
#include <string.h>

/* ====== 外部系统函数（需要OS提供） ====== */
extern uint32_t get_memory_usage(void);      // 获取内存使用量（bytes）
extern uint32_t get_cpu_usage(void);         // 获取CPU占用率（千分比0-1000）
extern void *kmalloc(unsigned int size);
extern void kfree(void *ptr);

/* ====== 双缓冲 ====== */
// 🔥 使用静态分配（在 BSS 中），但放在这里而不是堆中
static uint16_t video_buf[2][VIDEO_W * VIDEO_H];
static int front_buf = 0;
static int back_buf  = 1;

/* ====== LVGL对象 ====== */
static lv_obj_t *video_img;
static lv_img_dsc_t video_img_dsc;

/* ====== JPEG接收缓冲 ====== */
// 🔥 使用静态分配（在 BSS 中）
static uint8_t jpeg_buf[MAX_JPEG];

/* ====== 带宽计算 ====== */
static uint32_t last_bytes = 0;
static uint32_t last_time = 0;

/* ====== 状态统计 ====== */
static video_stats_t stats = {0};
static int video_initialized = 0;

/* ====== 初始化 ====== */
void video_player_init(void)
{
    if (video_initialized) {
        return;  // 已经初始化过了
    }

    // 🔥 静态缓冲区已在 BSS 中分配，无需动态分配
    memset(video_buf[0], 0, VIDEO_W * VIDEO_H * sizeof(uint16_t));
    memset(video_buf[1], 0, VIDEO_W * VIDEO_H * sizeof(uint16_t));
    memset(jpeg_buf, 0, MAX_JPEG);
    memset(&stats, 0, sizeof(stats));

    video_img = lv_img_create(lv_scr_act());

    video_img_dsc.header.always_zero = 0;
    video_img_dsc.header.w = VIDEO_W;
    video_img_dsc.header.h = VIDEO_H;
    video_img_dsc.data_size = VIDEO_W * VIDEO_H * sizeof(uint16_t);
    video_img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    video_img_dsc.data = (uint8_t *)video_buf[front_buf];

    lv_img_set_src(video_img, &video_img_dsc);
    lv_obj_center(video_img);

    stats.init_time = lv_tick_get();
    video_initialized = 1;
}

/* ====== 提交一帧给LVGL ====== */
static void video_submit_frame(void)
{
    int tmp = front_buf;
    front_buf = back_buf;
    back_buf = tmp;

    video_img_dsc.data = (uint8_t *)video_buf[front_buf];
    lv_img_set_src(video_img, &video_img_dsc);
    lv_obj_invalidate(video_img);

    stats.frames_displayed++;
}

/* ====== 播放任务（循环调用） ====== */
void video_player_task(void)
{
    int len;
    uint32_t decode_start, decode_end;

    /* 🔥 确保已初始化 */
    if (!video_initialized) {
        return;
    }

    /* 更新运行时间 */
    stats.uptime = lv_tick_get() - stats.init_time;

    /* 1. 接收一帧 JPEG */
    len = net_recv_frame(jpeg_buf, MAX_JPEG);
    if (len <= 0) {
        stats.recv_errors++;
        return;
    }

    stats.bytes_received += len;
    stats.total_packets++;

    /* 2. 解码成 RGB565 到 back_buf（测量解码时间） */
    decode_start = lv_tick_get();
    if (jpeg_decode_to_rgb565(jpeg_buf, len, video_buf[back_buf]) < 0) {
        stats.decode_errors++;
        return;
    }
    decode_end = lv_tick_get();

    /* 更新解码时间统计 */
    uint32_t decode_time = decode_end - decode_start;
    stats.total_decode_time += decode_time;

    if (decode_time > stats.max_decode_time) {
        stats.max_decode_time = decode_time;
    }

    if (stats.frames_displayed > 0) {
        stats.avg_decode_time = stats.total_decode_time / stats.frames_displayed;
    }

    /* 3. 提交给 LVGL */
    video_submit_frame();

    /* 4. 更新带宽统计（每秒计算一次） */
    uint32_t current_time = lv_tick_get();
    if (current_time - last_time >= 1000) {
        uint32_t bytes_delta = stats.bytes_received - last_bytes;
        stats.last_bandwidth = bytes_delta;  // bytes per second
        stats.avg_bandwidth = stats.bytes_received / (stats.uptime / 1000 + 1);
        last_bytes = stats.bytes_received;
        last_time = current_time;

        /* 5. 更新内存和CPU统计（如果OS支持） */
        #ifdef HAS_MEMORY_STATS
        stats.memory_used = get_memory_usage();
        if (stats.memory_used > stats.memory_peak) {
            stats.memory_peak = stats.memory_used;
        }
        #endif

        #ifdef HAS_CPU_STATS
        stats.cpu_usage = get_cpu_usage();
        stats.cpu_time_used += (stats.cpu_usage * (current_time - stats.last_update_time)) / 1000;
        #endif

        stats.last_update_time = current_time;
    }
}

/* ====== 获取统计信息 ====== */
const video_stats_t* video_player_get_stats(void)
{
    uint32_t elapsed = lv_tick_get() - stats.init_time;
    if (elapsed > 0) {
        stats.fps = (stats.frames_displayed * 1000) / elapsed;
    }
    return &stats;
}

/* ====== 重置统计信息 ====== */
void video_player_reset_stats(void)
{
    memset(&stats, 0, sizeof(stats));
    stats.init_time = lv_tick_get();
}
