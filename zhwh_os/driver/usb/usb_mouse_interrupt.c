/**
 * @file usb_mouse_interrupt.c
 * @brief USB Mouse interrupt-mode support for Hillson OS
 *
 * Provides interrupt-based mouse polling. Currently delegates to
 * periodic polling since the EHCI driver uses polling, not interrupts.
 */
#include <string.h>
#include <kernel/hillsonos.h>
#include <usb_mouse.h>
#include <usb_hcd.h>
#include <mm/kheap.h>

/* Poll all registered mice — called from main loop or timer */
void usb_mouse_poll_all(void)
{
    int count = usb_mouse_get_count();
    if (count == 0) return;

    for (int i = 0; i < count; i++) {
        usb_mouse_data_available(i);
    }
}
