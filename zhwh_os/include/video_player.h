#ifndef VIDEO_PLAYER_H
#define VIDEO_PLAYER_H

#include "lvgl.h"
#include <stdint.h>

// BSD socket兼容类型
#ifndef socklen_t
typedef uint32_t socklen_t;
#endif

#ifndef ssize_t
typedef int ssize_t;
#endif

struct sockaddr {
    uint16_t sa_family;
    char sa_data[14];
};

struct sockaddr_in {
    uint16_t sin_family;
    uint16_t sin_port;
    uint32_t sin_addr;
    char sin_zero[8];
};

// socket函数（需要在OS中实现）
int socket(int domain, int type, int protocol);
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int listen(int sockfd, int backlog);
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
int close(int fd);
int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);

// 地址转换（已在net.h中定义）
#define AF_INET 2
#define SOCK_STREAM 1
#define SOCK_DGRAM 2
#define SOL_SOCKET 1
#define SO_REUSEADDR 2

/* ====== 可配置参数 ====== */
#define VIDEO_W     320
#define VIDEO_H     240
#define MAX_JPEG    (200*1024)

/* ====== 统计信息结构 ====== */
typedef struct {
    uint32_t init_time;
    uint32_t frames_displayed;
    uint32_t bytes_received;
    uint32_t recv_errors;
    uint32_t decode_errors;
    uint32_t fps;

    // 性能统计
    uint32_t total_decode_time;    // 总解码时间（ms）
    uint32_t avg_decode_time;     // 平均解码时间（ms/帧）
    uint32_t max_decode_time;     // 最大解码时间（ms）

    // 网络统计
    uint32_t last_bandwidth;      // 最近带宽（bytes/s）
    uint32_t avg_bandwidth;       // 平均带宽（bytes/s）
    uint32_t total_packets;       // 总包数

    // 内存统计（需要OS支持）
    uint32_t memory_used;         // 已使用内存（bytes）
    uint32_t memory_peak;         // 峰值内存（bytes）

    // CPU统计（需要OS支持）
    uint32_t cpu_usage;           // CPU占用率（千分比，0-1000）
    uint32_t cpu_time_used;       // CPU时间（ms）

    // 时间统计
    uint32_t last_update_time;    // 上次更新时间
    uint32_t uptime;              // 运行时间（ms）
} video_stats_t;

/* ====== 核心API ====== */
void video_player_init(void);
void video_player_task(void);
const video_stats_t* video_player_get_stats(void);
void video_player_reset_stats(void);
lv_img_dsc_t* video_player_get_current_image(void);  // 获取当前显示的图像描述符
lv_obj_t* video_player_get_image_obj(void);          // 获取视频图像对象

/* ====== 网络 API ====== */
int net_recv_init(const char *bind_ip, int port, int use_udp);
int net_recv_accept(void);
int net_recv_frame(uint8_t *buf, int maxlen);
int net_recv_frame_simple(uint8_t *buf, int maxlen);
void net_recv_close(void);

/* ====== 解码器 API ====== */
int jpeg_decode_to_rgb565(uint8_t *jpeg, int len, uint16_t *out);
const char* jpeg_decoder_version(void);

/* ====== 依赖的外部接口 ====== */
//extern int net_recv_frame(uint8_t *buf, int maxlen);
extern int jpeg_decode_to_rgb565(uint8_t *jpeg, int len, uint16_t *out);

#endif /* VIDEO_PLAYER_H */
