/**
 * @file usbh_hid.h
 * @brief CherryUSB HID Class Driver — RT-Thread port to HillsonOS x86
 *
 * Original: CherryUSB class/hid/usbh_hid.h
 */

#ifndef CHERRYUSB_USBH_HID_H
#define CHERRYUSB_USBH_HID_H

#include <stdint.h>

/* HID class requests */
#define HID_REQUEST_GET_REPORT   0x01
#define HID_REQUEST_GET_IDLE     0x02
#define HID_REQUEST_GET_PROTOCOL 0x03
#define HID_REQUEST_SET_REPORT   0x09
#define HID_REQUEST_SET_IDLE     0x0A
#define HID_REQUEST_SET_PROTOCOL 0x0B

/* HID protocol codes */
#define HID_PROTOCOL_NONE     0
#define HID_PROTOCOL_KEYBOARD 1
#define HID_PROTOCOL_MOUSE    2

/* Mouse report (boot protocol, standard 3-4 bytes) */
typedef struct {
    uint8_t buttons;
    int8_t  x;
    int8_t  y;
    int8_t  wheel;
} usbh_hid_mouse_report_t;

/* HID device info */
typedef struct {
    uint8_t  dev_addr;
    uint8_t  iface;
    uint8_t  ep_in;
    uint8_t  ep_in_max;
    uint8_t  ep_in_interval;
    uint8_t  protocol;       /* HID_PROTOCOL_MOUSE/KEYBOARD */
    uint8_t  ready;
    usbh_hid_mouse_report_t report;
} usbh_hid_device_t;

#define USBH_HID_MAX_DEVICES 4

/* ---- CherryUSB-style API ---- */

/**
 * Register a HID device with the CherryUSB HID class driver.
 * Called after USB enumeration finds a HID interface.
 */
int usbh_hid_register(uint8_t dev_addr, uint8_t iface,
                       uint8_t ep_in, uint8_t ep_max, uint8_t interval,
                       uint8_t protocol);

/**
 * Read HID report (interrupt IN transfer).
 * Returns bytes read or -1 on error.
 */
int usbh_hid_read(int idx, void *report, int max_len);

/**
 * Set boot protocol (0=boot, 1=report) — CherryUSB pattern.
 */
int usbh_hid_set_protocol(uint8_t dev_addr, uint8_t iface, uint8_t proto);

/**
 * Set idle rate — CherryUSB pattern.
 */
int usbh_hid_set_idle(uint8_t dev_addr, uint8_t iface, uint8_t duration);

int usbh_hid_count(void);

#endif /* CHERRYUSB_USBH_HID_H */
