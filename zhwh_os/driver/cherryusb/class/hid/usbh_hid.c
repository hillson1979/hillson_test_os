/**
 * @file usbh_hid.c
 * @brief CherryUSB HID Class Driver — RT-Thread port to HillsonOS x86
 *
 * Handles HID device registration, SET_PROTOCOL, SET_IDLE,
 * and interrupt IN transfers for mouse/keyboard reports.
 *
 * Original: CherryUSB class/hid/usbh_hid.c (RT-Thread)
 */

#include <stdint.h>
#include "string.h"
#include "printf.h"
#include "usbh_hid.h"
#include "../../core/usbh_core.h"
#include "../../port/hillson_ehci.h"

static usbh_hid_device_t g_hid_devs[USBH_HID_MAX_DEVICES];
static int g_hid_count = 0;

int usbh_hid_set_protocol(uint8_t dev_addr, uint8_t iface, uint8_t proto) {
    usb_setup_packet_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.bmRequestType = 0x21;  /* Host-to-Device, Class, Interface */
    setup.bRequest      = HID_REQUEST_SET_PROTOCOL;
    setup.wValue        = proto;
    setup.wIndex        = iface;
    setup.wLength       = 0;
    return usbh_port_control_xfer(dev_addr, &setup, NULL);
}

int usbh_hid_set_idle(uint8_t dev_addr, uint8_t iface, uint8_t duration) {
    usb_setup_packet_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.bmRequestType = 0x21;
    setup.bRequest      = HID_REQUEST_SET_IDLE;
    setup.wValue        = ((uint16_t)duration << 8) | 0;
    setup.wIndex        = iface;
    setup.wLength       = 0;
    return usbh_port_control_xfer(dev_addr, &setup, NULL);
}

int usbh_hid_register(uint8_t dev_addr, uint8_t iface,
                       uint8_t ep_in, uint8_t ep_max, uint8_t interval,
                       uint8_t protocol) {
    if (g_hid_count >= USBH_HID_MAX_DEVICES) {
        printf("[CherryUSB HID] Max devices reached\n");
        return -1;
    }

    usbh_hid_device_t *dev = &g_hid_devs[g_hid_count];
    memset(dev, 0, sizeof(*dev));
    dev->dev_addr      = dev_addr;
    dev->iface         = iface;
    dev->ep_in         = ep_in;
    dev->ep_in_max     = ep_max;
    dev->ep_in_interval= interval;
    dev->protocol      = protocol;

    /* CherryUSB pattern: Set boot protocol for mouse */
    if (protocol == HID_PROTOCOL_MOUSE) {
        int ret = usbh_hid_set_protocol(dev_addr, iface, 0);  /* boot protocol */
        if (ret < 0) {
            printf("[CherryUSB HID] SET_PROTOCOL failed (ret=%d)\n", ret);
            return -1;
        }
        printf("[CherryUSB HID] Boot protocol set for mouse\n");

        /* Set idle to 0 (indefinite) */
        usbh_hid_set_idle(dev_addr, iface, 0);
    }

    dev->ready = 1;
    printf("[CherryUSB HID] Device %d registered: addr=%d ep=0x%02x max=%d proto=%d\n",
           g_hid_count, dev_addr, ep_in, ep_max, protocol);

    return g_hid_count++;
}

int usbh_hid_read(int idx, void *report, int max_len) {
    if (idx < 0 || idx >= g_hid_count) return -1;
    usbh_hid_device_t *dev = &g_hid_devs[idx];
    if (!dev->ready) return -1;

    uint8_t buf[8];
    memset(buf, 0, sizeof(buf));
    int len = (max_len < (int)sizeof(buf)) ? max_len : (int)sizeof(buf);
    if (len > dev->ep_in_max) len = dev->ep_in_max;

    int n = usbh_port_intr_xfer(dev->dev_addr, dev->ep_in & 0x7F, buf, len);
    if (n < 0) return n;

    /* Parse mouse report */
    if (dev->protocol == HID_PROTOCOL_MOUSE && n >= 3) {
        usbh_hid_mouse_report_t *mr = (usbh_hid_mouse_report_t *)report;
        mr->buttons = buf[0];
        mr->x       = (int8_t)buf[1];
        mr->y       = (int8_t)buf[2];
        mr->wheel   = (n >= 4) ? (int8_t)buf[3] : 0;
        return sizeof(usbh_hid_mouse_report_t);
    }

    /* Generic HID: just copy raw data */
    memcpy(report, buf, n);
    return n;
}

int usbh_hid_count(void) {
    return g_hid_count;
}
