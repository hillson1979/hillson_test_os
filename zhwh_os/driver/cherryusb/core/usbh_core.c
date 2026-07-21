/**
 * @file usbh_core.c
 * @brief CherryUSB Host Core — x86 port
 *
 * Device enumeration state machine, address assignment,
 * endpoint discovery.
 *
 * Original: CherryUSB core/usbh_core.c (RT-Thread)
 */

#include <stdint.h>
#include "string.h"
#include "printf.h"
#include "usbh_core.h"
#include "../port/hillson_ehci.h"

static usbh_device_t g_devices[CHERRYUSB_MAX_DEVICES];
static int g_num_devices = 0;
static uint8_t g_next_addr = 2;  /* Start from address 2 (1=host) */

/* Get device by address */
usbh_device_t *usbh_get_device(uint8_t addr) {
    for (int i = 0; i < g_num_devices; i++)
        if (g_devices[i].address == addr) return &g_devices[i];
    return NULL;
}

/* Allocate new device entry */
static usbh_device_t *usbh_alloc_device(void) {
    if (g_num_devices >= CHERRYUSB_MAX_DEVICES) return NULL;
    usbh_device_t *dev = &g_devices[g_num_devices++];
    memset(dev, 0, sizeof(*dev));
    dev->address = g_next_addr++;
    return dev;
}

/* ---- CherryUSB-style enumeration state machine ---- */

int usbh_enumerate_device(uint8_t port, uint8_t speed) {
    usbh_device_t *dev = usbh_alloc_device();
    if (!dev) return -1;

    dev->speed = speed;
    dev->port  = port;

    printf("[CherryUSB] New device on port %d, speed=%d, addr=%d\n",
           port, speed, dev->address);

    /* 1. Reset port and get speed */
    uint8_t detected_speed;
    if (usbh_port_reset(port, &detected_speed) < 0) {
        printf("[CherryUSB] Port reset failed\n");
        g_num_devices--;
        return -1;
    }
    dev->speed = detected_speed;

    /* 2. Get device descriptor (first 8 bytes to learn EP0 max) */
    usb_setup_packet_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.bmRequestType = 0x80;  /* Device-to-Host, Standard, Device */
    setup.bRequest      = USB_REQUEST_GET_DESCRIPTOR;
    setup.wValue        = (USB_DESCRIPTOR_TYPE_DEVICE << 8) | 0;
    setup.wIndex        = 0;
    setup.wLength       = 8;

    uint8_t dev_desc_buf[18];
    int ret = usbh_port_control_xfer(dev->address, &setup, dev_desc_buf);
    if (ret < 0) {
        printf("[CherryUSB] Failed to get device descriptor\n");
        g_num_devices--;
        return -1;
    }
    dev->ep0_max = dev_desc_buf[7];  /* bMaxPacketSize0 */
    printf("[CherryUSB] EP0 max packet: %d\n", dev->ep0_max);

    /* 3. Set address */
    setup.bmRequestType = 0x00;  /* Host-to-Device, Standard, Device */
    setup.bRequest      = USB_REQUEST_SET_ADDRESS;
    setup.wValue        = dev->address;
    setup.wIndex        = 0;
    setup.wLength       = 0;
    ret = usbh_port_control_xfer(0, &setup, NULL);  /* Use address 0 for SET_ADDRESS */
    if (ret < 0) {
        printf("[CherryUSB] Failed to set address\n");
        g_num_devices--;
        return -1;
    }

    /* 4. Get full device descriptor */
    setup.bmRequestType = 0x80;
    setup.bRequest      = USB_REQUEST_GET_DESCRIPTOR;
    setup.wValue        = (USB_DESCRIPTOR_TYPE_DEVICE << 8) | 0;
    setup.wIndex        = 0;
    setup.wLength       = 18;
    ret = usbh_port_control_xfer(dev->address, &setup, dev_desc_buf);
    if (ret >= 18) {
        memcpy(&dev->desc, dev_desc_buf, 18);
        dev->vid = dev->desc.idVendor;
        dev->pid = dev->desc.idProduct;
        printf("[CherryUSB] VID=%04x PID=%04x\n", dev->vid, dev->pid);
    }

    /* 5. Set configuration #1 */
    setup.bmRequestType = 0x00;
    setup.bRequest      = USB_REQUEST_SET_CONFIGURATION;
    setup.wValue        = 1;
    setup.wIndex        = 0;
    setup.wLength       = 0;
    ret = usbh_port_control_xfer(dev->address, &setup, NULL);
    if (ret < 0) {
        printf("[CherryUSB] Failed to set config\n");
    } else {
        dev->configured = 1;
        printf("[CherryUSB] Device %d configured\n", dev->address);
    }

    return dev->address;
}

int usbh_device_count(void) { return g_num_devices; }

usbh_device_t *usbh_device_by_index(int idx) {
    if (idx < 0 || idx >= g_num_devices) return NULL;
    return &g_devices[idx];
}
