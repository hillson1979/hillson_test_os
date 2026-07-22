/**
 * @file hillson_hcd.c
 * @brief CherryUSB Port — delegates to HillsonOS HCD (EHCI/UHCI/OHCI)
 */
#include <stdint.h>
#include "hillson_hcd.h"

int usbh_port_init(void) { return 0; }
int usbh_port_reset(uint8_t p, uint8_t *s) { (void)p; if(s)*s=2; return 0; }

int usbh_port_control_xfer(uint8_t a, usb_setup_packet_t *s, void *d) {
    extern int usb_control_transfer(int c,uint8_t a,uint8_t e,void *r,void *b);
    return usb_control_transfer(0, a, 0, s, d);
}

int usbh_port_intr_xfer(uint8_t a, uint8_t e, void *d, int l) {
    (void)a;(void)e;(void)l;
    extern int usb_mouse_periodic_poll(uint8_t *r);
    return usb_mouse_periodic_poll((uint8_t*)d);
}
