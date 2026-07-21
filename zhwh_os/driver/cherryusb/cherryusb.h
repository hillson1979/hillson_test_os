/**
 * @file cherryusb.h
 * @brief CherryUSB Master Include — Ported from RT-Thread CherryUSB
 *
 * CherryUSB is a portable USB host/device stack from RT-Thread.
 * This is the x86 32-bit HillsonOS adaptation.
 *
 * Original: https://github.com/cherry-embedded/CherryUSB
 *
 * Architecture:
 *   Application
 *       ↓
 *   Class Driver (hid/msc/...)
 *       ↓
 *   USB Core (usbh_core)
 *       ↓
 *   Port Layer (hillson_ehci) → HillsonOS EHCI driver
 */

#ifndef CHERRYUSB_H
#define CHERRYUSB_H

#include <stdint.h>
#include "core/usbh_core.h"
#include "core/usbh_hub.h"
#include "class/hid/usbh_hid.h"

/* RT-Thread adaptation — CherryUSB OSAL */
#define USB_OSAL_WAITING_FOREVER  0xFFFFFFFF

#endif /* CHERRYUSB_H */
