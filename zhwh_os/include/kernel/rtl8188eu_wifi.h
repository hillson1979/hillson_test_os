#ifndef KERNEL_RTL8188EU_WIFI_H
#define KERNEL_RTL8188EU_WIFI_H

#include <stdint.h>
#include <stddef.h>

struct usb_dev_t;
struct usb_interface_t;

#define RTL8188EU_USB_VENDOR_ID  0x0BDA
#define RTL8188EU_USB_PRODUCT_ID 0x8179

enum rtl8188eu_wifi_state {
    RTL8188EU_WIFI_ABSENT = 0,
    RTL8188EU_WIFI_FIRMWARE_LOADING,
    RTL8188EU_WIFI_READY,
    RTL8188EU_WIFI_SCANNING,
    RTL8188EU_WIFI_TARGET_FOUND,
    RTL8188EU_WIFI_AUTHENTICATING,
    RTL8188EU_WIFI_ASSOCIATING,
    RTL8188EU_WIFI_WPA2_HANDSHAKE,
    RTL8188EU_WIFI_CONNECTED,
    RTL8188EU_WIFI_FAILED
};

int rtl8188eu_usb_match(const struct usb_dev_t *usb);
int rtl8188eu_usb_probe(struct usb_interface_t *iface);
void rtl8188eu_usb_remove(struct usb_dev_t *usb);

/* Firmware upload is enabled after the page-select/start sequence is added. */
int rtl8188eu_load_firmware(const uint8_t *firmware, size_t length);
int rtl8188eu_wifi_scan(void);
int rtl8188eu_wifi_connect(const char *ssid, const char *password);
int rtl8188eu_wifi_disconnect(void);
int rtl8188eu_wifi_status(void);
void rtl8188eu_wifi_poll(void);

#endif
