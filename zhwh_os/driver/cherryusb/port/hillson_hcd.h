/**
 * @file hillson_hcd.h
 * @brief CherryUSB Port Layer — EHCI/UHCI/OHCI adapter (x86 32-bit)
 */

#ifndef CHERRYUSB_PORT_HILLSON_HCD_H
#define CHERRYUSB_PORT_HILLSON_HCD_H

#include <stdint.h>

/* USB setup packet (标准 8 字节) */
typedef struct {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __attribute__((packed)) usb_setup_packet_t;

/* Init HillsonOS USB stack (calls existing usb_init) */
int usbh_port_init(void);

/* Reset USB port, return detected speed */
int usbh_port_reset(uint8_t port, uint8_t *speed);

/* Control transfer wrapper */
int usbh_port_control_xfer(uint8_t dev_addr, usb_setup_packet_t *setup, void *data);

/* Interrupt IN transfer wrapper */
int usbh_port_intr_xfer(uint8_t dev_addr, uint8_t ep, void *data, int len);

#endif
