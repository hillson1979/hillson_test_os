/**
 * @file cherryusb_hid_mouse.c
 * @brief CherryUSB-style HID Mouse Driver for HillsonOS x86
 *
 * Architecture (CherryUSB layered model):
 *   Layer 1: USB Core    — control/interrupt transfers (usb_hcd.h)
 *   Layer 2: HID Class   — SET_PROTOCOL, SET_IDLE, GET_REPORT (this file)
 *   Layer 3: Mouse App   — report parsing, callback dispatch (this file)
 *
 * Ported from CherryUSB class/hid/hid_mouse.c patterns:
 *   - init(): discover → configure → start polling
 *   - deinit(): stop polling → release resources
 *   - read(): interrupt IN transfer → parse report → callback
 */

#include <stdint.h>
#include "string.h"
#include "printf.h"
#include "usb.h"
#include "usb_hcd.h"
#include "cherryusb_hid_mouse.h"

/* ================================================================
 * HID Class Requests (CherryUSB usbd_hid.c equivalents)
 * ================================================================ */

#define HID_REQ_SET_PROTOCOL   0x0B
#define HID_REQ_SET_IDLE       0x0A
#define HID_REQ_GET_REPORT     0x01

/* ================================================================
 * Mouse device table
 * ================================================================ */

static cherryusb_mouse_dev_t g_mice[CHERRYUSB_MAX_MICE];
static int g_mouse_count = 0;

/* ================================================================
 * CherryUSB Layer 2: HID Class Operations
 * ================================================================ */

/**
 * Set HID protocol (boot=0, report=1)
 * Equivalent to: CherryUSB usbd_hid_set_protocol()
 */
static int hid_set_protocol(int ctrl_id, uint8_t dev_addr, uint8_t iface, uint16_t proto) {
    usb_device_request_t req;
    req.bmRequestType = USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE;
    req.bRequest      = HID_REQ_SET_PROTOCOL;
    req.wValue        = proto;
    req.wIndex        = iface;
    req.wLength       = 0;
    return usb_control_transfer(ctrl_id, dev_addr, 0, &req, NULL);
}

/**
 * Set HID idle rate
 * Equivalent to: CherryUSB usbd_hid_set_idle()
 */
static int hid_set_idle(int ctrl_id, uint8_t dev_addr, uint8_t iface,
                         uint8_t duration, uint8_t report_id) {
    usb_device_request_t req;
    req.bmRequestType = USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE;
    req.bRequest      = HID_REQ_SET_IDLE;
    req.wValue        = ((uint16_t)duration << 8) | report_id;
    req.wIndex        = iface;
    req.wLength       = 0;
    return usb_control_transfer(ctrl_id, dev_addr, 0, &req, NULL);
}

/* ================================================================
 * CherryUSB Layer 3: Mouse Driver
 * ================================================================ */

int cherryusb_hid_mouse_init(int controller_id, uint8_t dev_addr, uint8_t interface,
                              uint8_t ep_in, uint8_t ep_max, uint8_t interval) {
    if (g_mouse_count >= CHERRYUSB_MAX_MICE) {
        printf("[CherryUSB] Max mice reached (%d)\n", CHERRYUSB_MAX_MICE);
        return -1;
    }

    int idx = g_mouse_count;
    cherryusb_mouse_dev_t *dev = &g_mice[idx];

    /* Fill device info */
    dev->dev_addr       = dev_addr;
    dev->interface      = interface;
    dev->ep_in          = ep_in;
    dev->ep_in_max      = ep_max;
    dev->ep_in_interval = interval;
    dev->controller_id  = controller_id;
    dev->protocol       = 0;  /* Boot protocol */
    dev->cb             = NULL;
    dev->ready          = 0;
    memset(&dev->last, 0, sizeof(dev->last));

    printf("[CherryUSB] Init mouse %d: addr=%d ep=0x%02x max=%d interval=%dms\n",
           idx, dev_addr, ep_in, ep_max, interval);

    /* Step 1: Set boot protocol (CherryUSB pattern) */
    int ret = hid_set_protocol(controller_id, dev_addr, interface, 0);
    if (ret < 0) {
        printf("[CherryUSB] SET_PROTOCOL failed: %d\n", ret);
        return -1;
    }
    printf("[CherryUSB] Boot protocol set\n");

    /* Step 2: Set idle rate (CherryUSB pattern) */
    ret = hid_set_idle(controller_id, dev_addr, interface, 0, 0);
    if (ret < 0) {
        printf("[CherryUSB] SET_IDLE failed (non-fatal): %d\n", ret);
    }

    /* Step 3: Mark ready */
    dev->ready = 1;
    g_mouse_count++;

    printf("[CherryUSB] Mouse %d ready\n", idx);
    return idx;
}

int cherryusb_hid_mouse_read(int idx, cherryusb_mouse_report_t *rpt) {
    if (idx < 0 || idx >= g_mouse_count) return -1;
    cherryusb_mouse_dev_t *dev = &g_mice[idx];
    if (!dev->ready) return -1;

    uint8_t buf[8];
    memset(buf, 0, sizeof(buf));

    /* CherryUSB pattern: interrupt IN transfer */
    extern int usb_interrupt_transfer(int ctrl_id, uint8_t dev_addr,
                                       uint8_t ep, void *data, int len);
    int n = usb_interrupt_transfer(dev->controller_id, dev->dev_addr,
                                    dev->ep_in & 0x7F, buf, dev->ep_in_max);
    if (n < 0) return n;

    /* Parse CherryUSB-style HID report */
    if (n >= 3) {
        rpt->buttons = buf[0];
        rpt->x       = (int8_t)buf[1];
        rpt->y       = (int8_t)buf[2];
        rpt->wheel   = (n >= 4) ? (int8_t)buf[3] : 0;
    }

    /* Update last report & fire callback */
    dev->last = *rpt;
    if (dev->cb && (rpt->x || rpt->y || rpt->wheel || rpt->buttons)) {
        dev->cb(rpt->x, rpt->y, rpt->buttons, rpt->wheel);
    }

    return n;
}

int cherryusb_hid_mouse_poll(int idx) {
    if (idx < 0 || idx >= g_mouse_count) return -1;
    cherryusb_mouse_dev_t *dev = &g_mice[idx];
    if (!dev->ready) return -1;

    /* Non-blocking poll via EHCI periodic check */
    extern int usb_mouse_data_available(int mouse_index);
    return usb_mouse_data_available(idx);
}

void cherryusb_hid_mouse_set_callback(int idx, cherryusb_mouse_callback_t cb) {
    if (idx < 0 || idx >= g_mouse_count) return;
    g_mice[idx].cb = cb;
}

int cherryusb_hid_mouse_count(void) {
    return g_mouse_count;
}

void cherryusb_hid_mouse_deinit(int idx) {
    if (idx < 0 || idx >= g_mouse_count) return;
    cherryusb_mouse_dev_t *dev = &g_mice[idx];

    printf("[CherryUSB] Deinit mouse %d\n", idx);
    dev->ready = 0;
    dev->cb    = NULL;
    memset(&dev->last, 0, sizeof(dev->last));
}
