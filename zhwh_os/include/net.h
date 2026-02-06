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

// ==================== 用户态-内核态共享数据结构 ====================

/**
 * @brief 用户缓冲区描述符（用于安全传递大块数据）
 *
 * 用户态只传这个结构体指针，内核态自己分段 copy 数据
 */
struct user_buf {
    const void *ptr;   // 用户态地址
    uint32_t len;      // 数据长度
};

// ==================== 以太网 ====================

#define ETH_ALEN 6              // 以太网地址长度
#define ETH_HDR_LEN 14          // 以太网头部长度
#define ETH_MTU 1500            // 最大传输单元
#define ETH_MAX_FRAME 1518      // 最大以太网帧

// 以太网帧类型
#define ETH_P_IP   0x0800       // IPv4
#define ETH_P_ARP  0x0806       // ARP
#define ETH_P_RARP 0x8035       // RARP
#define ETH_P_IPV6 0x86DD       // IPv6

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

// 🔥 静态断言：确保 ARP 头部大小为 28 字节
_Static_assert(sizeof(arp_hdr_t) == 28, "arp_hdr_t must be 28 bytes");

// ARP缓存表项（简化版）
#define ARP_TABLE_SIZE 8

typedef struct {
    uint32_t ip;      // 主机字节序 (host byte order)
    uint8_t  mac[6];
    int      valid;
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
    void *pci_dev;              // PCI设备指针（用于获取厂商/设备信息）

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

// ==================== 全局配置 ====================

// 🔥 本机 MAC 和 IP（全局变量，用于接收包过滤）
extern uint8_t local_mac[ETH_ALEN];  // 本机 MAC 地址
extern uint32_t local_ip;             // 本机 IP 地址（网络字节序）

// ==================== 函数声明 ====================

// 网络初始化
void net_init(void);

// 网络设备管理
int net_device_register(net_device_t *dev);
net_device_t *net_device_get(const char *name);
net_device_t *net_device_get_default(void);
int net_get_device_count(void);  // 🔥 新增：获取设备数量
net_device_t **net_get_all_devices(void);  // 🔥 新增：获取所有设备数组

// 数据包接收/发送
int net_rx_packet(net_device_t *dev, uint8_t *data, uint32_t len);
int net_tx_packet(net_device_t *dev, uint8_t *data, uint32_t len);

// 协议处理
int eth_input(net_device_t *dev, uint8_t *data, uint32_t len);
int arp_input(net_device_t *dev, uint8_t *data, uint32_t len);
int ip_input(net_device_t *dev, uint8_t *data, uint32_t len);
int ip_output(net_device_t *dev, uint32_t dst_ip, uint8_t protocol,
              uint8_t *data, uint32_t len);
int icmp_input(net_device_t *dev, uint8_t *data, uint32_t len);
int icmp_send_echo(net_device_t *dev, uint32_t dst_ip, uint16_t id, uint16_t seq);
int udp_input(net_device_t *dev, uint8_t *data, uint32_t len);
int udp_output(net_device_t *dev, uint32_t dst_ip, uint16_t src_port,
               uint16_t dst_port, uint8_t *data, uint32_t len);
int tcp_input(net_device_t *dev, uint8_t *data, uint32_t len);
int tcp_output(net_device_t *dev, uint32_t dst_ip, uint16_t src_port,
               uint16_t dst_port, uint32_t seq, uint32_t ack,
               uint8_t flags, uint8_t *data, uint32_t len);

// ARP
int arp_request(net_device_t *dev, uint32_t ip_addr);
void arp_handle(net_device_t *dev, uint8_t *data, uint32_t len);
uint8_t *arp_cache_lookup(uint32_t ip);
void arp_send_request(net_device_t *dev, uint32_t target_ip);

// 🔥 通用网络设备轮询接收和诊断
void net_poll_rx(net_device_t *dev);
void net_dump_rx_regs(net_device_t *dev);

// 工具函数
uint16_t internet_checksum(uint16_t *data, uint32_t len);
void print_mac(uint8_t *mac);
void print_ip(uint32_t ip);

// 以太网广播地址
extern uint8_t eth_broadcast[ETH_ALEN];

// 配置
int net_set_ipaddr(uint32_t ip);
int net_set_netmask(uint32_t netmask);
int net_set_gateway(uint32_t gateway);

// ==================== 802.11 WiFi 帧 ====================

// 802.11 帧类型
#define IEEE80211_FTYPE_MGMT  0x00
#define IEEE80211_FTYPE_CTL   0x01
#define IEEE80211_FTYPE_DATA  0x02

// 802.11 管理帧子类型
#define IEEE80211_STYPE_ASSOC_REQ    0x00
#define IEEE80211_STYPE_ASSOC_RESP   0x01
#define IEEE80211_STYPE_REASSOC_REQ  0x02
#define IEEE80211_STYPE_REASSOC_RESP 0x03
#define IEEE80211_STYPE_PROBE_REQ    0x04
#define IEEE80211_STYPE_PROBE_RESP   0x05
#define IEEE80211_STYPE_BEACON       0x08
#define IEEE80211_STYPE_AUTH         0x0B
#define IEEE80211_STYPE_DEAUTH       0x0C
#define IEEE80211_STYPE_DISASSOC     0x0A

// 802.11 数据帧子类型
#define IEEE80211_STYPE_DATA         0x00
#define IEEE80211_STYPE_DATA_CFACK   0x01
#define IEEE80211_STYPE_DATA_CFPOLL  0x02
#define IEEE80211_STYPE_QOS_DATA     0x08

// 802.11 帧控制
#define IEEE80211_FCTL_VERS         0x0003
#define IEEE80211_FCTL_FTYPE        0x000C
#define IEEE80211_FCTL_STYPE        0x00F0
#define IEEE80211_FCTL_TODS         0x0100
#define IEEE80211_FCTL_FROMDS       0x0200
#define IEEE80211_FCTL_MOREFRAGS    0x0400
#define IEEE80211_FCTL_RETRY        0x0800
#define IEEE80211_FCTL_PM           0x1000
#define IEEE80211_FCTL_MOREDATA     0x2000
#define IEEE80211_FCTL_PROTECTED    0x4000
#define IEEE80211_FCTL_ORDER        0x8000

// 802.11 capability
#define IEEE80211_CAPINFO_ESS       0x0001
#define IEEE80211_CAPINFO_IBSS      0x0002
#define IEEE80211_CAPINFO_CF_POLL   0x0004
#define IEEE80211_CAPINFO_CF_POLL_REQ 0x0008
#define IEEE80211_CAPINFO_PRIVACY   0x0010
#define IEEE80211_CAPINFO_SHORT_PREAMBLE 0x0020
#define IEEE80211_CAPINFO_PBCC      0x0040
#define IEEE80211_CAPINFO_CH_AGILITY 0x0080
#define IEEE80211_CAPINFO_SHORT_SLOT 0x0400

// 802.11 帧头部（最小 24 字节）
typedef struct {
    uint16_t frame_control;
    uint16_t duration_id;
    uint8_t addr1[ETH_ALEN];  // 目标地址
    uint8_t addr2[ETH_ALEN];  // 源地址
    uint8_t addr3[ETH_ALEN];  // BSSID
    uint16_t seq_ctrl;
} __attribute__((packed)) ieee80211_hdr_t;

// 802.11 管理帧头部
typedef struct {
    uint16_t frame_control;
    uint16_t duration_id;
    uint8_t addr1[ETH_ALEN];
    uint8_t addr2[ETH_ALEN];
    uint8_t addr3[ETH_ALEN];
    uint16_t seq_ctrl;
} __attribute__((packed)) ieee80211_mgmt_hdr_t;

// Beacon/Probe Response 固定部分
typedef struct {
    uint64_t timestamp;
    uint16_t beacon_interval;
    uint16_t capability;
} __attribute__((packed)) ieee80211_beacon_fixed_t;

// 802.11 认证帧
typedef struct {
    uint16_t auth_alg;
    uint16_t auth_transaction;
    uint16_t status_code;
} __attribute__((packed)) ieee80211_auth_t;

// 802.11 元素 ID
#define IEEE80211_ELEM_SSID         0
#define IEEE80211_ELEM_SUPP_RATES   1
#define IEEE80211_ELEM_DS_PARAMS    3
#define IEEE80211_ELEM_TIM          5
#define IEEE80211_ELEM_COUNTRY      7
#define IEEE80211_ELEM_RSN          48
#define IEEE80211_ELEM_EXT_SUPP_RATES 50

// 802.11 元素头
typedef struct {
    uint8_t id;
    uint8_t len;
} __attribute__((packed)) ieee80211_elem_t;

// 802.11 Beacon 信息
typedef struct {
    char ssid[32];
    uint8_t bssid[ETH_ALEN];
    uint8_t channel;
    int signal;
    uint16_t capability;
    uint8_t privacy;
} wifi_beacon_t;

// ==================== WiFi ====================

// WiFi 设备初始化
int wifi_init(void);

// WiFi 操作
int wifi_scan(void);
int wifi_connect(const char *ssid, const char *password);
int wifi_disconnect(void);
void wifi_status(void);

// WiFi 数据包处理
int wifi_tx_packet(net_device_t *dev, uint8_t *data, uint32_t len);
int wifi_rx_packet(net_device_t *dev, uint8_t *data, uint32_t len);
int wifi_input_80211(net_device_t *dev, uint8_t *data, uint32_t len);

// 802.11 帧处理
int ieee80211_input_mgmt(net_device_t *dev, uint8_t *data, uint32_t len);
int ieee80211_send_probe_req(net_device_t *dev);
int ieee80211_send_auth(net_device_t *dev, uint8_t *bssid);
int ieee80211_send_assoc_req(net_device_t *dev, uint8_t *bssid, const char *ssid);

// 辅助函数
uint8_t *ieee80211_get_elem(uint8_t *data, uint32_t len, uint8_t elem_id);
void print_ieee80211_hdr(uint8_t *data);

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
