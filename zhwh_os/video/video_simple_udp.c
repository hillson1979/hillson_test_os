/**
 * @file video_simple_udp.c
 * @brief 视频播放器简化UDP接收器
 *
 * 这个文件提供一个简单的UDP接收机制，用于视频播放器
 * 使用系统调用从内核获取 UDP 数据
 */

#include "video_player.h"
#include <string.h>

// 外部系统调用函数（用户态系统调用包装）
extern int net_bind_udp(int port);
extern int net_recv_udp(char *buf, int len);
extern int net_poll_rx(void);
extern void *kmalloc(unsigned int size);
extern void kfree(void *ptr);
extern int printf(const char *fmt, ...);

// ====== UDP接收缓冲区 ======
static uint8_t udp_buffer[MAX_JPEG];
static int udp_data_len = 0;
static int udp_data_ready = 0;
static int udp_port_bound = 0;

// 统计接收到的包数
static int udp_packet_count = 0;

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
        printf("[VIDEO] ERROR: Unsupported socket type %d\n", type);
        return -1;
    }

    printf("[VIDEO] UDP socket created\n");
    return 1;  // 伪socket描述符
}

/**
 * @brief 绑定socket到端口
 */
int os_socket_bind(int sock, uint32_t ip, uint16_t port)
{
    // 转换网络字节序到主机字节序
    udp_port_bound = ((port & 0xFF) << 8) | ((port & 0xFF00) >> 8);

    printf("[VIDEO] Bound to port %d (network order port=0x%04x)\n",
           udp_port_bound, port);

    // 🔥 关键修复：调用系统调用在内核中绑定端口
    net_bind_udp(udp_port_bound);

    return 0;
}

/**
 * @brief 接收UDP数据
 */
int os_socket_recv(int sock, void *buf, int len)
{
    static int no_data_count = 0;
    static int error_count = 0;

    printf("[VIDEO] [os_socket_recv] 1 \n");

    // 检查缓冲区是否初始化
    if (udp_buffer == NULL) {
        printf("[VIDEO] ERROR: udp_buffer not initialized\n");
        return 0;
    }

    printf("[VIDEO] [os_socket_recv] 2 \n");
    printf("[VIDEO] [os_socket_recv] port is %d\n", udp_port_bound);

    // 🔥 轮询接收：主动检查是否有数据包
    net_poll_rx();

    // 🔥 使用系统调用从内核获取 UDP 数据（不需要传端口了）
    int ret = net_recv_udp((char *)buf, len);

    printf("[VIDEO] [os_socket_recv] 3 \n");

    if (ret <= 0) {
        no_data_count++;
        error_count++;
        if (error_count % 5 == 0) {
            printf("[VIDEO] No data received (%d errors)\n", error_count);
        }
        return 0;  // 没有数据
    }

    no_data_count = 0;
    error_count = 0;
    udp_packet_count++;

    // 打印包信息
    printf("[VIDEO] Recv UDP packet %d: %d bytes\n",
           udp_packet_count, ret);

    // 打印前几个字节用于调试 JPEG 格式
    if (ret >= 4) {
        uint8_t *data = (uint8_t *)buf;
        printf("[VIDEO] Data: 0x%02x%02x%02x%02x...\n",
               data[0], data[1], data[2], data[3]);
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
