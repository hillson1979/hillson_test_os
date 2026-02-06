/**
 * @file ieee80211.c
 * @brief 802.11 WiFi 帧处理实现
 */

#include "types.h"
#include "net.h"
#include "net/wifi/atheros.h"
#include "string.h"
#include "printf.h"

// ==================== 辅助函数 ====================

/**
 * @brief 从 802.11 帧中获取指定元素
 */
uint8_t *ieee80211_get_elem(uint8_t *data, uint32_t len, uint8_t elem_id) {
    uint32_t pos = 0;

    // 跳过管理帧头部 (24 字节)
    pos += 24;

    // 遍历元素
    while (pos + 2 <= len) {
        ieee80211_elem_t *elem = (ieee80211_elem_t *)(data + pos);

        if (elem->id == elem_id) {
            return (uint8_t *)elem;
        }

        pos += 2 + elem->len;
    }

    return NULL;
}

/**
 * @brief 打印 802.11 帧头部信息
 */
void print_ieee80211_hdr(uint8_t *data) {
    ieee80211_hdr_t *hdr = (ieee80211_hdr_t *)data;
    uint16_t fc = hdr->frame_control;
    uint8_t type = (fc & IEEE80211_FCTL_FTYPE) >> 2;
    uint8_t subtype = (fc & IEEE80211_FCTL_STYPE) >> 4;

    printf("[802.11] FC=0x%04X, Type=%d, Subtype=%d\n", fc, type, subtype);
    printf("[802.11] addr1=%02x:%02x:%02x:%02x:%02x:%02x\n",
           hdr->addr1[0], hdr->addr1[1], hdr->addr1[2],
           hdr->addr1[3], hdr->addr1[4], hdr->addr1[5]);
    printf("[802.11] addr2=%02x:%02x:%02x:%02x:%02x:%02x\n",
           hdr->addr2[0], hdr->addr2[1], hdr->addr2[2],
           hdr->addr2[3], hdr->addr2[4], hdr->addr2[5]);
    printf("[802.11] addr3=%02x:%02x:%02x:%02x:%02x:%02x\n",
           hdr->addr3[0], hdr->addr3[1], hdr->addr3[2],
           hdr->addr3[3], hdr->addr3[4], hdr->addr3[5]);
}

// ==================== 发送 802.11 管理帧 ====================

/**
 * @brief 发送 Probe Request 帧
 */
int ieee80211_send_probe_req(net_device_t *dev) {
    if (!dev) return -1;

    atheros_priv_t *priv = (atheros_priv_t *)dev->priv;

    printf("[802.11] Sending Probe Request\n");

    // 使用栈上缓冲区（足够小）
    uint8_t frame[256];
    uint32_t pos = 0;

    // 构造管理帧头部
    ieee80211_mgmt_hdr_t *mgmt = (ieee80211_mgmt_hdr_t *)frame;
    mgmt->frame_control = IEEE80211_FTYPE_MGMT << 2 | IEEE80211_STYPE_PROBE_REQ << 4;
    mgmt->duration_id = 0;
    memset(mgmt->addr1, 0xFF, ETH_ALEN);  // 广播
    memcpy(mgmt->addr2, dev->mac_addr, ETH_ALEN);  // 本机 MAC
    memset(mgmt->addr3, 0xFF, ETH_ALEN);  // BSSID (广播)
    mgmt->seq_ctrl = 0;
    pos += 24;

    // 添加 SSID 元素 (空 SSID = 通配符)
    frame[pos++] = IEEE80211_ELEM_SSID;
    frame[pos++] = 0;  // 空 SSID

    // 添加支持速率元素
    frame[pos++] = IEEE80211_ELEM_SUPP_RATES;
    frame[pos++] = 8;  // 8 个速率
    frame[pos++] = 0x82;  // 1 Mbps
    frame[pos++] = 0x84;  // 2 Mbps
    frame[pos++] = 0x8B;  // 5.5 Mbps
    frame[pos++] = 0x96;  // 11 Mbps
    frame[pos++] = 0x0C;  // 6 Mbps
    frame[pos++] = 0x12;  // 9 Mbps
    frame[pos++] = 0x18;  // 12 Mbps
    frame[pos++] = 0x24;  // 18 Mbps

    // 发送帧
    int ret = dev->send(dev, frame, pos);

    if (ret < 0) {
        printf("[802.11] Failed to send Probe Request\n");
        return -1;
    }

    printf("[802.11] Probe Request sent (%d bytes)\n", pos);
    return 0;
}

/**
 * @brief 发送 Authentication 帧
 */
int ieee80211_send_auth(net_device_t *dev, uint8_t *bssid) {
    if (!dev || !bssid) return -1;

    printf("[802.11] Sending Authentication frame\n");

    uint8_t frame[256];
    uint32_t pos = 0;

    // 构造管理帧头部
    ieee80211_mgmt_hdr_t *mgmt = (ieee80211_mgmt_hdr_t *)frame;
    mgmt->frame_control = IEEE80211_FTYPE_MGMT << 2 | IEEE80211_STYPE_AUTH << 4;
    mgmt->duration_id = 0;
    memcpy(mgmt->addr1, bssid, ETH_ALEN);  // AP MAC
    memcpy(mgmt->addr2, dev->mac_addr, ETH_ALEN);  // 本机 MAC
    memcpy(mgmt->addr3, bssid, ETH_ALEN);  // BSSID
    mgmt->seq_ctrl = 0;
    pos += 24;

    // 添加认证信息
    ieee80211_auth_t *auth = (ieee80211_auth_t *)(frame + pos);
    auth->auth_alg = htons(1);  // Open System
    auth->auth_transaction = htons(1);  // 第 1 个事务
    auth->status_code = 0;
    pos += sizeof(ieee80211_auth_t);

    // 发送帧
    int ret = dev->send(dev, frame, pos);

    if (ret < 0) {
        printf("[802.11] Failed to send Auth frame\n");
        return -1;
    }

    printf("[802.11] Auth frame sent (%d bytes)\n", pos);
    return 0;
}

/**
 * @brief 发送 Association Request 帧
 */
int ieee80211_send_assoc_req(net_device_t *dev, uint8_t *bssid, const char *ssid) {
    if (!dev || !bssid || !ssid) return -1;

    printf("[802.11] Sending Association Request to %s\n", ssid);

    uint8_t frame[256];
    uint32_t pos = 0;

    // 构造管理帧头部
    ieee80211_mgmt_hdr_t *mgmt = (ieee80211_mgmt_hdr_t *)frame;
    mgmt->frame_control = IEEE80211_FTYPE_MGMT << 2 | IEEE80211_STYPE_ASSOC_REQ << 4;
    mgmt->duration_id = 0;
    memcpy(mgmt->addr1, bssid, ETH_ALEN);  // AP MAC
    memcpy(mgmt->addr2, dev->mac_addr, ETH_ALEN);  // 本机 MAC
    memcpy(mgmt->addr3, bssid, ETH_ALEN);  // BSSID
    mgmt->seq_ctrl = 0;
    pos += 24;

    // Capability 信息
    uint16_t capability = IEEE80211_CAPINFO_ESS | IEEE80211_CAPINFO_SHORT_PREAMBLE;
    *(uint16_t *)(frame + pos) = htons(capability);
    pos += 2;

    // Listen interval
    *(uint16_t *)(frame + pos) = htons(10);
    pos += 2;

    // SSID 元素
    int ssid_len = strlen(ssid);
    frame[pos++] = IEEE80211_ELEM_SSID;
    frame[pos++] = ssid_len;
    memcpy(frame + pos, ssid, ssid_len);
    pos += ssid_len;

    // 支持速率元素
    frame[pos++] = IEEE80211_ELEM_SUPP_RATES;
    frame[pos++] = 8;
    frame[pos++] = 0x82;  // 1 Mbps
    frame[pos++] = 0x84;  // 2 Mbps
    frame[pos++] = 0x8B;  // 5.5 Mbps
    frame[pos++] = 0x96;  // 11 Mbps
    frame[pos++] = 0x0C;  // 6 Mbps
    frame[pos++] = 0x12;  // 9 Mbps
    frame[pos++] = 0x18;  // 12 Mbps
    frame[pos++] = 0x24;  // 18 Mbps

    // 发送帧
    int ret = dev->send(dev, frame, pos);

    if (ret < 0) {
        printf("[802.11] Failed to send Assoc Request\n");
        return -1;
    }

    printf("[802.11] Assoc Request sent (%d bytes)\n", pos);
    return 0;
}

// ==================== 接收 802.11 管理帧 ====================

/**
 * @brief 处理接收到的 Beacon 帧
 */
static int ieee80211_rx_beacon(net_device_t *dev, uint8_t *data, uint32_t len) {
    ieee80211_mgmt_hdr_t *mgmt = (ieee80211_mgmt_hdr_t *)data;
    uint32_t pos = 24;  // 跳过管理帧头部

    // 跳过 Beacon 固定部分
    pos += 12;  // timestamp (8) + beacon_interval (2) + capability (2)

    wifi_beacon_t beacon;
    memset(&beacon, 0, sizeof(beacon));

    memcpy(beacon.bssid, mgmt->addr3, ETH_ALEN);

    // 解析元素
    while (pos + 2 <= len) {
        ieee80211_elem_t *elem = (ieee80211_elem_t *)(data + pos);

        switch (elem->id) {
            case IEEE80211_ELEM_SSID:
                if (elem->len < 32) {
                    memcpy(beacon.ssid, elem + 1, elem->len);
                    beacon.ssid[elem->len] = '\0';
                }
                break;

            case IEEE80211_ELEM_DS_PARAMS:
                if (elem->len >= 1) {
                    beacon.channel = *((uint8_t *)(elem + 1));
                }
                break;

            case IEEE80211_ELEM_RSN:
                beacon.privacy = 1;
                break;
        }

        pos += 2 + elem->len;
    }

    // 打印 Beacon 信息
    printf("[802.11] Beacon: SSID=\"%s\", BSSID=%02x:%02x:%02x:%02x:%02x:%02x, CH=%d%s\n",
           beacon.ssid[0] ? beacon.ssid : "(hidden)",
           beacon.bssid[0], beacon.bssid[1], beacon.bssid[2],
           beacon.bssid[3], beacon.bssid[4], beacon.bssid[5],
           beacon.channel,
           beacon.privacy ? ", WPA2" : ", Open");

    return 0;
}

/**
 * @brief 处理接收到的 Probe Response 帧
 */
static int ieee80211_rx_probe_resp(net_device_t *dev, uint8_t *data, uint32_t len) {
    printf("[802.11] Received Probe Response\n");
    return ieee80211_rx_beacon(dev, data, len);  // 格式相同
}

/**
 * @brief 处理接收到的 Authentication 帧
 */
static int ieee80211_rx_auth(net_device_t *dev, uint8_t *data, uint32_t len) {
    ieee80211_mgmt_hdr_t *mgmt = (ieee80211_mgmt_hdr_t *)data;
    uint32_t pos = 24;
    ieee80211_auth_t *auth = (ieee80211_auth_t *)(data + pos);

    uint16_t transaction = ntohs(auth->auth_transaction);
    uint16_t status = ntohs(auth->status_code);

    printf("[802.11] Auth: Transaction=%d, Status=%d\n", transaction, status);

    if (status == 0 && transaction == 2) {
        printf("[802.11] Authentication successful\n");

        // 发送 Association Request
        atheros_priv_t *priv = (atheros_priv_t *)dev->priv;
        return ieee80211_send_assoc_req(dev, mgmt->addr3, priv->ssid);
    }

    return 0;
}

/**
 * @brief 处理接收到的 Association Response 帧
 */
static int ieee80211_rx_assoc_resp(net_device_t *dev, uint8_t *data, uint32_t len) {
    ieee80211_mgmt_hdr_t *mgmt = (ieee80211_mgmt_hdr_t *)data;
    uint32_t pos = 24;

    // Capability + Status Code + AID
    uint16_t capability = ntohs(*(uint16_t *)(data + pos));
    uint16_t status = ntohs(*(uint16_t *)(data + pos + 2));
    uint16_t aid = ntohs(*(uint16_t *)(data + pos + 4)) & 0x3FFF;

    printf("[802.11] Assoc Response: Status=%d, AID=%d\n", status, aid);

    if (status == 0) {
        printf("[802.11] Association successful!\n");
        atheros_priv_t *priv = (atheros_priv_t *)dev->priv;
        priv->connected = 1;
        memcpy(priv->ap_mac, mgmt->addr3, ETH_ALEN);
        printf("[802.11] Connected to AP: %02x:%02x:%02x:%02x:%02x:%02x\n",
               priv->ap_mac[0], priv->ap_mac[1], priv->ap_mac[2],
               priv->ap_mac[3], priv->ap_mac[4], priv->ap_mac[5]);
    }

    return 0;
}

/**
 * @brief 处理接收到的 802.11 管理帧
 */
int ieee80211_input_mgmt(net_device_t *dev, uint8_t *data, uint32_t len) {
    if (!dev || !data || len < 24) {
        return -1;
    }

    ieee80211_mgmt_hdr_t *mgmt = (ieee80211_mgmt_hdr_t *)data;
    uint16_t fc = mgmt->frame_control;
    uint8_t subtype = (fc & IEEE80211_FCTL_STYPE) >> 4;

    switch (subtype) {
        case IEEE80211_STYPE_BEACON:
            return ieee80211_rx_beacon(dev, data, len);

        case IEEE80211_STYPE_PROBE_RESP:
            return ieee80211_rx_probe_resp(dev, data, len);

        case IEEE80211_STYPE_AUTH:
            return ieee80211_rx_auth(dev, data, len);

        case IEEE80211_STYPE_ASSOC_RESP:
            return ieee80211_rx_assoc_resp(dev, data, len);

        default:
            printf("[802.11] Unknown mgmt frame: subtype=%d\n", subtype);
            break;
    }

    return 0;
}

/**
 * @brief 处理接收到的 802.11 数据帧
 */
static int ieee80211_rx_data(net_device_t *dev, uint8_t *data, uint32_t len) {
    printf("[802.11] Received data frame: %d bytes\n", len);
    // TODO: 将 802.11 数据帧转换为以太网帧并传递给网络协议栈
    return 0;
}

/**
 * @brief 处理输入的 802.11 帧
 */
int wifi_input_80211(net_device_t *dev, uint8_t *data, uint32_t len) {
    if (!dev || !data || len < sizeof(ieee80211_hdr_t)) {
        return -1;
    }

    ieee80211_hdr_t *hdr = (ieee80211_hdr_t *)data;
    uint16_t fc = hdr->frame_control;
    uint8_t type = (fc & IEEE80211_FCTL_FTYPE) >> 2;

    switch (type) {
        case IEEE80211_FTYPE_MGMT:
            return ieee80211_input_mgmt(dev, data, len);

        case IEEE80211_FTYPE_DATA:
            return ieee80211_rx_data(dev, data, len);

        default:
            printf("[802.11] Unknown frame type: %d\n", type);
            break;
    }

    return 0;
}

// ==================== WiFi 数据包处理 ====================

/**
 * @brief WiFi 发送数据包
 */
int wifi_tx_packet(net_device_t *dev, uint8_t *data, uint32_t len) {
    if (!dev || !data || len == 0) {
        return -1;
    }

    // 对于 WiFi，需要将以太网帧封装在 802.11 数据帧中
    // 这里简化处理，直接调用设备发送函数

    atheros_priv_t *priv = (atheros_priv_t *)dev->priv;

    if (!priv->connected) {
        printf("[wifi] Not connected, cannot send\n");
        return -1;
    }

    return dev->send(dev, data, len);
}

/**
 * @brief WiFi 接收数据包
 */
int wifi_rx_packet(net_device_t *dev, uint8_t *data, uint32_t len) {
    if (!dev || !data || len == 0) {
        return -1;
    }

    // 检查是否是 802.11 帧
    uint16_t fc = *(uint16_t *)data;
    uint8_t type = (fc & IEEE80211_FCTL_FTYPE) >> 2;

    if (type == IEEE80211_FTYPE_MGMT || type == IEEE80211_FTYPE_DATA) {
        return wifi_input_80211(dev, data, len);
    }

    // 否则假设是 802.3 以太网帧（经过转换）
    return eth_input(dev, data, len);
}
