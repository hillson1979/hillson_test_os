/**
 * @file core.c
 * @brief 网络协议栈核心实现
 */

#include "net.h"
#include "../include/printf.h"
#include "../include/string.h"

// 网络设备列表
static net_device_t *net_devices[16];
static int num_devices = 0;

// ARP缓存表
static arp_entry_t arp_table[ARP_TABLE_SIZE];

// 网络统计
net_stats_t net_stats;

// 本机IP配置
static uint32_t local_ip = 0xC0A80101;  // 192.168.1.1
static uint32_t netmask = 0xFFFFFF00;   // 255.255.255.0
static uint32_t gateway = 0xC0A80101;   // 192.168.1.1

/**
 * @brief 网络初始化
 */
void net_init(void) {
    printf("[net] Initializing network stack...\n");

    // 清零设备列表
    memset(net_devices, 0, sizeof(net_devices));
    num_devices = 0;

    // 清零ARP表
    memset(arp_table, 0, sizeof(arp_table));

    // 清零统计信息
    memset(&net_stats, 0, sizeof(net_stats));

    printf("[net] Network stack initialized\n");
    printf("[net] Local IP: %d.%d.%d.%d\n",
           (local_ip >> 24) & 0xFF,
           (local_ip >> 16) & 0xFF,
           (local_ip >> 8) & 0xFF,
           local_ip & 0xFF);
}

/**
 * @brief 注册网络设备
 */
int net_device_register(net_device_t *dev) {
    if (!dev || num_devices >= 16) {
        printf("[net] Failed to register device\n");
        return -1;
    }

    dev->ip_addr = local_ip;
    dev->netmask = netmask;
    dev->gateway = gateway;
    dev->mtu = ETH_MTU;

    net_devices[num_devices++] = dev;

    printf("[net] Registered device: %s\n", dev->name);
    printf("[net]   MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           dev->mac_addr[0], dev->mac_addr[1], dev->mac_addr[2],
           dev->mac_addr[3], dev->mac_addr[4], dev->mac_addr[5]);

    return 0;
}

/**
 * @brief 获取网络设备
 */
net_device_t *net_device_get(const char *name) {
    for (int i = 0; i < num_devices; i++) {
        if (strcmp(net_devices[i]->name, name) == 0) {
            return net_devices[i];
        }
    }
    return NULL;
}

/**
 * @brief 获取默认网络设备
 */
net_device_t *net_device_get_default(void) {
    if (num_devices > 0) {
        return net_devices[0];
    }
    return NULL;
}

/**
 * @brief 接收数据包
 */
int net_rx_packet(net_device_t *dev, uint8_t *data, uint32_t len) {
    if (!dev || !data || len < ETH_HDR_LEN) {
        net_stats.rx_errors++;
        return -1;
    }

    net_stats.rx_packets++;
    net_stats.rx_bytes += len;

    // 解析以太网帧
    return eth_input(dev, data, len);
}

/**
 * @brief 发送数据包
 */
int net_tx_packet(net_device_t *dev, uint8_t *data, uint32_t len) {
    if (!dev || !data || len > ETH_MAX_FRAME || len < ETH_HDR_LEN) {
        net_stats.tx_errors++;
        return -1;
    }

    if (!dev->send) {
        printf("[net] Device has no send function\n");
        net_stats.tx_errors++;
        return -1;
    }

    net_stats.tx_packets++;
    net_stats.tx_bytes += len;

    return dev->send(dev, data, len);
}

/**
 * @brief 以太网输入处理
 */
int eth_input(net_device_t *dev, uint8_t *data, uint32_t len) {
    eth_hdr_t *eth = (eth_hdr_t *)data;

    printf("[net] Eth frame: type=0x%x, len=%d\n", ntohs(eth->eth_type), len);

    // 根据以太网类型分发
    switch (ntohs(eth->eth_type)) {
        case ETH_P_IP:
            return ip_input(dev, data + ETH_HDR_LEN, len - ETH_HDR_LEN);
        case ETH_P_ARP:
            // TODO: 实现ARP处理
            printf("[net] ARP packet received (not yet implemented)\n");
            break;
        default:
            printf("[net] Unknown eth type: 0x%x\n", ntohs(eth->eth_type));
            break;
    }

    return 0;
}

/**
 * @brief IP输入处理
 */
int ip_input(net_device_t *dev, uint8_t *data, uint32_t len) {
    ip_hdr_t *ip = (ip_hdr_t *)data;

    printf("[net] IP packet: proto=%d, src=%d.%d.%d.%d, dst=%d.%d.%d.%d\n",
           ip->ip_proto,
           (ip->ip_src >> 24) & 0xFF, (ip->ip_src >> 16) & 0xFF,
           (ip->ip_src >> 8) & 0xFF, ip->ip_src & 0xFF,
           (ip->ip_dst >> 24) & 0xFF, (ip->ip_dst >> 16) & 0xFF,
           (ip->ip_dst >> 8) & 0xFF, ip->ip_dst & 0xFF);

    // 检查目标IP是否匹配
    if (ip->ip_dst != dev->ip_addr && ip->ip_dst != 0xFFFFFFFF) {
        printf("[net] IP not for us, dropping\n");
        return -1;
    }

    // 根据协议分发
    switch (ip->ip_proto) {
        case IPPROTO_ICMP:
            return icmp_input(dev, data + (ip->ip_verhlen & 0x0F) * 4,
                             len - (ip->ip_verhlen & 0x0F) * 4);
        case IPPROTO_UDP:
            return udp_input(dev, data + (ip->ip_verhlen & 0x0F) * 4,
                             len - (ip->ip_verhlen & 0x0F) * 4);
        case IPPROTO_TCP:
            return tcp_input(dev, data + (ip->ip_verhlen & 0x0F) * 4,
                             len - (ip->ip_verhlen & 0x0F) * 4);
        default:
            printf("[net] Unknown IP protocol: %d\n", ip->ip_proto);
            break;
    }

    return 0;
}

/**
 * @brief ICMP输入处理
 */
int icmp_input(net_device_t *dev, uint8_t *data, uint32_t len) {
    icmp_hdr_t *icmp = (icmp_hdr_t *)data;

    printf("[net] ICMP: type=%d, code=%d\n", icmp->icmp_type, icmp->icmp_code);

    switch (icmp->icmp_type) {
        case ICMP_ECHO_REQUEST:
            printf("[net] Ping request received\n");
            // TODO: 发送ping应答
            break;
        case ICMP_ECHO_REPLY:
            printf("[net] Ping reply received\n");
            break;
        default:
            printf("[net] Unknown ICMP type: %d\n", icmp->icmp_type);
            break;
    }

    return 0;
}

/**
 * @brief UDP输入处理
 */
int udp_input(net_device_t *dev, uint8_t *data, uint32_t len) {
    udp_hdr_t *udp = (udp_hdr_t *)data;

    printf("[net] UDP: sport=%d, dport=%d, len=%d\n",
           ntohs(udp->udp_sport), ntohs(udp->udp_dport), ntohs(udp->udp_len));

    // TODO: 处理UDP数据

    return 0;
}

/**
 * @brief TCP输入处理
 */
int tcp_input(net_device_t *dev, uint8_t *data, uint32_t len) {
    tcp_hdr_t *tcp = (tcp_hdr_t *)data;

    printf("[net] TCP: sport=%d, dport=%d, flags=0x%x\n",
           ntohs(tcp->tcp_sport), ntohs(tcp->tcp_dport), tcp->tcp_flags);

    // TODO: 处理TCP数据

    return 0;
}

/**
 * @brief ARP请求
 */
int arp_request(net_device_t *dev, uint32_t ip_addr) {
    printf("[net] ARP request for %d.%d.%d.%d\n",
           (ip_addr >> 24) & 0xFF, (ip_addr >> 16) & 0xFF,
           (ip_addr >> 8) & 0xFF, ip_addr & 0xFF);

    // TODO: 发送ARP请求包

    return 0;
}

/**
 * @brief ARP应答
 */
int arp_reply(net_device_t *dev, uint32_t ip_addr, uint8_t *mac_addr) {
    printf("[net] ARP reply: %d.%d.%d.%d -> %02x:%02x:%02x:%02x:%02x:%02x\n",
           (ip_addr >> 24) & 0xFF, (ip_addr >> 16) & 0xFF,
           (ip_addr >> 8) & 0xFF, ip_addr & 0xFF,
           mac_addr[0], mac_addr[1], mac_addr[2],
           mac_addr[3], mac_addr[4], mac_addr[5]);

    // 添加到ARP缓存
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (!arp_table[i].valid) {
            arp_table[i].ip_addr = ip_addr;
            memcpy(arp_table[i].mac_addr, mac_addr, ETH_ALEN);
            arp_table[i].valid = 1;
            break;
        }
    }

    return 0;
}

/**
 * @brief ARP查找
 */
int arp_lookup(uint32_t ip_addr, uint8_t *mac_addr) {
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (arp_table[i].valid && arp_table[i].ip_addr == ip_addr) {
            memcpy(mac_addr, arp_table[i].mac_addr, ETH_ALEN);
            return 0;
        }
    }
    return -1;
}

/**
 * @brief 计算互联网校验和
 */
uint16_t internet_checksum(uint16_t *data, uint32_t len) {
    uint32_t sum = 0;

    while (len > 1) {
        sum += *data++;
        len -= 2;
    }

    if (len == 1) {
        sum += *(uint8_t *)data;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum;
}

/**
 * @brief 打印MAC地址
 */
void print_mac(uint8_t *mac) {
    printf("%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

/**
 * @brief 打印IP地址
 */
void print_ip(uint32_t ip) {
    printf("%d.%d.%d.%d",
           (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
           (ip >> 8) & 0xFF, ip & 0xFF);
}

/**
 * @brief 设置IP地址
 */
int net_set_ipaddr(uint32_t ip) {
    local_ip = ip;
    printf("[net] Set IP address to %d.%d.%d.%d\n",
           (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
           (ip >> 8) & 0xFF, ip & 0xFF);

    // 更新所有设备的IP
    for (int i = 0; i < num_devices; i++) {
        net_devices[i]->ip_addr = ip;
    }

    return 0;
}

/**
 * @brief 设置子网掩码
 */
int net_set_netmask(uint32_t mask) {
    netmask = mask;
    printf("[net] Set netmask to %d.%d.%d.%d\n",
           (mask >> 24) & 0xFF, (mask >> 16) & 0xFF,
           (mask >> 8) & 0xFF, mask & 0xFF);

    for (int i = 0; i < num_devices; i++) {
        net_devices[i]->netmask = mask;
    }

    return 0;
}

/**
 * @brief 设置网关
 */
int net_set_gateway(uint32_t gw) {
    gateway = gw;
    printf("[net] Set gateway to %d.%d.%d.%d\n",
           (gw >> 24) & 0xFF, (gw >> 16) & 0xFF,
           (gw >> 8) & 0xFF, gw & 0xFF);

    for (int i = 0; i < num_devices; i++) {
        net_devices[i]->gateway = gw;
    }

    return 0;
}
