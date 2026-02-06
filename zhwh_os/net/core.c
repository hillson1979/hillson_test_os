/**
 * @file core.c
 * @brief 网络协议栈核心实现
 */

#include "net.h"
#include "../include/printf.h"
#include "../include/string.h"
#include "../include/kmalloc.h"

extern void vga_setcolor(uint8_t fg, uint8_t bg);
#define SET_COLOR_RED()     vga_setcolor(4, 0)   // 红字黑底
#define SET_COLOR_GREEN()     vga_setcolor(2, 0)   // 绿字黑底

// 网络设备列表
static net_device_t *net_devices[16];
static int num_devices = 0;

// ARP缓存表
static arp_entry_t arp_table[ARP_TABLE_SIZE];  // 🔥 改为全局可见，方便 dump_rx 访问

// 网络统计
net_stats_t net_stats;

// 本机IP配置（192.168.0.x 网段）
uint32_t local_ip = 0xC0A8000F;  // 192.168.0.15（全局变量）
static uint32_t netmask = 0xFFFFFF00;   // 255.255.255.0
static uint32_t gateway = 0xC0A80001;   // 192.168.0.1 (网关)

// 🔥 本机 MAC 地址（全局变量，用于接收包过滤）
uint8_t local_mac[ETH_ALEN] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};  // 默认值，会被设备初始化覆盖

// 🔥 当前选择的网络设备名称（从 syscall.c 导入）
extern char current_net_device[];

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

    // 🔥 预绑定：192.168.0.145 -> D8:D0:90:15:E2:68
    uint32_t target_ip = 0xC0A80091;  // 192.168.0.145 (主机字节序)
    uint8_t target_mac[6] = {0xD8, 0xD0, 0x90, 0x15, 0xE2, 0x68};

    arp_table[0].ip = target_ip;
    memcpy(arp_table[0].mac, target_mac, 6);
    arp_table[0].valid = 1;

    printf("[net] Pre-populated ARP cache:\n");
    printf("[net]   %d.%d.%d.%d -> %02x:%02x:%02x:%02x:%02x:%02x\n",
           (target_ip >> 24) & 0xFF, (target_ip >> 16) & 0xFF,
           (target_ip >> 8) & 0xFF, target_ip & 0xFF,
           target_mac[0], target_mac[1], target_mac[2],
           target_mac[3], target_mac[4], target_mac[5]);

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
    dev->gateway = gateway;  // 设置网关
    dev->mtu = ETH_MTU;

    net_devices[num_devices++] = dev;

    // printf("[net] Registered device: %s\n", dev->name);
    // printf("[net]   MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
    //        dev->mac_addr[0], dev->mac_addr[1], dev->mac_addr[2],
    //        dev->mac_addr[3], dev->mac_addr[4], dev->mac_addr[5]);

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
 * @brief 获取当前网络设备数量
 */
int net_get_device_count(void) {
    return num_devices;
}

/**
 * @brief 获取所有网络设备数组
 */
net_device_t **net_get_all_devices(void) {
    return net_devices;
}


/**
 * @brief 检查是否是有效的以太网类型
 * @return 1 = 有效, 0 = 无效
 */
static int is_valid_eth_type(uint16_t eth_type) {
    // 有效的以太网类型
    switch (eth_type) {
        case ETH_P_IP:      // IPv4 (0x0800)
        case ETH_P_ARP:     // ARP (0x0806)
        case ETH_P_IPV6:    // IPv6 (0x86DD)
        case 0x8100:        // VLAN tag (802.1Q)
            return 1;
        default:
            return 0;
    }
}

/**
 * @brief 接收数据包
 */
int net_rx_packet(net_device_t *dev, uint8_t *data, uint32_t len) {
    // 🔥 解析以太网帧头
    eth_hdr_t *eth = (eth_hdr_t *)data;
    uint16_t eth_type = ntohs(eth->eth_type);

    // 🔥🔥 过滤：检查是否是有效的以太网类型
    if (!is_valid_eth_type(eth_type)) {
        printf("[net] DROP: Invalid EtherType 0x%04x (not IP/ARP/VLAN)\n", eth_type);
        net_stats.rx_dropped++;
        return -1;
    }

    // 🔥🔥 优先处理 ARP 包（在最前面）
    if (eth_type == ETH_P_ARP) {
        printf("[net] -> Calling arp_handle\n");
        arp_handle(dev, data, len);
        return 0;
    }

    // 🔥🔥 调试：显示前 64 字节（限制输出长度）
    printf("[net] RAW %d bytes: ", len > 64 ? 64 : len);
    for (int i = 0; i < 64 && i < len; i++) {
        printf("%02x ", data[i]);
    }
    printf("\n");

    SET_COLOR_RED();
    printf("[net] GOT PACKET len=%d\n", len);
    printf("[net]   dst MAC: %02x:%02x:%02x:%02x:%02x:%02x (our MAC: ",
           eth->eth_dst[0], eth->eth_dst[1], eth->eth_dst[2],
           eth->eth_dst[3], eth->eth_dst[4], eth->eth_dst[5]);

    // 显示我们的 MAC 用于对比
    extern uint8_t local_mac[ETH_ALEN];
    printf("%02x:%02x:%02x:%02x:%02x:%02x)\n",
           local_mac[0], local_mac[1], local_mac[2],
           local_mac[3], local_mac[4], local_mac[5]);

    printf("[net]   src MAC: %02x:%02x:%02x:%02x:%02x:%02x",
           eth->eth_src[0], eth->eth_src[1], eth->eth_src[2],
           eth->eth_src[3], eth->eth_src[4], eth->eth_src[5]);

    // 显示协议类型（eth_type 已在前面定义）
    if (eth_type == ETH_P_IP) {
        printf(" (IP)");
    } else if (eth_type == ETH_P_ARP) {
        printf(" (ARP)");
    } else {
        printf(" (type=0x%04x)", eth_type);
    }

    // 如果是 IP 包（EtherType 0x0800），追加显示源 IP 和目标 IP
    if (eth_type == ETH_P_IP) {
        ip_hdr_t *ip = (ip_hdr_t *)(data + sizeof(eth_hdr_t));
        printf("\n[net]   src IP: %d.%d.%d.%d -> dst IP: %d.%d.%d.%d",
               (ntohl(ip->ip_src) >> 24) & 0xFF,
               (ntohl(ip->ip_src) >> 16) & 0xFF,
               (ntohl(ip->ip_src) >> 8) & 0xFF,
               ntohl(ip->ip_src) & 0xFF,
               (ntohl(ip->ip_dst) >> 24) & 0xFF,
               (ntohl(ip->ip_dst) >> 16) & 0xFF,
               (ntohl(ip->ip_dst) >> 8) & 0xFF,
               ntohl(ip->ip_dst) & 0xFF);
    }
    printf("\n");
    SET_COLOR_GREEN();

    if (!dev || !data || len < ETH_HDR_LEN) {
        //printf("[net] ERROR: Invalid parameters! dev=%p, data=%p, len=%d\n", dev, data, len);
        net_stats.rx_errors++;
        return -1;
    }

    // 🔥🔥 过滤：检查目标 MAC 是否匹配本机（广播、多播、本机 MAC）
    // 检查广播 MAC (FF:FF:FF:FF:FF:FF)
    if (eth->eth_dst[0] == 0xFF && eth->eth_dst[1] == 0xFF &&
        eth->eth_dst[2] == 0xFF && eth->eth_dst[3] == 0xFF &&
        eth->eth_dst[4] == 0xFF && eth->eth_dst[5] == 0xFF) {
        printf("[net] RX: Broadcast packet\n");
    }
    // 检查本机 MAC（使用全局 local_mac）
    else if (memcmp(eth->eth_dst, local_mac, ETH_ALEN) == 0) {
        printf("[net] RX: Unicast to us\n");
    }
    // 多播 MAC（01:00:5E 开头或 33:33 开头）
    else if (eth->eth_dst[0] == 0x01 || eth->eth_dst[0] == 0x33) {
        printf("[net] RX: Multicast packet\n");
    }
    // 不是给我们的包
    else {
        printf("[net] RX: NOT for us, dropping packet\n");
        return 0;  // 不是错误，只是不是给我们的
    }

    // 🔥 如果是 IP 包，检查目标 IP 是否匹配本机
    if (ntohs(eth->eth_type) == ETH_P_IP) {
        ip_hdr_t *ip = (ip_hdr_t *)(data + sizeof(eth_hdr_t));

        uint32_t dst_ip = ntohl(ip->ip_dst);
        uint32_t our_ip = local_ip;  // ✅ local_ip 已经是主机字节序

        // printf("[net] RX: dst IP=%d.%d.%d.%d, our IP=%d.%d.%d.%d\n",
        //        (dst_ip >> 24) & 0xFF, (dst_ip >> 16) & 0xFF,
        //        (dst_ip >> 8) & 0xFF, dst_ip & 0xFF,
        //        (our_ip >> 24) & 0xFF, (our_ip >> 16) & 0xFF,
        //        (our_ip >> 8) & 0xFF, our_ip & 0xFF);

        // 如果目标 IP 不是本机 IP，且不是广播 (255.255.255.255)
        if (dst_ip != our_ip && dst_ip != 0xFFFFFFFF) {
            //printf("[net] RX: NOT for us (dst IP != our IP), dropping\n");
            return 0;  // 不是错误，只是不是给我们的
        }
    }

    printf("[net] === net_rx_packet ENTRY ===\n");
    printf("[net] param dev = 0x%x\n", (uint32_t)dev);
    printf("[net] param data = 0x%x\n", (uint32_t)data);
    printf("[net] param len = %u\n", len);
    printf("[net] dev->name = %s\n", dev ? dev->name : "NULL");
    printf("[net] ===========================\n");

    net_stats.rx_packets++;
    net_stats.rx_bytes += len;
    printf("[net] Stats updated: rx_packets=%d, rx_bytes=%d\n",
           net_stats.rx_packets, net_stats.rx_bytes);

    
    
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

    // 🔥 打印完整的以太网帧（16 进制）
    SET_COLOR_RED();
    printf("[net] eth_send: %d bytes\n", ETH_HDR_LEN + len);
    for (uint32_t i = 0; i < ETH_HDR_LEN + len; i++) {
        printf("%02x ", frame[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    if ((ETH_HDR_LEN + len) % 16 != 0) printf("\n");
    SET_COLOR_GREEN();

    // 发送
    int ret = net_tx_packet(dev, frame, ETH_HDR_LEN + len);

    kfree(frame);
    return ret;
}

/**
 * @brief 以太网输入处理
 */
int eth_input(net_device_t *dev, uint8_t *data, uint32_t len) {
    if (len < ETH_HDR_LEN)
        return -1;
    eth_hdr_t *eth = (eth_hdr_t *)data;

    printf("[net] Eth frame: type=0x%04x, len=%d, dst=%02x:%02x:%02x:%02x:%02x:%02x\n",
           ntohs(eth->eth_type), len,
           eth->eth_dst[0], eth->eth_dst[1], eth->eth_dst[2],
           eth->eth_dst[3], eth->eth_dst[4], eth->eth_dst[5]);

    // 根据以太网类型分发
    switch (ntohs(eth->eth_type)) {
        case ETH_P_IP:
            printf("[net] -> Calling ip_input\n");
            return ip_input(dev, data + ETH_HDR_LEN, len - ETH_HDR_LEN);
        case ETH_P_ARP:
            printf("[net] -> Calling arp_input\n");
            return arp_input(dev, data + ETH_HDR_LEN, len - ETH_HDR_LEN);
        default:
            printf("[net] Unknown eth type: 0x%x\n", ntohs(eth->eth_type));
            return -1;
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
    uint32_t dst_ip = ntohl(ip->ip_dst);
    uint32_t our_ip = dev->ip_addr;  // dev->ip_addr 已经是主机字节序

    if (dst_ip != our_ip && dst_ip != 0xFFFFFFFF) {
        printf("[net] IP not for us (dst=%d.%d.%d.%d, our=%d.%d.%d.%d), dropping\n",
               (dst_ip >> 24) & 0xFF, (dst_ip >> 16) & 0xFF,
               (dst_ip >> 8) & 0xFF, dst_ip & 0xFF,
               (our_ip >> 24) & 0xFF, (our_ip >> 16) & 0xFF,
               (our_ip >> 8) & 0xFF, our_ip & 0xFF);
        return -1;
    }

    // 根据协议分发
    switch (ip->ip_proto) {
        case IPPROTO_ICMP:
            printf("[net] -> Calling icmp_input\n");
            return icmp_input(dev, data + (ip->ip_verhlen & 0x0F) * 4,
                             len - (ip->ip_verhlen & 0x0F) * 4);
        case IPPROTO_UDP:
            printf("[net] -> Calling udp_input\n");
            return udp_input(dev, data + (ip->ip_verhlen & 0x0F) * 4,
                             len - (ip->ip_verhlen & 0x0F) * 4);
        case IPPROTO_TCP:
            printf("[net] -> Calling tcp_input\n");
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
    uint32_t dst_network = dst_ip & dev->netmask;
    uint32_t local_network = dev->ip_addr & dev->netmask;

    printf("[net] Device IP: 0x%x (%d.%d.%d.%d)\n", dev->ip_addr,
           (dev->ip_addr >> 24) & 0xFF, (dev->ip_addr >> 16) & 0xFF,
           (dev->ip_addr >> 8) & 0xFF, dev->ip_addr & 0xFF);
    printf("[net] Netmask: 0x%x (%d.%d.%d.%d)\n", dev->netmask,
           (dev->netmask >> 24) & 0xFF, (dev->netmask >> 16) & 0xFF,
           (dev->netmask >> 8) & 0xFF, dev->netmask & 0xFF);
    printf("[net] Subnet check: dst_network=%d.%d.%d.%d, local_network=%d.%d.%d.%d\n",
           (dst_network >> 24) & 0xFF, (dst_network >> 16) & 0xFF,
           (dst_network >> 8) & 0xFF, dst_network & 0xFF,
           (local_network >> 24) & 0xFF, (local_network >> 16) & 0xFF,
           (local_network >> 8) & 0xFF, local_network & 0xFF);

    if (dst_network != local_network) {
        // 不同子网，使用网关
        printf("[net] Different subnet detected!\n");
        if (dev->gateway != 0) {
            printf("[net] Using gateway: %d.%d.%d.%d\n",
                   (dev->gateway >> 24) & 0xFF, (dev->gateway >> 16) & 0xFF,
                   (dev->gateway >> 8) & 0xFF, dev->gateway & 0xFF);
            net_dst = dev->gateway;
        } else {
            printf("[net] ERROR: Different subnet but no gateway configured\n");
            return -1;
        }
    } else {
        printf("[net] Same subnet, direct delivery\n");
    }

    // 🔥 解析目标MAC地址（使用新的 ARP cache lookup）
    uint8_t *dst_mac = arp_cache_lookup(net_dst);

    if (!dst_mac) {
        // 没有 MAC，先发送 ARP 请求
        printf("[net] ARP cache miss, sending request for %d.%d.%d.%d\n",
               (net_dst >> 24) & 0xFF, (net_dst >> 16) & 0xFF,
               (net_dst >> 8) & 0xFF, net_dst & 0xFF);
        arp_send_request(dev, net_dst);

        // 🔥 等待 ARP reply（中断驱动）
        printf("[net] Waiting for ARP reply (interrupt-driven)...\n");

        for (int retry = 0; retry < 5; retry++) {
            // 等待一段时间（约 100ms）
            for (volatile int i = 0; i < 10000000; i++) {
                asm volatile("nop");
            }

            // 检查 ARP 表（中断处理程序会更新）
            dst_mac = arp_cache_lookup(net_dst);
            if (dst_mac) {
                printf("[net] ARP resolved after %d retries!\n", retry + 1);
                break;
            }

            printf("[net] ARP retry %d/5...\n", retry + 1);
        }

        // 如果还是没有 MAC，放弃
        if (!dst_mac) {
            printf("[net] ARP resolution timeout, packet queued\n");
            return -1;
        }
    } else {
        printf("[net] ARP cache hit: %d.%d.%d.%d -> %02x:%02x:%02x:%02x:%02x:%02x\n",
               (net_dst >> 24) & 0xFF, (net_dst >> 16) & 0xFF,
               (net_dst >> 8) & 0xFF, net_dst & 0xFF,
               dst_mac[0], dst_mac[1], dst_mac[2],
               dst_mac[3], dst_mac[4], dst_mac[5]);
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
    ip->ip_src = htonl(dev->ip_addr);  // 🔥 转换为网络字节序
    ip->ip_dst = htonl(dst_ip);         // 🔥 转换为网络字节序

    // 计算IP校验和
    ip->ip_sum = internet_checksum((uint16_t *)ip, sizeof(ip_hdr_t));

    // 复制数据
    memcpy(packet + sizeof(ip_hdr_t), data, len);

    // 通过以太网发送
    printf("[net] -> Calling eth_send (IP packet)\n");
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
            SET_COLOR_RED();
            printf("[net] Ping request received, sending reply\n");

            // 获取IP头部（需要回退到IP层）
            // 注意：data 指向 ICMP 头，前面是 IP 头
            ip_hdr_t *ip = (ip_hdr_t *)((uint8_t *)data - sizeof(ip_hdr_t));

            // 🔥 调试：验证 IP 头位置
            printf("[net]   IP header located at: 0x%x (data at 0x%x)\n",
                   (uint32_t)ip, (uint32_t)data);
            printf("[net]   IP src (network): 0x%08x\n", ip->ip_src);
            printf("[net]   IP dst (network): 0x%08x\n", ip->ip_dst);

            // 打印原始 ICMP 包信息
            icmp_hdr_t *icmp_req = (icmp_hdr_t *)data;
            printf("[net]   ICMP id=0x%04x, seq=%d\n",
                   ntohs(icmp_req->icmp_id), ntohs(icmp_req->icmp_seq));

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

            // 🔥 调试：打印 payload 长度
            uint16_t payload_len = icmp_len - sizeof(icmp_hdr_t);
            printf("[net]   Payload len: %d bytes\n", payload_len);

            // 🔥 调试：打印 payload 内容（前 16 字节）
            if (payload_len > 0) {
                uint8_t *payload = reply_buf + sizeof(icmp_hdr_t);
                printf("[net]   Payload: ");
                for (int i = 0; i < 16 && i < payload_len; i++) {
                    printf("%02x ", payload[i]);
                }
                printf("\n");
            }

            // 计算新的校验和
            icmp_reply->icmp_sum = internet_checksum((uint16_t *)reply_buf, icmp_len);

            printf("[net]   ICMP checksum: 0x%04x\n", ntohs(icmp_reply->icmp_sum));
            printf("[net]   Reply ICMP id=0x%04x, seq=%d\n",
                   ntohs(icmp_reply->icmp_id), ntohs(icmp_reply->icmp_seq));

            // 通过IP发送应答（使用网络字节序的 IP 地址）
            uint32_t src_ip = ntohl(ip->ip_src);  // 🔥 从 IP 头获取源 IP（网络字节序）
            printf("[net]   Sending reply to %d.%d.%d.%d (network: 0x%08x)\n",
                   (src_ip >> 24) & 0xFF, (src_ip >> 16) & 0xFF,
                   (src_ip >> 8) & 0xFF, src_ip & 0xFF, ip->ip_src);
            printf("[net] -> Calling ip_output (ICMP reply)\n");
            ip_output(dev, src_ip, IPPROTO_ICMP, reply_buf, icmp_len);

            // 🔥 暂时不释放 buffer（调试用）
            // for (volatile int i = 0; i < 10000; i++) {
            //     asm volatile("nop");
            // }
            // kfree(reply_buf);

            SET_COLOR_GREEN();
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
    printf("[net] -> Calling ip_output (ICMP echo request)\n");
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
    printf("[net] -> Calling ip_output (UDP)\n");
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
    printf("[net] -> Calling ip_output (TCP)\n");
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

    printf("[net] ARP: op=%d\n", ntohs(arp->arp_op));
    printf("[net]   Sender:    MAC=%02x:%02x:%02x:%02x:%02x:%02x, IP=%d.%d.%d.%d\n",
           arp->arp_sha[0], arp->arp_sha[1], arp->arp_sha[2],
           arp->arp_sha[3], arp->arp_sha[4], arp->arp_sha[5],
           (ntohl(arp->arp_spa) >> 24) & 0xFF, (ntohl(arp->arp_spa) >> 16) & 0xFF,
           (ntohl(arp->arp_spa) >> 8) & 0xFF, ntohl(arp->arp_spa) & 0xFF);
    printf("[net]   Target:    MAC=%02x:%02x:%02x:%02x:%02x:%02x, IP=%d.%d.%d.%d\n",
           arp->arp_tha[0], arp->arp_tha[1], arp->arp_tha[2],
           arp->arp_tha[3], arp->arp_tha[4], arp->arp_tha[5],
           (ntohl(arp->arp_tpa) >> 24) & 0xFF, (ntohl(arp->arp_tpa) >> 16) & 0xFF,
           (ntohl(arp->arp_tpa) >> 8) & 0xFF, ntohl(arp->arp_tpa) & 0xFF);

    // 检查硬件类型和协议类型
    if (ntohs(arp->arp_hrd) != ARPHRD_ETHER || ntohs(arp->arp_pro) != ETH_P_IP) {
        printf("[net] ARP: unsupported hardware or protocol\n");
        return -1;
    }

    // 处理ARP请求
    if (ntohs(arp->arp_op) == ARPOP_REQUEST) {
        // 🔥 检查是否询问我们的IP（使用全局 local_ip）
        extern uint32_t local_ip;
        uint32_t tpa = ntohl(arp->arp_tpa);  // 🔥 转换为主机字节序

        if (tpa == local_ip) {
            printf("[net] ARP request for our IP, sending reply\n");
            printf("[net]   Target IP in ARP: %d.%d.%d.%d\n",
                   (tpa >> 24) & 0xFF, (tpa >> 16) & 0xFF,
                   (tpa >> 8) & 0xFF, tpa & 0xFF);
            printf("[net]   Our local_ip: %d.%d.%d.%d\n",
                   (local_ip >> 24) & 0xFF, (local_ip >> 16) & 0xFF,
                   (local_ip >> 8) & 0xFF, local_ip & 0xFF);

            // 构造ARP应答
            arp_hdr_t arp_reply;
            arp_reply.arp_hrd = htons(ARPHRD_ETHER);
            arp_reply.arp_pro = htons(ETH_P_IP);
            arp_reply.arp_hln = ETH_ALEN;
            arp_reply.arp_pln = 4;
            arp_reply.arp_op = htons(ARPOP_REPLY);

            // 发送方MAC和IP（我们）- 使用全局 local_mac
            extern uint8_t local_mac[ETH_ALEN];
            memcpy(arp_reply.arp_sha, local_mac, ETH_ALEN);
            arp_reply.arp_spa = local_ip;

            // 目标MAC和IP（请求方）
            memcpy(arp_reply.arp_tha, arp->arp_sha, ETH_ALEN);
            arp_reply.arp_tpa = arp->arp_spa;

            // 发送ARP应答
            eth_send(dev, arp->arp_sha, ETH_P_ARP, (uint8_t *)&arp_reply, sizeof(arp_hdr_t));
            printf("[net] ARP reply sent to %02x:%02x:%02x:%02x:%02x:%02x\n",
                   arp->arp_sha[0], arp->arp_sha[1], arp->arp_sha[2],
                   arp->arp_sha[3], arp->arp_sha[4], arp->arp_sha[5]);

            // 同时更新ARP缓存（转换为主机字节序）
            arp_cache_update(ntohl(arp->arp_spa), arp->arp_sha);
        } else {
            printf("[net] ARP request NOT for us (tpa=%08x, local_ip=%08x)\n",
                   tpa, local_ip);
        }
    }
    // 处理ARP应答
    else if (ntohs(arp->arp_op) == ARPOP_REPLY) {
        printf("[net] ARP reply received\n");
        // 🔥 转换为主机字节序再存入缓存
        arp_cache_update(ntohl(arp->arp_spa), arp->arp_sha);
    }
    else {
        printf("[net] Unknown ARP operation: %d\n", ntohs(arp->arp_op));
    }

    return 0;
}

/**
 * @brief 处理 ARP Request（别人问我"谁是某 IP"）
 */
void arp_handle_request(net_device_t *dev, eth_hdr_t *eth, arp_hdr_t *arp) {
    extern uint32_t local_ip;

    // 🔍 调试：打印完整的 ARP request 包内容
    uint32_t spa = ntohl(arp->arp_spa);
    uint32_t tpa = ntohl(arp->arp_tpa);
    printf("[arp] request: spa=%d.%d.%d.%d, tpa=%d.%d.%d.%d, local_ip=%d.%d.%d.%d\n",
           (spa >> 24) & 0xFF, (spa >> 16) & 0xFF,
           (spa >> 8) & 0xFF, spa & 0xFF,
           (tpa >> 24) & 0xFF, (tpa >> 16) & 0xFF,
           (tpa >> 8) & 0xFF, tpa & 0xFF,
           (local_ip >> 24) & 0xFF, (local_ip >> 16) & 0xFF,
           (local_ip >> 8) & 0xFF, local_ip & 0xFF);

    // 检查是否是给我们的
    if (tpa != local_ip) {
        printf("[arp] request: not for us (tpa != local_ip)\n");
        return;
    }

    extern uint8_t local_mac[ETH_ALEN];
    printf("[arp] REPLY from %d.%d.%d.%d (%02x:%02x:%02x:%02x:%02x:%02x) to %d.%d.%d.%d\n",
           (local_ip >> 24) & 0xFF, (local_ip >> 16) & 0xFF,
           (local_ip >> 8) & 0xFF, local_ip & 0xFF,
           local_mac[0], local_mac[1], local_mac[2],
           local_mac[3], local_mac[4], local_mac[5],
           (spa >> 24) & 0xFF, (spa >> 16) & 0xFF,
           (spa >> 8) & 0xFF, spa & 0xFF);

    uint8_t buf[64];

    eth_hdr_t *reth = (eth_hdr_t *)buf;
    arp_hdr_t *rarp = (arp_hdr_t *)(buf + sizeof(eth_hdr_t));

    // Ethernet header
    memcpy(reth->eth_dst, arp->arp_sha, 6);   // 对方 MAC
    extern uint8_t local_mac[ETH_ALEN];
    memcpy(reth->eth_src, local_mac, 6);      // 我 MAC
    reth->eth_type = htons(ETH_P_ARP);

    // ARP header
    rarp->arp_hrd = htons(1);
    rarp->arp_pro = htons(ETH_P_IP);
    rarp->arp_hln = 6;
    rarp->arp_pln = 4;
    rarp->arp_op = htons(2); // reply

    memcpy(rarp->arp_sha, local_mac, 6);
    rarp->arp_spa = htonl(local_ip);        // 🔥 转换为网络字节序

    memcpy(rarp->arp_tha, arp->arp_sha, 6);
    rarp->arp_tpa = arp->arp_spa;            // ✅ 已经是网络字节序，直接复制

    // 发送
    dev->send(dev, buf, sizeof(eth_hdr_t) + sizeof(arp_hdr_t));

    printf("[arp] reply sent to %02x:%02x:%02x:%02x:%02x:%02x\n",
           arp->arp_sha[0], arp->arp_sha[1], arp->arp_sha[2],
           arp->arp_sha[3], arp->arp_sha[4], arp->arp_sha[5]);
}

/**
 * @brief ARP cache 查找
 * @return MAC 地址指针，如果未找到返回 NULL
 */
uint8_t *arp_cache_lookup(uint32_t ip) {
    // 🔍 调试：打印查找的 IP（主机字节序）
    printf("[arp] lookup ip=%08x (%d.%d.%d.%d)\n",
           ip, (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
           (ip >> 8) & 0xFF, ip & 0xFF);

    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (arp_table[i].valid) {
            printf("[arp]   entry[%d]: ip=%08x -> %d.%d.%d.%d\n", i,
                   arp_table[i].ip,
                   (arp_table[i].ip >> 24) & 0xFF,
                   (arp_table[i].ip >> 16) & 0xFF,
                   (arp_table[i].ip >> 8) & 0xFF,
                   arp_table[i].ip & 0xFF);
        }
        if (arp_table[i].valid && arp_table[i].ip == ip) {
            printf("[arp] cache HIT!\n");
            return arp_table[i].mac;
        }
    }

    printf("[arp] cache MISS\n");
    return NULL;
}

/**
 * @brief 发送 ARP 请求（谁是 target_ip）
 */
void arp_send_request(net_device_t *dev, uint32_t target_ip) {
    uint8_t buf[64];
    memset(buf, 0, sizeof(buf));

    eth_hdr_t *eth = (eth_hdr_t *)buf;
    arp_hdr_t *arp = (arp_hdr_t *)(buf + sizeof(eth_hdr_t));

    // Ethernet header
    memset(eth->eth_dst, 0xFF, 6);          // broadcast
    extern uint8_t local_mac[ETH_ALEN];
    memcpy(eth->eth_src, local_mac, 6);
    eth->eth_type = htons(ETH_P_ARP);

    // ARP header
    arp->arp_hrd = htons(1);                  // Ethernet
    arp->arp_pro = htons(ETH_P_IP);           // IPv4
    arp->arp_hln = 6;
    arp->arp_pln = 4;
    arp->arp_op = htons(1);                  // request

    memcpy(arp->arp_sha, local_mac, 6);
    arp->arp_spa = htonl(local_ip);          // 🔥 转换为网络字节序

    memset(arp->arp_tha, 0x00, 6);
    arp->arp_tpa = htonl(target_ip);         // 🔥 转换为网络字节序

    dev->send(dev, buf, sizeof(eth_hdr_t) + sizeof(arp_hdr_t));

    printf("[arp] send request: who-has %d.%d.%d.%d\n",
           (target_ip >> 24) & 0xFF, (target_ip >> 16) & 0xFF,
           (target_ip >> 8) & 0xFF, target_ip & 0xFF);
}

/**
 * @brief 处理 ARP Reply（别人告诉我"某 IP 的 MAC 是多少"）
 */
void arp_handle_reply(net_device_t *dev, eth_hdr_t *eth, arp_hdr_t *arp) {
    extern uint32_t local_ip;

    // 🔥 验证：ARP reply 的目标必须是我们
    if (ntohl(arp->arp_tpa) != local_ip) {
        printf("[arp] reply not for us (tpa=%d.%d.%d.%d, local_ip=%d.%d.%d.%d)\n",
               (ntohl(arp->arp_tpa) >> 24) & 0xFF,
               (ntohl(arp->arp_tpa) >> 16) & 0xFF,
               (ntohl(arp->arp_tpa) >> 8) & 0xFF,
               ntohl(arp->arp_tpa) & 0xFF,
               (local_ip >> 24) & 0xFF,
               (local_ip >> 16) & 0xFF,
               (local_ip >> 8) & 0xFF,
               local_ip & 0xFF);
        return;
    }

    // 🔥 先转换为主机字节序
    uint32_t spa = ntohl(arp->arp_spa);

    printf("[arp] reply: %d.%d.%d.%d is at %02x:%02x:%02x:%02x:%02x:%02x\n",
           (spa >> 24) & 0xFF,
           (spa >> 16) & 0xFF,
           (spa >> 8) & 0xFF,
           spa & 0xFF,
           arp->arp_sha[0], arp->arp_sha[1], arp->arp_sha[2],
           arp->arp_sha[3], arp->arp_sha[4], arp->arp_sha[5]);

    // 🔥 使用主机字节序存入 ARP 表
    arp_cache_update(spa, arp->arp_sha);
}

/**
 * @brief 简化的 ARP 处理函数（专注于发送 Reply）
 */
void arp_handle(net_device_t *dev, uint8_t *data, uint32_t len) {
    if (len < sizeof(eth_hdr_t) + sizeof(arp_hdr_t)) {
        printf("[arp] packet too short\n");
        return;
    }

    eth_hdr_t *eth = (eth_hdr_t *)data;
    arp_hdr_t *arp = (arp_hdr_t *)(data + sizeof(eth_hdr_t));

    uint16_t oper = ntohs(arp->arp_op);

    // 分两种情况处理
    if (oper == 1) {
        // ARP request：别人问我
        arp_handle_request(dev, eth, arp);
    } else if (oper == 2) {
        // ARP reply：别人告诉我
        arp_handle_reply(dev, eth, arp);
    } else {
        printf("[arp] unknown oper=%d\n", oper);
    }
}

/**
 * @brief 更新ARP缓存
 */
static void arp_cache_update(uint32_t ip_addr, uint8_t *mac_addr) {
    // 🔍 调试：打印存入的 IP（主机字节序）
    printf("[arp] cache insert ip=%08x (%d.%d.%d.%d) -> %02x:%02x:%02x:%02x:%02x:%02x\n",
           ip_addr,
           (ip_addr >> 24) & 0xFF, (ip_addr >> 16) & 0xFF,
           (ip_addr >> 8) & 0xFF, ip_addr & 0xFF,
           mac_addr[0], mac_addr[1], mac_addr[2],
           mac_addr[3], mac_addr[4], mac_addr[5]);

    // 查找空闲位置或更新已存在项
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (!arp_table[i].valid || arp_table[i].ip == ip_addr) {
            arp_table[i].ip = ip_addr;
            memcpy(arp_table[i].mac, mac_addr, 6);
            arp_table[i].valid = 1;
            return;
        }
    }
    printf("[arp] cache full!\n");
}

/**
 * @brief 发送 ARP 请求（广播到局域网）
 * @param dev 网络设备
 * @param ip_addr 要查询的 IP 地址
 * @return 0 成功，-1 失败
 */
static int arp_request_broadcast(net_device_t *dev, uint32_t ip_addr) {
    if (!dev) {
        printf("[arp] ERROR: Device is NULL\n");
        return -1;
    }

    // 构造 ARP 请求包
    uint8_t packet[ETH_HDR_LEN + ARP_HDR_LEN];
    eth_hdr_t *eth = (eth_hdr_t *)packet;
    arp_hdr_t *arp = (arp_hdr_t *)(packet + ETH_HDR_LEN);

    // 以太网头部
    memset(eth->eth_dst, 0xFF, ETH_ALEN);  // 广播 MAC
    memcpy(eth->eth_src, dev->mac_addr, ETH_ALEN);
    eth->eth_type = htons(ETH_P_ARP);

    // ARP 头部
    arp->arp_hrd = htons(ARPHRD_ETHER);
    arp->arp_pro = htons(ETH_P_IP);
    arp->arp_hln = ETH_ALEN;
    arp->arp_pln = 4;
    arp->arp_op = htons(ARPOP_REQUEST);
    memcpy(arp->arp_sha, dev->mac_addr, ETH_ALEN);
    arp->arp_spa = htonl(dev->ip_addr);
    memset(arp->arp_tha, 0, ETH_ALEN);
    arp->arp_tpa = htonl(ip_addr);

    // 🔍 调试：打印发送的 ARP 包内容（网络字节序）
    printf("[arp] send arp req spa=%08x tpa=%08x\n",
           arp->arp_spa, arp->arp_tpa);
    printf("[arp]     (spa=%d.%d.%d.%d, tpa=%d.%d.%d.%d)\n",
           (ntohl(arp->arp_spa) >> 24) & 0xFF,
           (ntohl(arp->arp_spa) >> 16) & 0xFF,
           (ntohl(arp->arp_spa) >> 8) & 0xFF,
           ntohl(arp->arp_spa) & 0xFF,
           (ntohl(arp->arp_tpa) >> 24) & 0xFF,
           (ntohl(arp->arp_tpa) >> 16) & 0xFF,
           (ntohl(arp->arp_tpa) >> 8) & 0xFF,
           ntohl(arp->arp_tpa) & 0xFF);

    return dev->send(dev, packet, sizeof(packet));
}

/**
 * @brief 显示 ARP 缓存表并扫描局域网（类似 arp -a）
 * @param dev 网络设备
 * @param scan_and_update 是否发送 ARP 请求来更新缓存
 */
void arp_show_cache(net_device_t *dev, int scan_and_update) {
    printf("\n=== ARP Cache (%s) ===\n", dev ? dev->name : "NULL");

    if (!dev) {
        printf("[arp] ERROR: Device is NULL\n");
        return;
    }

    // 如果需要扫描并更新
    if (scan_and_update) {
        printf("Scanning local network for active hosts...\n");

        {
            uint32_t netmask = dev->netmask;
            uint32_t base_ip = dev->ip_addr & netmask;

            // 扫描本地网段的前 30 个主机
            printf("Scanning %d.%d.%d.%d/%d...\n",
                   (base_ip >> 24) & 0xFF,
                   (base_ip >> 16) & 0xFF,
                   (base_ip >> 8) & 0xFF,
                   (base_ip) & 0xFF,
                   32 - __builtin_ctz(netmask));

            // 扫描整个子网（除了网络地址和广播地址）
            for (uint32_t i = 145; i < 146 && i < (0xFFFFFFFF - base_ip); i++) {
                uint32_t target_ip = base_ip + i;

                // 跳过本机和广播地址
                if (target_ip == dev->ip_addr ||
                    target_ip == (base_ip | ~netmask)) {
                    continue;
                }

                // 发送 ARP 请求
                arp_request_broadcast(dev, target_ip);

                // 🔥 中断模式：等待中断处理回复
                // 短暂延迟让中断有时间处理
                for (volatile int j = 0; j < 50000; j++) {
                    asm volatile("nop");
                }
            }

            printf("Scan complete.\n\n");
        }
    }

    // 显示 ARP 缓存表
    printf("%-18s %-17s %s\n", "IP Address", "MAC Address", "Status");
    printf("----------------- ----------------- ------\n");

    int count = 0;
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (arp_table[i].valid) {
            printf("%d.%d.%d.%d    %02x:%02x:%02x:%02x:%02x:%02x  %s\n",
                   (arp_table[i].ip >> 24) & 0xFF,
                   (arp_table[i].ip >> 16) & 0xFF,
                   (arp_table[i].ip >> 8) & 0xFF,
                   arp_table[i].ip & 0xFF,
                   arp_table[i].mac[0],
                   arp_table[i].mac[1],
                   arp_table[i].mac[2],
                   arp_table[i].mac[3],
                   arp_table[i].mac[4],
                   arp_table[i].mac[5],
                   "VALID");
            count++;
        }
    }

    if (count == 0) {
        printf("(No entries)\n");
    }

    printf("----------------- ----------------- ------\n");
    printf("Total: %d entries\n\n", count);
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

    // 更新所有设备的网关
    for (int i = 0; i < num_devices; i++) {
        net_devices[i]->gateway = gw;
    }

    return 0;
}

/**
 * @brief 通用网络设备轮询接收
 * @param dev 网络设备指针
 */
void net_poll_rx(net_device_t *dev) {
    if (!dev) {
        printf("[net] ERROR: NULL device\n");
        return;
    }

    // 直接调用 E1000 的轮询函数
    extern void e1000_debug_poll_rx(void);
    e1000_debug_poll_rx();
    // extern void e1000_poll_rx(net_device_t *dev);
    // e1000_poll_rx(dev);
}

/**
 * @brief 转储��络设备的 RX 寄存器状态（用于诊断）
 * @param dev 网络设备指针
 */
void net_dump_rx_regs(net_device_t *dev) {
    if (!dev) {
        printf("[net] ERROR: NULL device\n");
        return;
    }

    // 直接调用 E1000 的 RX 寄存器转储函数
    extern void e1000_dump_rx_regs(void);
    

    // 🔥 添加网络统计信息
    printf("\n[net] ============== Network Statistics ==============\n");
    extern net_stats_t net_stats;
    printf("[net] RX packets: %d\n", net_stats.rx_packets);
    printf("[net] TX packets: %d\n", net_stats.tx_packets);
    printf("[net] RX bytes:   %d\n", net_stats.rx_bytes);
    printf("[net] TX bytes:   %d\n", net_stats.tx_bytes);
    printf("[net] RX errors:  %d\n", net_stats.rx_errors);
    printf("[net] TX errors:  %d\n", net_stats.tx_errors);
    printf("[net] RX dropped: %d\n", net_stats.rx_dropped);
    printf("[net] TX dropped: %d\n", net_stats.tx_dropped);
    printf("[net] ===============================================\n");

    // 🔥 添加 ARP 缓存表
    printf("\n[net] ============== ARP Cache Table ==============\n");
    printf("[net] %-18s %-17s %s\n", "IP Address", "MAC Address", "Status");
    printf("[net] ----------------- ----------------- ------\n");

    extern arp_entry_t arp_table[];
    int count = 0;
    for (int i = 0; i < 32; i++) {  // ARP_TABLE_SIZE
        if (arp_table[i].valid) {
            printf("[net] %d.%d.%d.%d    %02x:%02x:%02x:%02x:%02x:%02x  VALID\n",
                   (arp_table[i].ip >> 24) & 0xFF,
                   (arp_table[i].ip >> 16) & 0xFF,
                   (arp_table[i].ip >> 8) & 0xFF,
                   arp_table[i].ip & 0xFF,
                   arp_table[i].mac[0], arp_table[i].mac[1],
                   arp_table[i].mac[2], arp_table[i].mac[3],
                   arp_table[i].mac[4], arp_table[i].mac[5]);
            count++;
        }
    }

    if (count == 0) {
        printf("[net] (No entries)\n");
    }

    printf("[net] ----------------- ----------------- ------\n");
    printf("[net] Total: %d entries\n", count);
    printf("[net] ===========================================\n\n");
    e1000_dump_rx_regs();
}
