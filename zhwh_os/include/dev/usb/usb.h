/**
 * @file usb.h
 * @brief Devfs USB ioctl definitions (stubs for usb_ioctl.c)
 */
#ifndef DEV_USB_USB_H
#define DEV_USB_USB_H

#include <stdint.h>

/* IOCTL command codes */
#define USB_DEVICEINFO            0
#define USB_GET_DEVICEINFO        1
#define USB_GET_DEVICE_DESC       2
#define USB_GET_DEVICE_DDESC      3
#define USB_GET_CONFIG_DESC       4
#define USB_GET_CDESC             5
#define USB_GET_FULL_DESC         6
#define USB_GET_FDESC             7
#define USB_REQUEST               8
#define USB_DO_REQUEST            9
#define USB_GET_CONFIG           10
#define USB_SET_CONFIG           11
#define USB_GET_ALTINTERFACE     12
#define USB_SET_ALTINTERFACE     13
#define USB_SET_SHORT_XFER       14
#define USB_SET_TIMEOUT          15
#define USB_DEVICESTATS          16
#define USB_CURRENT_CONFIG_INDEX 17

#define USB_MAX_DEVNAMES     16
#define USB_MAX_DEVNAMELEN   32

#define USB_PORT_ENABLED     1
#define USB_PORT_DISABLED    2
#define USB_PORT_SUSPENDED   3
#define USB_PORT_POWERED     4

/* Network byte order macros */
#define UGETW(p)  (*((uint16_t *)(p)))
#define USETW(p, v)  (*((uint16_t *)(p)) = (v))

/* Setup packet request type bits */
#define UT_WRITE_DEVICE    0x00
#define UR_SET_ADDRESS     0x05
#define UR_SET_CONFIG      0x09
#define UR_SET_INTERFACE   0x0B

/* ioctl argument structs (simplified stubs) */
struct usb_device_info {
    uint8_t  bus;
    uint8_t  addr;
    char     product[USB_MAX_DEVNAMELEN];
    char     vendor[USB_MAX_DEVNAMELEN];
    uint16_t product_id;
    uint16_t vendor_id;
    uint8_t  class;
    uint8_t  subclass;
    uint8_t  protocol;
    uint8_t  speed;
    uint8_t  power;
    uint8_t  port;
    uint16_t release;
};

struct usb_device_ddesc {
    uint8_t  data[18];
};

struct usb_device_cdesc {
    uint8_t  data[9];
};

struct usb_device_fdesc {
    uint16_t size;
    uint8_t  data[];
};

struct usb_ctl_request {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
    void    *data;
    uint16_t actlen;
    uint16_t flags;
};

struct usb_alt_interface {
    uint8_t  interface;
    uint8_t  altsetting;
};

#endif /* DEV_USB_USB_H */
