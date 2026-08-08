/**
 * @file usb_hcd.c
 * @brief USB HCD bridge â€?public API for Hillson OS
 *
 * Focus: EHCI controller bringup.
 */

#include <string.h>
#include <kernel/hillsonos.h>
#include <kernel/task.h>
#include <task.h>
#include <kernel/usb.h>
#include <kernel/usb_ehci.h>
#include <kernel/usb_uhci.h>
#include <kernel/usb_ohci.h>
#include <kernel/usb_xhci.h>
#include <kernel/usb_hub.h>
#include <kernel/usb_hid.h>
#include <kernel/errno.h>
#include <kernel/ata.h>
#include <mm/kheap.h>

#include <pci.h>
#include <usb.h>
#include <usb_hcd.h>

/* ================================================================
 * Global variables (referenced by syscall.c / usb_display.h)
 * ================================================================ */
int g_usb_uhci_count   = 0;
int g_usb_ehci_count   = 0;
int g_usb_uhci2_count  = 0;
int g_usb_mouse_count  = 0;
/* 0 off, 1 PCI scan, 2 xHCI found/start pending, 3 controller ready,
 * 4 port scan complete, -1 no supported controller/failed. */
static int g_usb_init_state = 0;
static unsigned g_usb_scan_bus;
static unsigned g_usb_scan_dev;
static unsigned g_usb_xhci_bus;
static unsigned g_usb_xhci_dev;
static pci_compat_t g_usb_async_pci;
static struct pci_bar_t g_usb_async_bar;
static int g_usb_async_bar_valid;

void usb_init_poll(void);

int usb_get_init_state(void)
{
    if (g_usb_init_state == 3 && xhci_all_ports_enabled())
        g_usb_init_state = 4;
    return g_usb_init_state;
}

int usb_request_init(void)
{
    if (g_usb_init_state != 0) return g_usb_init_state;
    pci_dev_t **devices = pci_get_devices();

    /* pci_init() already enumerated the machine at boot. Consume that cached
     * device table instead of probing all 256x32 PCI locations again. */
    for (int i = 0; devices && i < 64 && devices[i]; i++) {
        pci_dev_t *dev = devices[i];
        if (dev->header.class != 0x0C ||
            dev->header.subclass != 0x03 ||
            dev->header.prog_if != USB_TYPE_XHCI)
            continue;

        g_usb_xhci_bus = dev->bus_id;
        g_usb_xhci_dev = dev->dev_id;
        pci_to_compat_raw(dev->bus_id, dev->dev_id, dev->fn_id,
                          &g_usb_async_pci);

        struct pci_bar_t bars[6];
        pci_get_bar_info_raw(dev->bus_id, dev->dev_id, dev->fn_id, bars);
        g_usb_async_bar_valid = 0;
        for (int b = 0; b < 6; b++) {
            if (bars[b].iotype == PCI_IOTYPE_MMIO && bars[b].base != 0) {
                g_usb_async_bar = bars[b];
                g_usb_async_bar_valid = 1;
                break;
            }
        }

        if (!g_usb_async_bar_valid) {
            usb_printk("usb: cached xHCI %02x:%02x.%x has no MMIO BAR\n",
                       dev->bus_id, dev->dev_id, dev->fn_id);
            g_usb_init_state = -1;
            return -1;
        }

        g_usb_init_state = 2;
        usb_printk("usb: cached xHCI at %02x:%02x.%x; async start pending\n",
                   dev->bus_id, dev->dev_id, dev->fn_id);
        return 0;
    }

    g_usb_init_state = -1;
    usb_printk("usb: no xHCI controller in cached PCI table\n");
    return -1;
}

void usb_init_poll(void)
{
    static volatile int poll_busy;
    if (poll_busy) return;
    poll_busy = 1;

    if (g_usb_init_state == 3) {
        xhci_init_ports_step();
        if (xhci_all_ports_enabled())
            g_usb_init_state = 4;
        poll_busy = 0;
        return;
    }

    if (g_usb_init_state == 2) {
        if (!g_usb_async_bar_valid) { g_usb_init_state = -1; poll_busy = 0; return; }
        int ret = xhci_async_reset_step(&g_usb_async_pci, &g_usb_async_bar);
        if (ret > 0) {
            xhci_mark_reset_complete();
            usb_printk("usb: xHCI reset complete, installing rings\n");
            ret = xhci_install(&g_usb_async_pci, &g_usb_async_bar);
            if (ret == 0) {
                g_usb_init_state = 3;
                usb_printk("usb: xHCI controller ready\n");
            } else {
                g_usb_init_state = -1;
                usb_printk("usb: xHCI install failed (%d)\n", ret);
            }
        }
        else if (ret < 0) g_usb_init_state = -1;
        poll_busy = 0;
        return;
    }
    if (g_usb_init_state != 1) { poll_busy = 0; return; }

    unsigned bus = g_usb_scan_bus;
    unsigned dev = g_usb_scan_dev;
    if (++g_usb_scan_dev >= 32) {
        g_usb_scan_dev = 0;
        g_usb_scan_bus++;
    }

    uint32_t vd = pci_read_config_dword(bus, dev, 0, 0);
    if (vd != 0xFFFFFFFF && vd != 0) {
        uint8_t cls = pci_read_config_byte(bus, dev, 0, 0x0B);
        uint8_t sub = pci_read_config_byte(bus, dev, 0, 0x0A);
        uint8_t prg = pci_read_config_byte(bus, dev, 0, 0x09);
        if (cls == 0x0C && sub == 0x03) {
            usb_printk("usb: async scan found %02x:%02x.0 prog=%02x\n",
                       bus, dev, prg);
            if (prg == USB_TYPE_XHCI) {
                g_usb_xhci_bus = bus;
                g_usb_xhci_dev = dev;
                pci_to_compat_raw(bus, dev, 0, &g_usb_async_pci);
                struct pci_bar_t bars[6];
                pci_get_bar_info_raw(bus, dev, 0, bars);
                g_usb_async_bar_valid = 0;
                for (int b = 0; b < 6; b++) {
                    if (bars[b].iotype == PCI_IOTYPE_MMIO && bars[b].base != 0) {
                        g_usb_async_bar = bars[b];
                        g_usb_async_bar_valid = 1;
                        break;
                    }
                }
                g_usb_init_state = 2;
                usb_printk("usb: xHCI found at %02x:%02x.0; async start pending\n",
                           bus, dev);
                poll_busy = 0;
                return;
            }
        }
    }

    if (g_usb_scan_bus >= 256) {
        g_usb_init_state = -1;
        usb_printk("usb: no supported xHCI controller found\n");
    }
    poll_busy = 0;
}

/* EHCI debug globals */
int g_ehci_qh_caps      = 0;
int g_ehci_qh_caps2     = 0;
int g_ehci_mouse_port   = 0;
int g_ehci_port_owner   = 0;
int g_ehci_port_enabled = 0;
int g_ehci_port_speed   = 0;
int g_ehci_fl_entry     = 0;
int g_ehci_td0_token    = 0;
unsigned g_ehci_portsc  = 0;
unsigned g_ehci_qh_ovl  = 0;
unsigned g_ehci_qh_nxt  = 0;
unsigned g_ehci_qh_alt  = 0;
unsigned g_ehci_td_nxt  = 0;
unsigned g_ehci_eecp    = 0;
unsigned g_ehci_legsup  = 0;
unsigned g_ehci_qh_b0   = 0;
unsigned g_ehci_qh_cur  = 0;

/* Per-core current task stub */
struct cpu_core cpu0 = { NULL };
/* ticks is defined in interrupt.c */

/* ATA disk stubs */
struct ata_dev_s *ahci_disk_dev[MAX_AHCI_DEVICES];
struct parttab_s  *ahci_disk_part[MAX_AHCI_DEVICES];

/* Additional globals referenced by syscall.c / interrupt.c */
int num_uhci_controllers = 0;
int g_using_ehci = 1;  /* EHCI is primary */
int g_ehci_cmd = 0;
int g_ehci_sts = 0;
int g_ehci_qh_phys = 0;
int g_ehci_fl_phys = 0;
int g_mouse_report_count = 0;
uint8_t g_dma_bytes[8] = {0};
int g_ehci_debug_len = 0;
char g_ehci_debug_buf[256] = {0};

/* UHCI IRQ handler stub (interrupt.c expects this) */
void uhci_irq_handler(void) {}

/* ================================================================
 * usb_init(void) â€?scans PCI for USB HC, initializes each
 * ================================================================ */
int usb_init(void)
{
    if (g_usb_init_state == 4)
        return 0;
    if (g_usb_init_state > 0 && g_usb_init_state < 4)
        return g_usb_init_state;

    pci_dev_t **devices = pci_get_devices();

    for (int i = 0; devices && i < 64 && devices[i]; i++) {
        pci_dev_t *dev = devices[i];
        if (dev->header.class != 0x0C ||
            dev->header.subclass != 0x03 ||
            dev->header.prog_if != USB_TYPE_XHCI)
            continue;

        pci_compat_t pci;
        struct pci_bar_t bars[6];
        struct pci_bar_t *bar = NULL;

        pci_to_compat_raw(dev->bus_id, dev->dev_id, dev->fn_id, &pci);
        pci_get_bar_info_raw(dev->bus_id, dev->dev_id, dev->fn_id, bars);
        for (int b = 0; b < 6; b++) {
            if (bars[b].iotype == PCI_IOTYPE_MMIO && bars[b].base != 0) {
                bar = &bars[b];
                break;
            }
        }

        if (!bar) {
            usb_printk("usb: xHCI %02x:%02x.%x has no MMIO BAR\n",
                       dev->bus_id, dev->dev_id, dev->fn_id);
            g_usb_init_state = -1;
            return -1;
        }

        g_usb_init_state = 2;
        usb_printk("usb: synchronous xHCI init at %02x:%02x.%x\n",
                   dev->bus_id, dev->dev_id, dev->fn_id);

        int ret = xhci_install(&pci, bar);
        if (ret == 0) {
            g_usb_init_state = 4;
            usb_printk("usb: synchronous xHCI init complete\n");
            return 0;
        }

        g_usb_init_state = -1;
        usb_printk("usb: synchronous xHCI init failed (%d)\n", ret);
        return ret;
    }

    g_usb_init_state = -1;
    usb_printk("usb: no xHCI controller in cached PCI table\n");
    return -1;
}

/* ================================================================
 * ================================================================ */
int usb_hcd_init(void) { return usb_init(); }

int usb_control_transfer(int cid, uint8_t addr, uint8_t ep,
                          usb_device_request_t *req, void *data) {
    (void)cid; (void)addr; (void)ep; (void)req; (void)data;
    return -ENOSYS;
}

int usb_interrupt_transfer(int cid, uint8_t addr, uint8_t ep,
                            void *data, int len) {
    (void)cid; (void)addr; (void)ep; (void)data; (void)len;
    return -ENOSYS;
}

int usb_interrupt_poll(int cid) { (void)cid; return 0; }

/* ================================================================
 * usb_mouse_periodic_init â€?The real interrupt transfer is set up
 * by init_hid() â†?usb_schedule_inttransfer().  Here we just record
 * that the init was requested so usb_mouse_init() succeeds.
 * ================================================================ */
int usb_mouse_periodic_init(int cid, uint8_t addr, uint8_t ep, int ls) {
    (void)cid; (void)addr; (void)ep; (void)ls;
    usb_printk("usb_mouse_periodic_init: ctrl=%d addr=%d ep=0x%02x ls=%d\n",
               cid, addr, ep, ls);
    return 0;  /* OK â€?actual transfer set up by init_hid */
}

/* ================================================================
 * usb_mouse_periodic_poll â€?Read latest mouse report from the HID
 * device that was set up by init_hid().  The polling task
 * (usb_task_func) handles the actual EHCI transfer; we just read
 * the last-known-good data from g_first_mouse_hid->buf.
 * ================================================================ */
int usb_mouse_periodic_poll(uint8_t *report) {
    extern struct usb_hid_dev_t *g_first_mouse_hid;
    struct usb_hid_dev_t *hid = g_first_mouse_hid;
    if (!hid || !hid->new_data)
        return 0;

    memcpy(report, hid->buf, hid->report_len ? hid->report_len : 4);
    hid->new_data = 0;
    return hid->report_len ? hid->report_len : 4;
}

int usb_mouse_is_absolute(void) {
    extern struct usb_hid_dev_t *g_first_mouse_hid;
    struct usb_hid_dev_t *hid = g_first_mouse_hid;
    return hid && hid->mouse_absolute;
}

int usb_mouse_get_absolute(int *x, int *y, uint8_t *buttons) {
    extern struct usb_hid_dev_t *g_first_mouse_hid;
    struct usb_hid_dev_t *hid = g_first_mouse_hid;
    if (!hid || !hid->mouse_absolute)
        return 0;
    if (x) *x = hid->abs_x;
    if (y) *y = hid->abs_y;
    if (buttons) *buttons = hid->buf[0] & 0x07;
    return 1;
}

int usb_hcd_scan_ports(int cid) {
    (void)cid;
    xhci_poll();
    return 0;
}

int usb_hcd_scan_ehci_ports(void) {
    xhci_poll();
    return 0;
}

int usb_hcd_get_device_speed(int cid) {
    (void)cid;
    return USB_SPEED_FULL;
}

int usb_hcd_poll_hotplug(int cid) {
    if (usb_get_init_state() != 4)
        return 0;
    (void)cid;
    xhci_poll();
    usb_hub_poll();
    return 0;
}
