/**
 * @file usb.h
 * @brief USB core definitions
 */

#ifndef USB_H
#define USB_H

#include <stdint.h>

#define USB_MAX_DEVICES  16

// USB Standard Request Codes
#define USB_REQ_GET_STATUS        0x00
#define USB_REQ_CLEAR_FEATURE     0x01
#define USB_REQ_SET_FEATURE       0x03
#define USB_REQ_SET_ADDRESS       0x05
#define USB_REQ_GET_DESCRIPTOR    0x06
#define USB_REQ_SET_DESCRIPTOR    0x07
#define USB_REQ_GET_CONFIGURATION 0x08
#define USB_REQ_SET_CONFIGURATION 0x09
#define USB_REQ_GET_INTERFACE     0x0A
#define USB_REQ_SET_INTERFACE     0x0B
#define USB_REQ_SYNCH_FRAME       0x0C

// USB Descriptor Types
#define USB_DT_DEVICE       0x01
#define USB_DT_CONFIG       0x02
#define USB_DT_STRING       0x03
#define USB_DT_INTERFACE    0x04
#define USB_DT_ENDPOINT     0x05
#define USB_DT_HID          0x21
#define USB_DT_REPORT       0x22
#define USB_DT_PHYSICAL     0x23

// USB Direction
#define USB_DIR_OUT         0x00
#define USB_DIR_IN          0x80

// USB Request Types
#define USB_TYPE_STANDARD   (0x00 << 5)
#define USB_TYPE_CLASS      (0x01 << 5)
#define USB_TYPE_VENDOR     (0x02 << 5)
#define USB_TYPE_RESERVED   (0x03 << 5)

// USB Recipients
#define USB_RECIP_DEVICE    0x00
#define USB_RECIP_INTERFACE 0x01
#define USB_RECIP_ENDPOINT  0x02
#define USB_RECIP_OTHER     0x03

// USB HID Class
#define USB_CLASS_HID       0x03

// HID Subclass
#define HID_SUBCLASS_BOOT   0x01

// HID Protocol
#define HID_PROTOCOL_MOUSE  0x02
#define HID_PROTOCOL_KEYBOARD 0x01

// USB Device Request
typedef struct {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __attribute__((packed)) usb_device_request_t;

// USB Device Descriptor
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

// USB Configuration Descriptor
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wTotalLength;
    uint8_t  bNumInterfaces;
    uint8_t  bConfigurationValue;
    uint8_t  iConfiguration;
    uint8_t  bmAttributes;
    uint8_t  MaxPower;
} __attribute__((packed)) usb_config_descriptor_t;

// USB Interface Descriptor
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

// USB Endpoint Descriptor
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bEndpointAddress;
    uint8_t  bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t  bInterval;
} __attribute__((packed)) usb_endpoint_descriptor_t;

// HID Descriptor
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdHID;
    uint8_t  bCountryCode;
    uint8_t  bNumDescriptors;
    uint8_t  bReportDescriptorType;
    uint16_t wReportDescriptorLength;
} __attribute__((packed)) usb_hid_descriptor_t;

/**
 * @brief Initialize USB stack
 * @return 0 on success, negative on error
 */
int usb_init(void);

/**
 * @brief Enumerate a USB device
 * @param controller_id Controller index
 * @param port Port number
 * @return Device address or negative error
 */
int usb_enumerate_device(int controller_id, uint8_t port);

/**
 * @brief Get USB device descriptor string
 */
const char *usb_descriptor_type_string(uint8_t type);

/**
 * @brief Print device descriptor
 */
void usb_print_device_descriptor(usb_device_descriptor_t *desc);

#endif  // USB_H
