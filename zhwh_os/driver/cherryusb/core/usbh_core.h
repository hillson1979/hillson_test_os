/**
 * @file usbh_core.h
 * @brief CherryUSB Host Core — RT-Thread port to HillsonOS x86
 *
 * Manages USB device enumeration, address assignment,
 * transfer submission, and device lifecycle.
 *
 * Original: CherryUSB core/usbh_core.h
 */

#ifndef CHERRYUSB_USBH_CORE_H
#define CHERRYUSB_USBH_CORE_H

#include <stdint.h>

/* USB speeds */
#define USB_SPEED_LOW    0
#define USB_SPEED_FULL   1
#define USB_SPEED_HIGH   2

/* Endpoint direction */
#define USB_EP_DIR_OUT   0x00
#define USB_EP_DIR_IN    0x80

/* Transfer types */
#define USB_TRANSFER_TYPE_CONTROL     0
#define USB_TRANSFER_TYPE_ISOCHRONOUS 1
#define USB_TRANSFER_TYPE_BULK        2
#define USB_TRANSFER_TYPE_INTERRUPT   3

/* Standard descriptor types */
#define USB_DESCRIPTOR_TYPE_DEVICE       1
#define USB_DESCRIPTOR_TYPE_CONFIG       2
#define USB_DESCRIPTOR_TYPE_STRING       3
#define USB_DESCRIPTOR_TYPE_INTERFACE    4
#define USB_DESCRIPTOR_TYPE_ENDPOINT     5
#define USB_DESCRIPTOR_TYPE_HID         33
#define USB_DESCRIPTOR_TYPE_HID_REPORT  34

/* Standard requests */
#define USB_REQUEST_GET_DESCRIPTOR       6
#define USB_REQUEST_SET_ADDRESS          5
#define USB_REQUEST_SET_CONFIGURATION    9

/* ---- CherryUSB structures (x86 32-bit) ---- */

/* USB setup packet (8 bytes, standard format) */
typedef struct {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __attribute__((packed)) usb_setup_packet_t;

/* Endpoint descriptor (7 bytes) */
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bEndpointAddress;
    uint8_t  bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t  bInterval;
} __attribute__((packed)) usb_endpoint_descriptor_t;

/* Interface descriptor (9 bytes) */
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bInterfaceNumber;
    uint8_t  bAlternateSetting;
    uint8_t  bNumEndpoints;
    uint8_t  bInterfaceClass;
    uint8_t  bInterfaceSubClass;
    uint8_t  bInterfaceProtocol;
    uint8_t  iInterface;
} __attribute__((packed)) usb_interface_descriptor_t;

/* Config descriptor (9 bytes header) */
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wTotalLength;
    uint8_t  bNumInterfaces;
    uint8_t  bConfigurationValue;
    uint8_t  iConfiguration;
    uint8_t  bmAttributes;
    uint8_t  bMaxPower;
} __attribute__((packed)) usb_config_descriptor_t;

/* Device descriptor (18 bytes) */
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} __attribute__((packed)) usb_device_descriptor_t;

/* USB bus/device info — CherryUSB host core tracking */
typedef struct {
    uint8_t  address;       /* Assigned USB address */
    uint8_t  speed;         /* USB_SPEED_LOW/FULL/HIGH */
    uint8_t  port;          /* Root hub port number */
    uint8_t  pad;
    uint16_t vid;
    uint16_t pid;
    uint8_t  ep0_max;       /* Max packet size for EP0 */
    usb_device_descriptor_t desc;
    uint8_t  configured;
} usbh_device_t;

#define CHERRYUSB_MAX_DEVICES 8

/* ---- Port layer API (implemented by port/hillson_ehci.c) ---- */
int  usbh_port_init(void);
int  usbh_port_reset(uint8_t port, uint8_t *speed);
int  usbh_port_control_xfer(uint8_t dev_addr, usb_setup_packet_t *setup, void *data);
int  usbh_port_intr_xfer(uint8_t dev_addr, uint8_t ep, void *data, int len);

#endif /* CHERRYUSB_USBH_CORE_H */
