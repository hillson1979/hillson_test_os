/**
 * @file usb.h
 *
 * mapped to the Hillson OS environment.
 */
#ifndef KERNEL_USB_H
#define KERNEL_USB_H

#include <stdint.h>
#include "task.h"
#include "pciio.h"

/* ================================================================
 * USB speed constants
 * ================================================================ */
#define USB_SPEED_LOW    1
#define USB_SPEED_FULL   2
#define USB_SPEED_HIGH   3
#define USB_SPEED_SUPER  4

/* ================================================================
 * USB HCI types (prog_if values)
 * ================================================================ */
#define USB_TYPE_UHCI    0x00
#define USB_TYPE_OHCI    0x10
#define USB_TYPE_EHCI    0x20
#define USB_TYPE_XHCI    0x30
#define USB_TYPE_NOHCI   0x80
#define USB_TYPE_ANYHCI  0xFF

/* ================================================================
 * Max devices and buses
 * ================================================================ */
#define MAX_USB_BUSES          32
#define MAX_DEV_PER_HC        128
#define MAX_USB_MSD_DEVICES   208

/* ================================================================
 * Transfer and transaction types
 * ================================================================ */
#define USB_TRANSFER_CTRL          0
#define USB_TRANSFER_BULK          1
#define USB_TRANSFER_INTERRUPT     2
#define USB_TRANSFER_ISOCHRONOUS   3

#define USB_TRANS_SETUP   0
#define USB_TRANS_IN      1
#define USB_TRANS_OUT     2

/* ================================================================
 * Endpoint constants
 * ================================================================ */
#define USB_ENDPOINT_OUT      0x00
#define USB_ENDPOINT_IN       0x80
#define USB_ENDPOINT_BI       0x03
#define USB_ENDPOINT_CONTROL  0x00
#define USB_ENDPOINT_ISOC     0x01
#define USB_ENDPOINT_BULK     0x02
#define USB_ENDPOINT_INTERRUPT 0x03

/* ================================================================
 * Device ID macros
 * ================================================================ */
#define USB_MAKE_DEVID(bus, dev, ep)  (((bus) << 16) | ((dev) << 8) | (ep))
#define USB_DEVID_BUS(id)             (((id) >> 16) & 0xFF)
#define USB_DEVID_DEVADDR(id)         (((id) >> 8) & 0xFF)
#define USB_DEVID_EPADDR(id)          ((id) & 0xFF)

/* ================================================================
 * Device node type (for add_dev_node stub)
 * ================================================================ */
#define S_IFCHR  0x2000

/* ================================================================
 * Device node stubs
 * ================================================================ */
static inline void add_dev_node(const char *name, uint32_t devid, uint32_t mode) {
    (void)name; (void)devid; (void)mode;
}
static inline void remove_dev_node(uint32_t devid) {
    (void)devid;
}

/* ================================================================
 * ksprintf stub
 * ================================================================ */
#include <stdarg.h>
static inline int ksprintf(char *buf, int sz, const char *fmt, ...) {
    (void)sz;
    int ret;
    va_list ap;
    va_start(ap, fmt);
    /* Simple stub — just write a null terminator */
    *buf = '\0';
    va_end(ap);
    return 0;
}

/* ================================================================
 * kpanic stub
 * ================================================================ */
static inline void kpanic(const char *msg) {
    printk("PANIC: %s\n", msg);
    while(1) { __asm__ volatile("hlt"); }
}

/* Switch TTY stub (debug) */
static inline void switch_tty(int n) { (void)n; }

/* ================================================================
 * USB Descriptor Types
 * ================================================================ */

/* Device descriptor */
struct usb_device_descriptor_t {
    uint8_t  len;
    uint8_t  type;
    uint16_t bcd_usb;
    uint8_t  class;
    uint8_t  subclass;
    uint8_t  protocol;
    uint8_t  mps;
    uint16_t vendorid;
    uint16_t productid;
    uint16_t bcd_dev;
    uint8_t  manufacturer;
    uint8_t  product;
    uint8_t  serial;
    uint8_t  configs;
} __attribute__((packed));

/* Config descriptor */
struct usb_config_descriptor_t {
    uint8_t  len;
    uint8_t  type;
    uint16_t totlen;
    uint8_t  interfaces;
    uint8_t  configval;
    uint8_t  config;
    uint8_t  attribs;
    uint8_t  maxpower;
} __attribute__((packed));

/* Interface descriptor */
struct usb_interface_descriptor_t {
    uint8_t  len;
    uint8_t  type;
    uint8_t  interfacenum;
    uint8_t  altsetting;
    uint8_t  endpoints;
    uint8_t  class;
    uint8_t  subclass;
    uint8_t  protocol;
    uint8_t  interface;
} __attribute__((packed));

/* Endpoint descriptor */
struct usb_endpoint_descriptor_t {
    uint8_t  len;
    uint8_t  type;
    uint8_t  addr;
    uint8_t  attribs;
    uint16_t mps;
    uint8_t  interval;
} __attribute__((packed));

/* HID descriptor */
struct usb_hid_descriptor_t {
    uint8_t  len;
    uint8_t  type;
    uint16_t hid_bcd;
    uint8_t  country_code;
    uint8_t  descriptor_count;
    uint8_t  descriptors[];
} __attribute__((packed));

/* String descriptor */
struct usb_string_descriptor_t {
    uint8_t  len;
    uint8_t  type;
    uint16_t langid[10];
} __attribute__((packed));

/* Unicode string descriptor */
struct usb_unistring_descriptor_t {
    uint8_t  len;
    uint8_t  type;
    uint16_t wch[31];
} __attribute__((packed));

/* ================================================================
 * Core USB transfer structures
 * ================================================================ */

/* Forward declarations */
struct usb_dev_t;
struct usb_endpoint_t;
struct usb_interface_t;
struct usb_transfer_t;
struct usb_transaction_t;

/* USB Request (setup packet - 8 bytes) */
struct usb_request_t {
    uint8_t  type;     /* bmRequestType */
    uint8_t  req;      /* bRequest */
    uint8_t  hival;    /* wValue high byte */
    uint8_t  loval;    /* wValue low byte */
    uint16_t index;    /* wIndex */
    uint16_t len;      /* wLength */
} __attribute__((packed));

/* USB Endpoint */
struct usb_endpoint_t {
    uint8_t  addr;       /* endpoint number (0-15) */
    uint16_t mps;        /* max packet size */
    uint8_t  interval;   /* polling interval */
    uint8_t  toggle;     /* data toggle bit */
    uint8_t  type;       /* control/interrupt/bulk/isoc */
    uint8_t  direction;  /* IN/OUT/BI */
    struct usb_endpoint_t *next;
};

/* USB HCI operations vtable */
struct usb_ops_t {
    void (*setup_transfer)(struct usb_transfer_t *transfer);
    void (*schedule_transfer)(struct usb_transfer_t *transfer);
    void (*wait_transfer)(struct usb_transfer_t *transfer);
    int  (*poll_transfer)(struct usb_transfer_t *transfer);
    void (*delete_transfer)(struct usb_transfer_t *transfer);
    int  (*setup_transaction)(struct usb_transaction_t *transaction);
    void (*in_transaction)(struct usb_transaction_t *transaction);
    void (*out_transaction)(struct usb_transaction_t *transaction);
    void (*free_transaction_data)(volatile struct usb_transaction_t *transaction);
    unsigned int (*get_next_addr)(void *hc);
    void (*free_addr)(void *hc, unsigned int addr);
};

/* USB Transaction */
struct usb_transaction_t {
    struct usb_dev_t *dev;
    uint8_t  type;          /* USB_TRANS_SETUP/IN/OUT */
    uint8_t  req;
    uint8_t  hival;
    uint8_t  loval;
    uint16_t index;
    uint16_t len;
    uint8_t  toggle;
    void    *buf;
    struct usb_transfer_t *transfer;
    void    *data;          /* HC-specific transaction data */
    uintptr_t hc_trb_phys;  /* HC-specific queued TRB physical address */
    struct usb_transaction_t *next;
};

/* USB Transfer */
struct usb_transfer_t {
    struct usb_dev_t *dev;
    struct usb_endpoint_t *endpoint;
    uint8_t  type;          /* USB_TRANSFER_CTRL/BULK/INTERRUPT/ISOCHRONOUS */
    uint8_t  success;
    uint16_t pktsz;
    uint8_t  freq;
    uint8_t  need_clear_halt; /* Set by STALL handler, processed deferred */
    void    *data;          /* HC-specific transfer data (QH/ED pointer) */
    void   (*callback)(void *);
    void    *callback_arg;
    struct usb_transaction_t *trans_head;
    struct usb_transaction_t *trans_tail;
    struct usb_transfer_t *next_inttransfer;
};

/* USB Interface */
struct usb_interface_t {
    struct usb_interface_descriptor_t desc;
    uint16_t hid_report_len;       /* From the HID descriptor (type 0x21). */
    uint32_t bytes_per_sector;
    struct usb_dev_t *usb;
    struct usb_endpoint_t *endpoint_in;
    struct usb_endpoint_t *endpoint_out;
    struct usb_endpoint_t *endpoint_interrupt;
    uint32_t cur_tag;
    struct usb_interface_t *next;
};

/* SCSI Sense Data (used by MSD driver) */
struct sense_data_t {
    uint8_t  err_code;
    uint8_t  reserved1;
    uint8_t  sense_key;
    uint8_t  reserved2[9];
    uint8_t  additional_sense_code;
    uint8_t  additional_sense_code_qualifier;
    uint8_t  reserved3[4];
} __attribute__((packed));

/* USB Mass Storage Command Block Wrapper (CBW, 31 bytes) */
struct usb_cmd_blk_wrapper_t {
    uint32_t sig;       /* 0x43425355 = "USBC" */
    uint32_t tag;       /* command tag */
    uint32_t len;       /* data transfer length */
    uint8_t  flags;     /* 0x80 = device-to-host */
    uint8_t  lun;       /* LUN (bits 0-3) */
    uint8_t  cblen;     /* length of CBWCB (bits 0-4) */
    uint8_t  cmd[16];   /* command block */
} __attribute__((packed));

/* USB Device */
struct usb_dev_t {
    uint8_t  bus;
    uint8_t  num;
    uint8_t  port;
    uint8_t  speed;
    uint8_t  type;
    uint8_t  class;
    uint8_t  subclass;
    uint8_t  protocol;
    uint16_t spec;
    uint16_t vendor;
    uint16_t product;
    uint16_t release;
    uint8_t  manufacturerid;
    uint8_t  productid;
    uint8_t  serialid;
    uint8_t  configs;
    uint8_t  cur_config;
    char     product_name[32];
    char     serial[32];
    void    *priv;          /* HC-specific data */
    struct usb_ops_t *ops;
    struct usb_endpoint_t *endpoints;
    struct usb_interface_t *interfaces;
    kernel_mutex_t lock;
};

/*
 * HC controller types — each HCI driver defines its own dev struct.
 * These are declared in usb_uhci.h, usb_ehci.h, usb_ohci.h.
 */

/* ================================================================
 * Function declarations (implemented in usb.c)
 * ================================================================ */

struct usb_dev_t *usb_create_dev(uint8_t bus, unsigned int port, uint8_t speed);
void usb_destroy_dev(struct usb_dev_t *dev);

/* Interrupt transfer management */
void remove_interrupt_transfer(struct usb_transfer_t *transfer);
void usb_schedule_inttransfer(struct usb_dev_t *usb, struct usb_endpoint_t *endpoint,
                               struct usb_transfer_t *transfer,
                               void *buf, size_t bufsz,
                               void (*callback)(void *), void *callback_arg,
                               uint8_t freq);

/* Transfer lifecycle */
void usb_setup_transfer(struct usb_dev_t *dev, struct usb_endpoint_t *endpoint,
                         struct usb_transfer_t *transfer, uint8_t type);
void usb_schedule_transfer(struct usb_transfer_t *transfer);
int  usb_poll_transfer(struct usb_transfer_t *transfer);
void usb_wait_transfer(struct usb_transfer_t *transfer);
void usb_delete_transfer(struct usb_transfer_t *transfer);

/* Transaction building */
uint8_t usb_setup_transaction(struct usb_transfer_t *transfer,
                               uint8_t type, uint8_t req,
                               uint8_t hival, uint8_t loval,
                               uint16_t index, uint16_t len);
void usb_in_transaction(struct usb_transfer_t *transfer,
                         int ctrl_handshake, void *buf, size_t len);
void usb_out_transaction(struct usb_transfer_t *transfer,
                          int ctrl_handshake, void *buf, size_t len);

/* Control transfer helpers */
int usb_ctrl_in(struct usb_dev_t *dev, void *buf,
                uint8_t type, uint8_t req,
                uint8_t hival, uint8_t loval,
                uint16_t index, uint16_t len);
int usb_ctrl_out(struct usb_dev_t *dev, void *buf,
                 uint8_t type, uint8_t req,
                 uint8_t hival, uint8_t loval,
                 uint16_t index, uint16_t len);
int usb_ctrl_set(struct usb_dev_t *dev,
                 uint8_t type, uint8_t req,
                 uint8_t hival, uint8_t loval, uint16_t index);

/* Device descriptor access */
int usb_get_device_descriptor(struct usb_dev_t *dev, uint8_t len);
int usb_get_config_descriptor(struct usb_dev_t *dev);

/* Configuration */
uint8_t usb_get_iface(struct usb_dev_t *dev, uint16_t iface);
void usb_set_iface(struct usb_dev_t *dev, uint16_t iface, uint8_t alt_iface);
uint8_t usb_get_config(struct usb_dev_t *dev);
unsigned int usb_set_device_addr(struct usb_dev_t *dev, unsigned int addr);
int usb_set_config(struct usb_dev_t *dev, uint32_t config);
void usb_clear_feature_halt(struct usb_dev_t *dev, struct usb_endpoint_t *endpoint);

/* Device setup */
int usb_setup_device(struct usb_dev_t *dev, unsigned int addr);

/* USB init entry point (per-controller, renamed for Hillson OS) */
int usb_init_controller(pci_compat_t *pci);

/* Periodic polling */
void uhci_poll(void);
void ehci_poll(void);
void usb_hub_poll(void);
void usb_poll_interrupts(void);

#endif /* KERNEL_USB_H */
