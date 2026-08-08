/**
 * @file usb_hci_stubs.c
 * @brief Stubs for UHCI/OHCI/Hub/HID/MSD — focus is EHCI bringup
 */
#include <kernel/hillsonos.h>
#include <kernel/usb.h>
#include <kernel/usb_uhci.h>
#include <kernel/usb_ohci.h>
#include <kernel/usb_hub.h>
#include <kernel/usb_hid.h>
#include <kernel/mouse.h>
#include <kernel/kbd.h>
#include <mm/kheap.h>

/* --- UHCI functions are defined in usb_uhci.c --- */

/* pci_write_config_byte stub (UHCI needs this, -O0 doesn't inline) */
void pci_write_config_byte(unsigned bus, unsigned dev,
                            unsigned fn, unsigned reg, uint8_t val) {
    (void)bus; (void)dev; (void)fn; (void)reg; (void)val;
}

/* get_phys_addr stub */
#include <page.h>
uint32_t get_phys_addr(void *vaddr) {
    return virt_to_phys((uint32_t)vaddr);
}

/* --- OHCI stubs --- */
struct usb_dev_t *ohci_get_dev_struct(pci_compat_t *bus, uint8_t num) {
    (void)bus; (void)num; return NULL;
}
int ohci_install(pci_compat_t *pci, struct pci_bar_t *bar) {
    (void)pci; (void)bar; return -1;
}

/* --- Hub globals/functions are defined in usb_hub.c --- */

/* --- Keyboard/mouse globals --- */
struct task_t *kbd_task = NULL;
struct task_t *mouse_task = NULL;
mouse_buttons_t cur_button_state = 0;

/* --- HID stubs --- */
/* HID globals/functions are defined in usb_hid.c */

/* --- printk is defined as printf alias in kernel.h --- */
void acquire(struct spinlock *lk) { (void)lk; }
void release(struct spinlock *lk) { (void)lk; }

/* --- MSD functions are defined in usb_msd.c --- */
