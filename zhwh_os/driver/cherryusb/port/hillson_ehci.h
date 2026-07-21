/**
 * @file hillson_ehci.h
 * @brief CherryUSB Port Layer — HillsonOS EHCI adapter (x86 32-bit)
 *
 * Maps CherryUSB's usbh_port_xxx API to HillsonOS's existing
 * USB HCD functions (usb_control_transfer, etc.)
 */

#ifndef CHERRYUSB_PORT_HILLSON_EHCI_H
#define CHERRYUSB_PORT_HILLSON_EHCI_H

#include <stdint.h>
#include "../core/usbh_core.h"

/* Init HillsonOS USB stack (calls existing usb_init) */
int usbh_port_init(void);

/* Reset USB port, return detected speed */
int usbh_port_reset(uint8_t port, uint8_t *speed);

/* Control transfer wrapper */
int usbh_port_control_xfer(uint8_t dev_addr, usb_setup_packet_t *setup, void *data);

/* Interrupt IN transfer wrapper */
int usbh_port_intr_xfer(uint8_t dev_addr, uint8_t ep, void *data, int len);

#endif
