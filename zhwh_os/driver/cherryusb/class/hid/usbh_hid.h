#ifndef CHERRYUSB_USBH_HID_H
#define CHERRYUSB_USBH_HID_H
#include <stdint.h>
#define HID_REQUEST_SET_PROTOCOL 0x0B
#define HID_REQUEST_SET_IDLE     0x0A
#define HID_PROTOCOL_MOUSE       2
#define USBH_HID_MAX_DEVICES 4
typedef struct {
    uint8_t dev_addr, iface, ep_in, ep_in_max, ep_in_interval;
    uint8_t protocol, ready;
} usbh_hid_device_t;
int usbh_hid_register(uint8_t addr, uint8_t iface, uint8_t ep, uint8_t max, uint8_t interval, uint8_t proto);
int usbh_hid_read(int idx, void *rpt, int max);
int usbh_hid_count(void);
#endif
