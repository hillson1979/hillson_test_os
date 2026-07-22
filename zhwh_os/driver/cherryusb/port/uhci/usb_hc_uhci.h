/**
 * @file usb_hc_uhci.h
 * @brief CherryUSB UHCI Host Controller Driver (x86 32-bit)
 *
 * UHCI (Universal Host Controller Interface) — Intel USB 1.1
 * Uses I/O ports for registers, frame list in main memory.
 */
#ifndef USB_HC_UHCI_H
#define USB_HC_UHCI_H
#include <stdint.h>

/* UHCI I/O registers */
#define UHCI_USBCMD    0x00
#define UHCI_USBSTS    0x02
#define UHCI_USBINTR   0x04
#define UHCI_FRNUM     0x06
#define UHCI_FRBASEADD 0x08
#define UHCI_SOFMOD    0x0C
#define UHCI_PORTSC1   0x10
#define UHCI_PORTSC2   0x12

/* USBCMD bits */
#define UHCI_CMD_RUN       0x0001
#define UHCI_CMD_HCRESET   0x0002
#define UHCI_CMD_GRESET    0x0004
#define UHCI_CMD_MAXP      0x0080
#define UHCI_CMD_CONFIGURE 0x0040

/* USBSTS bits */
#define UHCI_STS_USBINT 0x01
#define UHCI_STS_ERROR  0x02
#define UHCI_STS_RD     0x04
#define UHCI_STS_HSE    0x08
#define UHCI_STS_HCPE   0x10
#define UHCI_STS_HCH    0x20

/* PORTSC bits */
#define UHCI_PORT_CONNECT     0x0001
#define UHCI_PORT_ENABLE      0x0002
#define UHCI_PORT_RESET       0x0200
#define UHCI_PORT_LSDA        0x0100
#define UHCI_PORT_SUSPEND     0x1000

/* TD (Transfer Descriptor) — 32 bytes */
typedef struct uhci_td {
    uint32_t link_ptr;
    uint32_t ctrl_status;
    uint32_t token;
    uint32_t buffer;
} __attribute__((packed, aligned(16))) uhci_td_t;

/* TD control bits */
#define UHCI_TD_ACTIVE   0x80
#define UHCI_TD_IOC      0x08000000
#define UHCI_TD_ERROR    0x40
#define UHCI_TD_SPD      0x20000000

/* QH (Queue Head) — 32 bytes */
typedef struct uhci_qh {
    uint32_t head_ptr;
    uint32_t element_ptr;
} __attribute__((packed, aligned(16))) uhci_qh_t;

/* Frame list: 1024 x 32-bit pointers */
#define UHCI_FRAME_COUNT 1024

int uhci_hc_init(uint16_t io_base);
int uhci_hc_control_xfer(uint8_t addr, uint8_t ep, void *setup, void *data, int len, int dir_in);
int uhci_hc_intr_xfer(uint8_t addr, uint8_t ep, void *data, int len);
#endif
