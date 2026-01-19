/**
 * @file net.h
 * @brief 网络协议栈头文件
 *
 * 支持的功能：
 * - 以太网帧处理
 * - ARP 协议
 * - IP 协议
 * - ICMP 协议（ping）
 * - UDP 协议
 * - TCP 协议
 */

#ifndef NET_H
#define NET_H

#include "types.h"

// ==================== 以太网 ====================

#define ETH_ALEN 6              // 以太网地址长度
#define ETH_HDR_LEN 14          // 以太网头部长度
#define ETH_MTU 1500            // 最大传输单元
#define ETH_MAX_FRAME 1518      // 最大以太网帧

// 以太网帧类型
#define ETH_P_IP   0x0800       // IPv4
#define ETH_P_ARP  0x0806       // ARP
#define ETH_P_RARP 0x8035       // RARP

// 以太网头部
typedef struct {
    uint8_t eth_dst[ETH_ALEN];    // 目标MAC地址
    uint8_t eth_src[ETH_ALEN];    // 源MAC地址
    uint16_t eth_type;             // 帧类型
} __attribute__((packed)) eth_hdr_t;

// ==================== ARP ====================

#define ARP_HDR_LEN 28           // ARP头部长度

// 硬件类型
#define ARPHRD_ETHER 1           // 以太网

// 协议类型
#define ARPOP_REQUEST 1          // ARP请求
#define ARPOP_REPLY 2            // ARP应答

// ARP头部
typedef struct {
    uint16_t arp_hrd;    // 硬件类型
    uint16_t arp_pro;    // 协议类型
    uint8_t arp_hln;     // 硬件地址长度
    uint8_t arp_pln;     // 协议地址长度
    uint16_t arp_op;     // 操作码
    uint8_t arp_sha[ETH_ALEN];  // 发送方硬件地址
    uint32_t arp_spa;           // 发送方协议地址
    uint8_t arp_tha[ETH_ALEN];  // 目标硬件地址
    uint32_t arp_tpa;           // 目标协议地址
} __attribute__((packed)) arp_hdr_t;

// ARP缓存表项
#define ARP_TABLE_SIZE 32

typedef struct {
    uint32_t ip_addr;           // IP地址
    uint8_t mac_addr[ETH_ALEN]; // MAC地址
    uint8_t valid;              // 是否有效
    uint32_t last_used;         // 最后使用时间
} arp_entry_t;

// ==================== IP ====================

#define IP_HDR_LEN 20            // IP头部长度
#define IP_TTL 64                // 默认TTL

// IP协议类型
#define IPPROTO_ICMP 1           // ICMP
#define IPPROTO_TCP  6           // TCP
#define IPPROTO_UDP  17          // UDP

// IP头部
typedef struct {
    uint8_t  ip_verhlen;   // 版本(4位) + 头长度(4位)
    uint8_t  ip_tos;       // 服务类型
    uint16_t ip_len;       // 总长度
    uint16_t ip_id;        // 标识
    uint16_t ip_off;       // 片偏移
    uint8_t  ip_ttl;       // 生存时间
    uint8_t  ip_proto;     // 协议
    uint16_t ip_sum;       // 校验和
    uint32_t ip_src;       // 源IP地址
    uint32_t ip_dst;       // 目标IP地址
} __attribute__((packed)) ip_hdr_t;

// ==================== ICMP ====================

#define ICMP_HDR_LEN 8           // ICMP头部长度

// ICMP类型
#define ICMP_ECHO_REPLY 0        // 回显应答
#define ICMP_ECHO_REQUEST 8      // 回显请求

// ICMP头部
typedef struct {
    uint8_t  icmp_type;    // 类型
    uint8_t  icmp_code;    // 代码
    uint16_t icmp_sum;     // 校验和
    uint32_t icmp_id;      // 标识符
    uint32_t icmp_seq;     // 序列号
} __attribute__((packed)) icmp_hdr_t;

// ==================== UDP ====================

#define UDP_HDR_LEN 8            // UDP头部长度

// UDP头部
typedef struct {
    uint16_t udp_sport;   // 源端口
    uint16_t udp_dport;   // 目标端口
    uint16_t udp_len;     // 长度
    uint16_t udp_sum;     // 校验和
} __attribute__((packed)) udp_hdr_t;

// ==================== TCP ====================

#define TCP_HDR_LEN 20            // TCP头部长度

// TCP头部
typedef struct {
    uint16_t tcp_sport;   // 源端口
    uint16_t tcp_dport;   // 目标端口
    uint32_t tcp_seq;     // 序列号
    uint32_t tcp_ack;     // 确认号
    uint8_t  tcp_off;     // 数据偏移(4位) + 保留(4位)
    uint8_t  tcp_flags;   // 标志位
    uint16_t tcp_win;     // 窗口大小
    uint16_t tcp_sum;     // 校验和
    uint16_t tcp_urg;     // 紧急指针
} __attribute__((packed)) tcp_hdr_t;

// TCP标志位
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
#define TCP_URG 0x20

// ==================== 网卡接口 ====================

// 网卡设备结构
typedef struct net_device {
    char name[16];              // 设备名称
    uint8_t mac_addr[ETH_ALEN]; // MAC地址
    uint32_t ip_addr;           // IP地址
    uint32_t netmask;           // 子网掩码
    uint32_t gateway;           // 网关
    uint16_t mtu;               // 最大传输单元
    void *priv;                 // 私有数据

    // 驱动操作
    int (*send)(struct net_device *dev, uint8_t *data, uint32_t len);
    int (*recv)(struct net_device *dev, uint8_t *data, uint32_t len);
    int (*ioctl)(struct net_device *dev, int cmd, void *arg);
} net_device_t;

// ==================== 网络统计 ====================

typedef struct {
    uint32_t rx_packets;        // 接收包数
    uint32_t tx_packets;        // 发送包数
    uint32_t rx_bytes;          // 接收字节数
    uint32_t tx_bytes;          // 发送字节数
    uint32_t rx_errors;         // 接收错误
    uint32_t tx_errors;         // 发送错误
    uint32_t rx_dropped;        // 丢弃包数
    uint32_t tx_dropped;        // 丢弃包数
} net_stats_t;

// ==================== 函数声明 ====================

// 网络初始化
void net_init(void);

// 网络设备管理
int net_device_register(net_device_t *dev);
net_device_t *net_device_get(const char *name);
net_device_t *net_device_get_default(void);

// 数据包接收/发送
int net_rx_packet(net_device_t *dev, uint8_t *data, uint32_t len);
int net_tx_packet(net_device_t *dev, uint8_t *data, uint32_t len);

// 协议处理
int eth_input(net_device_t *dev, uint8_t *data, uint32_t len);
int ip_input(net_device_t *dev, uint8_t *data, uint32_t len);
int icmp_input(net_device_t *dev, uint8_t *data, uint32_t len);
int udp_input(net_device_t *dev, uint8_t *data, uint32_t len);
int tcp_input(net_device_t *dev, uint8_t *data, uint32_t len);

// ARP
int arp_request(net_device_t *dev, uint32_t ip_addr);
int arp_reply(net_device_t *dev, uint32_t ip_addr, uint8_t *mac_addr);
int arp_lookup(uint32_t ip_addr, uint8_t *mac_addr);

// 工具函数
uint16_t internet_checksum(uint16_t *data, uint32_t len);
void print_mac(uint8_t *mac);
void print_ip(uint32_t ip);

// 配置
int net_set_ipaddr(uint32_t ip);
int net_set_netmask(uint32_t netmask);
int net_set_gateway(uint32_t gateway);

// ==================== 字节序转换 ====================

// 主机到网络字节序（大端）
static inline uint16_t htons(uint16_t hostshort) {
    return ((hostshort & 0xFF) << 8) | ((hostshort >> 8) & 0xFF);
}

static inline uint32_t htonl(uint32_t hostlong) {
    return ((hostlong & 0xFF) << 24) |
           ((hostlong & 0xFF00) << 8) |
           ((hostlong >> 8) & 0xFF00) |
           ((hostlong >> 24) & 0xFF);
}

// 网络到主机字节序
static inline uint16_t ntohs(uint16_t netshort) {
    return htons(netshort);  // 相同操作
}

static inline uint32_t ntohl(uint32_t netlong) {
    return htonl(netlong);  // 相同操作
}

#endif // NET_H
