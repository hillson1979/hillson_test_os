/**
 * @file core.c
 * @brief 网络协议栈核心实现
 */

#include "net.h"
#include "../include/printf.h"
#include "../include/string.h"
#include "../include/kmalloc.h"

// 网络设备列表
static net_device_t *net_devices[16];
static int num_devices = 0;

// ARP缓存表
static arp_entry_t arp_table[ARP_TABLE_SIZE];

// 网络统计
net_stats_t net_stats;

// 本机IP配置（QEMU 用户网络默认：10.0.2.0/24）
static uint32_t local_ip = 0x0A00020F;  // 10.0.2.15
static uint32_t netmask = 0xFFFFFF00;   // 255.255.255.0
static uint32_t gateway = 0x0A000002;   // 10.0.2.2 (QEMU 默认网关)

// 以太网广播地址
uint8_t eth_broadcast[ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// 前向声明
static void arp_cache_update(uint32_t ip_addr, uint8_t *mac_addr);

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
 * @brief 构建以太网帧并发送
 */
static int eth_send(net_device_t *dev, uint8_t *dst_mac, uint16_t eth_type,
                    uint8_t *data, uint32_t len) {
    // 分配以太网帧缓冲区
    uint8_t *frame = (uint8_t *)kmalloc(ETH_HDR_LEN + len);
    if (!frame) {
        printf("[net] Failed to allocate Ethernet frame\n");
        return -1;
    }

    eth_hdr_t *eth = (eth_hdr_t *)frame;

    // 填充以太网头部
    memcpy(eth->eth_dst, dst_mac, ETH_ALEN);
    memcpy(eth->eth_src, dev->mac_addr, ETH_ALEN);
    eth->eth_type = htons(eth_type);

    // 复制数据
    memcpy(frame + ETH_HDR_LEN, data, len);

    // 发送
    int ret = net_tx_packet(dev, frame, ETH_HDR_LEN + len);

    kfree(frame);
    return ret;
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
            return arp_input(dev, data + ETH_HDR_LEN, len - ETH_HDR_LEN);
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
 * @brief IP输出处理
 */
int ip_output(net_device_t *dev, uint32_t dst_ip, uint8_t protocol,
              uint8_t *data, uint32_t len) {
    printf("[net] IP output: dst=%d.%d.%d.%d, proto=%d, len=%d\n",
           (dst_ip >> 24) & 0xFF, (dst_ip >> 16) & 0xFF,
           (dst_ip >> 8) & 0xFF, dst_ip & 0xFF, protocol, len);

    // 检查是否在同一子网
    uint32_t net_dst = dst_ip;
    if ((dst_ip & dev->netmask) != (dev->ip_addr & dev->netmask)) {
        // 不同子网，发送到网关
        net_dst = dev->gateway;
        printf("[net] Different subnet, sending to gateway\n");
    }

    // 解析目标MAC地址
    uint8_t dst_mac[ETH_ALEN];
    if (arp_resolve(dev, net_dst, dst_mac) != 0) {
        printf("[net] ARP resolution failed, packet queued\n");
        // TODO: 实际应该将包排队等待ARP完成
        return -1;
    }

    // 分配IP包缓冲区
    uint32_t total_len = sizeof(ip_hdr_t) + len;
    uint8_t *packet = (uint8_t *)kmalloc(total_len);
    if (!packet) {
        printf("[net] Failed to allocate IP packet\n");
        return -1;
    }

    ip_hdr_t *ip = (ip_hdr_t *)packet;

    // 填充IP头部
    ip->ip_verhlen = 0x45;  // Version=4, IHL=5 (20 bytes)
    ip->ip_tos = 0;
    ip->ip_len = htons(total_len);
    ip->ip_id = htons(1);  // 简单的ID
    ip->ip_off = 0;
    ip->ip_ttl = IP_TTL;
    ip->ip_proto = protocol;
    ip->ip_sum = 0;
    ip->ip_src = dev->ip_addr;
    ip->ip_dst = dst_ip;

    // 计算IP校验和
    ip->ip_sum = internet_checksum((uint16_t *)ip, sizeof(ip_hdr_t));

    // 复制数据
    memcpy(packet + sizeof(ip_hdr_t), data, len);

    // 通过以太网发送
    int ret = eth_send(dev, dst_mac, ETH_P_IP, packet, total_len);

    kfree(packet);
    return ret;
}

/**
 * @brief ICMP输入处理
 */
int icmp_input(net_device_t *dev, uint8_t *data, uint32_t len) {
    icmp_hdr_t *icmp = (icmp_hdr_t *)data;

    printf("[net] ICMP: type=%d, code=%d\n", icmp->icmp_type, icmp->icmp_code);

    switch (icmp->icmp_type) {
        case ICMP_ECHO_REQUEST: {
            printf("[net] Ping request received, sending reply\n");

            // 获取IP头部（需要回退到IP层）
            ip_hdr_t *ip = (ip_hdr_t *)((uint8_t *)data - sizeof(ip_hdr_t));

            // 构造ICMP应答
            uint16_t icmp_len = len;
            uint8_t *reply_buf = (uint8_t *)kmalloc(icmp_len);
            if (!reply_buf) {
                printf("[net] Failed to allocate ICMP reply\n");
                return -1;
            }

            icmp_hdr_t *icmp_reply = (icmp_hdr_t *)reply_buf;

            // 复制原始ICMP包并修改类型
            memcpy(reply_buf, data, icmp_len);
            icmp_reply->icmp_type = ICMP_ECHO_REPLY;
            icmp_reply->icmp_sum = 0;  // 清零校验和

            // 计算新的校验和
            icmp_reply->icmp_sum = internet_checksum((uint16_t *)reply_buf, icmp_len);

            // 通过IP发送应答
            ip_output(dev, ip->ip_src, IPPROTO_ICMP, reply_buf, icmp_len);

            kfree(reply_buf);
            break;
        }
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
 * @brief 发送 ICMP Echo Request (Ping)
 */
int icmp_send_echo(net_device_t *dev, uint32_t dst_ip, uint16_t id, uint16_t seq) {
    // 分配 ICMP 包
    uint32_t icmp_len = sizeof(icmp_hdr_t) + 4;  // 头部 + 4 字节时间戳/数据
    uint8_t *packet = (uint8_t *)kmalloc(icmp_len);
    if (!packet) {
        printf("[net] Failed to allocate ICMP echo packet\n");
        return -1;
    }

    icmp_hdr_t *icmp = (icmp_hdr_t *)packet;

    // 填充 ICMP 头部
    icmp->icmp_type = ICMP_ECHO_REQUEST;
    icmp->icmp_code = 0;
    icmp->icmp_sum = 0;
    icmp->icmp_id = htons(id);
    icmp->icmp_seq = htons(seq);

    // 添加一些数据（时间戳或填充）
    uint32_t *data = (uint32_t *)(packet + sizeof(icmp_hdr_t));
    *data = 0x12345678;  // 简单的魔术数字作为数据

    // 计算校验和
    icmp->icmp_sum = internet_checksum((uint16_t *)packet, icmp_len);

    // 通过 IP 发送
    int ret = ip_output(dev, dst_ip, IPPROTO_ICMP, packet, icmp_len);

    kfree(packet);
    return ret;
}

/**
 * @brief UDP输入处理
 */
int udp_input(net_device_t *dev, uint8_t *data, uint32_t len) {
    udp_hdr_t *udp = (udp_hdr_t *)data;

    if (len < sizeof(udp_hdr_t)) {
        printf("[net] UDP packet too short\n");
        return -1;
    }

    printf("[net] UDP: sport=%d, dport=%d, len=%d\n",
           ntohs(udp->udp_sport), ntohs(udp->udp_dport), ntohs(udp->udp_len));

    // 提取UDP数据
    uint8_t *udp_data = data + sizeof(udp_hdr_t);
    uint32_t udp_data_len = len - sizeof(udp_hdr_t);

    if (udp_data_len > 0) {
        printf("[net] UDP data: ");
        for (uint32_t i = 0; i < udp_data_len && i < 32; i++) {
            printf("%c", udp_data[i]);
        }
        printf("\n");
    }

    // TODO: 将数据传递给应用层套接字

    return 0;
}

/**
 * @brief UDP输出处理
 */
int udp_output(net_device_t *dev, uint32_t dst_ip, uint16_t src_port,
               uint16_t dst_port, uint8_t *data, uint32_t len) {
    printf("[net] UDP output: dst=%d.%d.%d.%d, sport=%d, dport=%d, len=%d\n",
           (dst_ip >> 24) & 0xFF, (dst_ip >> 16) & 0xFF,
           (dst_ip >> 8) & 0xFF, dst_ip & 0xFF,
           src_port, dst_port, len);

    // 分配UDP包缓冲区
    uint32_t total_len = sizeof(udp_hdr_t) + len;
    uint8_t *packet = (uint8_t *)kmalloc(total_len);
    if (!packet) {
        printf("[net] Failed to allocate UDP packet\n");
        return -1;
    }

    udp_hdr_t *udp = (udp_hdr_t *)packet;

    // 填充UDP头部
    udp->udp_sport = htons(src_port);
    udp->udp_dport = htons(dst_port);
    udp->udp_len = htons(total_len);
    udp->udp_sum = 0;  // UDP校验和可选，这里设为0

    // 复制数据
    memcpy(packet + sizeof(udp_hdr_t), data, len);

    // 通过IP发送
    int ret = ip_output(dev, dst_ip, IPPROTO_UDP, packet, total_len);

    kfree(packet);
    return ret;
}

/**
 * @brief TCP输入处理
 */
int tcp_input(net_device_t *dev, uint8_t *data, uint32_t len) {
    tcp_hdr_t *tcp = (tcp_hdr_t *)data;

    if (len < sizeof(tcp_hdr_t)) {
        printf("[net] TCP packet too short\n");
        return -1;
    }

    uint8_t tcp_hdr_len = (tcp->tcp_off >> 4) * 4;

    printf("[net] TCP: sport=%d, dport=%d, flags=0x%x, seq=%d, ack=%d\n",
           ntohs(tcp->tcp_sport), ntohs(tcp->tcp_dport),
           tcp->tcp_flags, ntohl(tcp->tcp_seq), ntohl(tcp->tcp_ack));

    // 处理TCP标志位
    if (tcp->tcp_flags & TCP_SYN) {
        printf("[net] TCP SYN received\n");
        // TODO: 发送 SYN-ACK
    }

    if (tcp->tcp_flags & TCP_ACK) {
        printf("[net] TCP ACK received\n");
    }

    if (tcp->tcp_flags & TCP_FIN) {
        printf("[net] TCP FIN received\n");
        // TODO: 关闭连接
    }

    if (tcp->tcp_flags & TCP_PSH) {
        printf("[net] TCP PSH (data) received\n");

        // 提取TCP数据
        uint8_t *tcp_data = data + tcp_hdr_len;
        uint32_t tcp_data_len = len - tcp_hdr_len;

        if (tcp_data_len > 0) {
            printf("[net] TCP data: ");
            for (uint32_t i = 0; i < tcp_data_len && i < 32; i++) {
                printf("%c", tcp_data[i]);
            }
            printf("\n");
        }
    }

    // TODO: 将数据传递给应用层套接字

    return 0;
}

/**
 * @brief 计算TCP校验和（包含伪头部）
 */
static uint16_t tcp_checksum(net_device_t *dev, uint32_t dst_ip,
                              uint8_t *data, uint32_t len) {
    // TCP伪头部
    struct {
        uint32_t src_ip;
        uint32_t dst_ip;
        uint8_t  zero;
        uint8_t  protocol;
        uint16_t tcp_len;
    } pseudo_hdr;

    pseudo_hdr.src_ip = dev->ip_addr;
    pseudo_hdr.dst_ip = dst_ip;
    pseudo_hdr.zero = 0;
    pseudo_hdr.protocol = IPPROTO_TCP;
    pseudo_hdr.tcp_len = htons(len);

    // 分配临时缓冲区
    uint32_t total_len = sizeof(pseudo_hdr) + len;
    uint16_t *buffer = (uint16_t *)kmalloc(total_len);
    if (!buffer) {
        return 0;
    }

    // 复制伪头部和数据
    memcpy(buffer, &pseudo_hdr, sizeof(pseudo_hdr));
    memcpy((uint8_t *)buffer + sizeof(pseudo_hdr), data, len);

    // 计算校验和
    uint16_t sum = internet_checksum(buffer, total_len);

    kfree(buffer);
    return sum;
}

/**
 * @brief TCP输出处理
 */
int tcp_output(net_device_t *dev, uint32_t dst_ip, uint16_t src_port,
               uint16_t dst_port, uint32_t seq, uint32_t ack,
               uint8_t flags, uint8_t *data, uint32_t len) {
    printf("[net] TCP output: dst=%d.%d.%d.%d, sport=%d, dport=%d, flags=0x%x\n",
           (dst_ip >> 24) & 0xFF, (dst_ip >> 16) & 0xFF,
           (dst_ip >> 8) & 0xFF, dst_ip & 0xFF,
           src_port, dst_port, flags);

    // 计算TCP头部长度（至少20字节）
    uint8_t tcp_hdr_len = 5;  // 5 * 4 = 20 bytes

    // 分配TCP包缓冲区
    uint32_t total_len = tcp_hdr_len * 4 + len;
    uint8_t *packet = (uint8_t *)kmalloc(total_len);
    if (!packet) {
        printf("[net] Failed to allocate TCP packet\n");
        return -1;
    }

    tcp_hdr_t *tcp = (tcp_hdr_t *)packet;

    // 填充TCP头部
    tcp->tcp_sport = htons(src_port);
    tcp->tcp_dport = htons(dst_port);
    tcp->tcp_seq = htonl(seq);
    tcp->tcp_ack = htonl(ack);
    tcp->tcp_off = (tcp_hdr_len << 4);  // 数据偏移
    tcp->tcp_flags = flags;
    tcp->tcp_win = htons(8192);  // 窗口大小
    tcp->tcp_urg = 0;
    tcp->tcp_sum = 0;

    // 复制数据
    if (data && len > 0) {
        memcpy(packet + tcp_hdr_len * 4, data, len);
    }

    // 计算TCP校验和（包含伪头部）
    tcp->tcp_sum = tcp_checksum(dev, dst_ip, packet, total_len);

    // 通过IP发送
    int ret = ip_output(dev, dst_ip, IPPROTO_TCP, packet, total_len);

    kfree(packet);
    return ret;
}

/**
 * @brief ARP输入处理
 */
int arp_input(net_device_t *dev, uint8_t *data, uint32_t len) {
    if (len < sizeof(arp_hdr_t)) {
        printf("[net] ARP packet too short\n");
        return -1;
    }

    arp_hdr_t *arp = (arp_hdr_t *)data;

    printf("[net] ARP: op=%d, sip=%d.%d.%d.%d, tip=%d.%d.%d.%d\n",
           ntohs(arp->arp_op),
           (arp->arp_spa >> 24) & 0xFF, (arp->arp_spa >> 16) & 0xFF,
           (arp->arp_spa >> 8) & 0xFF, arp->arp_spa & 0xFF,
           (arp->arp_tpa >> 24) & 0xFF, (arp->arp_tpa >> 16) & 0xFF,
           (arp->arp_tpa >> 8) & 0xFF, arp->arp_tpa & 0xFF);

    // 检查硬件类型和协议类型
    if (ntohs(arp->arp_hrd) != ARPHRD_ETHER || ntohs(arp->arp_pro) != ETH_P_IP) {
        printf("[net] ARP: unsupported hardware or protocol\n");
        return -1;
    }

    // 处理ARP请求
    if (ntohs(arp->arp_op) == ARPOP_REQUEST) {
        // 检查是否询问我们的IP
        if (arp->arp_tpa == dev->ip_addr) {
            printf("[net] ARP request for our IP, sending reply\n");

            // 构造ARP应答
            arp_hdr_t arp_reply;
            arp_reply.arp_hrd = htons(ARPHRD_ETHER);
            arp_reply.arp_pro = htons(ETH_P_IP);
            arp_reply.arp_hln = ETH_ALEN;
            arp_reply.arp_pln = 4;
            arp_reply.arp_op = htons(ARPOP_REPLY);

            // 发送方MAC和IP（我们）
            memcpy(arp_reply.arp_sha, dev->mac_addr, ETH_ALEN);
            arp_reply.arp_spa = dev->ip_addr;

            // 目标MAC和IP（请求方）
            memcpy(arp_reply.arp_tha, arp->arp_sha, ETH_ALEN);
            arp_reply.arp_tpa = arp->arp_spa;

            // 发送ARP应答
            eth_send(dev, arp->arp_sha, ETH_P_ARP, (uint8_t *)&arp_reply, sizeof(arp_hdr_t));

            // 同时更新ARP缓存
            arp_cache_update(arp->arp_spa, arp->arp_sha);
        }
    }
    // 处理ARP应答
    else if (ntohs(arp->arp_op) == ARPOP_REPLY) {
        printf("[net] ARP reply received\n");
        arp_cache_update(arp->arp_spa, arp->arp_sha);
    }
    else {
        printf("[net] Unknown ARP operation: %d\n", ntohs(arp->arp_op));
    }

    return 0;
}

/**
 * @brief 更新ARP缓存
 */
static void arp_cache_update(uint32_t ip_addr, uint8_t *mac_addr) {
    // 首先查找是否已存在
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (arp_table[i].valid && arp_table[i].ip_addr == ip_addr) {
            memcpy(arp_table[i].mac_addr, mac_addr, ETH_ALEN);
            printf("[net] ARP cache updated: %d.%d.%d.%d -> %02x:%02x:%02x:%02x:%02x:%02x\n",
                   (ip_addr >> 24) & 0xFF, (ip_addr >> 16) & 0xFF,
                   (ip_addr >> 8) & 0xFF, ip_addr & 0xFF,
                   mac_addr[0], mac_addr[1], mac_addr[2],
                   mac_addr[3], mac_addr[4], mac_addr[5]);
            return;
        }
    }

    // 查找空闲位置
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (!arp_table[i].valid) {
            arp_table[i].ip_addr = ip_addr;
            memcpy(arp_table[i].mac_addr, mac_addr, ETH_ALEN);
            arp_table[i].valid = 1;
            printf("[net] ARP cache added: %d.%d.%d.%d -> %02x:%02x:%02x:%02x:%02x:%02x\n",
                   (ip_addr >> 24) & 0xFF, (ip_addr >> 16) & 0xFF,
                   (ip_addr >> 8) & 0xFF, ip_addr & 0xFF,
                   mac_addr[0], mac_addr[1], mac_addr[2],
                   mac_addr[3], mac_addr[4], mac_addr[5]);
            return;
        }
    }

    printf("[net] ARP cache full!\n");
}

/**
 * @brief ARP请求
 */
int arp_request(net_device_t *dev, uint32_t ip_addr) {
    printf("[net] ARP request for %d.%d.%d.%d\n",
           (ip_addr >> 24) & 0xFF, (ip_addr >> 16) & 0xFF,
           (ip_addr >> 8) & 0xFF, ip_addr & 0xFF);

    // 构造ARP请求包
    arp_hdr_t arp_req;
    arp_req.arp_hrd = htons(ARPHRD_ETHER);
    arp_req.arp_pro = htons(ETH_P_IP);
    arp_req.arp_hln = ETH_ALEN;
    arp_req.arp_pln = 4;
    arp_req.arp_op = htons(ARPOP_REQUEST);

    // 发送方MAC和IP（我们）
    memcpy(arp_req.arp_sha, dev->mac_addr, ETH_ALEN);
    arp_req.arp_spa = dev->ip_addr;

    // 目标MAC（未知，全0）和IP（询问的IP）
    memset(arp_req.arp_tha, 0, ETH_ALEN);
    arp_req.arp_tpa = ip_addr;

    // 广播ARP请求
    return eth_send(dev, eth_broadcast, ETH_P_ARP, (uint8_t *)&arp_req, sizeof(arp_hdr_t));
}

/**
 * @brief ARP查找并解析
 */
int arp_resolve(net_device_t *dev, uint32_t ip_addr, uint8_t *mac_addr) {
    // 首先在缓存中查找
    if (arp_lookup(ip_addr, mac_addr) == 0) {
        return 0;
    }

    // 未找到，发送ARP请求
    printf("[net] ARP cache miss for %d.%d.%d.%d, sending request\n",
           (ip_addr >> 24) & 0xFF, (ip_addr >> 16) & 0xFF,
           (ip_addr >> 8) & 0xFF, ip_addr & 0xFF);

    arp_request(dev, ip_addr);

    return -1;  // 未找到
}

/**
 * @brief 简单的ARP查找（仅查询缓存）
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
