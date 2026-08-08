/**
 * @file usb_hub.h
 * @brief USB Hub driver compatibility header
 */
#ifndef KERNEL_USB_HUB_H
#define KERNEL_USB_HUB_H

#include "usb.h"

/* Hub descriptor */
struct usb_hub_desc_t {
    uint8_t  len;
    uint8_t  type;
    uint8_t  ports;
    uint16_t characteristics;
    uint8_t  pwr_on_2_pwr_good;
    uint8_t  current;
};

struct usb_hub_port_status_t {
    /* wPortStatus bits */
    uint8_t curstat      : 1;   /* bit 0: Current Connect Status */
    uint8_t enabled      : 1;   /* bit 1: Port Enabled */
    uint8_t suspend      : 1;   /* bit 2: Suspend */
    uint8_t overcur      : 1;   /* bit 3: Over-current */
    uint8_t reset        : 1;   /* bit 4: Reset */
    uint8_t res1         : 3;   /* bits 5-7: reserved */
    uint8_t power        : 1;   /* bit 8: Port Power */
    uint8_t lospeed      : 1;   /* bit 9: Low Speed */
    uint8_t hispeed      : 1;   /* bit 10: High Speed */
    uint8_t testmode     : 1;   /* bit 11: Port Test Mode */
    uint8_t control      : 1;   /* bit 12: Port Indicator Control */
    uint8_t res2         : 3;   /* bits 13-15: reserved */
    /* wPortChange bits */
    uint8_t state_change     : 1;   /* bit 16: Connect Status Change */
    uint8_t enabled_change   : 1;   /* bit 17: Port Enable Change */
    uint8_t suspend_change   : 1;   /* bit 18: Suspend Change */
    uint8_t overcur_change   : 1;   /* bit 19: Over-current Change */
    uint8_t reset_change     : 1;   /* bit 20: Reset Change */
    uint8_t res3             : 3;   /* bits 21-23: reserved */
} __attribute__((packed));

#define HUB_PORT_FLAG_ENABLED   0x02
#define HUB_PORT_FLAG_SUSPENDED 0x04
#define HUB_PORT_FLAG_POWERED   0x08

/* Per-port state */
struct usb_hub_port_t {
    uint8_t           port;
    struct usb_dev_t *usb;
    uint32_t          flags;
};

#define HUB_PORT_FLAG_CONNECTED  0x01

/* Hub device */
struct usb_hub_t {
    struct usb_hub_desc_t desc;
    struct usb_dev_t     *usb;
    struct usb_hub_port_t *ports;
    struct usb_hub_t     *next;
    uint8_t               status;
};

extern struct usb_hub_t hub_list;
extern kernel_mutex_t usb_hub_tablock;

/* Function declarations */
struct usb_hub_t *get_hub_struct(struct usb_dev_t *usb);
void usb_hub_remove_dev(struct usb_dev_t *dev);
void usb_hub_poll(void);
int  init_hub(struct usb_dev_t *dev);

#endif /* KERNEL_USB_HUB_H */
