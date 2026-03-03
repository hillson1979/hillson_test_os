#include "video_player.h"
#include "net.h"
#include <string.h>

/* ====== 简单的inet_addr实现 ====== */
static uint32_t inet_addr_simple(const char *cp)
{
    uint32_t addr = 0;
    int parts[4];
    int part = 0;
    const char *p = cp;

    // 解析 "a.b.c.d" 格式
    while (part < 4) {
        int val = 0;
        while (*p >= '0' && *p <= '9') {
            val = val * 10 + (*p - '0');
            p++;
        }
        parts[part++] = val;
        if (*p == '.') p++;
        else break;
    }

    // 组合成32位地址（大端序）
    addr = (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8) | parts[3];
    return addr;
}

/* ====== 网络配置 ====== */
static int video_sock = -1;
static int is_udp = 1;

/* ====== 简单帧头协议 ====== */
typedef struct {
    uint32_t magic;      // 0xAABBCCDD
    uint32_t frame_len;  // JPEG data length
    uint32_t frame_num;  // frame counter
    uint32_t reserved;   // reserved
} frame_header_t;

#define FRAME_MAGIC 0xAABBCCDD

/* ====== 初始化网络接收器 ====== */
int net_recv_init(const char *bind_ip, int port, int use_udp)
{
    struct sockaddr_in addr;
    int sock;
    int reuse = 1;

    is_udp = use_udp;

    /* 创建socket */
    sock = socket(AF_INET, is_udp ? SOCK_DGRAM : SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }

    /* 设置SO_REUSEADDR */
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    /* 绑定端口 */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr = inet_addr_simple(bind_ip);
    addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -2;
    }

    /* TCP模式：listen */
    if (!is_udp) {
        if (listen(sock, 1) < 0) {
            close(sock);
            return -3;
        }
    }

    video_sock = sock;
    return 0;
}

/* ====== 接受TCP连接（阻塞） ====== */
int net_recv_accept(void)
{
    if (is_udp) return video_sock;

    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    int client_sock = accept(video_sock, (struct sockaddr *)&client_addr, &len);
    if (client_sock < 0) {
        return -1;
    }

    close(video_sock);
    video_sock = client_sock;
    return 0;
}

/* ====== 接收一帧（带简单帧头） ====== */
int net_recv_frame(uint8_t *buf, int maxlen)
{
    frame_header_t header;
    int total_len = sizeof(frame_header_t);
    int received = 0;
    int n;

    /* 接收帧头 */
    if (is_udp) {
        /* UDP：直接recv */
        n = recv(video_sock, buf, maxlen, 0);
        return n;
    }

    /* TCP：先读帧头 */
    while (received < sizeof(frame_header_t)) {
        n = recv(video_sock, (uint8_t*)&header + received,
                 sizeof(frame_header_t) - received, 0);
        if (n <= 0) return -1;
        received += n;
    }

    /* 验证魔数 */
    if (header.magic != FRAME_MAGIC) {
        return -2;
    }

    /* 检查长度 */
    if (header.frame_len > maxlen) {
        return -3;
    }

    /* 接收JPEG数据 */
    received = 0;
    while (received < header.frame_len) {
        n = recv(video_sock, buf + received, header.frame_len - received, 0);
        if (n <= 0) return -4;
        received += n;
    }

    return received;
}

/* ====== 简单UDP版本（无帧头） ====== */
int net_recv_frame_simple(uint8_t *buf, int maxlen)
{
    return recv(video_sock, buf, maxlen, 0);
}

/* ====== 关闭网络 ====== */
void net_recv_close(void)
{
    if (video_sock >= 0) {
        close(video_sock);
        video_sock = -1;
    }
}
