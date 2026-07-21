/**
 * @file hillson_ehci.c
 * @brief CherryUSB → HillsonOS 适配层
 *
 * ╔══════════════════════════════════════════════╗
 * ║  CherryUSB (独立 USB 协议栈)                 ║
 * ║  core/usbh_core.c  — 设备枚举              ║
 * ║  class/hid/usbh_hid.c — HID 类驱动         ║
 * ║  ═══════════════════════════════════════    ║
 * ║  port/hillson_ehci.c  ← 适配边界           ║
 * ║     ↓ 调用 HillsonOS HCD API               ║
 * ║  driver/usb_ehci.c    — 硬件控制           ║
 * ╚══════════════════════════════════════════════╝
 */

#include <stdint.h>
#include "string.h"
#include "printf.h"
#include "hillson_ehci.h"

/* ---- HillsonOS HCD 接口 (适配边界) ---- */
extern int usb_control_transfer(int ctrl, uint8_t addr, uint8_t ep,
                                 void *req, void *buf);
extern int usb_interrupt_transfer(int ctrl, uint8_t addr, uint8_t ep,
                                   void *buf, int len);

int usbh_port_init(void) {
    printf("[CherryUSB] Port init OK (HillsonOS HCD)\n");
    return 0;
}

int usbh_port_reset(uint8_t port, uint8_t *speed) {
    (void)port;
    if (speed) *speed = 2;
    return 0;
}

int usbh_port_control_xfer(uint8_t dev_addr, usb_setup_packet_t *setup, void *data) {
    return usb_control_transfer(0, dev_addr, 0, setup, data);
}

int usbh_port_intr_xfer(uint8_t dev_addr, uint8_t ep, void *data, int len) {
    (void)dev_addr; (void)ep; (void)len;
    extern int usb_mouse_periodic_poll(uint8_t *report);
    return usb_mouse_periodic_poll((uint8_t *)data);
}
