/**
 * @file simulate.h
 * @brief WiFi 模拟响应接口
 */

#ifndef SIMULATE_H
#define SIMULATE_H

#include "net.h"

// 模拟 AP 响应
void wifi_simulate_beacon(net_device_t *dev, int channel);
void wifi_simulate_probe_resp(net_device_t *dev, int channel);
int wifi_simulate_auth_resp(net_device_t *dev, uint8_t *bssid);
int wifi_simulate_assoc_resp(net_device_t *dev, uint8_t *bssid);

#endif // SIMULATE_H
