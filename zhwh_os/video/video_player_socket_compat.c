/**
 * @file video_player_socket_compat.c
 * @brief BSD socket 兼容层
 *
 * 为视频播放器提供 BSD socket API 的兼容实现
 * 实际网络操作由你的OS网络栈提供
 */

#include "video_player.h"
#include <string.h>

// 外部网络函数（你的OS实现）
extern int os_socket_create(int type);  // 1=TCP, 2=UDP
extern int os_socket_bind(int sock, uint32_t ip, uint16_t port);
extern int os_socket_recv(int sock, void *buf, int len);
extern int os_socket_close(int sock);

/**
 * @brief 创建 socket
 */
int socket(int domain, int type, int protocol)
{
    // 只支持 AF_INET
    if (domain != AF_INET) return -1;

    int os_type;
    if (type == SOCK_STREAM) os_type = 1;      // TCP
    else if (type == SOCK_DGRAM) os_type = 2;  // UDP
    else return -1;

    return os_socket_create(os_type);
}

/**
 * @brief 绑定 socket 到地址
 */
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    if (!addr || addr->sa_family != AF_INET) return -1;

    const struct sockaddr_in *addr_in = (const struct sockaddr_in *)addr;
    return os_socket_bind(sockfd, addr_in->sin_addr, addr_in->sin_port);
}

/**
 * @brief 接收数据
 */
ssize_t recv(int sockfd, void *buf, size_t len, int flags)
{
    // 忽略 flags 参数
    return os_socket_recv(sockfd, buf, len);
}

/**
 * @brief 主机字节序到网络字节序（32位）
 */
uint32_t htonl(uint32_t hostlong)
{
    // 小端系统需要转换
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return ((hostlong & 0xFF) << 24) |
           ((hostlong & 0xFF00) << 8) |
           ((hostlong & 0xFF0000) >> 8) |
           ((hostlong & 0xFF000000) >> 24);
    #else
    return hostlong;
    #endif
}

/**
 * @brief 主机字节序到网络字节序（16位）
 */
uint16_t htons(uint16_t hostshort)
{
    // 小端系统需要转换
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return ((hostshort & 0xFF) << 8) | ((hostshort & 0xFF00) >> 8);
    #else
    return hostshort;
    #endif
}

/**
 * @brief 字符串IP地址转数值
 */
uint32_t inet_addr(const char *cp)
{
    uint32_t addr = 0;
    int shift = 0;

    if (!cp) return 0;

    // 简单实现，只支持 "a.b.c.d" 格式
    for (int i = 0; i < 4; i++) {
        uint8_t byte = 0;
        while (*cp && *cp != '.') {
            if (*cp >= '0' && *cp <= '9') {
                byte = byte * 10 + (*cp - '0');
            }
            cp++;
        }
        addr |= (byte << shift);
        shift += 8;
        if (*cp) cp++;  // 跳过 '.'
    }

    return addr;
}

// 以下是TCP相关函数（如果需要）
int listen(int sockfd, int backlog) {
    // UDP不需要listen，TCP需要实现
    return 0;
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    // TCP需要实现
    return -1;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    // TCP需要实现
    return -1;
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    // 如果需要发送功能
    return -1;
}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    // 可选实现
    return 0;
}
