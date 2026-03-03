/*
 * 视频播放器集成示例
 *
 * 集成到你的OS主循环中
 */

#include "video_player.h"
#include <stdio.h>

#define VIDEO_PORT 1234
#define VIDEO_BIND_IP "0.0.0.0"

/* ====== 示例1：最简单的UDP集成 ====== */
void example_udp_simple(void)
{
    /* 初始化网络接收器（UDP模式） */
    if (net_recv_init(VIDEO_BIND_IP, VIDEO_PORT, 1) < 0) {
        printf("Failed to init UDP receiver\n");
        return;
    }

    /* 初始化视频播放器 */
    video_player_init();

    printf("Video player ready, waiting for UDP stream on port %d...\n", VIDEO_PORT);
    printf("FFmpeg command:\n");
    printf("ffmpeg -re -i test.mp4 -vf scale=320:240 -f mjpeg udp://<YOUR_IP>:%d\n", VIDEO_PORT);

    /* 主循环 */
    while (1) {
        video_player_task();   /* 处理视频帧 */
        lv_timer_handler();    /* LVGL定时器处理 */
        /* 其他OS任务... */
    }
}

/* ====== 示例2：带统计信息的UDP播放 ====== */
void example_udp_with_stats(void)
{
    uint32_t last_stats_time = 0;
    const video_stats_t *stats;

    net_recv_init(VIDEO_BIND_IP, VIDEO_PORT, 1);
    video_player_init();

    while (1) {
        video_player_task();
        lv_timer_handler();

        /* 每5秒打印一次统计信息 */
        if (lv_tick_get() - last_stats_time > 5000) {
            stats = video_player_get_stats();
            printf("=== Video Stats ===\n");
            printf("Frames: %u\n", stats->frames_displayed);
            printf("Received: %u bytes\n", stats->bytes_received);
            printf("Recv errors: %u\n", stats->recv_errors);
            printf("Decode errors: %u\n", stats->decode_errors);
            printf("FPS: %u\n", stats->fps);
            printf("===================\n");

            last_stats_time = lv_tick_get();
        }
    }
}

/* ====== 示例3：TCP模式 ====== */
void example_tcp_mode(void)
{
    net_recv_init(VIDEO_BIND_IP, VIDEO_PORT, 0);  /* TCP模式 */
    video_player_init();

    printf("Waiting for TCP connection on port %d...\n", VIDEO_PORT);

    /* 等待客户端连接 */
    if (net_recv_accept() < 0) {
        printf("Accept failed\n");
        return;
    }

    printf("Client connected!\n");

    while (1) {
        video_player_task();
        lv_timer_handler();
    }
}

/* ====== 示例4：集成到现有OS主循环 ====== */
/*
 * 在你的主函数中：
 *
 * int main(void)
 * {
 *     // ... 现有初始化代码 ...
 *     lv_init();
 *     // ... 显示驱动初始化 ...
 *
 *     // 添加视频播放器初始化
 *     net_recv_init(VIDEO_BIND_IP, VIDEO_PORT, 1);
 *     video_player_init();
 *
 *     // 主循环
 *     while (1) {
 *         // 添加视频任务
 *         video_player_task();
 *
 *         // 现有代码
 *         lv_timer_handler();
 *         // ... 其他任务 ...
 *     }
 * }
 */

/* ====== 示例5：FFmpeg推送命令 ====== */
/*
 * PC端推流命令：
 *
 * 1. 基础MJPEG推流：
 *    ffmpeg -re -i test.mp4 -vf scale=320:240 -f mjpeg udp://<OS_IP>:1234
 *
 * 2. 使用QSV硬件加速：
 *    ffmpeg -hwaccel qsv -i test.mp4 -c:v mjpeg_qsv -vf scale=320:240 -f mjpeg udp://<OS_IP>:1234
 *
 * 3. TCP推流：
 *    ffmpeg -re -i test.mp4 -vf scale=320:240 -f mjpeg tcp://<OS_IP>:1234
 *
 * 4. 摄像头实时推流：
 *    ffmpeg -f v4l2 -i /dev/video0 -vf scale=320:240 -f mjpeg udp://<OS_IP>:1234
 */
