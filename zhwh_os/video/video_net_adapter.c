/**
 * @file video_net_adapter.c
 * @brief 视频播放器网络适配器 - 连接到你的OS网络栈
 *
 * 这个文件适配你的OS现有的网络API到视频播放器的简化接口
 */

#include "video_player.h"
#include "net.h"
#include <string.h>

// ====== UDP接收回调 ======
static uint8_t *video_recv_buffer = NULL;
static int video_recv_maxlen = 0;
static int video_data_ready = 0;
static int video_data_len = 0;

// ====== 全局状态 ======
static int video_socket = -1;
static int video_port = 0;

/**
 * @brief UDP接收回调函数
 *
 * 这个函数会被网络栈调用，当接收到目标端口的数据时
 */
static int video_udp_callback(uint16_t src_port, uint32_t src_ip,
                              uint8_t *data, uint32_t len)
{
    if (!video_recv_buffer || len > video_recv_maxlen) {
        return -1;
    }

    // 复制数据到接收缓冲区
    memcpy(video_recv_buffer, data, len);
    video_data_len = len;
    video_data_ready = 1;

    return 0;
}

/**
 * @brief 注册UDP端口接收（需要添加到你的OS UDP处理）
 *
 * TODO: 需要在 udp_input() 中添加端口回调注册机制
 */
static int video_udp_register(int port)
{
    // 这里需要你的OS支持UDP端口回调注册
    // 示例：extern int udp_register_callback(int port, udp_callback_fn cb);
    // return udp_register_callback(port, video_udp_callback);

    // 暂时返回成功（需要你的OS实现这个功能）
    video_port = port;
    return 0;
}

/**
 * @brief 创建socket（简化版 - 只支持UDP）
 */
int os_socket_create(int type)
{
    // 只支持UDP (type=2)
    if (type != 2) {
        return -1;
    }

    // 返回一个伪socket描述符
    video_socket = 1;  // 伪FD
    return video_socket;
}

/**
 * @brief 绑定socket到端口
 */
int os_socket_bind(int sock, uint32_t ip, uint16_t port)
{
    // 转换网络字节序到主机字节序
    video_port = ((port & 0xFF) << 8) | ((port & 0xFF00) >> 8);

    // 注册UDP端口回调
    return video_udp_register(video_port);
}

/**
 * @brief 接收UDP数据
 */
int os_socket_recv(int sock, void *buf, int len)
{
    // 设置接收缓冲区
    video_recv_buffer = buf;
    video_recv_maxlen = len;
    video_data_ready = 0;
    video_data_len = 0;

    // 等待数据（非阻塞）
    // 在实际使用中，这个函数会在主循环中被反复调用
    // 当网络栈接收到数据时，会调用 video_udp_callback 设置 video_data_ready

    if (video_data_ready) {
        int ret = video_data_len;
        video_data_ready = 0;
        return ret;
    }

    return 0;  // 没有数据
}

/**
 * @brief 关闭socket
 */
int os_socket_close(int sock)
{
    video_socket = -1;
    video_port = 0;
    video_recv_buffer = NULL;
    return 0;
}
