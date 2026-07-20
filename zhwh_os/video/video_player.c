#include "video_player.h"
#include <string.h>
#include "libuser_minimal.h"

/* ====== 外部系统函数（需要OS提供） ====== */
extern uint32_t get_memory_usage(void);      // 获取内存使用量（bytes）
extern uint32_t get_cpu_usage(void);         // 获取CPU占用率（千分比0-1000）
extern void *kmalloc(unsigned int size);
extern void kfree(void *ptr);
extern int printf(const char *fmt, ...);

/* ====== 双缓冲 ====== */
// 🔥 使用静态分配（在 BSS 中），但放在这里而不是堆中
static uint32_t video_buf[2][VIDEO_W * VIDEO_H];  // 现在使用 32 位 XRGB8888
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

/* ====== 函数声明 ====== */
static void video_submit_frame(void);

/* ====== 初始化 ====== */
void video_player_init(void)
{
    printf("[VIDEO] video_player_init() called\n");

    if (video_initialized) {
        printf("[VIDEO] Already initialized!\n");
        return;  // 已经初始化过了
    }

    // 🔥 静态缓冲区已在 BSS 中分配，无需动态分配
    memset(video_buf[0], 0, VIDEO_W * VIDEO_H * sizeof(uint32_t));
    memset(video_buf[1], 0, VIDEO_W * VIDEO_H * sizeof(uint32_t));
    memset(jpeg_buf, 0, MAX_JPEG);
    memset(&stats, 0, sizeof(stats));

    printf("[VIDEO] Creating LVGL image object\n");
    video_img = lv_img_create(lv_scr_act());
    lv_obj_set_size(video_img, VIDEO_W, VIDEO_H);
    lv_obj_set_pos(video_img, (1024 - VIDEO_W)/2, 768 - VIDEO_H - 10);  // 放在界面最下方
    lv_obj_set_style_border_width(video_img, 2, 0);
    lv_obj_set_style_border_color(video_img, lv_palette_main(LV_PALETTE_CYAN), 0);

    printf("[VIDEO] Setting up image descriptor\n");
    video_img_dsc.header.always_zero = 0;
    video_img_dsc.header.w = VIDEO_W;
    video_img_dsc.header.h = VIDEO_H;
    video_img_dsc.data_size = VIDEO_W * VIDEO_H * sizeof(uint32_t);
    video_img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;  // 现在匹配 LV_COLOR_DEPTH 32
    video_img_dsc.data = (uint8_t *)video_buf[front_buf];

    lv_img_set_src(video_img, &video_img_dsc);

    printf("[VIDEO] Drawing test pattern\n");
    // 🔥 调试：显示一个彩色测试画面，验证显示路径是否正常
    // 画渐变色彩条 - 保存到 front_buf (0)
    for (int y = 0; y < VIDEO_H; y++) {
        for (int x = 0; x < VIDEO_W; x++) {
            // 创建彩色渐变
            uint8_t r = (x * 255) / VIDEO_W;
            uint8_t g = (y * 255) / VIDEO_H;
            uint8_t b = 255 - ((x + y) * 255) / (VIDEO_W + VIDEO_H);
            video_buf[front_buf][y * VIDEO_W + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }

    printf("[VIDEO] Displaying test pattern\n");
    // 通过 LVGL 显示
    video_img_dsc.data = (uint8_t *)video_buf[front_buf];
    lv_img_set_src(video_img, &video_img_dsc);
    lv_obj_invalidate(video_img);
    lv_refr_now(NULL);  // 立即刷新

    stats.init_time = lv_tick_get();
    video_initialized = 1;
    printf("[VIDEO] video_player_init() complete!\n");
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
    static int task_call_count = 0;

    /* 🔥 确保已初始化 */
    if (!video_initialized) {
        return;
    }

    task_call_count++;

    // 每 1000 次调用打印一次日志，避免刷屏
    if (task_call_count % 1000 == 0) {
        printf("[VIDEO] video_player_task called %d times\n", task_call_count);
    }

    /* 更新运行时间 */
    stats.uptime = lv_tick_get() - stats.init_time;

    /* 1. 接收一帧 JPEG */
    static int packet_count = 0;
    len = net_recv_frame(jpeg_buf, MAX_JPEG);
    if (len <= 0) {
        stats.recv_errors++;
        if (stats.recv_errors == 1 || stats.recv_errors % 100 == 0) {
            printf("[VIDEO] No data received (%d errors)\n", stats.recv_errors);
        }
        return;
    }

    stats.bytes_received += len;
    stats.total_packets++;
    packet_count++;

    // 🔥 调试：打印每个包的大小
    printf("[VIDEO] Packet %d: %d bytes received\n", packet_count, len);
    printf("[VIDEO] JPEG header: 0x%02x%02x%02x%02x\n",
           jpeg_buf[0], jpeg_buf[1], jpeg_buf[2], jpeg_buf[3]);

    /* 2. 解码成 XRGB8888 (32位颜色) 到 back_buf（测量解码时间） */
    decode_start = lv_tick_get();
    if (jpeg_decode_to_rgb565(jpeg_buf, len, (uint16_t *)video_buf[back_buf]) < 0) {
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

/* ====== 获取当前显示的图像描述符 ====== */
lv_img_dsc_t* video_player_get_current_image(void)
{
    if (!video_initialized) {
        return NULL;
    }
    return &video_img_dsc;
}

/* ====== 获取视频图像对象 ====== */
lv_obj_t* video_player_get_image_obj(void)
{
    return video_img;
}

/* ====== 重置统计信息 ====== */
void video_player_reset_stats(void)
{
    memset(&stats, 0, sizeof(stats));
    stats.init_time = lv_tick_get();
}
