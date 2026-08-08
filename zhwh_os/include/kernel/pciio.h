/**
 * @file pciio.h
 */
#ifndef KERNEL_PCIIO_H
#define KERNEL_PCIIO_H

#include <stdint.h>
#include <io.h>
#include <pci.h>

/* PCI BAR I/O types */
#define PCI_IOTYPE_INVALID  0
#define PCI_IOTYPE_IO       1
#define PCI_IOTYPE_MMIO     2

/* PCI BAR descriptor */
struct pci_bar_t {
    uintptr_t base;
    uint32_t  iosize;
    int       iotype;   /* PCI_IOTYPE_IO or PCI_IOTYPE_MMIO */
};

/* pci_write_config_byte stub is in usb_hci_stubs.c */

/* PCI config access wrappers */
#define pci_config_read(dev, reg) \
    pci_read_config_byte((dev)->bus_id, (dev)->dev_id, (dev)->fn_id, (reg))
#define pci_config_read_word(dev, reg) \
    pci_read_config_word((dev)->bus_id, (dev)->dev_id, (dev)->fn_id, (reg))
#define pci_config_read_long(dev, reg) \
    pci_read_config_dword((dev)->bus_id, (dev)->dev_id, (dev)->fn_id, (reg))
#define pci_config_write(dev, reg, val) \
    pci_write_config_byte((dev)->bus_id, (dev)->dev_id, (dev)->fn_id, (reg), (val))
#define pci_config_write_word(dev, reg, val) \
    pci_write_config_word((dev)->bus_id, (dev)->dev_id, (dev)->fn_id, (reg), (val))
#define pci_config_write_long(dev, reg, val) \
    pci_write_config_dword((dev)->bus_id, (dev)->dev_id, (dev)->fn_id, (reg), (val))

/* PCI device capability helpers */
#define pci_read_cap_byte(dev, cap_ptr, reg) \
    pci_read_config_byte((dev)->bus_id, (dev)->dev_id, (dev)->fn_id, (cap_ptr) + (reg))
#define pci_read_cap_word(dev, cap_ptr, reg) \
    pci_read_config_word((dev)->bus_id, (dev)->dev_id, (dev)->fn_id, (cap_ptr) + (reg))
#define pci_read_cap_long(dev, cap_ptr, reg) \
    pci_read_config_dword((dev)->bus_id, (dev)->dev_id, (dev)->fn_id, (cap_ptr) + (reg))
#define pci_write_cap_byte(dev, cap_ptr, reg, val) \
    pci_write_config_byte((dev)->bus_id, (dev)->dev_id, (dev)->fn_id, (cap_ptr) + (reg), (val))
#define pci_write_cap_word(dev, cap_ptr, reg, val) \
    pci_write_config_word((dev)->bus_id, (dev)->dev_id, (dev)->fn_id, (cap_ptr) + (reg), (val))

typedef struct {
    uint16_t     vendor_id, device_id;
    uint16_t     command, status;
    uint8_t      revision_id, prog_if, subclass, class;
    uint8_t      cache_line_sz, latency_timer, header_type, bist;
    uint32_t     bar[6];
    uint32_t     cardbus_cis_ptr;
    uint16_t     subsys_vendor_id, subsys_id;
    uint32_t     expansion_rom_addr;
    uint8_t      capabilities, resvd1[3];
    uint32_t     resvd2;
    uint8_t      interrupt_line, interrupt_pin, min_grant, max_latency;
    uint8_t      unit;
    uint8_t      irq[4];
    uint16_t     bus_id;
    uint16_t     dev_id;
    uint16_t     fn_id;
} pci_compat_t;

#define PCI_DEV_BUS(p)       ((p)->bus_id)
#define PCI_DEV_DEV(p)       ((p)->dev_id)
#define PCI_DEV_FN(p)        ((p)->fn_id)
#define PCI_DEV_PROGIF(p)    ((p)->prog_if)
#define PCI_DEV_CLASS(p)     ((p)->class)
#define PCI_DEV_SUBCLASS(p)  ((p)->subclass)
#define PCI_DEV_IRQ(p)       ((p)->interrupt_line)

/* PCI bus master / memory space enable */
static inline void pci_enable_busmastering(pci_compat_t *dev) {
    uint16_t cmd = pci_config_read_word(dev, 0x04);
    pci_config_write_word(dev, 0x04, cmd | 0x04);
}

static inline void pci_enable_memoryspace(pci_compat_t *dev) {
    uint16_t cmd = pci_config_read_word(dev, 0x04);
    pci_config_write_word(dev, 0x04, cmd | 0x02);
}

static inline void pci_enable_interrupts(pci_compat_t *dev) {
    uint16_t cmd = pci_config_read_word(dev, 0x04);
    pci_config_write_word(dev, 0x04, cmd & ~0x0400);
}

/* Build pci_compat_t from raw bus/dev/fn (bypass pci_dev_t) */
static inline void pci_to_compat_raw(unsigned bus, unsigned dev, unsigned fn, pci_compat_t *dst) {
    uint32_t vd = pci_read_config_dword(bus, dev, fn, 0x00);
    dst->vendor_id = vd & 0xFFFF;
    dst->device_id = (vd >> 16) & 0xFFFF;
    dst->command   = pci_read_config_word(bus, dev, fn, 0x04);
    dst->status    = pci_read_config_word(bus, dev, fn, 0x06);
    dst->revision_id = pci_read_config_byte(bus, dev, fn, 0x08);
    dst->prog_if   = pci_read_config_byte(bus, dev, fn, 0x09);
    dst->subclass  = pci_read_config_byte(bus, dev, fn, 0x0A);
    dst->class     = pci_read_config_byte(bus, dev, fn, 0x0B);
    dst->cache_line_sz = pci_read_config_byte(bus, dev, fn, 0x0C);
    dst->latency_timer = pci_read_config_byte(bus, dev, fn, 0x0D);
    dst->header_type = pci_read_config_byte(bus, dev, fn, 0x0E);
    dst->bist      = pci_read_config_byte(bus, dev, fn, 0x0F);
    dst->interrupt_line = pci_read_config_byte(bus, dev, fn, 0x3C);
    dst->interrupt_pin  = pci_read_config_byte(bus, dev, fn, 0x3D);
    dst->bus_id = bus;
    dst->dev_id = dev;
    dst->fn_id  = fn;
    dst->unit = 0;
    dst->irq[0] = dst->irq[1] = dst->irq[2] = dst->irq[3] = 0;
}

/* Get BAR info from raw bus/dev/fn */
static inline void pci_get_bar_info_raw(unsigned bus, unsigned dev, unsigned fn, struct pci_bar_t *bar_out) {
    int i;
    for (i = 0; i < 6; i++) {
        bar_out[i].iotype = PCI_IOTYPE_INVALID;
        bar_out[i].base = 0;
        bar_out[i].iosize = 0;
    }
    for (i = 0; i < 6; i++) {
        uint8_t reg = 0x10 + i * 4;
        uint32_t bar_val = pci_read_config_dword(bus, dev, fn, reg);
        if (bar_val == 0 || bar_val == 0xFFFFFFFF) {
            bar_out[i].iotype = PCI_IOTYPE_INVALID;
            bar_out[i].base = 0;
            bar_out[i].iosize = 0;
            continue;
        }
        if (bar_val & 0x01) {
            pci_write_config_dword(bus, dev, fn, reg, 0xFFFFFFFF);
            uint32_t bar_sz = pci_read_config_dword(bus, dev, fn, reg);
            pci_write_config_dword(bus, dev, fn, reg, bar_val);
            bar_out[i].iotype = PCI_IOTYPE_IO;
            bar_out[i].base = bar_val & ~0x03;
            bar_out[i].iosize = (~(bar_sz & ~0x03) + 1) & 0xFFFF;
        } else if ((bar_val & 0x06) == 0x04 && i < 5) {
            uint32_t bar_hi = pci_read_config_dword(bus, dev, fn, reg + 4);
            uint32_t size_lo, size_hi;
            uint64_t base64, mask64, size64;

            pci_write_config_dword(bus, dev, fn, reg, 0xFFFFFFFF);
            pci_write_config_dword(bus, dev, fn, reg + 4, 0xFFFFFFFF);
            size_lo = pci_read_config_dword(bus, dev, fn, reg);
            size_hi = pci_read_config_dword(bus, dev, fn, reg + 4);
            pci_write_config_dword(bus, dev, fn, reg + 4, bar_hi);
            pci_write_config_dword(bus, dev, fn, reg, bar_val);

            base64 = ((uint64_t)bar_hi << 32) | (bar_val & ~0xFULL);
            mask64 = ((uint64_t)size_hi << 32) | (size_lo & ~0xFULL);
            size64 = ~mask64 + 1;
            if ((base64 >> 32) == 0 && size64 != 0 && (size64 >> 32) == 0) {
                bar_out[i].iotype = PCI_IOTYPE_MMIO;
                bar_out[i].base = (uintptr_t)base64;
                bar_out[i].iosize = (uint32_t)size64;
            }
            /* BAR i+1 is the high dword, never a separate BAR. */
            i++;
        } else {
            pci_write_config_dword(bus, dev, fn, reg, 0xFFFFFFFF);
            uint32_t bar_sz = pci_read_config_dword(bus, dev, fn, reg);
            pci_write_config_dword(bus, dev, fn, reg, bar_val);
            bar_out[i].iotype = PCI_IOTYPE_MMIO;
            bar_out[i].base = bar_val & ~0x0F;
            bar_out[i].iosize = ~(bar_sz & ~0x0F) + 1;
        }
    }
}

/* Convert Hillson pci_dev_t to pci_compat_t */
static inline void pci_to_compat(pci_dev_t *src, pci_compat_t *dst) {
    dst->vendor_id = src->header.vendor_id;
    dst->device_id = src->header.device_id;
    dst->command   = src->header.command;
    dst->status    = src->header.status;
    dst->revision_id = src->header.revision_id;
    dst->prog_if   = src->header.prog_if;
    dst->subclass  = src->header.subclass;
    dst->class     = src->header.class;
    dst->cache_line_sz = src->header.cache_line_sz;
    dst->latency_timer = src->header.latency_timer;
    dst->header_type = src->header.header_type;
    dst->bist      = src->header.bist;
    dst->interrupt_line = src->header.u.h00.interrupt_line;
    dst->interrupt_pin  = src->header.u.h00.interrupt_pin;
    dst->bus_id = src->bus_id;
    dst->dev_id = src->dev_id;
    dst->fn_id  = src->fn_id;
    dst->unit = 0;
    dst->irq[0] = dst->irq[1] = dst->irq[2] = dst->irq[3] = 0;
}

/*
 * pcidev_inl/outl etc. — MMIO or I/O port access based on HC type.
 *
 * UHCI typically uses I/O ports (inb/inw/inl/outb/outw/outl).
 * EHCI and OHCI typically use MMIO (memory-mapped I/O).
 *
 * For MMIO, the base address is a virtual address, and we use volatile
 * pointer dereferences. For I/O ports, we use the in/out functions.
 *
 * Each HC driver stores its base and a flag (mmio=1 for MMIO, =0 for I/O).
 */

/* MMIO read/write macros (volatile pointer access) */
#define mmio_read32(addr)   (*(volatile uint32_t *)(uintptr_t)(addr))
#define mmio_read16(addr)   (*(volatile uint16_t *)(uintptr_t)(addr))
#define mmio_read8(addr)    (*(volatile uint8_t  *)(uintptr_t)(addr))
#define mmio_write32(addr, val) (*(volatile uint32_t *)(uintptr_t)(addr) = (val))
#define mmio_write16(addr, val) (*(volatile uint16_t *)(uintptr_t)(addr) = (val))
#define mmio_write8(addr, val)  (*(volatile uint8_t  *)(uintptr_t)(addr) = (val))

/*
 * Per-controller I/O access macros.
 * Called as: pcidev_inl(hc_dev, reg_offset)
 * where hc_dev is an HC-specific struct with iobase and mmio fields.
 *
 * These are used like:
 *   pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBCMD)
 *   pcidev_outl(uhci, UHCI_REG_USBCMD, val)
 *   pcidev_inw(uhci, UHCI_REG_PORTSC1)
 */

/* Input macros return a value */
#define pcidev_inl(hc, reg) \
    ((hc)->mmio ? mmio_read32((hc)->iobase + (reg)) : inl((hc)->iobase + (reg)))
#define pcidev_inw(hc, reg) \
    ((hc)->mmio ? mmio_read16((hc)->iobase + (reg)) : inw((hc)->iobase + (reg)))
#define pcidev_inb(hc, reg) \
    ((hc)->mmio ? mmio_read8((hc)->iobase + (reg)) : inb((hc)->iobase + (reg)))

/* Output macros are statements only (use do-while) */
#define pcidev_outl(hc, reg, val) do { \
    if ((hc)->mmio) mmio_write32((hc)->iobase + (reg), (val)); \
    else outl(((hc)->iobase + (reg)), (val)); \
} while(0)
#define pcidev_outw(hc, reg, val) do { \
    if ((hc)->mmio) mmio_write16((hc)->iobase + (reg), (val)); \
    else outw(((hc)->iobase + (reg)), (val)); \
} while(0)
#define pcidev_outb(hc, reg, val) do { \
    if ((hc)->mmio) mmio_write8((hc)->iobase + (reg), (val)); \
    else outb(((hc)->iobase + (reg)), (val)); \
} while(0)

/*
 * pci_get_bar_info() — extract BAR information from a PCI device.
 * This is used by usb_init() to get MMIO base/size for each BAR.
 */
static inline void pci_get_bar_info(pci_dev_t *pci, struct pci_bar_t *bar_out) {
    uint32_t bar_val, bar_sz;
    int i;

    for (i = 0; i < 6; i++) {
        uint8_t reg = 0x10 + (i * 4);
        bar_val = pci_read_config_dword(pci->bus_id, pci->dev_id, pci->fn_id, reg);

        if (bar_val == 0) {
            bar_out[i].iotype = PCI_IOTYPE_INVALID;
            bar_out[i].base = 0;
            bar_out[i].iosize = 0;
            continue;
        }

        /* Write all-ones to determine BAR size */
        pci_write_config_dword(pci->bus_id, pci->dev_id, pci->fn_id, reg, 0xFFFFFFFF);
        bar_sz = pci_read_config_dword(pci->bus_id, pci->dev_id, pci->fn_id, reg);
        /* Restore original value */
        pci_write_config_dword(pci->bus_id, pci->dev_id, pci->fn_id, reg, bar_val);

        if (bar_val & 0x01) {
            /* I/O space BAR */
            bar_out[i].iotype = PCI_IOTYPE_IO;
            bar_out[i].base = bar_val & ~0x03;
            bar_out[i].iosize = (~(bar_sz & ~0x03) + 1) & 0xFFFF;
        } else {
            /* Memory space BAR */
            bar_out[i].iotype = PCI_IOTYPE_MMIO;
            bar_out[i].base = bar_val & ~0x0F;
            bar_out[i].iosize = ~(bar_sz & ~0x0F) + 1;
        }
    }
}

/*
 * Register IRQ handler (stub — we use polling, not interrupts for USB).
 */
static inline int pci_register_irq_handler(pci_dev_t *pci,
                                            void (*handler)(void *, void *),
                                            void *arg) {
    (void)pci; (void)handler; (void)arg;
    return 0;  /* success — handler will never actually be called */
}

#endif /* KERNEL_PCIIO_H */
