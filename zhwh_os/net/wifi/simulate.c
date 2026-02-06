/**
 * @file simulate.c
 * @brief WiFi 模拟响应（用于测试）
 *
 * 在没有真实 AP 的情况下，模拟 AP 发送 Beacon 和 Probe Response
 */

#include "types.h"
#include "net.h"
#include "net/wifi/atheros.h"
#include "string.h"
#include "printf.h"

// 模拟的 AP 列表（包括你的真实网络）
typedef struct {
    char ssid[32];
    uint8_t bssid[ETH_ALEN];
    uint8_t channel;
    uint8_t signal;
    uint8_t privacy;
} simulated_ap_t;

static const simulated_ap_t simulated_aps[] = {
    {
        .ssid = "CMCC-ZH",  // 你的真实 WiFi 网络
        .bssid = {0xAC, 0x23, 0x3F, 0xA5, 0x12, 0x88},
        .channel = 6,
        .signal = 92,
        .privacy = 1
    },
    {
        .ssid = "vivo Y50 5G",
        .bssid = {0x10, 0x5B, 0xAD, 0x47, 0xD4, 0xE3},
        .channel = 1,
        .signal = 85,
        .privacy = 1
    },
    {
        .ssid = "ChinaNet-xxxx",
        .bssid = {0xD8, 0x07, 0xB6, 0xC2, 0xA1, 0x4F},
        .channel = 11,
        .signal = 70,
        .privacy = 1
    },
    {
        .ssid = "TP-LINK_2.4G",
        .bssid = {0xC8, 0x3A, 0x35, 0x12, 0x44, 0x56},
        .channel = 6,
        .signal = 65,
        .privacy = 1
    }
};

/**
 * @brief 模拟发送 Beacon 帧
 */
void wifi_simulate_beacon(net_device_t *dev, int channel) {
    atheros_priv_t *priv = (atheros_priv_t *)dev->priv;

    // 查找当前信道的 AP
    for (int i = 0; i < sizeof(simulated_aps) / sizeof(simulated_aps[0]); i++) {
        const simulated_ap_t *ap = &simulated_aps[i];

        if (ap->channel == channel) {
            // 构造 Beacon 帧
            uint8_t beacon[256];
            uint32_t pos = 0;

            // 802.11 头部
            ieee80211_mgmt_hdr_t *mgmt = (ieee80211_mgmt_hdr_t *)beacon;
            mgmt->frame_control = IEEE80211_FTYPE_MGMT << 2 | IEEE80211_STYPE_BEACON << 4;
            mgmt->duration_id = 0;
            memcpy(mgmt->addr1, dev->mac_addr, ETH_ALEN);  // 目标（本机）
            memcpy(mgmt->addr2, ap->bssid, ETH_ALEN);       // 源（AP）
            memcpy(mgmt->addr3, ap->bssid, ETH_ALEN);       // BSSID
            mgmt->seq_ctrl = 0;
            pos += 24;

            // Beacon 固定部分
            uint64_t timestamp = 0x123456789ABC;
            uint16_t beacon_interval = htons(100);
            uint16_t capability = htons(IEEE80211_CAPINFO_ESS | IEEE80211_CAPINFO_PRIVACY);

            memcpy(beacon + pos, &timestamp, 8);
            pos += 8;
            memcpy(beacon + pos, &beacon_interval, 2);
            pos += 2;
            memcpy(beacon + pos, &capability, 2);
            pos += 2;

            // SSID 元素
            int ssid_len = strlen(ap->ssid);
            beacon[pos++] = IEEE80211_ELEM_SSID;
            beacon[pos++] = ssid_len;
            memcpy(beacon + pos, ap->ssid, ssid_len);
            pos += ssid_len;

            // 支持速率元素
            beacon[pos++] = IEEE80211_ELEM_SUPP_RATES;
            beacon[pos++] = 8;
            beacon[pos++] = 0x82;  // 1 Mbps
            beacon[pos++] = 0x84;  // 2 Mbps
            beacon[pos++] = 0x8B;  // 5.5 Mbps
            beacon[pos++] = 0x96;  // 11 Mbps
            beacon[pos++] = 0x0C;  // 6 Mbps
            beacon[pos++] = 0x12;  // 9 Mbps
            beacon[pos++] = 0x18;  // 12 Mbps
            beacon[pos++] = 0x24;  // 18 Mbps

            // 信道元素
            beacon[pos++] = IEEE80211_ELEM_DS_PARAMS;
            beacon[pos++] = 1;
            beacon[pos++] = ap->channel;

            // RSN 元素（WPA2）
            if (ap->privacy) {
                beacon[pos++] = IEEE80211_ELEM_RSN;
                beacon[pos++] = 20;  // 简化的 RSN
                memset(beacon + pos, 0x01, 20);  // 填充
                pos += 20;
            }

            // 调用处理函数
            printf("[802.11] Simulated Beacon received on channel %d\n", channel);
            wifi_input_80211(dev, beacon, pos);

            break;
        }
    }
}

/**
 * @brief 模拟发送 Probe Response
 */
void wifi_simulate_probe_resp(net_device_t *dev, int channel) {
    // 复用 Beacon 生成逻辑，因为格式相似
    wifi_simulate_beacon(dev, channel);
}

/**
 * @brief 模拟发送 Authentication Response
 */
int wifi_simulate_auth_resp(net_device_t *dev, uint8_t *bssid) {
    uint8_t auth[64];
    uint32_t pos = 0;

    // 802.11 头部
    ieee80211_mgmt_hdr_t *mgmt = (ieee80211_mgmt_hdr_t *)auth;
    mgmt->frame_control = IEEE80211_FTYPE_MGMT << 2 | IEEE80211_STYPE_AUTH << 4;
    mgmt->duration_id = 0;
    memcpy(mgmt->addr1, dev->mac_addr, ETH_ALEN);
    memcpy(mgmt->addr2, bssid, ETH_ALEN);
    memcpy(mgmt->addr3, bssid, ETH_ALEN);
    mgmt->seq_ctrl = 0;
    pos += 24;

    // 认证信息
    ieee80211_auth_t *auth_data = (ieee80211_auth_t *)(auth + pos);
    auth_data->auth_alg = htons(1);  // Open System
    auth_data->auth_transaction = htons(2);  // Response
    auth_data->status_code = htons(0);  // Success
    pos += sizeof(ieee80211_auth_t);

    printf("[802.11] Simulated Auth Response received\n");
    return wifi_input_80211(dev, auth, pos);
}

/**
 * @brief 模拟发送 Association Response
 */
int wifi_simulate_assoc_resp(net_device_t *dev, uint8_t *bssid) {
    uint8_t assoc[128];
    uint32_t pos = 0;

    // 802.11 头部
    ieee80211_mgmt_hdr_t *mgmt = (ieee80211_mgmt_hdr_t *)assoc;
    mgmt->frame_control = IEEE80211_FTYPE_MGMT << 2 | IEEE80211_STYPE_ASSOC_RESP << 4;
    mgmt->duration_id = 0;
    memcpy(mgmt->addr1, dev->mac_addr, ETH_ALEN);
    memcpy(mgmt->addr2, bssid, ETH_ALEN);
    memcpy(mgmt->addr3, bssid, ETH_ALEN);
    mgmt->seq_ctrl = 0;
    pos += 24;

    // Capability + Status + AID
    *(uint16_t *)(assoc + pos) = htons(IEEE80211_CAPINFO_ESS);
    pos += 2;
    *(uint16_t *)(assoc + pos) = htons(0);  // Success
    pos += 2;
    *(uint16_t *)(assoc + pos) = htons(1001);  // AID
    pos += 2;

    // 支持速率
    assoc[pos++] = IEEE80211_ELEM_SUPP_RATES;
    assoc[pos++] = 8;
    assoc[pos++] = 0x82;  // 1 Mbps
    assoc[pos++] = 0x84;  // 2 Mbps
    assoc[pos++] = 0x8B;  // 5.5 Mbps
    assoc[pos++] = 0x96;  // 11 Mbps
    assoc[pos++] = 0x0C;  // 6 Mbps
    assoc[pos++] = 0x12;  // 9 Mbps
    assoc[pos++] = 0x18;  // 12 Mbps
    assoc[pos++] = 0x24;  // 18 Mbps

    printf("[802.11] Simulated Assoc Response received\n");
    return wifi_input_80211(dev, assoc, pos);
}
