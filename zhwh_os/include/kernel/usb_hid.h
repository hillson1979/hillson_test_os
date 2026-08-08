/**
 * @file usb_hid.h
 * @brief HID class driver compatibility header
 */
#ifndef KERNEL_USB_HID_H
#define KERNEL_USB_HID_H

#include "usb.h"

/* HID device descriptor */
struct usb_hid_dev_t {
    struct usb_interface_t *iface;
    struct usb_transfer_t   transfer;
    uint8_t  buf[8];            /* Interrupt transfer buffer */
    uint8_t  last_packet[8];
    uint8_t  last_key_pressed;
    int      last_key_counter;
    uint8_t  leds;
    int      new_data;          /* Set by callback, cleared by consumer */
    uint8_t  is_mouse;
    uint8_t  mouse_absolute;
    uint8_t  report_id;
    uint8_t  report_len;
    uint16_t logical_max_x;
    uint16_t logical_max_y;
    int      abs_x;
    int      abs_y;
    struct usb_hid_dev_t *next;
};

/* Global HID device list */
extern struct usb_hid_dev_t hid_list;
extern kernel_mutex_t usb_hid_tablock;

/* Function declarations (implemented in usb_hid.c) */
int  usb_hid_set_protocol(struct usb_dev_t *usb, uint8_t protocol,
                          uint16_t interface);
void usb_hid_remove(struct usb_interface_t *iface);
int  init_hid(struct usb_interface_t *iface);
int  usb_hid_register_async(struct usb_interface_t *iface,
                            uint8_t *rdesc, int rdesc_len);

#endif /* KERNEL_USB_HID_H */
