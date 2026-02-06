/**
 * @file loopback.c
 * @brief 回环网络设备实现
 *
 * 用于测试网络协议栈，发送的数据包会立即被接收
 */

#include "net.h"
#include "../include/printf.h"
#include "../include/string.h"
#include "../include/kmalloc.h"

static net_device_t loopback_dev;
static uint8_t loopback_mac[ETH_ALEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

/**
 * @brief 回环设备发送函数（直接调用接收）
 */
static int loopback_send(net_device_t *dev, uint8_t *data, uint32_t len) {
    printf("[loopback] Sending %d bytes\n", len);

    // 分配缓冲区并复制数据
    uint8_t *buf = (uint8_t *)kmalloc(len);
    if (!buf) {
        printf("[loopback] Failed to allocate buffer\n");
        return -1;
    }

    memcpy(buf, data, len);

    // 直接调用接收函数
    int ret = net_rx_packet(dev, buf, len);

    kfree(buf);

    return ret;
}

/**
 * @brief 初始化回环设备
 */
int loopback_init(void) {
    printf("[loopback] Initializing loopback device...\n");

    memset(&loopback_dev, 0, sizeof(loopback_dev));

    strcpy(loopback_dev.name, "lo");
    memcpy(loopback_dev.mac_addr, loopback_mac, ETH_ALEN);
    loopback_dev.mtu = ETH_MTU;
    loopback_dev.send = loopback_send;
    loopback_dev.recv = NULL;
    loopback_dev.ioctl = NULL;
    loopback_dev.priv = NULL;

    // 注册设备
    if (net_device_register(&loopback_dev) < 0) {
        printf("[loopback] Failed to register device\n");
        return -1;
    }

    // 设置loopback设备的IP为127.0.0.1
    loopback_dev.ip_addr = 0x7F000001;  // 127.0.0.1
    loopback_dev.netmask = 0xFF000000;   // 255.0.0.0
    loopback_dev.gateway = 0;            // 无网关

    printf("[loopback] Loopback device ready (IP: 127.0.0.1)\n");
    return 0;
}

/**
 * @brief 发送测试数据包
 */
int loopback_send_test(void) {
    printf("[loopback] Sending test packet...\n");

    // 构造一个简单的以太网帧
    uint8_t frame[128];
    eth_hdr_t *eth = (eth_hdr_t *)frame;

    // 填充以太网头部
    memset(eth->eth_dst, 0xFF, ETH_ALEN);  // 广播地址
    memcpy(eth->eth_src, loopback_mac, ETH_ALEN);
    eth->eth_type = htons(ETH_P_IP);

    // 填充IP头部
    ip_hdr_t *ip = (ip_hdr_t *)(frame + ETH_HDR_LEN);
    memset(ip, 0, sizeof(ip_hdr_t));
    ip->ip_verhlen = 0x45;  // Version 4, Header 5
    ip->ip_len = htons(20 + 8);  // IP header + ICMP header
    ip->ip_ttl = IP_TTL;
    ip->ip_proto = IPPROTO_ICMP;
    ip->ip_src = htonl(0xC0A80101);  // 192.168.1.1
    ip->ip_dst = htonl(0xC0A80101);  // 192.168.1.1

    // 填充ICMP头部（Echo Request）
    icmp_hdr_t *icmp = (icmp_hdr_t *)(frame + ETH_HDR_LEN + 20);
    memset(icmp, 0, sizeof(icmp_hdr_t));
    icmp->icmp_type = ICMP_ECHO_REQUEST;
    icmp->icmp_code = 0;
    icmp->icmp_id = htonl(1);
    icmp->icmp_seq = htonl(1);

    // 填充一些数据
    uint8_t *payload = frame + ETH_HDR_LEN + 20 + 8;
    const char *test_data = "Hello from loopback!";
    int data_len = strlen(test_data);
    memcpy(payload, test_data, data_len);

    // 更新长度
    int total_len = ETH_HDR_LEN + 20 + 8 + data_len;

    printf("[loopback] Sending %d bytes test packet\n", total_len);

    return net_tx_packet(&loopback_dev, frame, total_len);
}
