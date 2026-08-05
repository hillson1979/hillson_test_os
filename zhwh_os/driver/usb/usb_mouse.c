/**
 * @file usb_mouse.c
 * @brief USB Mouse public API for Hillson OS
 *
 * Provides the functions that kernel.c and syscall.c call for USB mouse support.
 * EHCI-focused: mouse data comes from periodic interrupt polling.
 */

#include <string.h>
#include <kernel/hillsonos.h>
#include <kernel/usb.h>
#include <usb_mouse.h>
#include <usb_hcd.h>
#include <usb_display.h>
#include <kernel/usb_hid.h>
#include <mm/kheap.h>

/* ================================================================
 * Mouse device tracking
 * ================================================================ */
#define USB_MAX_MICE  4

struct mouse_dev {
    int  controller_id;
    uint8_t dev_addr;
    uint8_t interface;
    uint8_t ep;
    uint8_t maxpkt;
    uint8_t interval;
    uint8_t data_ready;
    uint8_t report[8];
};

static struct mouse_dev g_mice[USB_MAX_MICE];
static int g_mouse_count = 0;

/* Global variables referenced by syscall.c */
uint8_t g_usb_mouse_ep        = 0;
uint8_t g_usb_mouse_maxpkt    = 0;
uint8_t g_usb_mouse_interval  = 0;
int     g_usb_setproto_result = 0;

/* ================================================================
 * usb_mouse_init() — Initialize a USB mouse device
 * ================================================================ */
int usb_mouse_init(int controller_id, uint8_t dev_addr, uint8_t interface,
                   uint8_t endpoint_in, uint8_t max_packet)
{
    if (g_mouse_count >= USB_MAX_MICE) {
        usb_printk("usb_mouse: too many mice\n");
        return -1;
    }

    struct mouse_dev *m = &g_mice[g_mouse_count];
    m->controller_id = controller_id;
    m->dev_addr      = dev_addr;
    m->interface     = interface;
    m->ep            = endpoint_in;
    m->maxpkt        = max_packet;
    m->interval      = 10; /* default 10ms polling */
    m->data_ready    = 0;

    /* Save first mouse info for syscall display */
    if (g_mouse_count == 0) {
        g_usb_mouse_ep       = endpoint_in;
        g_usb_mouse_maxpkt   = max_packet;
        g_usb_mouse_interval = 10;
    }

    int idx = g_mouse_count++;
    g_usb_mouse_count = g_mouse_count;

    usb_printk("usb_mouse: mouse %d initialized (dev=%d ep=0x%02x)\n",
           idx, dev_addr, endpoint_in);

    /* Set up periodic polling via HCD */
    usb_mouse_periodic_init(controller_id, dev_addr, endpoint_in, 1);

    return idx;
}

/* ================================================================
 * usb_mouse_read() — Read mouse report
 * ================================================================ */
int usb_mouse_read(int mouse_index, void *report)
{
    if (mouse_index < 0 || mouse_index >= g_mouse_count) {
        return -1;
    }

    struct mouse_dev *m = &g_mice[mouse_index];

    /* Poll for new data — this consumes hid->new_data */
    int bytes = usb_mouse_periodic_poll(m->report);
    if (bytes <= 0) {
        return 0; /* No data available */
    }

    /* Copy report to caller */
    memcpy(report, m->report, bytes > 8 ? 8 : bytes);
    return bytes;
}

/* ================================================================
 * usb_mouse_data_available() — Check if mouse data is ready
 * ================================================================ */
int usb_mouse_data_available(int mouse_index)
{
    if (mouse_index < 0 || mouse_index >= g_mouse_count) {
        return 0;
    }

    /* This runs in kernel context after the user process enters the mouse
     * input syscall. Only service queued interrupt transfers here; root-port
     * hotplug scanning belongs to the kernel USB polling path. */
    usb_poll_interrupts();

    /* Peek only — don't consume new_data.
     * The actual read happens in usb_mouse_read(). */
    extern struct usb_hid_dev_t *g_first_mouse_hid;
    return (g_first_mouse_hid && g_first_mouse_hid->new_data);
}

/* ================================================================
 * usb_mouse_get_count() — Return number of detected mice
 * ================================================================ */
int usb_mouse_get_count(void)
{
    return g_mouse_count;
}

/* ================================================================
 * usb_mouse_register_callback() — Register mouse event callback (stub)
 * ================================================================ */
void usb_mouse_register_callback(int mouse_index,
                                  void (*callback)(int dx, int dy, uint8_t buttons))
{
    (void)mouse_index;
    (void)callback;
}

/* ================================================================
 * usb_mouse_test() — Basic test function
 * ================================================================ */
void usb_mouse_test(void)
{
    usb_printk("usb_mouse_test: %d mouse(s) detected\n", g_mouse_count);
    if (g_mouse_count == 0) {
        usb_printk("usb_mouse_test: no mice to test\n");
        return;
    }

    uint8_t report[8];
    int bytes = usb_mouse_read(0, report);
    if (bytes > 0) {
        usb_printk("usb_mouse_test: read %d bytes: %02x %02x %02x %02x\n",
               bytes, report[0], report[1], report[2], report[3]);
    } else {
        usb_printk("usb_mouse_test: no data from mouse 0\n");
    }
}

/* ================================================================
 * usb_mouse_test_periodic() — Periodic polling test (called from kernel.c)
 * ================================================================ */
void usb_mouse_test_periodic(void)
{
    if (g_mouse_count == 0) {
        usb_printk("usb_mouse_test_periodic: no mouse found\n");
        return;
    }
    usb_printk("usb_mouse_test_periodic: polling mouse 0...\n");
}
