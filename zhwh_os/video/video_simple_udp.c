/**
 * @file video_simple_udp.c
 * @brief 视频播放器简化UDP接收器
 *
 * 这个文件提供一个简单的UDP接收机制，用于视频播放器
 * 使用系统调用从内核获取 UDP 数据
 */

#include "video_player.h"
#include "net.h"
#include <string.h>

// 外部系统调用函数
extern int net_recv_udp(char *buf, int len, int *port);
extern void *kmalloc(unsigned int size);
extern void kfree(void *ptr);

// ====== UDP接收缓冲区 ======
// 🔥 改为动态分配，避免 200KB 的静态 BSS
// static uint8_t udp_buffer[MAX_JPEG];
static uint8_t *udp_buffer = NULL;
static int udp_data_len = 0;
static int udp_data_ready = 0;
static int udp_port_bound = 0;

// ====== 外部：UDP端口回调（需要在 udp_input 中调用） ======
// 注意：由于内核和用户态分离，这个钩子不再使用
// 改为使用系统调用 net_recv_udp
int video_udp_hook(uint16_t dport, uint8_t *data, uint32_t len)
{
    // 用户态的钩子不会被内核调用
    // 这个函数保留用于兼容性，但不会被使用
    return -1;
}

/**
 * @brief 创建socket
 */
int os_socket_create(int type)
{
    // 只支持UDP (type=2)
    if (type != 2) {
        return -1;
    }

    return 1;  // 伪socket描述符
}

/**
 * @brief 绑定socket到端口
 */
int os_socket_bind(int sock, uint32_t ip, uint16_t port)
{
    // 转换网络字节序到主机字节序
    udp_port_bound = ((port & 0xFF) << 8) | ((port & 0xFF00) >> 8);

    printf("[VIDEO UDP] Bound to port %d\n", udp_port_bound);

    return 0;
}

/**
 * @brief 接收UDP数据
 */
int os_socket_recv(int sock, void *buf, int len)
{
    // 使用系统调用从内核获取 UDP 数据
    int port = 0;
    int ret = net_recv_udp((char *)buf, len, &port);

    if (ret <= 0) {
        return 0;  // 没有数据
    }

    return ret;
}

/**
 * @brief 关闭socket
 */
int os_socket_close(int sock)
{
    udp_port_bound = 0;
    udp_data_ready = 0;
    udp_data_len = 0;
    return 0;
}
