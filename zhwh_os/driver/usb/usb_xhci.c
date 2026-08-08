/**
 * @file usb_xhci.c
 * @brief XHCI (USB 3.0+) Host Controller Driver
 *
 * Implements the usb_ops_t vtable for the XHCI controller.
 * Based on eXtensible Host Controller Interface spec rev 1.2.
 * 32-bit only �?upper 32 bits of 64-bit registers are always 0.
 */
#include <kernel/errno.h>
#include <kernel/pciio.h>
#include <kernel/asm.h>
#include <kernel/usb.h>
#include <kernel/usb_hid.h>
#include <kernel/usb_xhci.h>
#include <kernel/ata.h>
#include <kernel/hillsonos.h>
#include <mm/kheap.h>
#include <string.h>

static void xhci_status_regs(const char *stage, const char *fmt,
                             uint32_t a, uint32_t b, uint32_t c);

/* ================================================================
 * 64-bit MMIO helpers (32-bit system: high dword always 0)
 * ================================================================ */
STATIC_INLINE uint64_t xhci_read64(volatile struct xhci_dev_t *xhci, uint32_t reg)
{
    uint32_t lo = mmio_read32(xhci->mmio_base + reg);
    uint32_t hi = mmio_read32(xhci->mmio_base + reg + 4);
    return ((uint64_t)hi << 32) | lo;
}

STATIC_INLINE void xhci_mmio_barrier(void)
{
    __asm__ volatile("mfence" : : : "memory");
}

STATIC_INLINE void xhci_write64(volatile struct xhci_dev_t *xhci, uint32_t reg, uint64_t val)
{
    /* Match the reference driver's Write*Reg32 pairs: low dword first,
     * high dword second, with no MMIO read between the two stores. */
    mmio_write32(xhci->mmio_base + reg, (uint32_t)val);
    xhci_mmio_barrier();
    mmio_write32(xhci->mmio_base + reg + 4, (uint32_t)(val >> 32));
    xhci_mmio_barrier();
}

static uint64_t xhci_write64_checked(volatile struct xhci_dev_t *xhci,
                                     uint32_t reg, uint64_t val, uint64_t mask)
{
    uint64_t got;

    xhci_write64(xhci, reg, val);
    got = xhci_read64(xhci, reg);
    if ((got & mask) != (val & mask))
        usb_printk("xhci: 64-bit register readback differs reg=%x want=%x got=%x\n",
                   reg, (uint32_t)val, (uint32_t)got);
    return got;
}

static uint64_t xhci_program_crcr(volatile struct xhci_dev_t *xhci,
                                  uint64_t val)
{
    /* Use raw operational-register addresses here so a broken 64-bit helper
     * cannot hide whether the controller accepted either dword. */
    uintptr_t lo_addr = xhci->op_base + XHCI_OP_CRCR_LO;
    uintptr_t hi_addr = xhci->op_base + XHCI_OP_CRCR_HI;
    uint32_t lo = (uint32_t)val;
    uint32_t hi = (uint32_t)(val >> 32);

    /* Complete the split 64-bit update before reading CRCR back.  A read
     * between the low and high writes can interrupt the pending update on
     * controllers which commit the pair when the high dword is written. */
    mmio_write32(lo_addr, lo);
    xhci_mmio_barrier();
    mmio_write32(hi_addr, hi);
    xhci_mmio_barrier();

    /* Flush the completed pair and then perform diagnostic readback. */
    lo = mmio_read32(lo_addr);
    hi = mmio_read32(hi_addr);
    usb_printk("xhci: CRCR raw lo@%x=%x hi@%x=%x\n",
               (uint32_t)lo_addr, lo, (uint32_t)hi_addr, hi);
    xhci_status_regs("xhci CRCR raw", "lo=%x hi=%x addr=%x",
                     lo, hi, (uint32_t)lo_addr);

    return ((uint64_t)hi << 32) | lo;
}

static uint64_t xhci_read_crcr(volatile struct xhci_dev_t *xhci)
{
    return xhci_read64(xhci, xhci->caplen + XHCI_OP_CRCR_LO);
}

/* ================================================================
 * Forward declarations
 * ================================================================ */
static int xhci_enable_slot(volatile struct xhci_dev_t *xhci);
static int xhci_disable_slot(volatile struct xhci_dev_t *xhci, int slot_id);
static int xhci_address_device(volatile struct xhci_dev_t *xhci, int slot_id,
                               uint8_t port, uint8_t speed, int bsr);
static int xhci_configure_endpoint(volatile struct xhci_dev_t *xhci, int slot_id, uint8_t ep_addr, uint8_t ep_type, uint16_t mps, uint8_t interval);
static int xhci_alloc_transfer_ring(volatile struct xhci_dev_t *xhci, int slot_id, int ep_id);
static void *xhci_alloc_dev_ctx(volatile struct xhci_dev_t *xhci, uint64_t *phys_out);
static int xhci_prepare_slot_context(volatile struct xhci_dev_t *xhci,
                                     int slot_id);
static inline int xhci_ep_addr_to_dci(uint8_t ep_addr);
static uint8_t xhci_interval(uint32_t speed, uint8_t binterval);
static int xhci_find_slot_by_port(volatile struct xhci_dev_t *xhci,
                                  uint32_t port);
extern void kernel_usb_status_line(const char *stage, const char *detail);
extern void kernel_usb_msc_status_line(const char *stage, const char *detail);
extern void kernel_usb_error_line(const char *stage, const char *detail);
extern int snprintf(char *str, unsigned int size, const char *fmt, ...);
extern void usb_poll_interrupts(void);

static void xhci_status_regs(const char *stage, const char *fmt,
                             uint32_t a, uint32_t b, uint32_t c)
{
    char line[96];
    snprintf(line, sizeof(line), fmt, a, b, c);
    kernel_usb_status_line(stage, line);
}

static void xhci_error_regs(const char *stage, const char *fmt,
                            uint32_t a, uint32_t b, uint32_t c)
{
    char line[96];
    snprintf(line, sizeof(line), fmt, a, b, c);
    kernel_usb_error_line(stage, line);
}

static void xhci_dump_trb(const char *stage, const struct xhci_trb_t *trb)
{
    char line[96];
    const uint8_t *p = (const uint8_t *)trb;
    snprintf(line, sizeof(line),
             "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
             p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
             p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
    kernel_usb_status_line(stage, line);
}


/* ================================================================
 * Global linked list
 * ================================================================ */
static volatile struct xhci_dev_t *first_xhci = NULL;
static uint64_t xhci_last_cmd_phys;
static uint64_t xhci_last_completion_phys;
static uint8_t xhci_last_completion_slot;
static uint32_t xhci_last_cmd_control;

/* Physical-machine diagnostics: solve the low-speed HID device first. */
#define XHCI_ENUM_SLOT_LIMIT 2
#define XHCI_MOUSE_ONLY      0

static void xhci_parse_supported_protocols(volatile struct xhci_dev_t *xhci,
                                           uint32_t hccparams1)
{
    uint32_t off = ((hccparams1 >> 16) & 0xFFFF) * 4;
    unsigned int guard = 64;

    xhci->usb2_ports = 0;
    xhci->usb3_ports = 0;
    while (off && guard--) {
        uint32_t cap = mmio_read32(xhci->mmio_base + off);
        uint8_t id = cap & 0xFF;
        uint8_t next = (cap >> 8) & 0xFF;

        if (id == 2) { /* Supported Protocol Capability */
            uint8_t major = (cap >> 24) & 0xFF;
            uint32_t ports = mmio_read32(xhci->mmio_base + off + 8);
            uint32_t start = ports & 0xFF;
            uint32_t count = (ports >> 8) & 0xFF;
            if (start && count) {
                if (major == 2) {
                    xhci->usb2_port_start = start - 1;
                    xhci->usb2_ports = count;
                } else if (major >= 3) {
                    xhci->usb3_port_start = start - 1;
                    xhci->usb3_ports = count;
                }
            }
        }
        if (!next)
            break;
        off += (uint32_t)next * 4;
    }
}

static inline uint32_t xhci_ctx_dwords(volatile struct xhci_dev_t *xhci)
{
    return xhci->context_size / sizeof(uint32_t);
}

static inline uint32_t *xhci_input_slot_ctx(volatile struct xhci_dev_t *xhci,
                                             uint32_t *input)
{
    return input + xhci_ctx_dwords(xhci);
}

static inline uint32_t *xhci_input_ep_ctx(volatile struct xhci_dev_t *xhci,
                                           uint32_t *input, int dci)
{
    return input + (dci + 1) * xhci_ctx_dwords(xhci);
}

static inline uint32_t *xhci_output_ep_ctx(volatile struct xhci_dev_t *xhci,
                                            uint32_t *output, int dci)
{
    return output + dci * xhci_ctx_dwords(xhci);
}
static struct usb_ops_t xhci_ops;
/* Set by the asynchronous front-end after it has completed HC reset. */
static int xhci_reset_precompleted;

void xhci_mark_reset_complete(void)
{
    xhci_reset_precompleted = 1;
}

/* Give the OS ownership of xHCI when firmware exposed the USB Legacy
 * Support capability. The xECP field is a dword offset from CAPBASE. */
static int xhci_handoff_legacy(volatile struct xhci_dev_t *xhci,
                               uint32_t hccparams1)
{
    uint32_t off = HCCPARAMS1_XECP(hccparams1) << 2;
    int n;

    usb_printk("xhci: xECP=%x first_cap=%x\n", HCCPARAMS1_XECP(hccparams1), off);
    xhci_status_regs("xhci xECP", "xecp=%x first=%x hcc=%x",
                     HCCPARAMS1_XECP(hccparams1), off, hccparams1);

    for (n = 0; off && n < 256; n++) {
        uint32_t cap = mmio_read32(xhci->mmio_base + off);
        uint8_t id = cap & 0xFF;
        uint32_t next = (cap >> 8) & 0xFF;

        usb_printk("xhci: extcap[%d] off=%x id=%x next=%x\n",
                   n, off, id, next);
        // if (n < 4)
        //     xhci_status_regs("xhci extcap", "off=%x id=%x next=%x",
        //                      off, id, next);

        if (id == 1) { /* USB Legacy Support */
            uint32_t legsup = cap;
            uint32_t legctl = mmio_read32(xhci->mmio_base + off + 4);
            const uint32_t BIOS_OWNED = 1u << 16;
            const uint32_t OS_OWNED = 1u << 24;

            usb_printk("xhci: legacy cap=%x legsup=%x legctl=%x\n",
                       off, legsup, legctl);
            xhci_status_regs("xhci legacy", "off=%x sup=%x ctl=%x",
                             off, legsup, legctl);
            if (legsup & BIOS_OWNED) {
                mmio_write32(xhci->mmio_base + off, legsup | OS_OWNED);
                xhci_mmio_barrier();
                for (int wait = 0; wait < 1000; wait++) {
                    legsup = mmio_read32(xhci->mmio_base + off);
                    if (!(legsup & BIOS_OWNED)) break;
                    tick_delay(1000);
                }
                if (legsup & BIOS_OWNED) {
                    usb_printk("xhci: BIOS ownership did not clear\n");
                    kernel_usb_status_line("xhci", ": BIOS still owns controller.\n");
                    return -ETIMEDOUT;
                }
            }

            /* Keep OS ownership asserted and disable legacy SMI enables. */
            legsup = mmio_read32(xhci->mmio_base + off);
            mmio_write32(xhci->mmio_base + off, legsup | OS_OWNED);
            xhci_mmio_barrier();
            mmio_write32(xhci->mmio_base + off + 4, 0);
            xhci_mmio_barrier();
            legsup = mmio_read32(xhci->mmio_base + off);
            legctl = mmio_read32(xhci->mmio_base + off + 4);
            xhci_status_regs("xhci legacy", "after=%x ctl=%x bios=%x",
                             legsup, legctl, !!(legsup & BIOS_OWNED));
            return 0;
        }
        if (!next) break;
        off += next << 2;
    }
    return 0;
}

/* ================================================================
 * DCI (Device Context Index) helpers
 * ================================================================ */

/* Get the expected DCI for a transfer.
 * For EP0 (control), returns 1. For non-EP0, computes DCI from
 * endpoint number + direction.                                     */
static inline int xhci_transfer_ep_id(struct usb_transfer_t *transfer)
{
    if (transfer->endpoint && transfer->endpoint->addr != 0) {
        int dci = transfer->endpoint->addr * 2;
        if (transfer->endpoint->direction == USB_ENDPOINT_IN)
            dci |= 1;
        return dci;
    }
    return 1;  /* EP0 Default Control */
}

/* ================================================================
 * Utility functions
 * ================================================================ */
static unsigned int xhci_get_next_addr(void *__xhci)
{
    struct xhci_dev_t *xhci = __xhci;
    volatile unsigned int i, j;
    for (i = 0; i < ((XHCI_MAX_SLOTS / 32) + 1); i++) {
        for (j = 1; j < 32; j++) {  /* skip bit 0 (slot 0 is reserved) */
            if (!(xhci->addr_bitmap[i] & (1 << j))) {
                xhci->addr_bitmap[i] |= (1 << j);
                return (i * 32) + j;
            }
        }
    }
    return 0;
}

static void xhci_free_addr(void *__xhci, unsigned int slot_id)
{
    struct xhci_dev_t *xhci = __xhci;
    struct xhci_ring_t *ep_rings;

    if (slot_id < 1 || slot_id > (unsigned int)xhci->max_slots) return;
    xhci->addr_bitmap[slot_id / 32] &= ~(1 << (slot_id % 32));

    if (xhci->dcbaa) {
        xhci->dcbaa[slot_id] = 0;
        xhci_mmio_barrier();
    }

    /* Clear slot state to prevent stale context on reuse */
    ep_rings = xhci->slots[slot_id].ep_rings;
    A_memset(&xhci->slots[slot_id], 0, sizeof(xhci->slots[0]));
    xhci->slots[slot_id].ep_rings = ep_rings;
    if (ep_rings)
        A_memset(ep_rings, 0,
                 XHCI_MAX_ENDPOINTS * sizeof(struct xhci_ring_t));
}

struct usb_dev_t *xhci_get_dev_struct(pci_compat_t *bus, uint8_t num)
{
    volatile struct xhci_dev_t *xhci = first_xhci;
    if (!bus || num < 1) return NULL;
    while (xhci) {
        if (xhci->pci.bus_id == bus->bus_id &&
            xhci->pci.dev_id == bus->dev_id &&
            xhci->pci.fn_id == bus->fn_id) {
            if (num > xhci->max_slots) return NULL;
            return xhci->slots[num].usb;
        }
        xhci = xhci->next;
    }
    return NULL;
}

/* ================================================================
 * TRB Ring Management
 * ================================================================ */
static int xhci_ring_init(struct xhci_ring_t *ring, uint32_t num_trbs)
{
    uint32_t size = num_trbs * sizeof(struct xhci_trb_t);
    uint32_t dma_handle;

    void *virt = dma_alloc_coherent(size, &dma_handle);
    if (!virt)
        return -ENOMEM;

    A_memset(virt, 0, size);

    ring->ring = (struct xhci_trb_t *)virt;
    ring->phys = (uint64_t)dma_handle;
    ring->size = num_trbs;

    ring->enqueue = 0;
    ring->dequeue = 0;
    ring->ccs     = 1;  /* Start cycle=1, HC internal CCS also starts at 1 */

    /* Pre-fill permanent Link TRB at ring[size-1].
     * TRB_TC is CRITICAL: tells HC to toggle its internal cycle state
     * when following the link. Without it, HC cycle �?our ccs �?HC skips
     * all TRBs after the first wrap. */
    {
        struct xhci_trb_t *link = &ring->ring[num_trbs - 1];
        link->param   = ring->phys;
        link->status  = 0;
        link->control = TRB_TYPE(TRB_TYPE_LINK)    /* Link TRB type       */
                      | TRB_TC                      /* Toggle Cycle: MUST  */
                      | TRB_CYCLE;                  /* Cycle bit = 1       */
    }

    return 0;
}
/* Enqueue a TRB onto a producer ring. Returns index, or -1 if full.
 * The last slot (ring[size-1]) is a permanent Link TRB �?never overwrite it. */
static int ring_enqueue(struct xhci_ring_t *ring, struct xhci_trb_t *trb)
{
    /* If we're at the Link TRB slot, wrap to beginning and toggle cycle */
    if (ring->enqueue == ring->size - 1) {
        ring->ring[ring->size - 1].control =
            TRB_TYPE(TRB_TYPE_LINK) | TRB_TC | (ring->ccs & 0x1);
        ring->enqueue = 0;
        ring->ccs ^= 1;
    }

    /* Full check: next slot must not be dequeue */
    uint32_t next = ring->enqueue + 1;
    if (next == ring->dequeue)
        return -1;

    int idx = ring->enqueue;
    A_memcpy(&ring->ring[idx], trb, sizeof(struct xhci_trb_t));
    ring->ring[idx].control =
        (ring->ring[idx].control & ~TRB_CYCLE) | (ring->ccs & 0x1);
    ring->enqueue = next;

    return idx;
}

/* ================================================================
 * Event Ring Management
 * ================================================================ */

/* Read the next event TRB from the event ring. Returns 1 if an event
 * was available, 0 if not, -1 on error. */
static int xhci_get_event(volatile struct xhci_dev_t *xhci,
                          struct xhci_trb_t *event)
{
    struct xhci_trb_t *trb = &xhci->event_ring.ring[xhci->event_ring.dequeue];

    /* Check if HC has written a valid TRB (cycle bit matches CCS) */
    uint8_t trb_cycle = trb->control & 0x1;
    if (trb_cycle != xhci->event_ring.ccs) return 0;

    A_memcpy(event, (void *)trb, sizeof(struct xhci_trb_t));

    /* Advance dequeue */
    xhci->event_ring.dequeue++;
    if (xhci->event_ring.dequeue >= xhci->event_ring.size) {
        xhci->event_ring.dequeue = 0;
        xhci->event_ring.ccs ^= 1;
    }

    /* Update ERDP (Event Ring Dequeue Pointer) */
    uint64_t erdp = xhci->event_ring.phys +
                    (xhci->event_ring.dequeue * sizeof(struct xhci_trb_t));
    erdp |= (1 << 3);  /* Set EHB (Event Handler Busy) to clear */
    xhci_write64(xhci, XHCI_RT_ERDP_LO(xhci->rt_base - xhci->mmio_base), erdp);

    return 1;
}

static uint32_t xhci_event_ready_count(volatile struct xhci_dev_t *xhci)
{
    uint32_t count = 0;
    uint32_t index = xhci->event_ring.dequeue;
    uint8_t cycle = xhci->event_ring.ccs;

    while (count < xhci->event_ring.size) {
        struct xhci_trb_t *trb = &xhci->event_ring.ring[index];
        if ((trb->control & TRB_CYCLE) != cycle)
            break;
        count++;
        if (++index == xhci->event_ring.size) {
            index = 0;
            cycle ^= 1;
        }
    }
    return count;
}

/* Wait for command completion, return completion code */
static int xhci_wait_cmd(volatile struct xhci_dev_t *xhci, int timeout_ms)
{
    volatile int timeout = timeout_ms * 1000;  /* Convert to tick_delay units */
    struct xhci_trb_t event;

    while (timeout--) {
        if (xhci_get_event(xhci, &event) > 0) {
            uint8_t trb_type = (event.control >> TRB_TRB_TYPE_SHIFT) & 0x3F;
            if (trb_type == TRB_TYPE_CMD_COMP_EVENT) {
                uint64_t completed = event.param & ~0xFULL;
                if (completed != (xhci_last_cmd_phys & ~0xFULL)) {
                    usb_printk("xhci: ignoring stale command completion ptr=%x expected=%x\n",
                               (uint32_t)completed,
                               (uint32_t)xhci_last_cmd_phys);
                    continue;
                }
                xhci_last_completion_phys = completed;
                xhci_last_completion_slot =
                    (uint8_t)((event.control >> 24) & 0xFF);
                uint8_t cc = (event.status >> 24) & 0xFF;
                return cc;
            }
            /* Other events (transfer, port status) �?ignore for now */
        }
        tick_delay(1);
    }
    usb_printk("%s: command timeout\n", "xhci");
    return -ETIMEDOUT;
}

/* Post a command TRB to the Command Ring, ring doorbell.
 * ring_enqueue() handles Link TRB wrapping internally (toggle ccs on wrap).
 * Completion is polled separately by the caller. */
static int xhci_post_cmd(volatile struct xhci_dev_t *xhci, struct xhci_trb_t *cmd)
{
    int idx = ring_enqueue(&xhci->cmd_ring, cmd);
    if (idx < 0)
        return idx;

    xhci_last_cmd_phys = xhci->cmd_ring.phys +
        (uint64_t)idx * sizeof(struct xhci_trb_t);
    xhci_last_cmd_control =
        xhci->cmd_ring.ring[idx].control;
    xhci_mmio_barrier();

    /* Ring Command Doorbell (DB offset 0, target 0) */
    mmio_write32(xhci->db_base, 0);
    mmio_read32(xhci->db_base);  /* flush */

    return 0;
}

/* Recover the host-controller side of a halted endpoint. The USB-side
 * ENDPOINT_HALT must be cleared separately before this function is called. */
static int xhci_reset_endpoint_ring(volatile struct xhci_dev_t *xhci,
                                    int slot_id, int ep_id)
{
    struct xhci_ring_t *ring = &xhci->slots[slot_id].ep_rings[ep_id];
    struct xhci_trb_t cmd;
    int cc;

    A_memset(&cmd, 0, sizeof(cmd));
    cmd.control = TRB_TYPE(TRB_TYPE_RESET_EP)
                | ((uint32_t)ep_id << 16)
                | ((uint32_t)slot_id << 24);
    if (xhci_post_cmd(xhci, &cmd) < 0)
        return -ENOMEM;
    cc = xhci_wait_cmd(xhci, 1000);
    if (cc != CC_SUCCESS) {
        usb_printk("xhci: Reset Endpoint failed slot=%d ep=%d cc=%d\n",
                   slot_id, ep_id, cc);
        return -EIO;
    }

    A_memset(ring->ring, 0, ring->size * sizeof(struct xhci_trb_t));
    ring->ring[ring->size - 1].param = ring->phys;
    ring->ring[ring->size - 1].control =
        TRB_TYPE(TRB_TYPE_LINK) | TRB_TC | TRB_CYCLE;
    ring->enqueue = 0;
    ring->dequeue = 0;
    ring->ccs = 1;

    A_memset(&cmd, 0, sizeof(cmd));
    cmd.param = ring->phys | 1; /* DCS = producer cycle state */
    cmd.control = TRB_TYPE(TRB_TYPE_SET_TR_DEQUEUE)
                | ((uint32_t)ep_id << 16)
                | ((uint32_t)slot_id << 24);
    xhci_post_cmd(xhci, &cmd);
    cc = xhci_wait_cmd(xhci, 1000);
    if (cc != CC_SUCCESS) {
        usb_printk("xhci: Set TR Dequeue failed slot=%d ep=%d cc=%d\n",
                   slot_id, ep_id, cc);
        return -EIO;
    }

    usb_printk("xhci: endpoint recovered slot=%d ep=%d dequeue=%x\n",
               slot_id, ep_id, (uint32_t)ring->phys);
    return 0;
}

/* ================================================================
 * Controller Initialization
 * ================================================================ */

static int xhci_reset_bak(volatile struct xhci_dev_t *xhci)
{
    volatile uint32_t dword;
    volatile int timeout;

    /* Stop the HC */
    dword = mmio_read32(xhci->op_base + XHCI_OP_USBCMD);
    dword &= ~USBCMD_RS;
    mmio_write32(xhci->op_base + XHCI_OP_USBCMD, dword);
    xhci_mmio_barrier();
    (void)mmio_read32(xhci->op_base + XHCI_OP_USBCMD);

    /* Wait for HCHalted */
    timeout = 100;
    while (timeout--) {
        dword = mmio_read32(xhci->op_base + XHCI_OP_USBSTS);
        if (dword & USBSTS_HCH) break;
        tick_delay(100);
    }
    if (timeout <= 0) {
        usb_printk("%s: stop timeout\n", "xhci");
        return -ETIMEDOUT;
    }
    usb_printk("%s: HC stopped\n", "xhci");

    /* Reset HC */
    usb_printk("%s: resetting HC...\n", "xhci");
    mmio_write32(xhci->op_base + XHCI_OP_USBCMD, USBCMD_HCRST);
    xhci_mmio_barrier();
    (void)mmio_read32(xhci->op_base + XHCI_OP_USBCMD);

    timeout = 200;
    while (timeout--) {
        dword = mmio_read32(xhci->op_base + XHCI_OP_USBCMD);
        if (!(dword & USBCMD_HCRST)) break;
        tick_delay(100);
    }
    if (timeout <= 0) {
        usb_printk("%s: reset timeout\n", "xhci");
        return -ETIMEDOUT;
    }

    /* Wait for CNR (Controller Not Ready) to clear */
    timeout = 200;
    while (timeout--) {
        dword = mmio_read32(xhci->op_base + XHCI_OP_USBSTS);
        if (!(dword & USBSTS_CNR)) break;
        tick_delay(100);
    }
    if (timeout <= 0) {
        usb_printk("%s: CNR timeout\n", "xhci");
        return -ETIMEDOUT;
    }

    /* Clear reset-time write-1-to-clear status bits before programming rings. */
    mmio_write32(xhci->op_base + XHCI_OP_USBSTS,
                 USBSTS_HSE | USBSTS_EINT | USBSTS_PCD);
    xhci_mmio_barrier();
    dword = mmio_read32(xhci->op_base + XHCI_OP_USBSTS);
    xhci_status_regs("xhci reset", "USBCMD=%x USBSTS=%x CONFIG=%x",
                     mmio_read32(xhci->op_base + XHCI_OP_USBCMD),
                     dword,
                     mmio_read32(xhci->op_base + XHCI_OP_CONFIG));
    usb_printk("%s: reset done\n", "xhci");

    return 0;
}

/* 统一�?64 �?MMIO 读取函数 */
static inline uint64_t mmio_read64_op(volatile struct xhci_dev_t *xhci, uint32_t relative_op_offset) {
    uintptr_t addr = xhci->op_base + relative_op_offset;
    
    uint32_t lo = mmio_read32(addr);
    uint32_t hi = mmio_read32(addr + 4);

    return ((uint64_t)hi << 32) | lo;
}

static int xhci_reset(volatile struct xhci_dev_t *xhci)
{
    uint32_t dword;  // 移除 volatile
    int timeout;     // 移除 volatile

    /* 确保 R/S (Run/Stop) �?0 */
    uint32_t cmd = mmio_read32(xhci->op_base + XHCI_OP_USBCMD);
    if (cmd & USBCMD_RS) {
        cmd &= ~USBCMD_RS;
        mmio_write32(xhci->op_base + XHCI_OP_USBCMD, cmd);
        xhci_mmio_barrier();

        /* 等待 USBSTS_HCH (Host Controller Halted) 变为 1 */
        timeout = 10000;
        while (!(mmio_read32(xhci->op_base + XHCI_OP_USBSTS) & USBSTS_HCH) && --timeout) {
            tick_delay(1000);
        }
    }

    // xhci_status_regs("xhci: ", "USBCMD_HCRST=%x USBSTS_CNR1=%x USBSTS_HCH=%x",
    //                 mmio_read32(xhci->op_base + XHCI_OP_USBCMD),(mmio_read32(xhci->op_base + XHCI_OP_USBSTS)& USBSTS_CNR), (mmio_read32(xhci->op_base + XHCI_OP_USBSTS)& USBSTS_HCH));


    /* Step 1: Issue HCRST */
    mmio_write32(xhci->op_base + XHCI_OP_USBCMD, USBCMD_HCRST);
    xhci_mmio_barrier();

    /* Step 2: Wait for HCRST to clear */
    timeout = 1000000;
    while (timeout--) {
        uint32_t cmd = mmio_read32(xhci->op_base + XHCI_OP_USBCMD);
        if (!(cmd & USBCMD_HCRST))
            break;
        tick_delay(1000);
    }
    if (timeout <= 0)
        return -ETIMEDOUT;

    /* Step 3: Wait for CNR to clear */
    timeout = 1000000;
    while (timeout--) {
        uint32_t sts = mmio_read32(xhci->op_base + XHCI_OP_USBSTS);
        if (!(sts & USBSTS_CNR))
            break;
        tick_delay(1000);
    }
    if (timeout <= 0)
        return -ETIMEDOUT;

     // xhci_status_regs("xhci: ", "USBCMD_HCRST=%x USBSTS_CNR2=%x 0=%x",
     //                mmio_read32(xhci->op_base + XHCI_OP_USBCMD),(mmio_read32(xhci->op_base + XHCI_OP_USBSTS)& USBSTS_CNR), 0);

    xhci_status_regs("xhci: ", "before Clear RW1C bits ,USBCMD_HCRST=%x USBSTS_CNR1=%x USBSTS_HCH=%x",
                    mmio_read32(xhci->op_base + XHCI_OP_USBCMD),(mmio_read32(xhci->op_base + XHCI_OP_USBSTS)& USBSTS_CNR), (mmio_read32(xhci->op_base + XHCI_OP_USBSTS)& USBSTS_HCH));

    /* Step 4: Clear RW1C bits */
    mmio_write32(xhci->op_base + XHCI_OP_USBSTS,
                 USBSTS_HSE | USBSTS_EINT | USBSTS_PCD);
    xhci_mmio_barrier();

    xhci_status_regs("xhci: ", "after Clear RW1C bits ,USBCMD_HCRST=%x USBSTS_CNR1=%x USBSTS_HCH=%x",
                    mmio_read32(xhci->op_base + XHCI_OP_USBCMD),(mmio_read32(xhci->op_base + XHCI_OP_USBSTS)& USBSTS_CNR), (mmio_read32(xhci->op_base + XHCI_OP_USBSTS)& USBSTS_HCH));

    // /* 1. 确保控制器停�?*/
    // dword = mmio_read32(xhci->op_base + XHCI_OP_USBCMD);
    // dword &= ~USBCMD_RS;
    // mmio_write32(xhci->op_base + XHCI_OP_USBCMD, dword);
    
    // timeout = 1000; // 假设 tick_delay(1) �?1ms，给�?1 秒时�?
    // while (timeout--) {
    //     dword = mmio_read32(xhci->op_base + XHCI_OP_USBSTS);
    //     // 如果读取�?0xFFFFFFFF，说�?PCIe 设备已经掉线/故障
    //     if (dword == 0xFFFFFFFF) {
    //         kernel_usb_status_line("xhci",": Fatal error, hardware disconnected (0xFFFFFFFF)\n");
    //         return -ENODEV;
    //     }
    //     if (dword & USBSTS_HCH) break;
    //     tick_delay(1000); // 等待 1ms
    // }
    // if (timeout <= 0) return -ETIMEDOUT;

    // /* 2. 复位控制�?*/
    // mmio_write32(xhci->op_base + XHCI_OP_USBCMD, USBCMD_HCRST);
    // xhci_mmio_barrier();
    
    // // 复位最长可能需�?1 秒，不要设得太短
    // timeout = 1000; 
    // while (timeout--) {
    //     dword = mmio_read32(xhci->op_base + XHCI_OP_USBCMD);
    //     if (!(dword & USBCMD_HCRST)) break;
    //     tick_delay(1000);
    // }
    // if (timeout <= 0) return -ETIMEDOUT;

    // /* 3. 等待 CNR (Controller Not Ready) 清零 */
    // timeout = 1000000;
    // while (timeout--) {
    //     dword = mmio_read32(xhci->op_base + XHCI_OP_USBSTS);
    //     if (!(dword & USBSTS_CNR)) break;
    //     tick_delay(1000);
    // }
    // if (timeout <= 0) return -ETIMEDOUT;

    // /* 4. 清理 RW1C 状态位 (这一步代码本身写得很完美) */
    // mmio_write32(xhci->op_base + XHCI_OP_USBSTS,
    //              USBSTS_HSE | USBSTS_EINT | USBSTS_PCD);
    // xhci_mmio_barrier();

     // xhci_status_regs("xhci: ", "USBCMD_HCRST=%x USBSTS_CNR3=%x 0=%x",
     //                mmio_read32(xhci->op_base + XHCI_OP_USBCMD),(mmio_read32(xhci->op_base + XHCI_OP_USBSTS)& USBSTS_CNR), 0);

    
    //tick_delay(100000);
    // xhci_status_regs("xhci: ", "XHCI_OP_USBCMD=%x XHCI_OP_DNCTRL=%x XHCI_OP_CONFIG=%x",
    //                  mmio_read32(xhci->op_base + XHCI_OP_USBCMD),mmio_read32(xhci->op_base + XHCI_OP_DNCTRL ), mmio_read32(xhci->op_base + XHCI_OP_CONFIG));

    // xhci_status_regs("xhci: ", "XHCI_OP_CRCR_LO=%x XHCI_OP_DCBAAP_LO=%x 0=%x",
    //                 (unsigned long long) mmio_read64_op(xhci, XHCI_OP_CRCR_LO),(unsigned long long) mmio_read64_op(xhci, XHCI_OP_DCBAAP_LO), 0);


    if (mmio_read32(xhci->op_base + XHCI_OP_USBCMD) != 0 ||
        mmio_read32(xhci->op_base + XHCI_OP_DNCTRL )!= 0 ||
        mmio_read64_op(xhci, XHCI_OP_CRCR_LO) != 0 ||
        mmio_read64_op(xhci, XHCI_OP_DCBAAP_LO) != 0 ||
        mmio_read32(xhci->op_base + XHCI_OP_CONFIG) != 0) {
        kernel_usb_status_line("xhci",": operational registers not at defaults after reset");
        return -1;
    }



    kernel_usb_status_line("xhci",": reset done\n");
    return 0;
}

/* 统一�?64 �?MMIO 写入函数 */
static inline void mmio_write64_op(volatile struct xhci_dev_t *xhci, uint32_t relative_op_offset, uint64_t val) {
    uintptr_t addr = xhci->op_base + relative_op_offset;
    
    uint32_t lo = (uint32_t)(val & 0xFFFFFFFFULL);
    uint32_t hi = (uint32_t)(val >> 32);

    /* Intel xHCI 必须先写 Low 32位，后写 High 32�?*/
    mmio_write32(addr, lo);
    xhci_mmio_barrier();
    mmio_write32(addr + 4, hi);
    xhci_mmio_barrier();
    
}

static int xhci_prepare_event_ring(volatile struct xhci_dev_t *xhci)
{
    uint32_t erst_phys;
    uint32_t rt_off = xhci->rt_base - xhci->mmio_base;
    void *erst_virt;
    int res;

    erst_virt = dma_alloc_coherent(sizeof(struct xhci_erst_entry_t),
                                   &erst_phys);
    if (!erst_virt)
        return -ENOMEM;

    A_memset(erst_virt, 0, sizeof(struct xhci_erst_entry_t));
    xhci->erst = (struct xhci_erst_entry_t *)erst_virt;
    xhci->erst_phys = (uint64_t)erst_phys;

    res = xhci_ring_init(&xhci->event_ring, XHCI_EVENT_RING_SIZE);
    if (res < 0)
        return res;

    xhci->erst->addr_lo = (uint32_t)xhci->event_ring.phys;
    xhci->erst->addr_hi = (uint32_t)(xhci->event_ring.phys >> 32);
    xhci->erst->size = XHCI_EVENT_RING_SIZE;
    xhci->erst->rsvd = 0;

    /* Match the known-good initialization order: program the event path
     * completely before installing CRCR. */
    mmio_write32(xhci->mmio_base + XHCI_RT_ERSTSZ(rt_off), 1);
    (void)mmio_read32(xhci->mmio_base + XHCI_RT_ERSTSZ(rt_off));
    xhci_write64_checked(xhci, XHCI_RT_ERDP_LO(rt_off),
                         xhci->event_ring.phys, ~0xFULL);
    xhci_write64_checked(xhci, XHCI_RT_ERSTBA_LO(rt_off),
                         xhci->erst_phys, ~0x3FULL);

    /* Keep interrupts disabled while boot code polls the event ring. */
    {
        uint32_t iman = mmio_read32(xhci->mmio_base + XHCI_RT_IMAN(rt_off));
        iman |= IMAN_IP;
        iman &= ~IMAN_IE;
        mmio_write32(xhci->mmio_base + XHCI_RT_IMAN(rt_off), iman);
        mmio_write32(xhci->mmio_base + XHCI_RT_IMOD(rt_off), 0);
    }

    usb_printk("xhci: event path ready erst=%x event=%x\n",
               (uint32_t)xhci->erst_phys,
               (uint32_t)xhci->event_ring.phys);
    kernel_usb_status_line("xhci", "event ring programmed before CRCR");
    return 0;
}

static int xhci_start(volatile struct xhci_dev_t *xhci)
{
    volatile uint32_t dword;
    volatile int i;
    int res;

    /* Read capability registers */
    uint8_t  caplen = mmio_read8(xhci->mmio_base + XHCI_CAP_CAPLENGTH);
    uint32_t hcsparams1 = mmio_read32(xhci->mmio_base + XHCI_CAP_HCSPARAMS1);
    uint32_t hcsparams2 = mmio_read32(xhci->mmio_base + XHCI_CAP_HCSPARAMS2);
    uint32_t hccparams1 = mmio_read32(xhci->mmio_base + XHCI_CAP_HCCPARAMS1);
    uint32_t dboff    = mmio_read32(xhci->mmio_base + XHCI_CAP_DBOFF);
    uint32_t rtsoff   = mmio_read32(xhci->mmio_base + XHCI_CAP_RTSOFF);
    uint32_t hciver   = mmio_read16(xhci->mmio_base + XHCI_CAP_HCIVERSION);

    xhci->caplen      = caplen;
    xhci->op_base     = xhci->mmio_base + caplen;
    xhci->rt_base     = xhci->mmio_base + rtsoff;
    xhci->db_base     = xhci->mmio_base + dboff;  /* QEMU: byte offset */
    xhci->max_slots   = HCSPARAMS1_MAXSLOTS(hcsparams1);
    xhci->port_count  = HCSPARAMS1_MAXPORTS(hcsparams1);
    /* USB2/USB3 port split is in xECP, not HCSPARAMS1.
     * HCSPARAMS1[7:0] is MaxSlots, not USB2 ports!
     * For now, treat all ports as capable �?use PORTSC speed to determine type. */
    xhci_parse_supported_protocols(xhci, hccparams1);
    if (!xhci->usb2_ports && !xhci->usb3_ports) {
        xhci->usb2_port_start = 0;
        xhci->usb2_ports = xhci->port_count;
    }
    xhci->page_size   = mmio_read32(xhci->op_base + XHCI_OP_PAGESIZE);
    xhci->context_size = HCCPARAMS1_CSZ(hccparams1) ? 64 : 32;
    xhci_status_regs("xhci USB2 ports", "start=%x count=%x total=%x",
                     xhci->usb2_port_start, xhci->usb2_ports,
                     xhci->port_count);
    xhci_status_regs("xhci USB3 ports", "start=%x count=%x total=%x",
                     xhci->usb3_port_start, xhci->usb3_ports,
                     xhci->port_count);

    // xhci_status_regs("xhci caps", "mmio=%x op=%x rt=%x",
    //                  xhci->mmio_base, xhci->op_base, xhci->rt_base);
    // xhci_status_regs("xhci caps", "db=%x caplen=%x hciver=%x",
    //                  xhci->db_base, caplen, hciver);
    // xhci_status_regs("xhci caps", "HCSP1=%x HCSP2=%x HCCP1=%x",
    //                  hcsparams1, hcsparams2, hccparams1);
    // xhci_status_regs("xhci regs", "USBCMD=%x USBSTS=%x CONFIG=%x",
    //                  mmio_read32(xhci->op_base + XHCI_OP_USBCMD),
    //                  mmio_read32(xhci->op_base + XHCI_OP_USBSTS),
    //                  mmio_read32(xhci->op_base + XHCI_OP_CONFIG));
    usb_printk("xhci: CAPLEN=%x HCIVER=%x HCSP1=%x HCSP2=%x HCCP1=%x\n",
               caplen, hciver, hcsparams1, hcsparams2, hccparams1);
    usb_printk("xhci: slots=%u ports=%u pagesize=%x USBCMD=%x USBSTS=%x CONFIG=%x\n",
               xhci->max_slots, xhci->port_count, xhci->page_size,
               mmio_read32(xhci->op_base + XHCI_OP_USBCMD),
               mmio_read32(xhci->op_base + XHCI_OP_USBSTS),
               mmio_read32(xhci->op_base + XHCI_OP_CONFIG));

    usb_printk("%s: version 0x%x, slots=%u, ports=%u (USB2=%u, USB3=%u)\n",
        "xhci", hciver, xhci->max_slots, xhci->port_count,
        xhci->usb2_ports, xhci->usb3_ports);
    kernel_usb_status_line("xhci", "reading controller caps");
    usb_printk("%s: caplen=0x%x, dboff=0x%x, rtsoff=0x%x, 64bit=%d, CSZ=%d\n",
        "xhci", caplen, dboff, rtsoff,
        HCCPARAMS1_AC64(hccparams1), HCCPARAMS1_CSZ(hccparams1));
    xhci_status_regs("xhci caps", "CSZ=%x ctxbytes=%x HCCP1=%x",
                     HCCPARAMS1_CSZ(hccparams1), xhci->context_size,
                     hccparams1);

    /* Warn if 64-bit addressing �?we only support 32-bit */
    if (HCCPARAMS1_AC64(hccparams1))
        usb_printk("%s: HC supports 64-bit, using 32-bit mode\n", "xhci");

    kernel_usb_status_line("xhci", "resetting controller");
    res = xhci_handoff_legacy(xhci, hccparams1);
    if (res < 0)
        return res;
    /* Cap max slots to our array size */
    if (xhci->max_slots > XHCI_ENUM_SLOT_LIMIT) {
        usb_printk("%s: diagnostic slot cap %u -> %u\n", "xhci",
            xhci->max_slots, XHCI_ENUM_SLOT_LIMIT);
        xhci->max_slots = XHCI_ENUM_SLOT_LIMIT;
    }

    /* Synchronous boot path: always perform the standard stop/reset/start
     * sequence here so one usb_init() call fully owns controller bring-up. */
    /* xhci_async_reset_step() currently only advances its state machine; it
     * does not perform the full hardware reset, so always run the canonical
     * reset here. */
    xhci_reset_precompleted = 0;
    if ((res = xhci_reset(xhci)) < 0) return res;
    xhci_status_regs("xhci XHCI_OP_USBSTS after reset", "0 =%x sts=%x =%x", 0,
                       mmio_read32(xhci->op_base + XHCI_OP_USBSTS), 0);

    /* Program Max Slots Enabled */
    kernel_usb_status_line("xhci", "programming Max Slots and DCBAA");
    dword = mmio_read32(xhci->op_base + XHCI_OP_CONFIG);
    dword &= ~0xFF;
    dword |= xhci->max_slots;
    mmio_write32(xhci->op_base + XHCI_OP_CONFIG, dword);

    // xhci_status_regs("xhci XHCI_OP_USBSTS", "1 =%x sts=%x =%x", 0,
    //                     mmio_read32(xhci->op_base + XHCI_OP_USBSTS), 0);

    /* Allocate DCBAA (max_slots + 1 entries of 8 bytes, 64-byte aligned) */
    /* 1. Allocate DCBAA (max_slots + 1 entries of 8 bytes, 64-byte aligned) */
    {
        // 修正：使�?dma_addr_t 防止 64 位地址截断
        uint32_t dcbaa_phys32;
        uint64_t dcbaa_phys;
        uint32_t dcbaa_size = (xhci->max_slots + 1) * sizeof(uint64_t);

        void *dcbaa_virt = dma_alloc_coherent(dcbaa_size, &dcbaa_phys32);
        if (!dcbaa_virt) { 
            res = -ENOMEM; 
            goto err; 
        }

        // 检�?64 字节对齐
        dcbaa_phys = (uint64_t)dcbaa_phys32;
        if (dcbaa_phys & 0x3F) {
            kernel_usb_status_line("xhci"," error, DCBAA address is not 64-byte aligned!\n");
            // 进行释放或对齐处�?..
        }

        // DCBAA 必须全零初始�?(Slot 0 �?Scratchpad，Slot 1..N �?NULL)
        A_memset(dcbaa_virt, 0, dcbaa_size);

        xhci->dcbaa = (uint64_t *)dcbaa_virt;
        xhci->dcbaa_phys = (uint64_t)dcbaa_phys;

        /* 注意：不要在这里循环为每�?Slot 预分�?dev_ctx 并填�?xhci->dcbaa[i]�?
         * 正确的做法是：当�?USB 设备插入，调�?Enable Slot 拿到 slot_id 后，
         * 再分�?dev_ctx 并写�?xhci->dcbaa[slot_id]�?
         */
    }


    // xhci_status_regs("xhci XHCI_OP_USBSTS", "2 =%x sts=%x =%x", 0,
    //                    mmio_read32(xhci->op_base + XHCI_OP_USBSTS), 0);
    /* Allocate Scratchpad Buffers */
    #define HCSPARAMS2_MAXSCRPBUF_LO(p)  (((p) >> 27) & 0x1F)
    #define HCSPARAMS2_MAXSCRPBUF_HI(p)  (((p) >> 21) & 0x1F)
    #define HCSPARAMS2_MAXSCRPBUF(p)     ((HCSPARAMS2_MAXSCRPBUF_HI(p) << 5) | HCSPARAMS2_MAXSCRPBUF_LO(p))
    {
        /* 1. 确保宏提取出�?sp_count 是正确的 */
        uint32_t sp_count = HCSPARAMS2_MAXSCRPBUF(hcsparams2);
        xhci->scratchpad_count = sp_count;
        
        if (sp_count > 0) {
            uint32_t sp_arr_size = sp_count * sizeof(uint64_t);
            uint64_t sp_arr_phys; // 修正类型，防�?32 位截�?
            
            /* 分配 Array 物理指针�?*/
            sp_arr_phys = 0;
            void *sp_arr_virt = dma_alloc_coherent(sp_arr_size, (uint32_t *)&sp_arr_phys);
            if (!sp_arr_virt) { res = -ENOMEM; goto err; }
            
            // 校验：Scratchpad Array 地址必须 64 字节对齐
            if (sp_arr_phys & 0x3F) {
                usb_printk("xhci: FATAL - Scratchpad array not 64-byte aligned! phys=%lx\n", (unsigned long)sp_arr_phys);
            }

            A_memset(sp_arr_virt, 0, sp_arr_size);
            xhci->sp_array = (uint64_t *)sp_arr_virt;
            xhci->sp_array_phys = sp_arr_phys;

            /* 2. 为每一�?Scratchpad 分配实际 Page */
            for (i = 0; i < (int)sp_count; i++) {
                uint64_t sp_phys;
                sp_phys = 0;
                void *sp_virt = dma_alloc_coherent(xhci->page_size, (uint32_t *)&sp_phys);
                if (!sp_virt) { res = -ENOMEM; goto err; }

                // 校验：Page 地址必须�?xhci->page_size 对齐
                if (sp_phys & (xhci->page_size - 1)) {
                    usb_printk("xhci: FATAL - Scratchpad buffer not page aligned!\n");
                }

                xhci->sp_array[i] = sp_phys;
            }
            
            /* 3. �?Scratchpad 数组基地址填入 DCBAA[0] */
            xhci->dcbaa[0] = xhci->sp_array_phys;
        }

        //* 【核心修复】：分配完成后，强行清空 USBSTS 中被硬件触发�?PCD (0x10) 状态位�?/
        mmio_write32(xhci->op_base + XHCI_OP_USBSTS, USBSTS_PCD | USBSTS_EINT);
        xhci_mmio_barrier();
    }

   
    // xhci_status_regs("xhci XHCI_OP_USBSTS", "3 =%x sts=%x =%x", 0,
    //                    mmio_read32(xhci->op_base + XHCI_OP_USBSTS), 0);
    /* 1. Program DCBAAP (Offset 0x30 relative to op_base) */
    kernel_usb_status_line("xhci", "programming DCBAAP");
    mmio_write64_op(xhci, 0x30, xhci->dcbaa_phys);
    {
        uint32_t dcbaa_lo = mmio_read32(xhci->op_base + XHCI_OP_DCBAAP_LO);
        uint32_t dcbaa_hi = mmio_read32(xhci->op_base + XHCI_OP_DCBAAP_HI);
        // xhci_status_regs("xhci DCBAAP raw", "lo=%x hi=%x sts=%x",
        //                  dcbaa_lo, dcbaa_hi,
        //                  mmio_read32(xhci->op_base + XHCI_OP_USBSTS));
        if ((dcbaa_lo & ~0x3fU) != ((uint32_t)xhci->dcbaa_phys & ~0x3fU) ||
            dcbaa_hi != (uint32_t)(xhci->dcbaa_phys >> 32)) {
            kernel_usb_status_line("xhci", ": DCBAAP did not latch.\n");
            res = -EIO;
            goto err;
        }
    }

    /* 2. Allocate Command Ring */
    if ((res = xhci_ring_init(&xhci->cmd_ring, XHCI_CMD_RING_SIZE)) < 0) goto err;

    /* The reference driver installs ERST/Event Ring before CRCR. */
    if ((res = xhci_prepare_event_ring(xhci)) < 0)
        goto err;
    
    // xhci_status_regs("xhci Allocate Command Ring", "%s: cmd ring virt=%x phys=%x\n", "xhci",
    //                   (uint32_t)xhci->cmd_ring.ring, (uint32_t)xhci->cmd_ring.phys);

    // xhci_status_regs("xhci"," cmd_ring phys=0x%x aligned_mask=0x%x  --%x \n",  (unsigned long long)xhci->cmd_ring.phys,  (unsigned long long)(xhci->cmd_ring.phys & 0x3F),0);


    // xhci_status_regs("xhci XHCI_OP_USBSTS", "4 =%x sts=%x =%x", 0,
    //                   mmio_read32(xhci->op_base + 0x04), 0);

    tick_delay(1000);

    /* 3. Program CRCR (Offset 0x18 relative to op_base) */
    {
        uint64_t crcr_base = xhci->cmd_ring.phys & ~0x3FULL;
        
        /* Bit 0 �?RCS (Ring Cycle State) 初始设为 1 */
        uint64_t crcr_want = crcr_base | XHCI_CRCR_RCS;

      
        /* 等待控制器准备好 */
        uint32_t sts;
        uint32_t cmd;

        /* 等待控制�?Not Ready 清零 */
        kernel_usb_status_line("xhci", ":testing USBSTS_CNR.");
        int ready_timeout = 1000;
        do {
            sts = mmio_read32(xhci->op_base + XHCI_OP_USBSTS);
            if (!(sts & USBSTS_CNR))
                break;
            __asm__ volatile("pause" : : : "memory");
            tick_delay(1000);
        } while (--ready_timeout > 0);
        if (sts & USBSTS_CNR) {
            kernel_usb_status_line("xhci", ": controller remained not ready.\n");
            res = -ETIMEDOUT;
            goto err;
        }


        /* CRCR is writable only while the controller is completely halted. */
        cmd = mmio_read32(xhci->op_base + XHCI_OP_USBCMD);
        mmio_write32(xhci->op_base + XHCI_OP_USBCMD, cmd & ~USBCMD_RS);
        xhci_mmio_barrier();
        (void)mmio_read32(xhci->op_base + XHCI_OP_USBCMD);
        int halt_timeout = 1000;
        do {
            sts = mmio_read32(xhci->op_base + XHCI_OP_USBSTS);
            if (sts & USBSTS_HCH)
                break;
            tick_delay(1000);
        } while (--halt_timeout > 0);
        if (!(sts & USBSTS_HCH)) {
            xhci_status_regs("xhci CRCR state", "cmd=%x sts=%x cnr=%x",
                             mmio_read32(xhci->op_base + XHCI_OP_USBCMD),
                             sts, !!(sts & USBSTS_CNR));
            res = -ETIMEDOUT;
            goto err;
        }


        /* Match the reference driver exactly: issue one Stop pair, one Abort
         * pair, wait once, then report (but do not abort initialization) if
         * CRR is still asserted. */
        if (mmio_read32(xhci->op_base + XHCI_OP_CRCR_LO) & XHCI_CRCR_CRR) {
            uintptr_t crcr_lo_addr = xhci->op_base + XHCI_OP_CRCR_LO;
            uintptr_t crcr_hi_addr = xhci->op_base + XHCI_OP_CRCR_HI;

            mmio_write32(crcr_lo_addr, XHCI_CRCR_CS);
            mmio_write32(crcr_hi_addr, 0);
            mmio_write32(crcr_lo_addr, XHCI_CRCR_CA);
            mmio_write32(crcr_hi_addr, 0);
            tick_delay(1000);

            if (mmio_read32(crcr_lo_addr) & XHCI_CRCR_CRR) {
                usb_printk("xhci: command ring still running after stop/cancel\n");
                kernel_usb_status_line("xhci", ": command ring still running after stop/cancel.\n");
            }
        }

        /* Clear pending RW1C status after HCH=1 and immediately before CRCR. */
        mmio_write32(xhci->op_base + XHCI_OP_USBSTS,
                     USBSTS_PCD | USBSTS_EINT | USBSTS_HSE | USBSTS_SRE);
        xhci_mmio_barrier();
        (void)mmio_read32(xhci->op_base + XHCI_OP_USBSTS);

        /* Program while halted: low dword, barrier, high dword, barrier. */
        uint64_t crcr_got = xhci_program_crcr(xhci, crcr_want);

        /* 读回 CRCR 进行校验 */
        // xhci_status_regs("xhci", ": CRCR want=0x%x got=0x%x sts=0x%x\n",
        //                  (uint32_t)crcr_want, (uint32_t)crcr_got,
        //                  mmio_read32(xhci->op_base + 0x04));

        xhci_status_regs("xhci:"," op_base=0x%x --%x %x\n", xhci->op_base,0,0);


        xhci_status_regs("xhci", ": CRCR want=0x%x got=0x%x sts=0x%x\n",
                 (uint32_t)crcr_want,
                 (uint32_t)crcr_got,
                 mmio_read32(xhci->op_base + 0x04));

        uint32_t crcr_lo = mmio_read32(xhci->op_base + XHCI_OP_CRCR_LO);
        uint32_t crcr_hi = mmio_read32(xhci->op_base + XHCI_OP_CRCR_HI);

        xhci_status_regs("xhci", ": raw CRCR lo=0x%x hi=0x%x --%x\n", crcr_lo, crcr_hi,0);

        xhci_status_regs("xhci: ", "in crcr ,USBCMD_HCRST=%x USBSTS_CNR1=%x USBSTS_HCH=%x",
                    mmio_read32(xhci->op_base + XHCI_OP_USBCMD),(mmio_read32(xhci->op_base + XHCI_OP_USBSTS)& USBSTS_CNR), (mmio_read32(xhci->op_base + XHCI_OP_USBSTS)& USBSTS_HCH));


        /* Match the reference: CRR governs stop/cancel.  Pointer readback is
         * diagnostic only and must not prevent the subsequent RUN command. */
        if ((crcr_got & ~0x3FULL) != crcr_base) {
            usb_printk("xhci: CRCR readback differs; continuing (want=%x got=%x)\n",
                       (uint32_t)crcr_base,
                       (uint32_t)(crcr_got & ~0x3FULL));
            kernel_usb_status_line("xhci", ": CRCR readback differs; continuing to RS.");
        }
    }

    
    // /* Program CRCR (Command Ring Control Register) */
    // {
    //     /* 1. 物理�?Intel芯片必备：写 CRCR 前清�?PCD/EINT 等挂起状�?*/
    //     mmio_write32(xhci->op_base + XHCI_OP_USBSTS, USBSTS_PCD | USBSTS_EINT | USBSTS_HSE);
    //     xhci_mmio_barrier();

    //     uint64_t crcr_base = xhci->cmd_ring.phys & ~0x3FULL;
        
    //     /* 2. 修正：必须是 Bit 0 (RCS=1)，而不�?Bit 3 (CRR)�?*/
    //     uint64_t crcr = crcr_base | 1ULL; 
    //     uint64_t crcr_readback;

    //     crcr_readback = xhci_write_crcr(xhci, crcr);

    //     xhci_status_regs("xhci",": CRCR want=0x%x got=0x%x sts=0x%x\n",
    //                (unsigned long)crcr, (unsigned long)crcr_readback,
    //                mmio_read32(xhci->op_base + XHCI_OP_USBSTS));

    //     // 校验时掩码掉�?6 �?(因为读回的低 6 位包含硬件状态标�?
    //     if ((crcr_readback & ~0x3FULL) != crcr_base) {
    //         kernel_usb_status_line("xhci",": CRCR did not latch! Refusing to start RS.\n");
    //         res = -EIO;
    //         goto err;
    //     }
    // }

    xhci_status_regs("xhci XHCI_OP_USBSTS", "5 =%x sts=%x =%x", 0,
                       mmio_read32(xhci->op_base + XHCI_OP_USBSTS), 0);
    xhci_status_regs("xhci XHCI_OP_USBSTS", "6 =%x sts=%x =%x", 0,
                       mmio_read32(xhci->op_base + XHCI_OP_USBSTS), 0);
    /* Interrupter 0 was initialized before CRCR to match the reference. */
    if (0) {
        uint32_t rt_off = xhci->rt_base - xhci->mmio_base;

        /* ERSTSZ = 1 (one segment) */
        mmio_write32(xhci->mmio_base + XHCI_RT_ERSTSZ(rt_off), 1);
        (void)mmio_read32(xhci->mmio_base + XHCI_RT_ERSTSZ(rt_off));

        /* ERSTBA/ERDP are runtime interrupter registers. Use the same
         * high-dword-first split access as CRCR/DCBAAP. */
        xhci_write64_checked(xhci, XHCI_RT_ERSTBA_LO(rt_off),
                             xhci->erst_phys, ~0x3FULL);

        /* ERDP = start of event ring */
        xhci_write64_checked(xhci, XHCI_RT_ERDP_LO(rt_off),
                             xhci->event_ring.phys, ~0xFULL);

        /* IMAN: clear pending, disable interrupt (we poll) */
        uint32_t iman = mmio_read32(xhci->mmio_base + XHCI_RT_IMAN(rt_off));
        iman |= IMAN_IP;   /* Clear pending by writing 1 */
        iman &= ~IMAN_IE;  /* Disable interrupt */
        mmio_write32(xhci->mmio_base + XHCI_RT_IMAN(rt_off), iman);

        /* IMOD: set to 0 (no interrupt moderation �?we poll anyway) */
        mmio_write32(xhci->mmio_base + XHCI_RT_IMOD(rt_off), 0);
    }

    xhci_status_regs("xhci XHCI_OP_USBSTS", "7 =%x sts=%x =%x", 0,
                       mmio_read32(xhci->op_base + XHCI_OP_USBSTS), 0);
    kernel_usb_status_line("xhci", "interrupter programmed");
    {
        uint32_t diag_rt_off = xhci->rt_base - xhci->mmio_base;
        xhci_status_regs("xhci dma", "DCBAA=%x CMD=%x ERST=%x",
                         (uint32_t)xhci->dcbaa_phys,
                         (uint32_t)xhci->cmd_ring.phys,
                         (uint32_t)xhci->erst_phys);
        xhci_status_regs("xhci dma", "EVENT=%x ERSTSZ=%x ERDP=%x",
                         (uint32_t)xhci->event_ring.phys,
                         mmio_read32(xhci->mmio_base + XHCI_RT_ERSTSZ(diag_rt_off)),
                         (uint32_t)xhci_read64(xhci, XHCI_RT_ERDP_LO(diag_rt_off)));
        xhci_status_regs("xhci regs", "DCBAAP=%x CRCR=%x ERSTBA=%x",
                         (uint32_t)xhci_read64(xhci, xhci->caplen + XHCI_OP_DCBAAP_LO),
                         (uint32_t)xhci_read_crcr(xhci),
                         (uint32_t)xhci_read64(xhci, XHCI_RT_ERSTBA_LO(diag_rt_off)));
    }

    usb_printk("xhci: DMA dcbaa=%x cmd=%x erst=%x event=%x\n",
               (uint32_t)xhci->dcbaa_phys, (uint32_t)xhci->cmd_ring.phys,
               (uint32_t)xhci->erst_phys, (uint32_t)xhci->event_ring.phys);
    usb_printk("xhci: REGS DCBAAP=%x CRCR=%x ERSTBA=%x ERDP=%x ERSTSZ=%x\n",
               (uint32_t)xhci_read64(xhci, xhci->caplen + XHCI_OP_DCBAAP_LO),
               (uint32_t)xhci_read_crcr(xhci),
               (uint32_t)xhci_read64(xhci, XHCI_RT_ERSTBA_LO(xhci->rt_base - xhci->mmio_base)),
               (uint32_t)xhci_read64(xhci, XHCI_RT_ERDP_LO(xhci->rt_base - xhci->mmio_base)),
               mmio_read32(xhci->mmio_base + XHCI_RT_ERSTSZ(xhci->rt_base - xhci->mmio_base)));

    xhci_status_regs("xhci XHCI_OP_USBSTS", "8 =%x sts=%x =%x", 0,
                       mmio_read32(xhci->op_base + XHCI_OP_USBSTS), 0);

    /* Start the Host Controller */
    usb_printk("%s: starting HC...\n", "xhci");
    kernel_usb_status_line("xhci", "starting host controller");
    kernel_usb_status_line("xhci", "writing RS");
    {
        uint32_t status = mmio_read32(xhci->op_base + XHCI_OP_USBSTS);
        if (status & USBSTS_CNR) {
            usb_printk("xhci: refusing RS while CNR is set (sts=%x)\n", status);
            res = -EAGAIN; goto err;
        }
        if (status & USBSTS_HCE) {
            usb_printk("xhci: refusing RS with HCE set (sts=%x)\n", status);
            res = -EIO; goto err;
        }
    }
    dword = mmio_read32(xhci->op_base + XHCI_OP_USBCMD);
    dword |= USBCMD_RS;
    usb_printk("xhci: before RS USBCMD=%x USBSTS=%x write=%x\n",
               mmio_read32(xhci->op_base + XHCI_OP_USBCMD),
               mmio_read32(xhci->op_base + XHCI_OP_USBSTS), dword);
    xhci_status_regs("xhci RS", "before cmd=%x sts=%x write=%x",
                     mmio_read32(xhci->op_base + XHCI_OP_USBCMD),
                     mmio_read32(xhci->op_base + XHCI_OP_USBSTS), dword);
    mmio_write32(xhci->op_base + XHCI_OP_USBCMD, dword);
    xhci_mmio_barrier();
    dword = mmio_read32(xhci->op_base + XHCI_OP_USBCMD);
    {
        uint32_t status = mmio_read32(xhci->op_base + XHCI_OP_USBSTS);
        usb_printk("xhci: after RS USBCMD=%x USBSTS=%x RS=%d HCH=%d CNR=%d HCE=%d\n",
                   dword, status, !!(dword & USBCMD_RS),
                   !!(status & USBSTS_HCH), !!(status & USBSTS_CNR),
                   !!(status & USBSTS_HCE));
    }
    xhci_status_regs("xhci RS", "after cmd=%x sts=%x", dword,
                     mmio_read32(xhci->op_base + XHCI_OP_USBSTS), 0);
    if (!(dword & USBCMD_RS)) {
        usb_printk("xhci: controller rejected USBCMD.RS write (cmd=%x)\n", dword);
        kernel_usb_status_line("xhci", "controller rejected RS write");
        res = -EIO; goto err;
    }
    kernel_usb_status_line("xhci", "RS written and read back");

    usb_printk("xhci: waiting for HCH clear after RS\n");
    kernel_usb_status_line("xhci", "waiting for HCH clear");

    /* Wait for HCHalted to clear */
    {
        volatile int timeout = 5000;
        while (timeout--) {
            dword = mmio_read32(xhci->op_base + XHCI_OP_USBSTS);
            if (!(dword & USBSTS_HCH)) break;
            if ((timeout % 250) == 0) {
                usb_printk("xhci: waiting start, USBSTS=0x%x USBCMD=0x%x timeout=%d\n",
                           dword, mmio_read32(xhci->op_base + XHCI_OP_USBCMD), timeout);
                kernel_usb_status_line("xhci", "waiting for host controller to start");
            }
            tick_delay(100);
        }
        if (timeout <= 0) {
            usb_printk("%s: start timeout (HCH still set)\n", "xhci");
            xhci_status_regs("xhci start timeout", "cmd=%x sts=%x crcr=%x",
                             mmio_read32(xhci->op_base + XHCI_OP_USBCMD),
                             mmio_read32(xhci->op_base + XHCI_OP_USBSTS),
                             mmio_read32(xhci->op_base + XHCI_OP_CRCR_LO));
            kernel_usb_status_line("xhci", "start timeout HCH still set");
            res = -ETIMEDOUT; goto err;
        }
    }

    usb_printk("%s: HC started, status=0x%x\n", "xhci",
        mmio_read32(xhci->op_base + XHCI_OP_USBSTS));
    kernel_usb_status_line("xhci", "host controller started");

    /* Read-only CRCR snapshot after RS=1 and HCH=0.  Do not write CRCR while
     * the command ring is running. */
    {
        uint32_t crcr_lo = mmio_read32(xhci->op_base + XHCI_OP_CRCR_LO);
        uint32_t crcr_hi = mmio_read32(xhci->op_base + XHCI_OP_CRCR_HI);
        uint64_t crcr = ((uint64_t)crcr_hi << 32) | crcr_lo;

        usb_printk("xhci: CRCR after start=%x:%x base=%x RCS=%d CRR=%d\n",
                   crcr_hi, crcr_lo, (uint32_t)(crcr & ~0x3FULL),
                   !!(crcr_lo & XHCI_CRCR_RCS),
                   !!(crcr_lo & XHCI_CRCR_CRR));
        xhci_status_regs("xhci CRCR after start", "lo=%x hi=%x base=%x",
                         crcr_lo, crcr_hi,
                         (uint32_t)(crcr & ~0x3FULL));
        xhci_status_regs("xhci CRCR flags", "RCS=%x CRR=%x sts=%x",
                         !!(crcr_lo & XHCI_CRCR_RCS),
                         !!(crcr_lo & XHCI_CRCR_CRR),
                         mmio_read32(xhci->op_base + XHCI_OP_USBSTS));
    }

    /* Issue No-Op command to verify Command Ring works */
    {
        struct xhci_trb_t cmd;
        A_memset(&cmd, 0, sizeof(cmd));
        cmd.control = TRB_TYPE(TRB_TYPE_NOOP_CMD) ;
        xhci_post_cmd(xhci, &cmd);
        int cc = xhci_wait_cmd(xhci, 500);
        if (cc != CC_SUCCESS) {
            usb_printk("%s: No-Op command returned cc=%d\n", "xhci", cc);
            xhci_status_regs("xhci No-Op failed", "cc=%x CRCR=%x sts=%x",
                             (uint32_t)cc,
                             mmio_read32(xhci->op_base + XHCI_OP_CRCR_LO),
                             mmio_read32(xhci->op_base + XHCI_OP_USBSTS));
            kernel_usb_status_line("xhci", "No-Op command failed");
            res = (cc < 0) ? cc : -EIO;
            goto err;
        } else {
            usb_printk("%s: No-Op command succeeded\n", "xhci");
            kernel_usb_status_line("xhci", "No-Op command succeeded");
            xhci_status_regs("xhci No-Op command succeeded", "cc=%x CRCR=%x sts=%x",
                             (uint32_t)cc,
                             mmio_read32(xhci->op_base + XHCI_OP_CRCR_LO),
                             mmio_read32(xhci->op_base + XHCI_OP_USBSTS));
        }
    }

    xhci->flags |= XHCI_FLAG_RUN;
    kernel_usb_status_line("xhci", "controller ready");
    return 0;

err:
    xhci_status_regs("xhci init failed", "res=%x cmd=%x sts=%x",
                     (uint32_t)res,
                     mmio_read32(xhci->op_base + XHCI_OP_USBCMD),
                     mmio_read32(xhci->op_base + XHCI_OP_USBSTS));
    usb_printk("xhci: init failed res=%d\n", res);
    return res;
}

/* ================================================================
 * Port Management
 * ================================================================ */

/* Get the PORTSC register offset for a given port number */
static uint32_t xhci_port_offset(volatile struct xhci_dev_t *xhci, uint32_t port)
{
    /* USB2 ports start at operational + 0x400, stride 0x10
     * USB3 ports follow USB2 ports */
    return 0x400 + (port * 0x10);
}

static int xhci_port_is_usb3(volatile struct xhci_dev_t *xhci, uint32_t port)
{
    return xhci->usb3_ports &&
           port >= xhci->usb3_port_start &&
           port < xhci->usb3_port_start + xhci->usb3_ports;
}

/* Determine port speed from PORTSC [13:10].
 * XHCI speed IDs: 1=Full, 2=Low, 3=High, 4=SuperSpeed.
 * Trust the PORTSC speed field �?the HC knows the actual link speed. */
static uint8_t xhci_port_speed(uint32_t portsc, int is_usb3)
{
    UNUSED(is_usb3);
    uint32_t speed_id = (portsc >> 10) & 0xF;
    switch (speed_id) {
        case 1: return USB_SPEED_FULL;
        case 2: return USB_SPEED_LOW;
        case 3: return USB_SPEED_HIGH;
        case 4: return USB_SPEED_SUPER;
        default: return USB_SPEED_HIGH;
    }
}

static void xhci_reset_port(volatile struct xhci_dev_t *xhci, uint32_t port)
{
    if (port < 64 && (xhci->port_attempted & (1ULL << port))) {
        xhci_status_regs("xhci port skipped", "port=%x slot=%x attempted=%x",
                         port, xhci_find_slot_by_port(xhci, port), 1);
        return;
    }
    uint32_t reg = xhci->op_base + xhci_port_offset(xhci, port);
    uint32_t portsc = mmio_read32(reg);
    int is_usb3 = xhci_port_is_usb3(xhci, port);
    volatile int timeout;

    usb_printk("xhci: port %d reset: PORTSC=0x%x (USB%s)\n",
        port, portsc, is_usb3 ? "3" : "2");
    kernel_usb_status_line("port", "reset root port");

    /* Power on port if needed.
     * CRITICAL: always mask out PED in writes.  PED is write-1-to-disable
     * in xHCI �?writing 1 disables the port instead of enabling it. */
    if (!(portsc & XHCI_PORT_PP)) {
        portsc = (portsc & ~XHCI_PORT_PED) | XHCI_PORT_PP;
        mmio_write32(reg, portsc);
        tick_delay(20000);  /* 20ms power stabilization */
    }

    /* Assert reset */
    portsc = mmio_read32(reg);
    portsc = (portsc & ~XHCI_PORT_PED) | XHCI_PORT_PR;
    mmio_write32(reg, portsc);
    tick_delay(50000);  /* 50ms reset */

    /* De-assert reset */
    portsc = mmio_read32(reg);
    portsc = (portsc & ~(XHCI_PORT_PR | XHCI_PORT_PED));
    mmio_write32(reg, portsc);

    /* Wait for reset to complete */
    timeout = 100;
    while (timeout--) {
        portsc = mmio_read32(reg);
        if (!(portsc & XHCI_PORT_PR)) break;
        tick_delay(100);
    }

    /* Wait for port to be enabled */
    timeout = 200;
    while (timeout--) {
        portsc = mmio_read32(reg);
        if ((portsc & XHCI_PORT_CCS) &&
            (portsc & XHCI_PORT_PED) &&
            !(portsc & XHCI_PORT_PR)) {
            uint8_t speed = xhci_port_speed(portsc, is_usb3);
            usb_printk("xhci: port %d enabled, PORTSC=0x%x speed=%d\n",
                port, portsc, speed);
            xhci_status_regs("xhci port connected", "port=%x portsc=%x speed=%x",
                             port, portsc, speed);
            kernel_usb_status_line("port", "enabled, enumerating device");
            /* Clear CSC �?mask PED to avoid disabling the port */
            mmio_write32(reg, (portsc & ~XHCI_PORT_PED) | XHCI_PORT_CSC);

            /* Setup device using XHCI flow:
             * 1. Enable slot �?get slot_id
             * 2. Set dev->num = slot_id (usb_setup_device preserves for XHCI)
             * 3. Address device �?assigns USB address
             * 4. usb_setup_device �?enumerates with control transfers */
            struct usb_dev_t *usb;
            int slot_id;

            if (port < 64)
                xhci->port_attempted |= (1ULL << port);

            usb = usb_create_dev(xhci->pci.unit, port, speed);
            if (!usb) {
                usb_printk("%s: failed to create USB device\n", "xhci");
                return;
            }
            usb->type = USB_TYPE_XHCI;
            usb->priv = (void *)xhci;
            usb->ops  = &xhci_ops;

            /* DMA coherency test: write magic pattern to ALL output contexts.
             * After Enable Slot, HC should overwrite the assigned slot's context.
             * If magic persists �?HC didn't write. If zeros �?cache coherency issue. */
            /* Enable slot on the HC */
            slot_id = xhci_enable_slot(xhci);
            if (slot_id < 0) {
                usb_printk("%s: Enable Slot failed (%d)\n", "xhci", slot_id);
                usb_destroy_dev(usb);
                return;
            }

            usb_printk("%s: slot %d enabled for port %d\n", "xhci", slot_id, port);

            /* Initialize Output Slot Context: HC (QEMU) doesn't always write
             * after Enable Slot. Set Slot State = Enabled (1) so Address Device
             * won't fail with cc=5 (Context State Error). */
            if (xhci_prepare_slot_context(xhci, slot_id) < 0) {
                usb_printk("xhci: slot %d context allocation failed\n", slot_id);
                usb_destroy_dev(usb);
                return;
            }

            /* Mark slot in bitmap */
            xhci->addr_bitmap[slot_id / 32] |= (1 << (slot_id % 32));
            printf("A: dcbaa[%d]=%x (ptr=%p)\n", slot_id,
                (uint32_t)xhci->dcbaa[slot_id], &xhci->dcbaa[slot_id]);

            /* Allocate transfer ring for EP0 */
            if (xhci_alloc_transfer_ring(xhci, slot_id, 1) < 0) {
                usb_printk("xhci: slot %d EP0 ring allocation failed\n", slot_id);
                xhci_disable_slot(xhci, slot_id);
                xhci_free_addr((void *)xhci, slot_id);
                usb_destroy_dev(usb);
                return;
            }
            printf("B: dcbaa[%d]=%x\n", slot_id,
                (uint32_t)xhci->dcbaa[slot_id]);

            /* Set slot_id as the USB address (preserved by usb_setup_device for XHCI) */
            usb->num = slot_id;
            printf("C: dcbaa[%d]=%x\n", slot_id,
                (uint32_t)xhci->dcbaa[slot_id]);

            /* Step 1: Address Device �?validate context, enable slot/EP0.
             * BSR=0: full address assignment (BSR=1 is context-only test). */
            if (xhci_address_device(xhci, slot_id, port, speed, 0) < 0) {
                usb_printk("%s: Address Device failed for slot %d\n", "xhci", slot_id);
                if (xhci_disable_slot(xhci, slot_id) < 0)
                    usb_printk("xhci: slot %d cleanup skipped after Address failure\n",
                               slot_id);
                xhci_free_addr((void *)xhci, slot_id);
                usb_destroy_dev(usb);
                return;
            }

            /* Step 2: Enumerate (GetDescriptor at address 0 through TRB ring).
             * usb_setup_device will:
             *   a) Read 8-byte device descriptor at addr 0 �?learn real MPS
             *   b) Call usb_set_device_addr (SET_ADDRESS) �?for XHCI this
             *      is intercepted: we do Address Device BSR=0 instead.
             *   c) Read full descriptor + config at new address */
            if (usb_setup_device(usb, slot_id) < 0) {
                usb_printk("%s: failed to set up device on port %d\n",
                    "xhci", port);
                xhci_disable_slot(xhci, slot_id);
                xhci_free_addr((void *)xhci, slot_id);
                usb_destroy_dev(usb);
                return;
            }

            xhci->slots[slot_id].usb     = usb;
            xhci->slots[slot_id].enabled = 1;
            xhci->slots[slot_id].speed   = speed;
            xhci->slots[slot_id].port    = port;
            usb_printk("%s: port %d ready (slot %d, speed %d)\n",
                "xhci", port, slot_id, speed);
            kernel_usb_status_line("port", "device ready");
            return;
        }
        tick_delay(500);
    }
    usb_printk("%s: port %d enable timeout\n", "xhci", port);
}

static void xhci_enable_ports(volatile struct xhci_dev_t *xhci)
{
    volatile uint32_t i;
    uint32_t candidates[2];
    unsigned int candidate_count = 0;

    usb_printk("%s: enabling %u ports...\n", "xhci", xhci->port_count);
    kernel_usb_status_line("ports", "scanning root ports");

    /* One pass: collect up to the two connected devices covered by the
     * diagnostic slot limit (currently the mouse and mass-storage device). */
    for (i = 0; i < xhci->port_count; i++) {
        uint32_t reg = xhci->op_base + xhci_port_offset(xhci, i);
        uint32_t portsc = mmio_read32(reg);
        uint32_t speed_id = (portsc >> 10) & 0xF;

        if (!(portsc & XHCI_PORT_CCS))
            continue;
#if XHCI_MOUSE_ONLY
        if (speed_id != 2)
            continue;
#endif

        usb_printk("xhci: port %d PORTSC=0x%x\n", i, portsc);
        xhci_status_regs("port connected", "port=%x portsc=%x speedid=%x",
                         i, portsc, speed_id);

        /* Power on port �?mask PED to avoid disabling */
        if (!(portsc & XHCI_PORT_PP)) {
            mmio_write32(reg, (portsc & ~XHCI_PORT_PED) | XHCI_PORT_PP);
            tick_delay(10000);
        }

        /* Clear CSC (write-1-to-clear) �?mask PED */
        if (portsc & XHCI_PORT_CSC) {
            mmio_write32(reg, (portsc & ~XHCI_PORT_PED) | XHCI_PORT_CSC);
        }

        portsc = mmio_read32(reg);
        usb_printk("xhci: port %d after power PORTSC=0x%x\n", i, portsc);

        /* Check if device connected */
        if (portsc & XHCI_PORT_CCS) {
            if (candidate_count < 2) {
                if (speed_id == 2 && candidate_count > 0) {
                    candidates[1] = candidates[0];
                    candidates[0] = i;
                    candidate_count++;
                } else {
                    candidates[candidate_count++] = i;
                }
            }
        }
    }

    /* Port scan is complete.  Only now enter the Slot/EP0 state machine. */
    for (i = 0; i < candidate_count; i++) {
        usb_printk("xhci: device on port %d, resetting...\n", candidates[i]);
        xhci_reset_port(xhci, candidates[i]);
    }

    xhci->flags |= XHCI_FLAG_PORTSENABLED;
    kernel_usb_status_line("ports", "root port scan complete");
}

/* ================================================================
 * Device Management Commands
 * ================================================================ */

/* Returns a device context pointer.
 * xHCI spec: each context is 32 bytes, packed contiguously.
 * Input Context: InputCtrl (32B) + Slot (32B) + EP0..EP31 (32 �?32B)
 * = 34 �?32 = 1088 bytes. Allocate with 64-byte alignment for base. */
static void *xhci_alloc_dev_ctx(volatile struct xhci_dev_t *xhci, uint64_t *phys_out)
{
    uint32_t phys;
    int size = 34 * xhci->context_size;
    void *virt = dma_alloc_coherent(size, &phys);
    if (!virt) return NULL;
    A_memset(virt, 0, size);
    *phys_out = (uint64_t)phys;
    return virt;
}

static int xhci_prepare_slot_context(volatile struct xhci_dev_t *xhci,
                                     int slot_id)
{
    uint32_t *out;

    if (!xhci || !xhci->slots || !xhci->dcbaa ||
        slot_id < 1 || slot_id > (int)xhci->max_slots)
        return -EINVAL;

    if (!xhci->slots[slot_id].dev_ctx) {
        xhci->slots[slot_id].dev_ctx = xhci_alloc_dev_ctx(
            xhci, &xhci->slots[slot_id].dev_ctx_phys);
        if (!xhci->slots[slot_id].dev_ctx)
            return -ENOMEM;
    }

    xhci->dcbaa[slot_id] = xhci->slots[slot_id].dev_ctx_phys;
    xhci_mmio_barrier();

    out = (uint32_t *)xhci->slots[slot_id].dev_ctx;
    A_memset(out, 0, 34 * xhci->context_size);
    /* Output Context is owned by the controller.  Leave Slot State zero;
     * Address Device writes the enabled/addressed state on completion. */

    usb_printk("xhci: slot %d context ready virt=%p phys=%x dcbaa=%x\n",
               slot_id, out,
               (uint32_t)xhci->slots[slot_id].dev_ctx_phys,
               (uint32_t)xhci->dcbaa[slot_id]);
    xhci_status_regs("xhci slot context", "slot=%x phys=%x dcbaa=%x",
                     slot_id,
                     (uint32_t)xhci->slots[slot_id].dev_ctx_phys,
                     (uint32_t)xhci->dcbaa[slot_id]);
    return 0;
}

static int xhci_stop_and_rebase_endpoint(volatile struct xhci_dev_t *xhci,
                                         int slot_id, int ep_id)
{
    struct xhci_ring_t *ring = &xhci->slots[slot_id].ep_rings[ep_id];
    struct xhci_trb_t cmd;
    int cc;

    A_memset(&cmd, 0, sizeof(cmd));
    cmd.control = TRB_TYPE(TRB_TYPE_STOP_EP) |
                  ((uint32_t)ep_id << 16) |
                  ((uint32_t)slot_id << 24);
    if (xhci_post_cmd(xhci, &cmd) < 0)
        return -ENOMEM;
    cc = xhci_wait_cmd(xhci, 1000);
    if (cc != CC_SUCCESS)
        return -EIO;

    A_memset(ring->ring, 0, ring->size * sizeof(struct xhci_trb_t));
    ring->ring[ring->size - 1].param = ring->phys;
    ring->ring[ring->size - 1].control =
        TRB_TYPE(TRB_TYPE_LINK) | TRB_TC | TRB_CYCLE;
    ring->enqueue = 0;
    ring->dequeue = 0;
    ring->ccs = 1;
    xhci_mmio_barrier();

    A_memset(&cmd, 0, sizeof(cmd));
    cmd.param = ring->phys | 1;
    cmd.control = TRB_TYPE(TRB_TYPE_SET_TR_DEQUEUE) |
                  ((uint32_t)ep_id << 16) |
                  ((uint32_t)slot_id << 24);
    if (xhci_post_cmd(xhci, &cmd) < 0)
        return -ENOMEM;
    cc = xhci_wait_cmd(xhci, 1000);
    return cc == CC_SUCCESS ? 0 : -EIO;
}

/* Enable Slot �?returns slot_id (positive) or negative error */
static int xhci_enable_slot(volatile struct xhci_dev_t *xhci)
{
    struct xhci_trb_t cmd;
    struct xhci_trb_t event;
    volatile int timeout = 100000;

    A_memset(&cmd, 0, sizeof(cmd));
    cmd.control = TRB_TYPE(TRB_TYPE_ENABLE_SLOT) ;
    if (xhci_post_cmd(xhci, &cmd) < 0)
        return -ENOMEM;

    while (timeout--) {
        if (xhci_get_event(xhci, &event) > 0) {
            uint8_t trb_type = (event.control >> TRB_TRB_TYPE_SHIFT) & 0x3F;
            if (trb_type == TRB_TYPE_CMD_COMP_EVENT) {
                uint64_t completed = event.param & ~0xFULL;
                if (completed != (xhci_last_cmd_phys & ~0xFULL)) {
                    usb_printk("xhci: Enable Slot stale completion ptr=%x expected=%x\n",
                               (uint32_t)completed,
                               (uint32_t)xhci_last_cmd_phys);
                    continue;
                }
                uint8_t cc = (event.status >> 24) & 0xFF;
                uint8_t sid = (event.control >> 24) & 0xFF;
                xhci_last_completion_phys = completed;
                xhci_last_completion_slot = sid;
                if (cc == CC_SUCCESS && sid > 0) {
                    xhci_status_regs("xhci Enable Slot", "cc=%x slot=%x CRCR=%x",
                                     cc, sid,
                                     mmio_read32(xhci->op_base + XHCI_OP_CRCR_LO));
                    return (int)sid;
                }
                usb_printk("%s: Enable Slot failed, cc=%d\n", "xhci", cc);
                xhci_status_regs("xhci Enable Slot failed", "cc=%x slot=%x CRCR=%x",
                                 cc, sid,
                                 mmio_read32(xhci->op_base + XHCI_OP_CRCR_LO));
                return -EIO;
            }
        }
        tick_delay(1);
    }
    usb_printk("%s: Enable Slot timeout\n", "xhci");
    xhci_status_regs("xhci Enable Slot timeout", "CRCR=%x sts=%x ERDP=%x",
                     mmio_read32(xhci->op_base + XHCI_OP_CRCR_LO),
                     mmio_read32(xhci->op_base + XHCI_OP_USBSTS),
                     (uint32_t)xhci_read64(
                         xhci, XHCI_RT_ERDP_LO(
                             xhci->rt_base - xhci->mmio_base)));
    return -ETIMEDOUT;
}

static int xhci_disable_slot(volatile struct xhci_dev_t *xhci, int slot_id)
{
    struct xhci_trb_t cmd;
    int cc;

    if (slot_id < 1 || slot_id > (int)xhci->max_slots)
        return -EINVAL;
    A_memset(&cmd, 0, sizeof(cmd));
    cmd.control = TRB_TYPE(TRB_TYPE_DISABLE_SLOT) |
                  ((uint32_t)slot_id << 24);
    if (xhci_post_cmd(xhci, &cmd) < 0)
        return -ENOMEM;
    cc = xhci_wait_cmd(xhci, 1000);
    if (cc != CC_SUCCESS) {
        xhci_error_regs("xhci Disable Slot failed", "slot=%x cc=%x crcr=%x",
                        (uint32_t)slot_id, (uint32_t)cc,
                        mmio_read32(xhci->op_base + XHCI_OP_CRCR_LO));
        return -EIO;
    }
    xhci_status_regs("xhci Disable Slot", "slot=%x cc=%x crcr=%x",
                     (uint32_t)slot_id, (uint32_t)cc,
                     mmio_read32(xhci->op_base + XHCI_OP_CRCR_LO));
    return 0;
}


void dump_trb_raw(struct xhci_trb_t *trb)
{
    uint32_t *p = (uint32_t *)trb;

    printf("TRB raw:\n");
    printf(" DW0=%08x\n", p[0]);
    printf(" DW1=%08x\n", p[1]);
    printf(" DW2=%08x\n", p[2]);
    printf(" DW3=%08x\n", p[3]);
}

/* Address Device �?assign USB address, set slot context + EP0 context
 * dev_ctx + DCBAA[slot_id] are pre-allocated in xhci_start. */
static int xhci_address_device(volatile struct xhci_dev_t *xhci,
                               int slot_id, uint8_t port, uint8_t speed,
                               int bsr)
{
    struct xhci_trb_t cmd;
    uint32_t *ctx;
    uint32_t *slot_ctx;
    uint32_t *ep0_ctx;

    printf("ENTER addr_dev: dcbaa[%d]=%x\n", slot_id,
        (uint32_t)xhci->dcbaa[slot_id]);

    /* Allocate Input Context on first use */
    if (!xhci->slots[slot_id].input_ctx) {
        xhci->slots[slot_id].input_ctx = xhci_alloc_dev_ctx(xhci,
            &xhci->slots[slot_id].input_ctx_phys);
        if (!xhci->slots[slot_id].input_ctx) return -ENOMEM;
    }

    /* Flat dword array: each context is 32 bytes = 8 dwords.
     * Input Context layout (xHCI spec rev 1.2 �?.2.5.1):
     *   ctx[0..7]   = Input Control Context (offset 0x00)
     *   ctx[8..15]  = Slot Context           (offset 0x20)
     *   ctx[16..23] = EP0 Context            (offset 0x40)
     *
     * Output Device Context (at DCBAA[slot_id]) has NO Input Control:
     *   out[0..7]  = Slot Context
     *   out[8..15] = EP0 Context
     */
    {
        uint32_t *out = (uint32_t *)xhci->slots[slot_id].dev_ctx;
        uint32_t slot_state = (out[3] >> 27) & 0x1F;
        usb_printk("%s: Output Slot virt=%p phys=%x (dcbaa=%x)\n",
            "xhci", out,
            (uint32_t)xhci->slots[slot_id].dev_ctx_phys,
            (uint32_t)xhci->dcbaa[slot_id]);
        usb_printk("%s: Output Slot dw0=%x dw1=%x dw2=%x dw3=%x dw4=%x (state=%d)\n",
            "xhci", out[0], out[1], out[2], out[3], out[4], slot_state);
    }

    ctx = (uint32_t *)xhci->slots[slot_id].input_ctx;
    A_memset(ctx, 0, 34 * xhci->context_size);
    slot_ctx = xhci_input_slot_ctx(xhci, ctx);
    ep0_ctx = xhci_input_ep_ctx(xhci, ctx, 1);

    uint16_t mps = (speed == USB_SPEED_SUPER) ? 512 :
                   (speed == USB_SPEED_LOW) ? 8 : 64;

             
    uint32_t speed_code = (speed == USB_SPEED_SUPER) ? 4 :
                          (speed == USB_SPEED_LOW)   ? 2 :
                          (speed == USB_SPEED_FULL)  ? 1 : 3;
    xhci_status_regs("xhci EP0 initial", "slot=%x speed=%x mps=%x",
                     (uint32_t)slot_id, (uint32_t)speed, (uint32_t)mps);


    //mps=8;speed_code=1;
    /* ---- Input Control ---- */
    ctx[0] = 0;   /* drop_flags */
    ctx[1] = 3;   /* add_flags: Slot(A0) + EP0(A1) per spec */

    /* ---- Slot Context (ctx[8..15]) ----
     * dw0: Route String[19:0]=0 | Speed[23:20] | Context Entries[31:27]
     * dw1: Root Hub Port Number[15:8]
     * dw2: TT/Hub �?0 for root-hub devices
     * dw3: Interrupter Target[21:10]=0
     */
    slot_ctx[0] = (1 << 27) | (speed_code << 20);

    printf("Slot DW0=%08x\n", slot_ctx[0]);
    printf("CE=%d speed=%d\n",
       slot_ctx[0] >> 27,
       (slot_ctx[0] >> 20) & 0xf);

    slot_ctx[1] = ((uint32_t)(port + 1) << 16);
    slot_ctx[2] = 0;
    slot_ctx[3] = 0;

    /* ---- EP0 Context (ctx[16..23]) ----
     * dw0: EP State=0
     * dw1: MPS[31:16] | EP Type[5:3]=4 | CErr[2:1]=3 (Control)
     * dw2: TR Dequeue Pointer Lo | DCS (bit 0)=1
     * dw3: TR Dequeue Pointer Hi
     * dw4: Average TRB Length = 8
     */
    ep0_ctx[0] = 0;
    ep0_ctx[1] = ((uint32_t)mps << 16) | (4 << 3) | (3 << 1);
    
    printf("EP0 DW1=%08x\n", ep0_ctx[1]);
    printf("EP type=%d\n",
       (ep0_ctx[1] >> 3) & 7);

    printf("DEBUG slot=%d\n", slot_id);

    printf(" dev_ctx virt=%x phys=%x\n",
           xhci->slots[slot_id].dev_ctx,
           xhci->slots[slot_id].dev_ctx_phys);

    printf(" input_ctx phys=%x\n",
           xhci->slots[slot_id].input_ctx_phys);

    printf(" ep0 ring virt=%x phys=%x\n",
           xhci->slots[slot_id].ep_rings[1].ring,
           xhci->slots[slot_id].ep_rings[1].phys);

    if (xhci->slots[slot_id].ep_rings[1].ring &&
        xhci->slots[slot_id].ep_rings[1].phys != 0) {
        // ctx[18] = (uint32_t)xhci->slots[slot_id].ep_rings[1].phys | 1;  /* DCS=1 */
        // ctx[19] = 0;  /* 32-bit system: TR dequeue hi = 0 */

        uint64_t tr = xhci->slots[slot_id].ep_rings[1].phys;
        tr &= ~0xF;   // 寮哄�?16 瀛楄妭瀵归�?
        tr |= 1;      // DCS = 1

        ep0_ctx[2] = (uint32_t)(tr & 0xFFFFFFFF);
        ep0_ctx[3] = (uint32_t)(tr >> 32);

    }

    ep0_ctx[4] = 8;

    /* Dump all 24 dwords for debug */
    for (int i = 0; i < 24; i++)
        printf("ctx[%02d] = %08x\n", i, ctx[i]);

    /* Prepare Address Device command (BSR=0 �?full address assignment) */
    A_memset(&cmd, 0, sizeof(cmd));
    cmd.param  = xhci->slots[slot_id].input_ctx_phys;
    cmd.control = TRB_TYPE(TRB_TYPE_ADDRESS_DEVICE) |
                  ((slot_id & 0xFF) << 24);
    if (bsr)
        cmd.control |= (1U << 9);

    //cmd.control = (1 << 9);      // Address Device

    dump_trb_raw(&cmd);

    usb_printk("%s: Address Device: slot=%d ctx=0x%x speed=%d port=%d mps=%d bsr=%d\n",
        "xhci", slot_id, (uint32_t)xhci->slots[slot_id].input_ctx_phys,
        (int)speed, (int)port, (int)mps, bsr);
    usb_printk("%s:   InputCtrl: drop=%x add=%x\n", "xhci", ctx[0], ctx[1]);
    usb_printk("%s:   SlotCtx:   dw0=%x dw1=%x dw2=%x dw3=%x\n",
        "xhci", slot_ctx[0], slot_ctx[1], slot_ctx[2], slot_ctx[3]);
    usb_printk("%s:   EP0Ctx:    dw0=%x dw1=%x dw2=%x dw3=%x dw4=%x\n",
        "xhci", ep0_ctx[0], ep0_ctx[1], ep0_ctx[2], ep0_ctx[3], ep0_ctx[4]);
    xhci_status_regs("xhci Address EP0 input", "slot=%x dw1=%x speed=%x",
                     (uint32_t)slot_id, ep0_ctx[1], (uint32_t)speed_code);

    if (xhci_post_cmd(xhci, &cmd) < 0)
        return -ENOMEM;
    xhci_status_regs("xhci Address Device submit", "dw3=%x cmd=%x req=%x",
                     xhci_last_cmd_control,
                     (uint32_t)xhci_last_cmd_phys,
                     (uint32_t)slot_id);

    int cc = xhci_wait_cmd(xhci, 500);
    if (cc != CC_SUCCESS) {
        usb_printk("%s: Address Device  returned cc=%d\n", "xhci", cc);
        xhci_status_regs("xhci Address Device failed", "cc=%x req=%x evt=%x",
                         (uint32_t)cc, (uint32_t)slot_id,
                         (uint32_t)xhci_last_completion_slot);
        xhci_status_regs("xhci Address Device cmd", "dw3=%x want=%x got=%x",
                         xhci_last_cmd_control,
                         (uint32_t)xhci_last_cmd_phys,
                         (uint32_t)xhci_last_completion_phys);
        return -EIO;
    }
    usb_printk("%s:  Address Device succeeded\n", "xhci");
    xhci_status_regs("xhci Address Device", "cc=%x slot=%x state=%x",
                     (uint32_t)cc, (uint32_t)slot_id,
                     ((uint32_t *)xhci->slots[slot_id].dev_ctx)[3] >> 27);
    {
        uint32_t *out_ep0 = xhci_output_ep_ctx(
            xhci, (uint32_t *)xhci->slots[slot_id].dev_ctx, 1);
        xhci_status_regs("xhci Address EP0 output", "slot=%x dw1=%x mps=%x",
                         (uint32_t)slot_id, out_ep0[1],
                         (out_ep0[1] >> 16) & 0xFFFF);
    }
    xhci->slots[slot_id].addressed = 1;
    usb_printk("%s: Address Device slot %d OK\n", "xhci", slot_id);
    return 0;
}

int xhci_update_ep0_mps(struct usb_dev_t *dev, uint16_t mps)
{
    volatile struct xhci_dev_t *xhci;
    struct xhci_trb_t cmd;
    uint32_t *input;
    uint32_t *input_ep0;
    uint32_t *output_ep0;
    int slot_id;
    int cc;

    if (!dev || dev->type != USB_TYPE_XHCI || !dev->priv)
        return -EINVAL;

    xhci = (volatile struct xhci_dev_t *)dev->priv;
    slot_id = dev->num;
    if (slot_id < 1 || slot_id > (int)xhci->max_slots ||
        !xhci->slots[slot_id].input_ctx ||
        !xhci->slots[slot_id].dev_ctx)
        return -EINVAL;

    output_ep0 = xhci_output_ep_ctx(
        xhci, (uint32_t *)xhci->slots[slot_id].dev_ctx, 1);
    if (((output_ep0[1] >> 16) & 0xFFFFU) == mps) {
        xhci_status_regs("xhci EP0 MPS already set", "slot=%x mps=%x state=%x",
                         (uint32_t)slot_id, (uint32_t)mps,
                         output_ep0[0] & 0x7);
        return 0;
    }

    input = (uint32_t *)xhci->slots[slot_id].input_ctx;
    A_memset(input, 0, 34 * xhci->context_size);
    input[1] = (1U << 0) | (1U << 1); /* Add Slot and EP0 Contexts. */

    {
        uint32_t *input_slot = xhci_input_slot_ctx(xhci, input);
        uint32_t *output_slot = (uint32_t *)xhci->slots[slot_id].dev_ctx;
        A_memcpy(input_slot, output_slot, xhci->context_size);
    }
    input_ep0 = xhci_input_ep_ctx(xhci, input, 1);
    A_memcpy(input_ep0, output_ep0, xhci->context_size);
    /* EP State is HC-owned and must be zero in an Input Endpoint Context. */
    input_ep0[0] &= ~0x7U;
    input_ep0[1] = (input_ep0[1] & 0x0000FFFFU) |
                   ((uint32_t)mps << 16);

    A_memset(&cmd, 0, sizeof(cmd));
    cmd.param = xhci->slots[slot_id].input_ctx_phys;
    cmd.control = TRB_TYPE(TRB_TYPE_EVALUATE_CTX) |
                  ((uint32_t)slot_id << 24);
    if (xhci_post_cmd(xhci, &cmd) < 0)
        return -ENOMEM;

    cc = xhci_wait_cmd(xhci, 500);
    if (cc != CC_SUCCESS) {
        xhci_error_regs("xhci EP0 MPS update failed", "cc=%x slot=%x mps=%x",
                        (uint32_t)cc, (uint32_t)slot_id, (uint32_t)mps);
        return -EIO;
    }

    xhci_status_regs("xhci EP0 MPS updated", "cc=%x slot=%x mps=%x",
                     (uint32_t)cc, (uint32_t)slot_id, (uint32_t)mps);
    output_ep0 = xhci_output_ep_ctx(
        xhci, (uint32_t *)xhci->slots[slot_id].dev_ctx, 1);
    xhci_status_regs("xhci EP0 after update", "state=%x mps=%x deq=%x",
                     output_ep0[0] & 0x7,
                     (output_ep0[1] >> 16) & 0xFFFF,
                     output_ep0[2]);
    if (xhci_stop_and_rebase_endpoint(xhci, slot_id, 1) < 0) {
        xhci_error_regs("xhci EP0 rebase failed", "slot=%x mps=%x deq=%x",
                        (uint32_t)slot_id, (uint32_t)mps, output_ep0[2]);
        return -EIO;
    }
    xhci_status_regs("xhci EP0 rebased", "slot=%x ring=%x dcs=%x",
                     (uint32_t)slot_id,
                     (uint32_t)xhci->slots[slot_id].ep_rings[1].phys, 1);
    return 0;
}

/* ================================================================
 * Transfer Implementation
 * ================================================================ */

/* Allocate and initialize a transfer ring for an endpoint */
static int xhci_alloc_transfer_ring(volatile struct xhci_dev_t *xhci,
                                    int slot_id, int ep_id)
{
    if (xhci->slots[slot_id].ep_rings[ep_id].ring)
        return 0;  /* Already allocated */

    struct xhci_ring_t *ring = &xhci->slots[slot_id].ep_rings[ep_id];
    return xhci_ring_init(ring, XHCI_TRANSFER_RING_SIZE);
}

/* Set up a control transfer (SETUP stage) */
static int xhci_setup_transaction(struct usb_transaction_t *transaction)
{
    struct usb_dev_t      *usb      = transaction->dev;
    struct xhci_dev_t     *xhci     = (struct xhci_dev_t *)usb->priv;
    int slot_id = usb->num;
    int ep_id   = 1;  /* EP0: use endpoint ID 1 (EP0 OUT+IN are paired) */

    if (slot_id == 0 || slot_id > (int)xhci->max_slots) {
        usb_printk("%s: invalid slot %d\n", "xhci", slot_id);
        return -EINVAL;
    }

    /* Intercept SET_ADDRESS (bmRequestType=0x00, bRequest=5):
     * On XHCI, transfers are routed by slot ID, NOT USB device address.
     * The slot was already addressed by the first Address Device command.
     * SET_ADDRESS is a no-op here; just return success. */
    if (transaction->type == 0x00 && transaction->req == 5) {
        usb_printk("%s: SET_ADDRESS slot=%d addr=%d (no-op, slot already addressed)\n",
            "xhci", slot_id, transaction->loval);
        transaction->data = (void *)0x1;  /* sentinel: done */
        return 0;
    }

    /* Ensure transfer ring exists */
    if (xhci_alloc_transfer_ring(xhci, slot_id, ep_id) < 0) {
        usb_printk("%s: failed to alloc transfer ring\n", "xhci");
        return -ENOMEM;
    }

    /* Build Setup Stage TRB with IDT=1 (immediate data).
     * Per xHCI spec �?.6.10.1, the 8-byte setup packet is embedded
     * directly in the TRB Parameter field, NOT in a separate buffer. */
    struct xhci_trb_t setup_trb;
    struct usb_request_t req;
    struct xhci_ring_t *tr = &xhci->slots[slot_id].ep_rings[ep_id];

    req.type  = transaction->type;
    req.req   = transaction->req;
    req.hival = transaction->hival;
    req.loval = transaction->loval;
    req.index = transaction->index;
    req.len   = transaction->len;

    A_memset(&setup_trb, 0, sizeof(setup_trb));
    /* IDT=1: embed setup packet in param in USB wire order.
     * req struct has hival/loval at swapped positions �?build manually. */
    {
        uint8_t *b = (uint8_t *)&setup_trb.param;
        b[0] = req.type;              /* bmRequestType */
        b[1] = req.req;               /* bRequest */
        b[2] = req.loval;             /* wValue low  (USB wire: byte 2) */
        b[3] = req.hival;             /* wValue high (USB wire: byte 3) */
        b[4] = req.index & 0xFF;      /* wIndex low */
        b[5] = (req.index >> 8);       /* wIndex high */
        b[6] = req.len & 0xFF;        /* wLength low */
        b[7] = (req.len >> 8);         /* wLength high */
        {
            char setup_hex[96];
            snprintf(setup_hex, sizeof(setup_hex),
                     "slot=%x port=%x bytes=%02x %02x %02x %02x %02x %02x %02x %02x",
                     (uint32_t)slot_id, (uint32_t)usb->port,
                     b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7]);
            kernel_usb_status_line("xhci setup packet", setup_hex);
        }
    }
    setup_trb.status  = 8;                  /* transfer length = 8 */
    {
        uint32_t trt = 0;
        if (transaction->len > 0) {
            trt = (transaction->type & 0x80) ? (3 << 16) : (2 << 16);
        }
        setup_trb.control = TRB_TYPE(TRB_TYPE_SETUP_STAGE) | TRB_IDT | trt;
    }

    /* Compare the exact 16 bytes we constructed with the exact 16 bytes
     * copied into the xHCI transfer ring.  If these differ, ring_enqueue,
     * ring index, cache coherency, or physical mapping is wrong. */
    xhci_dump_trb("xhci SETUP built 16B", &setup_trb);

    /* Store the transfer context pointer for later stages */
    /* (We store the TRB index for completion tracking) */
    transaction->data = (void *)(uintptr_t)tr->enqueue;

    /* Enqueue */
    {
        int setup_idx = ring_enqueue(tr, &setup_trb);
        if (setup_idx < 0) {
            usb_printk("%s: setup ring full\n", "xhci");
            return -ENOMEM;
        }
        transaction->hc_trb_phys =
            tr->phys + (uintptr_t)setup_idx * sizeof(struct xhci_trb_t);
    }

    return 0;
}

static void xhci_in_transaction(struct usb_transaction_t *transaction)
{
    struct usb_transfer_t *transfer = (struct usb_transfer_t *)transaction->transfer;
    /* Skip if SET_ADDRESS already handled via Address Device command */
    if (transfer->trans_head && transfer->trans_head->data == (void *)0x1) {
        usb_printk("xhci: IN skip (SET_ADDRESS sentinel)\n");
        return;
    }

    struct usb_dev_t      *usb      = transaction->dev;
    struct xhci_dev_t     *xhci     = (struct xhci_dev_t *)usb->priv;
    int slot_id = usb->num;
    int ep_id = 1;
    struct xhci_ring_t *tr;

    ep_id = xhci_transfer_ep_id(transfer);
    tr = &xhci->slots[slot_id].ep_rings[ep_id];

    usb_printk("xhci: IN trans len=%d buf=%p ep=%d xfer_type=%d\n",
        transaction->len, transaction->buf, ep_id, transfer->type);

    if (!tr->ring) {
        usb_printk("%s: no transfer ring for IN ep=%d\n", "xhci", ep_id);
        return;
    }

    /* Build TRB based on transfer type */
    struct xhci_trb_t data_trb;
    A_memset(&data_trb, 0, sizeof(data_trb));

    int is_ctrl = (transfer->type == USB_TRANSFER_CTRL);

    if (transaction->len > 0 && transaction->buf) {
        /* Interrupt IN polls use MPS; bulk/control use the requested length. */
        uint32_t xfer_len = transaction->len;
        if (transfer->type == USB_TRANSFER_INTERRUPT)
            xfer_len = transfer->endpoint->mps;

        uint32_t data_phys;
        void *data_buf = dma_alloc_coherent(xfer_len, &data_phys);
        if (!data_buf) {
            usb_printk("%s: failed to alloc IN buffer\n", "xhci");
            return;
        }
        /* Start with a deterministic payload.  If the controller completes
         * without DMA-writing the buffer, the packet dump will be all zeroes
         * instead of unrelated allocator contents. */
        A_memset(data_buf, 0, xfer_len);
        /* Per-packet DMA status is intentionally quiet.  Dumping one line
         * for every USB packet makes FAT table scans appear to hang and can
         * itself dominate the transfer time. */
        data_trb.param   = (uint64_t)data_phys;
        data_trb.status  = TRB_XFER_LEN(xfer_len) |
                           TRB_TD_SIZE(0) |
                           TRB_INTR_TARGET(0);
        transaction->data = data_buf;

        if (is_ctrl) {
            /* Control Data Stage */
            /* Setup, Data and Status TRB types implicitly form a Control TD.
             * CH is not used to join control-transfer stages.  Only the
             * Status Stage requests a completion event. */
            data_trb.control = TRB_TYPE(TRB_TYPE_DATA_STAGE) |
                               TRB_DIR_IN;
        } else {
            /* Normal TRB direction is implied by the endpoint context. */
            data_trb.control = TRB_TYPE(TRB_TYPE_NORMAL) | TRB_IO;
            if (transfer->type == USB_TRANSFER_INTERRUPT) {
                /* HID reports are commonly shorter than the endpoint MPS.
                 * ISP makes a short interrupt-IN packet complete this TD and
                 * produce a Transfer Event on strict xHCI controllers. */
                data_trb.control |= TRB_ISP;
            }
        }
    } else {
        /* Zero-length: Status Stage for control, or skip for interrupt */
        data_trb.param   = 0;
        data_trb.status  = 0;
        if (is_ctrl) {
            data_trb.control = TRB_TYPE(TRB_TYPE_STATUS_STAGE) | TRB_DIR_IN | TRB_IO;
        } else {
            /* Interrupt/Bulk with no data �?nothing to do */
            return;
        }
    }

    {
        int idx = ring_enqueue(tr, &data_trb);
        if (idx >= 0) {
            transaction->hc_trb_phys =
                tr->phys + (uintptr_t)idx * sizeof(struct xhci_trb_t);
            if (transfer->type == USB_TRANSFER_INTERRUPT) {
                usb_printk("xhci: HID submit slot=%d dci=%d idx=%d "
                           "trb=%x len=%d ctrl=%x enq=%d deq=%d ccs=%d\n",
                           slot_id, ep_id, idx,
                           (uint32_t)transaction->hc_trb_phys,
                           (int)data_trb.status, data_trb.control,
                           tr->enqueue, tr->dequeue, tr->ccs);
            }
        }
    }
    (void)is_ctrl;
}

static int xhci_submit_address_async(volatile struct xhci_dev_t *xhci,
                                     uint32_t port)
{
    int slot = xhci->init_slot_id;
    uint8_t speed = xhci->init_port_speed;
    uint32_t *ctx;
    uint32_t *slot_ctx;
    uint32_t *ep0_ctx;
    struct xhci_trb_t cmd;

    if (!xhci->slots[slot].input_ctx) {
        xhci->slots[slot].input_ctx = xhci_alloc_dev_ctx(xhci,
            &xhci->slots[slot].input_ctx_phys);
        if (!xhci->slots[slot].input_ctx)
            return -ENOMEM;
    }

    ctx = (uint32_t *)xhci->slots[slot].input_ctx;
    A_memset(ctx, 0, 34 * xhci->context_size);
    slot_ctx = xhci_input_slot_ctx(xhci, ctx);
    ep0_ctx = xhci_input_ep_ctx(xhci, ctx, 1);

    uint16_t mps = speed == USB_SPEED_SUPER ? 512 :
                   speed == USB_SPEED_LOW ? 8 : 64;
    uint32_t speed_code = speed == USB_SPEED_SUPER ? 4 :
                          speed == USB_SPEED_LOW ? 2 :
                          speed == USB_SPEED_FULL ? 1 : 3;

    ctx[1] = 3; /* Add Slot and EP0 contexts. */
    slot_ctx[0] = (1 << 27) | (speed_code << 20);
    slot_ctx[1] = ((uint32_t)(port + 1) << 16);
    ep0_ctx[1] = ((uint32_t)mps << 16) | (4 << 3) | (3 << 1);
    ep0_ctx[2] = ((uint32_t)xhci->slots[slot].ep_rings[1].phys & ~0xFU) | 1;
    ep0_ctx[3] = (uint32_t)(xhci->slots[slot].ep_rings[1].phys >> 32);
    ep0_ctx[4] = 8;

    A_memset(&cmd, 0, sizeof(cmd));
    cmd.param = xhci->slots[slot].input_ctx_phys;
    cmd.control = TRB_TYPE(TRB_TYPE_ADDRESS_DEVICE) |
                  ((uint32_t)slot << 24);
    xhci_post_cmd(xhci, &cmd);
    return 0;
}

static int xhci_submit_get_descriptor_async(volatile struct xhci_dev_t *xhci,
                                            uint8_t desc_type, uint8_t desc_index,
                                            uint16_t len, uint16_t wIndex)
{
    int slot = xhci->init_slot_id;
    struct xhci_ring_t *ring = &xhci->slots[slot].ep_rings[1];
    struct xhci_trb_t trb;
    int idx;

    xhci->init_ctrl_buf = dma_alloc_coherent(len, &xhci->init_ctrl_phys);
    if (!xhci->init_ctrl_buf)
        return -ENOMEM;
    A_memset(xhci->init_ctrl_buf, 0, len);
    xhci->init_ctrl_len = len;

    /* GET_DESCRIPTOR setup packet embedded in the Setup Stage TRB. */
    A_memset(&trb, 0, sizeof(trb));
    {
        uint8_t *b = (uint8_t *)&trb.param;
        b[0] = (desc_type == 0x22) ? 0x81 : 0x80;
        b[1] = 6;
        b[2] = desc_index;
        b[3] = desc_type;
        b[4] = wIndex & 0xFF;
        b[5] = wIndex >> 8;
        b[6] = len & 0xFF;
        b[7] = len >> 8;
    }
    trb.status = 8;
    trb.control = TRB_TYPE(TRB_TYPE_SETUP_STAGE) | TRB_IDT |
                  (3 << 16);
    if (ring_enqueue(ring, &trb) < 0)
        return -ENOMEM;

    A_memset(&trb, 0, sizeof(trb));
    trb.param = xhci->init_ctrl_phys;
    trb.status = len;
    trb.control = TRB_TYPE(TRB_TYPE_DATA_STAGE) | TRB_DIR_IN;
    if (ring_enqueue(ring, &trb) < 0)
        return -ENOMEM;

    A_memset(&trb, 0, sizeof(trb));
    trb.control = TRB_TYPE(TRB_TYPE_STATUS_STAGE) | TRB_IO;
    idx = ring_enqueue(ring, &trb);
    if (idx < 0)
        return -ENOMEM;
    xhci->init_ctrl_trb_phys = (uint32_t)ring->phys +
        (uint32_t)idx * sizeof(struct xhci_trb_t);

    mmio_write32(xhci->db_base + slot * 4, 1);
    mmio_read32(xhci->db_base + slot * 4);
    xhci->init_cmd_wait = 2000;
    return 0;
}

/* Returns 1 complete, 0 pending, or a negative error. */
static int xhci_poll_init_transfer(volatile struct xhci_dev_t *xhci)
{
    struct xhci_trb_t event;
    if (xhci_get_event(xhci, &event) > 0) {
        uint8_t type = (event.control >> TRB_TRB_TYPE_SHIFT) & 0x3F;
        if (type == TRB_TYPE_TRANSFER_EVENT) {
            uint8_t cc = (event.status >> 24) & 0xFF;
            uint32_t ptr = (uint32_t)event.param;
            if (ptr != xhci->init_ctrl_trb_phys)
                return 0;
            if (cc == CC_SUCCESS || cc == CC_SHORT_PACKET)
                return 1;
            usb_printk("xhci: async descriptor transfer failed cc=%d\n", cc);
            return -EIO;
        }
    }
    if (xhci->init_cmd_wait) {
        xhci->init_cmd_wait--;
        return 0;
    }
    usb_printk("xhci: async descriptor transfer timeout\n");
    return -ETIMEDOUT;
}

static int xhci_parse_config_async(volatile struct xhci_dev_t *xhci)
{
    struct usb_dev_t *dev = xhci->init_usb;
    uint8_t *buf = (uint8_t *)xhci->init_ctrl_buf;
    uint32_t off = 0;
    struct usb_interface_t *iface = NULL;

    while (off + 2 <= xhci->init_config_len) {
        uint8_t len = buf[off];
        uint8_t type = buf[off + 1];
        if (len < 2 || off + len > xhci->init_config_len)
            return -EINVAL;

        if (type == 4 && len >= sizeof(struct usb_interface_descriptor_t)) {
            struct usb_interface_t *tail;
            iface = kmalloc(sizeof(*iface));
            if (!iface) return -ENOMEM;
            A_memset(iface, 0, sizeof(*iface));
            A_memcpy(&iface->desc, buf + off, sizeof(iface->desc));
            iface->bytes_per_sector = 512;
            iface->usb = dev;
            if (!dev->interfaces) dev->interfaces = iface;
            else {
                for (tail = dev->interfaces; tail->next; tail = tail->next) {}
                tail->next = iface;
            }
            usb_printk("xhci: async interface %d class=%x subclass=%x eps=%d\n",
                       iface->desc.interfacenum, iface->desc.class,
                       iface->desc.subclass, iface->desc.endpoints);
        } else if (type == 5 && len >= sizeof(struct usb_endpoint_descriptor_t)) {
            struct usb_endpoint_descriptor_t *ed =
                (struct usb_endpoint_descriptor_t *)(buf + off);
            struct usb_endpoint_t *ep = kmalloc(sizeof(*ep));
            struct usb_endpoint_t *tail;
            if (!ep) return -ENOMEM;
            A_memset(ep, 0, sizeof(*ep));
            ep->addr = ed->addr & 0x0F;
            ep->mps = ed->mps & 0x07FF;
            ep->interval = ed->interval;
            ep->type = ed->attribs & 0x03;
            ep->direction = (ed->addr & 0x80) ? USB_ENDPOINT_IN : USB_ENDPOINT_OUT;
            for (tail = dev->endpoints; tail->next; tail = tail->next) {}
            tail->next = ep;
            if (iface) {
                if (ep->type == USB_ENDPOINT_INTERRUPT)
                    iface->endpoint_interrupt = ep;
                else if (ep->direction == USB_ENDPOINT_IN)
                    iface->endpoint_in = ep;
                else
                    iface->endpoint_out = ep;
            }
            usb_printk("xhci: async EP%d %s type=%d mps=%d interval=%d\n",
                       ep->addr, ep->direction == USB_ENDPOINT_IN ? "IN" : "OUT",
                       ep->type, ep->mps, ep->interval);
        } else if (type == 0x21 && len >= 9) {
            /* HID descriptor: descriptor length follows the descriptor type
             * and is encoded little-endian at bytes 7..8. */
            xhci->init_hid_report_len = buf[off + 7] |
                ((uint16_t)buf[off + 8] << 8);
            if (!xhci->init_hid_report_len || xhci->init_hid_report_len > 4096)
                xhci->init_hid_report_len = 64;
            usb_printk("xhci: async HID report length=%d\n",
                       xhci->init_hid_report_len);
        }
        off += len;
    }
    return off == xhci->init_config_len ? 0 : -EINVAL;
}

static int xhci_submit_configure_ep_async(volatile struct xhci_dev_t *xhci,
                                          struct usb_endpoint_t *endpoint)
{
    int slot = xhci->init_slot_id;
    uint8_t addr = endpoint->addr |
        (endpoint->direction == USB_ENDPOINT_IN ? 0x80 : 0);
    int dci = xhci_ep_addr_to_dci(addr);
    uint8_t ep_type;
    uint32_t ctx_phys;
    uint32_t *ctx;
    struct xhci_trb_t cmd;

    if (endpoint->type == USB_ENDPOINT_INTERRUPT)
        ep_type = endpoint->direction == USB_ENDPOINT_IN ? 7 : 3;
    else if (endpoint->type == USB_ENDPOINT_BULK)
        ep_type = endpoint->direction == USB_ENDPOINT_IN ? 6 : 2;
    else
        return -EINVAL;

    if (xhci_alloc_transfer_ring(xhci, slot, dci) < 0)
        return -ENOMEM;
    ctx = dma_alloc_coherent((dci + 2) * xhci->context_size, &ctx_phys);
    if (!ctx) return -ENOMEM;
    A_memset(ctx, 0, (dci + 2) * xhci->context_size);
    ctx[1] = (1 << 0) | (1 << dci);
    {
        uint32_t *out = (uint32_t *)xhci->slots[slot].dev_ctx;
        uint32_t speed = (out[0] >> 20) & 0xF;
        uint32_t entries = (out[0] >> 27) & 0x1F;
        uint32_t *slot_ctx = xhci_input_slot_ctx(xhci, ctx);
        uint32_t *ep = xhci_input_ep_ctx(xhci, ctx, dci);
        if (entries < (uint32_t)dci) entries = dci;
        slot_ctx[0] = (entries << 27) | (speed << 20);
        slot_ctx[1] = out[1];
        slot_ctx[2] = out[2];
        slot_ctx[3] = out[3];
        ep[0] = (uint32_t)xhci_interval(speed, endpoint->interval) << 16;
        ep[1] = ((uint32_t)endpoint->mps << 16) |
                ((uint32_t)ep_type << 3) | (3 << 1);
        ep[2] = ((uint32_t)xhci->slots[slot].ep_rings[dci].phys & ~0xFU) | 1;
        ep[3] = (uint32_t)(xhci->slots[slot].ep_rings[dci].phys >> 32);
        ep[4] = endpoint->mps ? endpoint->mps : 1;
    }
    A_memset(&cmd, 0, sizeof(cmd));
    cmd.param = ctx_phys;
    cmd.control = TRB_TYPE(TRB_TYPE_CONFIGURE_EP) | ((uint32_t)slot << 24);
    xhci_post_cmd(xhci, &cmd);
    xhci->init_cmd_wait = 1000;
    return 0;
}

static int xhci_submit_set_config_async(volatile struct xhci_dev_t *xhci,
                                        uint8_t config)
{
    int slot = xhci->init_slot_id;
    struct xhci_ring_t *ring = &xhci->slots[slot].ep_rings[1];
    struct xhci_trb_t trb;
    int idx;

    A_memset(&trb, 0, sizeof(trb));
    {
        uint8_t *b = (uint8_t *)&trb.param;
        b[0] = 0x00;
        b[1] = 9;
        b[2] = config;
    }
    trb.status = 8;
    trb.control = TRB_TYPE(TRB_TYPE_SETUP_STAGE) | TRB_IDT;
    if (ring_enqueue(ring, &trb) < 0)
        return -ENOMEM;

    A_memset(&trb, 0, sizeof(trb));
    trb.control = TRB_TYPE(TRB_TYPE_STATUS_STAGE) | TRB_DIR_IN | TRB_IO;
    idx = ring_enqueue(ring, &trb);
    if (idx < 0)
        return -ENOMEM;
    xhci->init_ctrl_trb_phys = (uint32_t)ring->phys +
        (uint32_t)idx * sizeof(struct xhci_trb_t);
    mmio_write32(xhci->db_base + slot * 4, 1);
    mmio_read32(xhci->db_base + slot * 4);
    xhci->init_cmd_wait = 2000;
    return 0;
}

static int xhci_submit_bulk_async(volatile struct xhci_dev_t *xhci,
                                  struct usb_endpoint_t *endpoint,
                                  void *buf, uint32_t len, int in)
{
    struct usb_transfer_t *transfer = kmalloc(sizeof(*transfer));
    char status[96];
    if (!transfer) return -ENOMEM;
    usb_setup_transfer(xhci->init_usb, endpoint, transfer, USB_TRANSFER_BULK);
    if (in) usb_in_transaction(transfer, 0, buf, len);
    else usb_out_transaction(transfer, 0, buf, len);
    usb_schedule_transfer(transfer);
    usb_printk("xhci: MSC bulk submit slot=%d dci=%d dir=%s len=%d trb=%x\n",
               xhci->init_usb->num, xhci_transfer_ep_id(transfer),
               in ? "IN" : "OUT", len,
               transfer->trans_head ?
               (uint32_t)transfer->trans_head->hc_trb_phys : 0);
    snprintf(status, sizeof(status),
             "slot=%d dci=%d %s len=%d trb=%x",
             xhci->init_usb->num, xhci_transfer_ep_id(transfer),
             in ? "IN" : "OUT", len,
             transfer->trans_head ?
             (uint32_t)transfer->trans_head->hc_trb_phys : 0);
    kernel_usb_msc_status_line("MSC submit", status);
    xhci->init_transfer = transfer;
    xhci->init_cmd_wait = 4000;
    return 0;
}

static void xhci_dump_msc_packet(const char *name, const void *data,
                                 uint32_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    uint32_t i;
    char line[160];

    usb_printk("xhci: MSC %s packet len=%d\n", name, len);
    snprintf(line, sizeof(line), "%s packet len=%d", name, len);
    kernel_usb_msc_status_line("MSC", line);
    for (i = 0; i < len; i += 16) {
        uint32_t n = len - i;
        if (n > 16) n = 16;
        usb_printk("xhci:   %02x: %02x %02x %02x %02x %02x %02x %02x %02x "
                   "%02x %02x %02x %02x %02x %02x %02x %02x\n", i,
                   n > 0 ? p[i + 0] : 0, n > 1 ? p[i + 1] : 0,
                   n > 2 ? p[i + 2] : 0, n > 3 ? p[i + 3] : 0,
                   n > 4 ? p[i + 4] : 0, n > 5 ? p[i + 5] : 0,
                   n > 6 ? p[i + 6] : 0, n > 7 ? p[i + 7] : 0,
                   n > 8 ? p[i + 8] : 0, n > 9 ? p[i + 9] : 0,
                   n > 10 ? p[i + 10] : 0, n > 11 ? p[i + 11] : 0,
                   n > 12 ? p[i + 12] : 0, n > 13 ? p[i + 13] : 0,
                   n > 14 ? p[i + 14] : 0, n > 15 ? p[i + 15] : 0);
        snprintf(line, sizeof(line),
                 "%02x: %02x %02x %02x %02x %02x %02x %02x %02x "
                 "%02x %02x %02x %02x %02x %02x %02x %02x", i,
                 n > 0 ? p[i + 0] : 0, n > 1 ? p[i + 1] : 0,
                 n > 2 ? p[i + 2] : 0, n > 3 ? p[i + 3] : 0,
                 n > 4 ? p[i + 4] : 0, n > 5 ? p[i + 5] : 0,
                 n > 6 ? p[i + 6] : 0, n > 7 ? p[i + 7] : 0,
                 n > 8 ? p[i + 8] : 0, n > 9 ? p[i + 9] : 0,
                 n > 10 ? p[i + 10] : 0, n > 11 ? p[i + 11] : 0,
                 n > 12 ? p[i + 12] : 0, n > 13 ? p[i + 13] : 0,
                 n > 14 ? p[i + 14] : 0, n > 15 ? p[i + 15] : 0);
        kernel_usb_msc_status_line("MSC data", line);
    }
}

static int xhci_poll_bulk_async(volatile struct xhci_dev_t *xhci)
{
    if (usb_poll_transfer(xhci->init_transfer)) {
        int ok = xhci->init_transfer->success;
        char status[96];
        usb_printk("xhci: MSC bulk complete slot=%d dci=%d ok=%d bytes=%d\n",
                   xhci->init_transfer->dev->num,
                   xhci_transfer_ep_id(xhci->init_transfer), ok,
                   xhci->init_transfer->actual_length);
        snprintf(status, sizeof(status), "slot=%d dci=%d ok=%d bytes=%d",
                 xhci->init_transfer->dev->num,
                 xhci_transfer_ep_id(xhci->init_transfer), ok,
                 xhci->init_transfer->actual_length);
        kernel_usb_msc_status_line("MSC complete", status);
        usb_delete_transfer(xhci->init_transfer);
        kfree(xhci->init_transfer);
        xhci->init_transfer = NULL;
        return ok ? 1 : -EIO;
    }
    if (xhci->init_cmd_wait) {
        xhci->init_cmd_wait--;
        return 0;
    }
    usb_delete_transfer(xhci->init_transfer);
    kfree(xhci->init_transfer);
    xhci->init_transfer = NULL;
    return -ETIMEDOUT;
}

static void xhci_prepare_scsi_async(volatile struct xhci_dev_t *xhci)
{
    struct usb_cmd_blk_wrapper_t *cbw =
        (struct usb_cmd_blk_wrapper_t *)&xhci->init_cbw;
    uint8_t cmd;
    A_memset(cbw, 0, sizeof(*cbw));
    A_memset((void *)xhci->init_scsi_data, 0, sizeof(xhci->init_scsi_data));
    A_memset((void *)xhci->init_scsi_csw, 0, sizeof(xhci->init_scsi_csw));
    cbw->sig = 0x43425355;
    cbw->tag = ++xhci->init_scsi_tag;
    cbw->lun = 0;
    if (xhci->init_scsi_step == 0) {
        cmd = 0x12; cbw->len = 36; cbw->flags = 0x80; cbw->cblen = 6;
        cbw->cmd[4] = 36; xhci->init_scsi_len = 36;
    } else if (xhci->init_scsi_step == 1) {
        cmd = 0x00; cbw->len = 0; cbw->flags = 0; cbw->cblen = 6;
        xhci->init_scsi_len = 0;
    } else {
        cmd = 0x25; cbw->len = 8; cbw->flags = 0x80; cbw->cblen = 10;
        xhci->init_scsi_len = 8;
    }
    cbw->cmd[0] = cmd;
}

static int xhci_check_csw_async(volatile struct xhci_dev_t *xhci)
{
    uint8_t *csw = (uint8_t *)xhci->init_scsi_csw;
    uint32_t sig = *(uint32_t *)(csw + 0);
    uint32_t tag = *(uint32_t *)(csw + 4);
    usb_printk("xhci: MSC CSW raw: %02x %02x %02x %02x %02x %02x %02x "
               "%02x %02x %02x %02x %02x %02x\n",
               csw[0], csw[1], csw[2], csw[3], csw[4], csw[5], csw[6],
               csw[7], csw[8], csw[9], csw[10], csw[11], csw[12]);
    xhci_dump_msc_packet("CSW", csw, 13);
    if (sig != 0x53425355 || tag != xhci->init_cbw.tag || csw[12] != 0) {
        usb_printk("xhci: async MSC bad CSW sig=%x tag=%x status=%d\n",
                   sig, tag, csw[12]);
        return -EIO;
    }
    return 0;
}

/* Advance initial root-port enumeration by one port per periodic pass.
 * Keeping this out of xhci_install() lets the terminal regain control as soon
 * as the host controller itself is operational. */
static void xhci_enable_next_port(volatile struct xhci_dev_t *xhci)
{
    uint32_t i = xhci->init_port_cursor;
    if (i >= xhci->port_count) {
        xhci->flags |= XHCI_FLAG_PORTSENABLED;
        usb_printk("xhci: initial port scan complete\n");
        return;
    }

    uint32_t reg = xhci->op_base + xhci_port_offset(xhci, i);
    uint32_t portsc = mmio_read32(reg);

    switch (xhci->init_port_phase) {
    case 0: /* Inspect one root port. */
        if (i < 64 && (xhci->port_attempted & (1ULL << i))) {
            xhci_status_regs("xhci port skipped", "port=%x slot=%x attempted=%x",
                             i, xhci_find_slot_by_port(xhci, i), 1);
            xhci->init_port_cursor++;
            return;
        }
        usb_printk("xhci: async port %d PORTSC=0x%x\n", i, portsc);
        if (portsc & XHCI_PORT_CSC)
            mmio_write32(reg, (portsc & ~XHCI_PORT_PED) | XHCI_PORT_CSC);
        if (!(portsc & XHCI_PORT_CCS)) {
            xhci->init_port_cursor++;
            return;
        }
        if (!(portsc & XHCI_PORT_PP)) {
            mmio_write32(reg, (portsc & ~XHCI_PORT_PED) | XHCI_PORT_PP);
            xhci->init_port_wait = 20;
            xhci->init_port_phase = 1;
            return;
        }
        xhci->init_port_phase = 2;
        return;

    case 1: /* Let port power settle without busy-waiting. */
        if (xhci->init_port_wait) {
            xhci->init_port_wait--;
            return;
        }
        xhci->init_port_phase = 2;
        return;

    case 2: /* Assert Port Reset once. */
        mmio_write32(reg, (portsc & ~XHCI_PORT_PED) | XHCI_PORT_PR);
        xhci->init_port_wait = 50;
        xhci->init_port_phase = 3;
        usb_printk("xhci: async port %d reset asserted\n", i);
        return;

    case 3: /* Hold reset across periodic calls, then deassert once. */
        if (xhci->init_port_wait) {
            xhci->init_port_wait--;
            return;
        }
        portsc = mmio_read32(reg);
        mmio_write32(reg, portsc & ~(XHCI_PORT_PR | XHCI_PORT_PED));
        xhci->init_port_wait = 200;
        xhci->init_port_phase = 4;
        return;

    case 4: /* Poll enable status once per call. */
        if (!(portsc & XHCI_PORT_CCS)) {
            usb_printk("xhci: port %d disconnected during reset\n", i);
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        if (portsc & XHCI_PORT_PED) {
            xhci->init_port_speed = xhci_port_speed(
                portsc, xhci_port_is_usb3(xhci, i));
            xhci->init_port_phase = 5;
            usb_printk("xhci: async port %d enabled speed=%d; slot pending\n",
                       i, xhci->init_port_speed);
            return;
        }
        if (xhci->init_port_wait) {
            xhci->init_port_wait--;
            return;
        }
        usb_printk("xhci: async port %d enable timeout\n", i);
        xhci->init_port_phase = 0;
        xhci->init_port_cursor++;
        return;

    case 5: { /* Allocate software state and submit Enable Slot once. */
        struct xhci_trb_t cmd;
        if (i < 64)
            xhci->port_attempted |= (1ULL << i);
        struct usb_dev_t *usb = usb_create_dev(xhci->pci.unit, i,
                                                xhci->init_port_speed);
        if (!usb) {
            usb_printk("xhci: async port %d device allocation failed\n", i);
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        usb->type = USB_TYPE_XHCI;
        usb->priv = (void *)xhci;
        usb->ops = &xhci_ops;
        xhci->init_usb = usb;

        A_memset(&cmd, 0, sizeof(cmd));
        cmd.control = TRB_TYPE(TRB_TYPE_ENABLE_SLOT);
        xhci_post_cmd(xhci, &cmd);
        xhci->init_cmd_wait = 1000;
        xhci->init_port_phase = 6;
        usb_printk("xhci: async port %d Enable Slot submitted\n", i);
        return;
    }

    case 6: { /* Consume at most one event while waiting for Enable Slot. */
        struct xhci_trb_t event;
        if (xhci_get_event(xhci, &event) > 0) {
            uint8_t type = (event.control >> TRB_TRB_TYPE_SHIFT) & 0x3F;
            if (type == TRB_TYPE_CMD_COMP_EVENT) {
                uint8_t cc = (event.status >> 24) & 0xFF;
                uint8_t slot = (event.control >> 24) & 0xFF;
                if (cc != CC_SUCCESS || !slot || slot > xhci->max_slots) {
                    usb_printk("xhci: async Enable Slot failed cc=%d slot=%d\n",
                               cc, slot);
                    usb_destroy_dev(xhci->init_usb);
                    xhci->init_usb = NULL;
                    xhci->init_port_phase = 0;
                    xhci->init_port_cursor++;
                    return;
                }
                xhci->init_slot_id = slot;
                xhci->init_usb->num = slot;
                xhci->addr_bitmap[slot / 32] |= (1 << (slot % 32));
                if (xhci_prepare_slot_context(xhci, slot) < 0) {
                    usb_printk("xhci: async slot %d context allocation failed\n",
                               slot);
                    xhci->addr_bitmap[slot / 32] &= ~(1 << (slot % 32));
                    usb_destroy_dev(xhci->init_usb);
                    xhci->init_usb = NULL;
                    xhci->init_port_phase = 0;
                    xhci->init_port_cursor++;
                    return;
                }
                if (xhci_alloc_transfer_ring(xhci, slot, 1) < 0) {
                    usb_printk("xhci: async slot %d EP0 ring allocation failed\n",
                               slot);
                    usb_destroy_dev(xhci->init_usb);
                    xhci->init_usb = NULL;
                    xhci->init_port_phase = 0;
                    xhci->init_port_cursor++;
                    return;
                }
                xhci->init_port_phase = 7;
                usb_printk("xhci: async slot %d enabled for port %d\n", slot, i);
                return;
            }
        }
        if (xhci->init_cmd_wait) {
            xhci->init_cmd_wait--;
            return;
        }
        usb_printk("xhci: async Enable Slot timeout on port %d\n", i);
        usb_destroy_dev(xhci->init_usb);
        xhci->init_usb = NULL;
        xhci->init_port_phase = 0;
        xhci->init_port_cursor++;
        return;
    }

    case 7: /* Submit Address Device once. */
        if (xhci_submit_address_async(xhci, i) < 0) {
            usb_printk("xhci: async Address Device setup failed slot=%d\n",
                       xhci->init_slot_id);
            usb_destroy_dev(xhci->init_usb);
            xhci->init_usb = NULL;
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        xhci->init_cmd_wait = 1000;
        xhci->init_port_phase = 8;
        usb_printk("xhci: async Address Device submitted slot=%d port=%d\n",
                   xhci->init_slot_id, i);
        return;

    case 8: { /* Consume at most one Address Device completion event. */
        struct xhci_trb_t event;
        if (xhci_get_event(xhci, &event) > 0) {
            uint8_t type = (event.control >> TRB_TRB_TYPE_SHIFT) & 0x3F;
            if (type == TRB_TYPE_CMD_COMP_EVENT) {
                uint64_t completed = event.param & ~0xFULL;
                if (completed != (xhci_last_cmd_phys & ~0xFULL)) {
                    usb_printk("xhci: async Address ignored cmd ptr=%x expect=%x\n",
                               (uint32_t)completed,
                               (uint32_t)xhci_last_cmd_phys);
                    return;
                }
                uint8_t cc = (event.status >> 24) & 0xFF;
                if (cc != CC_SUCCESS) {
                    usb_printk("xhci: async Address Device failed cc=%d slot=%d\n",
                               cc, xhci->init_slot_id);
                    xhci_status_regs("xhci Address Device failed",
                                     "cc=%x slot=%x CRCR=%x",
                                     cc, xhci->init_slot_id,
                                     mmio_read32(xhci->op_base + XHCI_OP_CRCR_LO));
                    usb_destroy_dev(xhci->init_usb);
                    xhci->init_usb = NULL;
                    xhci->init_port_phase = 0;
                    xhci->init_port_cursor++;
                    return;
                }
                xhci->slots[xhci->init_slot_id].addressed = 1;
                xhci->init_port_phase = 9;
                usb_printk("xhci: async Address Device complete slot=%d\n",
                           xhci->init_slot_id);
                xhci_status_regs("xhci Address Device",
                                 "cc=%x slot=%x state=%x",
                                 cc, xhci->init_slot_id,
                                 ((uint32_t *)xhci->slots[
                                     xhci->init_slot_id].dev_ctx)[3] >> 27);
                return;
            }
        }
        if (xhci->init_cmd_wait) {
            xhci->init_cmd_wait--;
            return;
        }
        usb_printk("xhci: async Address Device timeout slot=%d\n",
                   xhci->init_slot_id);
        usb_destroy_dev(xhci->init_usb);
        xhci->init_usb = NULL;
        xhci->init_port_phase = 0;
        xhci->init_port_cursor++;
        return;
    }

    case 9: /* Submit the initial 8-byte Device Descriptor request. */
        if (xhci_submit_get_descriptor_async(xhci, 1, 0, 8, 0) < 0) {
            usb_printk("xhci: async GET_DESCRIPTOR(8) submit failed\n");
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        xhci->init_port_phase = 10;
        usb_printk("xhci: async GET_DESCRIPTOR(8) submitted slot=%d\n",
                   xhci->init_slot_id);
        return;

    case 10: { /* Complete and parse the first eight descriptor bytes. */
        int done = xhci_poll_init_transfer(xhci);
        if (done == 0) return;
        if (done < 0) {
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        uint8_t *d = (uint8_t *)xhci->init_ctrl_buf;
        uint16_t spec = d[2] | ((uint16_t)d[3] << 8);
        uint16_t mps = d[7];
        if (spec >= 0x0300 && mps >= 7 && mps <= 12)
            mps = 1U << mps;
        if (!mps || mps > 1024) {
            usb_printk("xhci: async invalid EP0 MPS=%d\n", mps);
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        xhci->init_usb->spec = spec;
        xhci->init_usb->class = d[4];
        xhci->init_usb->subclass = d[5];
        xhci->init_usb->protocol = d[6];
        xhci->init_usb->endpoints->mps = mps;
        xhci->init_port_phase = 11;
        usb_printk("xhci: async Device Descriptor(8) complete mps=%d\n", mps);
        return;
    }

    case 11: /* Submit the full Device Descriptor request. */
        if (xhci_submit_get_descriptor_async(xhci, 1, 0, 18, 0) < 0) {
            usb_printk("xhci: async GET_DESCRIPTOR(18) submit failed\n");
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        xhci->init_port_phase = 12;
        return;

    case 12: { /* Complete and parse the full Device Descriptor. */
        int done = xhci_poll_init_transfer(xhci);
        if (done == 0) return;
        if (done < 0) {
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        uint8_t *d = (uint8_t *)xhci->init_ctrl_buf;
        xhci->init_usb->vendor = d[8] | ((uint16_t)d[9] << 8);
        xhci->init_usb->product = d[10] | ((uint16_t)d[11] << 8);
        xhci->init_usb->release = d[12] | ((uint16_t)d[13] << 8);
        xhci->init_usb->manufacturerid = d[14];
        xhci->init_usb->productid = d[15];
        xhci->init_usb->serialid = d[16];
        xhci->init_usb->configs = d[17];
        xhci->init_port_phase = 13;
        usb_printk("xhci: async Device Descriptor complete %x:%x configs=%d\n",
                   xhci->init_usb->vendor, xhci->init_usb->product,
                   xhci->init_usb->configs);
        return;
    }

    case 13: /* Read the fixed Configuration Descriptor header first. */
        if (xhci_submit_get_descriptor_async(xhci, 2, 0, 9, 0) < 0) {
            usb_printk("xhci: async Config Descriptor header submit failed\n");
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        xhci->init_port_phase = 14;
        return;

    case 14: { /* Obtain wTotalLength and request the complete descriptor. */
        int done = xhci_poll_init_transfer(xhci);
        if (done == 0) return;
        if (done < 0) {
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        uint8_t *d = (uint8_t *)xhci->init_ctrl_buf;
        uint16_t total = d[2] | ((uint16_t)d[3] << 8);
        if (total < 9 || total > 4096) {
            usb_printk("xhci: async invalid config length=%d\n", total);
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        xhci->init_config_len = total;
        xhci->init_config_value = d[5];
        if (!xhci->init_config_value) {
            usb_printk("xhci: async invalid configuration value 0\n");
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        xhci->init_port_phase = 15;
        return;
    }

    case 15:
        if (xhci_submit_get_descriptor_async(xhci, 2, 0,
                                              xhci->init_config_len, 0) < 0) {
            usb_printk("xhci: async full Config Descriptor submit failed\n");
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        xhci->init_port_phase = 16;
        return;

    case 16: {
        int done = xhci_poll_init_transfer(xhci);
        if (done == 0) return;
        if (done < 0) {
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        usb_printk("xhci: async Config Descriptor complete len=%d\n",
                   xhci->init_config_len);
        xhci->init_port_phase = 17;
        return;
    }

    case 17:
        if (xhci_parse_config_async(xhci) < 0) {
            usb_printk("xhci: async Configuration Descriptor parse failed\n");
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        xhci->init_endpoint = xhci->init_usb->endpoints->next;
        xhci->init_port_phase = 18;
        return;

    case 18: /* Submit Configure Endpoint for one endpoint. */
        if (!xhci->init_endpoint) {
            xhci->init_port_phase = 20;
            return;
        }
        if (xhci_submit_configure_ep_async(xhci, xhci->init_endpoint) < 0) {
            usb_printk("xhci: async Configure Endpoint submit failed ep=%d\n",
                       xhci->init_endpoint->addr);
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        xhci->init_port_phase = 19;
        usb_printk("xhci: async Configure Endpoint submitted ep=%d\n",
                   xhci->init_endpoint->addr);
        return;

    case 19: { /* Wait for one Configure Endpoint command completion. */
        struct xhci_trb_t event;
        if (xhci_get_event(xhci, &event) > 0) {
            uint8_t type = (event.control >> TRB_TRB_TYPE_SHIFT) & 0x3F;
            if (type == TRB_TYPE_CMD_COMP_EVENT) {
                uint64_t completed = event.param & ~0xFULL;
                if (completed != (xhci_last_cmd_phys & ~0xFULL)) {
                    usb_printk("xhci: async Configure ignored cmd ptr=%x expect=%x\n",
                               (uint32_t)completed,
                               (uint32_t)xhci_last_cmd_phys);
                    return;
                }
                uint8_t cc = (event.status >> 24) & 0xFF;
                if (cc != CC_SUCCESS) {
                    usb_printk("xhci: async Configure Endpoint failed cc=%d ep=%d\n",
                               cc, xhci->init_endpoint->addr);
                    xhci->init_port_phase = 0;
                    xhci->init_port_cursor++;
                    return;
                }
                usb_printk("xhci: async Configure Endpoint complete ep=%d\n",
                           xhci->init_endpoint->addr);
                xhci->init_endpoint = xhci->init_endpoint->next;
                xhci->init_port_phase = 18;
                return;
            }
        }
        if (xhci->init_cmd_wait) {
            xhci->init_cmd_wait--;
            return;
        }
        usb_printk("xhci: async Configure Endpoint timeout ep=%d\n",
                   xhci->init_endpoint->addr);
        xhci->init_port_phase = 0;
        xhci->init_port_cursor++;
        return;
    }

    case 20:
        if (xhci_submit_set_config_async(xhci,
                                          xhci->init_config_value) < 0) {
            usb_printk("xhci: async SET_CONFIGURATION submit failed\n");
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        xhci->init_port_phase = 21;
        usb_printk("xhci: async SET_CONFIGURATION(%d) submitted\n",
                   xhci->init_config_value);
        return;

    case 21: {
        int done = xhci_poll_init_transfer(xhci);
        if (done == 0) return;
        if (done < 0) {
            usb_printk("xhci: async SET_CONFIGURATION failed\n");
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        xhci->init_usb->cur_config = xhci->init_config_value;
        xhci->init_port_phase = 22;
        usb_printk("xhci: async SET_CONFIGURATION(%d) complete\n",
                   xhci->init_config_value);
        return;
    }

    case 22: {
        struct usb_interface_t *iface;
        xhci->init_iface = NULL;
        for (iface = xhci->init_usb->interfaces; iface; iface = iface->next) {
            if ((iface->desc.class == 0x08 && iface->desc.subclass == 0x06) ||
                iface->desc.class == 0x03) {
                xhci->init_iface = iface;
                break;
            }
        }
        if (!xhci->init_iface) {
            usb_printk("xhci: async class setup deferred class=%x\n",
                       xhci->init_usb->interfaces ?
                       xhci->init_usb->interfaces->desc.class : 0xFF);
            xhci->init_port_phase = 30;
            return;
        }
        if (xhci->init_iface->desc.class == 0x03) {
            if (!xhci->init_hid_report_len)
                xhci->init_hid_report_len = 64;
            xhci->init_port_phase = 31;
            return;
        }
        if (!xhci->init_iface->endpoint_in || !xhci->init_iface->endpoint_out) {
            usb_printk("xhci: async MSC missing bulk endpoints\n");
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        xhci->init_iface->endpoint_in->toggle = 0;
        xhci->init_iface->endpoint_out->toggle = 0;
        xhci->init_iface->bytes_per_sector = 512;
        xhci->init_scsi_step = 0;
        xhci->init_scsi_tag = 0;
        xhci->init_port_phase = 23;
        usb_printk("xhci: async MSC initialization started\n");
        return;
    }

    case 31: /* Submit HID Report Descriptor request. */
        if (xhci_submit_get_descriptor_async(xhci, 0x22, 0,
                                              xhci->init_hid_report_len,
                                              xhci->init_iface->desc.interfacenum) < 0) {
            usb_printk("xhci: async HID report descriptor submit failed\n");
            xhci->init_port_phase = 30;
            return;
        }
        xhci->init_port_phase = 32;
        usb_printk("xhci: async HID report descriptor submitted len=%d\n",
                   xhci->init_hid_report_len);
        return;

    case 32: {
        int done = xhci_poll_init_transfer(xhci);
        if (done == 0) return;
        if (done < 0) {
            usb_printk("xhci: async HID report descriptor failed\n");
            xhci->init_port_phase = 30;
            return;
        }
        usb_printk("xhci: async HID report descriptor complete len=%d\n",
                   xhci->init_hid_report_len);
        if (usb_hid_register_async(xhci->init_iface,
                                   (uint8_t *)xhci->init_ctrl_buf,
                                   xhci->init_hid_report_len) < 0) {
            usb_printk("xhci: async HID register failed\n");
            xhci->init_port_phase = 30;
            return;
        }
        xhci->init_port_phase = 30;
        return;
    }

    case 23: /* Prepare and submit one SCSI CBW. */
        xhci_prepare_scsi_async(xhci);
        xhci_dump_msc_packet("CBW", (const void *)&xhci->init_cbw, 31);
        if (xhci_submit_bulk_async(xhci, xhci->init_iface->endpoint_out,
                                   (void *)&xhci->init_cbw, 31, 0) < 0) {
            usb_printk("xhci: async MSC CBW submit failed step=%d\n",
                       xhci->init_scsi_step);
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        xhci->init_port_phase = 24;
        usb_printk("xhci: async MSC CBW submitted cmd=%x tag=%x\n",
                   xhci->init_cbw.cmd[0], xhci->init_cbw.tag);
        return;

    case 24: {
        int done = xhci_poll_bulk_async(xhci);
        if (done == 0) return;
        if (done < 0) {
            usb_printk("xhci: async MSC CBW failed cmd=%x\n",
                       xhci->init_cbw.cmd[0]);
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        xhci->init_port_phase = xhci->init_scsi_len ? 25 : 27;
        return;
    }

    case 25: /* Submit the optional SCSI data-in stage. */
        if (xhci_submit_bulk_async(xhci, xhci->init_iface->endpoint_in,
                                   (void *)xhci->init_scsi_data,
                                   xhci->init_scsi_len, 1) < 0) {
            usb_printk("xhci: async MSC data submit failed cmd=%x\n",
                       xhci->init_cbw.cmd[0]);
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        xhci->init_port_phase = 26;
        return;

    case 26: {
        int done = xhci_poll_bulk_async(xhci);
        if (done == 0) return;
        if (done < 0) {
            usb_printk("xhci: async MSC data failed cmd=%x\n",
                       xhci->init_cbw.cmd[0]);
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        xhci_dump_msc_packet("DATA-IN", (const void *)xhci->init_scsi_data,
                             xhci->init_scsi_len);
        xhci->init_port_phase = 27;
        return;
    }

    case 27: /* Submit CSW read. */
        if (xhci_submit_bulk_async(xhci, xhci->init_iface->endpoint_in,
                                   (void *)xhci->init_scsi_csw, 13, 1) < 0) {
            usb_printk("xhci: async MSC CSW submit failed\n");
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        xhci->init_port_phase = 28;
        return;

    case 28: {
        int done = xhci_poll_bulk_async(xhci);
        if (done == 0) return;
        if (done < 0 || xhci_check_csw_async(xhci) < 0) {
            usb_printk("xhci: async MSC CSW failed cmd=%x\n",
                       xhci->init_cbw.cmd[0]);
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        usb_printk("xhci: async MSC command complete cmd=%x\n",
                   xhci->init_cbw.cmd[0]);
        if (xhci->init_scsi_step < 2) {
            xhci->init_scsi_step++;
            xhci->init_port_phase = 23;
            return;
        }
        xhci->init_port_phase = 29;
        return;
    }

    case 29: { /* Parse READ CAPACITY and create non-blocking disk metadata. */
        uint8_t *d = (uint8_t *)xhci->init_scsi_data;
        uint32_t lba = ((uint32_t)d[0] << 24) | ((uint32_t)d[1] << 16) |
                       ((uint32_t)d[2] << 8) | d[3];
        uint32_t blksz = ((uint32_t)d[4] << 24) | ((uint32_t)d[5] << 16) |
                         ((uint32_t)d[6] << 8) | d[7];
        if (!blksz || blksz > 4096) {
            usb_printk("xhci: async MSC invalid block size=%d\n", blksz);
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        xhci->init_disk = kmalloc(sizeof(*xhci->init_disk));
        if (!xhci->init_disk) {
            xhci->init_port_phase = 0;
            xhci->init_port_cursor++;
            return;
        }
        A_memset(xhci->init_disk, 0, sizeof(*xhci->init_disk));
        xhci->init_disk->type = IDE_UNKNOWN;
        xhci->init_disk->priv = xhci->init_iface;
        xhci->init_disk->bytes_per_sector = blksz;
        xhci->init_disk->size = ((uint64_t)lba + 1) * blksz;
        xhci->init_iface->bytes_per_sector = blksz;
        usb_printk("xhci: async MSC capacity lba=%d block=%d size_mb=%d\n",
                   lba, blksz,
                   (uint32_t)(xhci->init_disk->size / (1024 * 1024)));
        {
            char status[96];
            snprintf(status, sizeof(status), "lba=%d block=%d size_mb=%d",
                     lba, blksz,
                     (uint32_t)(xhci->init_disk->size / (1024 * 1024)));
            kernel_usb_msc_status_line("MSC capacity", status);
        }
        xhci->init_port_phase = 30;
        return;
    }

    case 30: { /* Publish the slot and continue with the next root port. */
        int slot = xhci->init_slot_id;
        xhci->slots[slot].usb = xhci->init_usb;
        xhci->slots[slot].enabled = 1;
        xhci->slots[slot].configured = 1;
        xhci->slots[slot].speed = xhci->init_port_speed;
        xhci->slots[slot].port = i;
        usb_printk("xhci: async port %d ready slot=%d speed=%d\n",
                   i, slot, xhci->init_port_speed);
        xhci->init_usb = NULL;
        xhci->init_iface = NULL;
        xhci->init_endpoint = NULL;
        xhci->init_port_phase = 0;
        xhci->init_port_cursor++;
            return;
        }
    }
}

void xhci_init_ports_step(void)
{
    volatile struct xhci_dev_t *xhci = first_xhci;
    while (xhci) {
        uint32_t ready;
        if ((xhci->flags & XHCI_FLAG_RUN) &&
            !(xhci->flags & XHCI_FLAG_PORTSENABLED))
            xhci_enable_ports(xhci);
        xhci = xhci->next;
    }
}

static int xhci_find_slot_by_port(volatile struct xhci_dev_t *xhci,
                                  uint32_t port)
{
    int slot;

    for (slot = 1; slot <= xhci->max_slots; slot++) {
        if (xhci->slots[slot].usb && xhci->slots[slot].port == port)
            return slot;
    }

    return 0;
}

static void xhci_remove_port_device(volatile struct xhci_dev_t *xhci,
                                    uint32_t port)
{
    int slot = xhci_find_slot_by_port(xhci, port);
    struct usb_dev_t *usb;
    struct usb_interface_t *iface;

    if (!slot)
        return;

    usb = xhci->slots[slot].usb;
    for (iface = usb ? usb->interfaces : NULL; iface; iface = iface->next)
        usb_hid_remove(iface);

    usb_printk("xhci: async remove port %d slot=%d\n", port, slot);
    if (usb)
        usb_destroy_dev(usb);

    xhci->slots[slot].usb = NULL;
    xhci->slots[slot].enabled = 0;
    xhci->slots[slot].addressed = 0;
    xhci->slots[slot].configured = 0;
    xhci->slots[slot].speed = 0;
    xhci->slots[slot].port = 0;
}

static void xhci_schedule_port_enum(volatile struct xhci_dev_t *xhci,
                                    uint32_t port)
{
    if (xhci->init_usb && xhci->init_port_cursor == port)
        return;

    if (xhci_find_slot_by_port(xhci, port)) {
        usb_printk("xhci: port %d already has a device, skip enum\n", port);
        return;
    }

    xhci->init_port_cursor = port;
    xhci->init_port_phase = 0;
    xhci->init_port_wait = 0;
    xhci->init_cmd_wait = 0;
    xhci->flags &= ~XHCI_FLAG_PORTSENABLED;
    usb_printk("xhci: scheduled async enum for hotplug port %d\n", port);
}

static void xhci_out_transaction(struct usb_transaction_t *transaction)
{
    struct usb_transfer_t *transfer = (struct usb_transfer_t *)transaction->transfer;
    /* Skip if SET_ADDRESS already handled via Address Device command */
    if (transfer->trans_head && transfer->trans_head->data == (void *)0x1) return;
    struct usb_dev_t      *usb      = transaction->dev;
    struct xhci_dev_t     *xhci     = (struct xhci_dev_t *)usb->priv;
    int slot_id = usb->num;
    int ep_id   = 1;
    int is_ctrl = (transfer->type == USB_TRANSFER_CTRL);

    ep_id = xhci_transfer_ep_id(transfer);
    struct xhci_ring_t *tr = &xhci->slots[slot_id].ep_rings[ep_id];

    if (!tr->ring) {
        usb_printk("%s: no transfer ring for OUT ep=%d\n", "xhci", ep_id);
        return;
    }

    struct xhci_trb_t out_trb;
    A_memset(&out_trb, 0, sizeof(out_trb));

    if (transaction->len > 0 && transaction->buf) {
        uint32_t data_phys;
        void *data_buf = dma_alloc_coherent(transaction->len, &data_phys);
        if (!data_buf) {
            usb_printk("%s: failed to alloc OUT buffer\n", "xhci");
            return;
        }
        A_memcpy(data_buf, transaction->buf, transaction->len);
        out_trb.param   = (uint64_t)data_phys;
        out_trb.status  = TRB_XFER_LEN(transaction->len) |
                          TRB_TD_SIZE(0) |
                          TRB_INTR_TARGET(0);
        transaction->data = data_buf;
        if (is_ctrl) {
            out_trb.control = TRB_TYPE(TRB_TYPE_DATA_STAGE);
        } else {
            out_trb.control = TRB_TYPE(TRB_TYPE_NORMAL) | TRB_IO;
        }
    } else {
        out_trb.param   = 0;
        out_trb.status  = 0;
        if (is_ctrl) {
            out_trb.control = TRB_TYPE(TRB_TYPE_STATUS_STAGE) | TRB_IO;
        } else {
            return;  /* Interrupt/Bulk zero-length �?nothing to do */
        }
    }

    {
        int idx = ring_enqueue(tr, &out_trb);
        if (idx >= 0) {
            transaction->hc_trb_phys =
                tr->phys + (uintptr_t)idx * sizeof(struct xhci_trb_t);
        }
    }
}

/* ================================================================
 * Configure Endpoint �?internal helpers
 * ================================================================ */

/* USB bEndpointAddress �?xHCI DCI */
static inline int xhci_ep_addr_to_dci(uint8_t ep_addr)
{
    int ep_num = ep_addr & 0x0F;
    int is_in  = ep_addr & 0x80;
    if (ep_num == 0)
        return 1; /* EP0 is one bidirectional control context */
    return ep_num * 2 + (is_in ? 1 : 0);
}

static uint8_t xhci_interval(uint32_t speed, uint8_t binterval)
{
    if (binterval == 0) binterval = 1;
    if (speed == USB_SPEED_HIGH || speed == USB_SPEED_SUPER)
        return (binterval > 0) ? (uint8_t)(binterval - 1) : 0;
    /* Full/low-speed bInterval is expressed in 1 ms frames.  xHCI stores
     * the periodic interval as floor(log2(frames * 8)) microframes.  Keep
     * the value in the range accepted for FS/LS periodic endpoints. */
    uint32_t microframes = (uint32_t)binterval * 8;
    uint8_t v = 0;
    while (microframes > 1 && v < 15) {
        microframes >>= 1;
        v++;
    }
    if (v < 3) v = 3;
    if (v > 10) v = 10;
    return v;
}

/* Shared: build input context + send Configure Endpoint command.
 * dci and xhci_ep_type are pre-computed by the caller.            */
static int xhci_configure_ep_do(volatile struct xhci_dev_t *xhci,
                                 int slot_id, int dci,
                                 uint8_t xhci_ep_type,
                                 uint16_t mps, uint8_t interval)
{
    uint32_t *ctx;
    struct xhci_trb_t cmd;

    int ctx_bytes = (dci + 2) * xhci->context_size;
    uint32_t ctx_phys;
    void *ctx_virt = dma_alloc_coherent(ctx_bytes, &ctx_phys);
    if (!ctx_virt) return -ENOMEM;
    A_memset(ctx_virt, 0, ctx_bytes);
    ctx = (uint32_t *)ctx_virt;

    /* Input Control */
    ctx[0] = 0;
    ctx[1] = (1 << 0) | (1 << dci);

    /* Slot Context */
    uint32_t *out  = (uint32_t *)xhci->slots[slot_id].dev_ctx;
    uint32_t speed = (out[0] >> 20) & 0xF;
    int ce = dci;
    if (ce < 2) ce = 2;
    uint32_t *slot_ctx = xhci_input_slot_ctx(xhci, ctx);
    slot_ctx[0] = (ce << 27) | (speed << 20);
    slot_ctx[1] = out[1];
    slot_ctx[2] = out[2];
    slot_ctx[3] = out[3];

    /* EP Context */
    uint32_t *ep = xhci_input_ep_ctx(xhci, ctx, dci);
    uint32_t deq_lo = (uint32_t)xhci->slots[slot_id].ep_rings[dci].phys;
    deq_lo |= 1;

    uint8_t encoded_interval = xhci_interval(speed, interval);
    ep[0] = ((uint32_t)encoded_interval << 16);
    ep[1] = ((uint32_t)mps << 16) | ((uint32_t)xhci_ep_type << 3) | (3 << 1);
    ep[2] = deq_lo;
    ep[3] = 0;
    /* Average TRB Length is a byte count, not packets-per-TRB. */
    ep[4] = mps ? mps : 1;
    ep[5] = 0; ep[6] = 0; ep[7] = 0;

    usb_printk("xhci: Configure EP dci=%d type=%d speed=%d mps=%d "
               "interval=%d encoded=%d deq=%x\n",
        dci, xhci_ep_type, speed, mps, interval, encoded_interval, deq_lo);

    A_memset(&cmd, 0, sizeof(cmd));
    cmd.param  = (uint64_t)ctx_phys;
    cmd.control = TRB_TYPE(TRB_TYPE_CONFIGURE_EP) |
                  ((slot_id & 0xFF) << 24);
    xhci_post_cmd(xhci, &cmd);

    int cc = xhci_wait_cmd(xhci, 500);
    usb_printk("xhci: Configure EP returned cc=%d\n", cc);

    /* Verify */
    {
        uint32_t *o   = (uint32_t *)xhci->slots[slot_id].dev_ctx;
        uint32_t *epc = xhci_output_ep_ctx(xhci, o, dci);
        usb_printk("xhci:   Output EP[dci=%d]: dw0=%x dw1=%x (state=%d type=%d)\n",
            dci, epc[0], epc[1], epc[0] & 0x7, (epc[1] >> 3) & 0x7);
    }

    return (cc == CC_SUCCESS) ? 0 : -EIO;
}

/* ---- mouse (interrupt) endpoint ---- */
static int xhci_configure_endpoint_mouse(volatile struct xhci_dev_t *xhci,
                                          int slot_id, uint8_t ep_addr,
                                          uint16_t mps, uint8_t interval)
{
    int dci  = xhci_ep_addr_to_dci(ep_addr);
    int is_in = ep_addr & 0x80;
    /* xHCI interrupt: 3=OUT, 7=IN */
    uint8_t xhci_type = is_in ? 7 : 3;
    return xhci_configure_ep_do(xhci, slot_id, dci, xhci_type, mps, interval);
}

/* ---- bulk (storage) endpoint ---- */
static int xhci_configure_endpoint_bulk(volatile struct xhci_dev_t *xhci,
                                         int slot_id, uint8_t ep_addr,
                                         uint16_t mps)
{
    int dci  = xhci_ep_addr_to_dci(ep_addr);
    int is_in = ep_addr & 0x80;
    /* xHCI bulk: 2=OUT, 6=IN */
    uint8_t xhci_type = is_in ? 6 : 2;

    /* Cap MPS for non-SuperSpeed: USB 2.0 HS max is 512 */
    uint32_t *out  = (uint32_t *)xhci->slots[slot_id].dev_ctx;
    uint32_t speed = (out[0] >> 20) & 0xF;
    if (speed != 4 && mps > 512)
        mps = 512;

    return xhci_configure_ep_do(xhci, slot_id, dci, xhci_type, mps, 0);
}

/* ---- unified entry: dispatch by ep_type ---- */
static int xhci_configure_endpoint(volatile struct xhci_dev_t *xhci,
                                   int slot_id, uint8_t ep_addr,
                                   uint8_t ep_type, uint16_t mps,
                                   uint8_t interval)
{
    if (ep_type == USB_ENDPOINT_INTERRUPT)
        return xhci_configure_endpoint_mouse(xhci, slot_id, ep_addr, mps, interval);
    if (ep_type == USB_ENDPOINT_BULK)
        return xhci_configure_endpoint_bulk(xhci, slot_id, ep_addr, mps);
    return -EINVAL;
}

static void xhci_setup_transfer(struct usb_transfer_t *transfer)
{
    struct xhci_dev_t *xhci = (struct xhci_dev_t *)transfer->dev->priv;
    int slot_id = transfer->dev->num;
    int ep_id   = 1;  /* Default: EP0 */
    int new_ring = 0;

    if (transfer->type == USB_TRANSFER_ISOCHRONOUS) {
        usb_printk("%s: isochronous not implemented\n", "xhci");
        return;
    }

    /* For non-control endpoints, use the endpoint address */
    ep_id = xhci_transfer_ep_id(transfer);

    /* Allocate transfer ring if needed; for non-EP0, also configure endpoint */
    if (!xhci->slots[slot_id].ep_rings[ep_id].ring) {
        if (xhci_ring_init(&xhci->slots[slot_id].ep_rings[ep_id],
                           XHCI_TRANSFER_RING_SIZE) < 0)
            return;
        new_ring = 1;
    }

    /* If endpoint is HALTED (e.g. after STALL), re-configure to move
     * it back to RUNNING.  CLEAR_FEATURE clears the device side but
     * the HC's endpoint context stays halted until re-configured.    */
    if (ep_id > 1) {
        uint32_t *o = (uint32_t *)xhci->slots[slot_id].dev_ctx;
        uint32_t *epc = xhci_output_ep_ctx(xhci, o, ep_id);
        uint32_t ep_state = epc[0] & 0x7;
        usb_printk("xhci: SETUP_TRANSFER ep=%d state=%d new_ring=%d\n",
                   ep_id, ep_state, new_ring);
        if (new_ring || ep_state == EP_STATE_DISABLED) {
            uint8_t ep_addr = transfer->endpoint->addr;
            if (transfer->endpoint->direction == USB_ENDPOINT_IN)
                ep_addr |= 0x80;
            usb_printk("xhci: SETUP_TRANSFER configuring ep=0x%02x (was state=%d)\n",
                ep_addr, ep_state);
            xhci_configure_endpoint(xhci, slot_id, ep_addr,
                transfer->endpoint->type, transfer->endpoint->mps,
                transfer->endpoint->interval);
        } else if (ep_state != EP_STATE_RUNNING) {
            uint8_t ep_addr = transfer->endpoint->addr;
            if (transfer->endpoint->direction == USB_ENDPOINT_IN)
                ep_addr |= 0x80;
            usb_printk("xhci: SETUP_TRANSFER resetting ep=0x%02x (was state=%d)\n",
                ep_addr, ep_state);
            /* Reset ring 鈥斺�?HC already processed old TRBs, start fresh */
            xhci_reset_endpoint_ring(xhci, slot_id, ep_id);
        }
    }

    if (transfer->type == USB_TRANSFER_INTERRUPT) {
        usb_printk("xhci: HID interrupt transfer ready slot=%d dci=%d mps=%d\n",
                   slot_id, ep_id,
                   transfer->endpoint ? transfer->endpoint->mps : 0);
    }

    transfer->data = (void *)&xhci->slots[slot_id].ep_rings[ep_id];
}


static void xhci_schedule_transfer(struct usb_transfer_t *transfer)
{
    struct xhci_dev_t *xhci = (struct xhci_dev_t *)transfer->dev->priv;
    int slot_id = transfer->dev->num;

    /* Check if SET_ADDRESS was already handled via Address Device command */
    if (transfer->trans_head && transfer->trans_head->data == (void *)0x1) {
        return;  /* Already done �?no TRBs to queue */
    }

    /* Ring doorbell for the slot's EP0 (endpoint 1)
     * Doorbell target = slot_id, stream = 0
     * DB register value = slot_id | (endpoint_id << 0)?
     * Actually: DB target = endpoint_id (1 for EP0), DB stream = 0
     *
     * Wait �?the doorbell format is:
     *   DB[slot_id] = endpoint_id | (stream_id << 16)
     * For EP0, endpoint_id = 1, stream_id = 0
     */
    /* Compute DCI: EP0=1, EPn OUT = n*2, EPn IN = n*2+1 */
    int ep_id = xhci_transfer_ep_id(transfer);
    uint32_t db_val = (uint32_t)ep_id;

    if (transfer->type == USB_TRANSFER_CTRL) {
        struct xhci_ring_t *ring =
            &xhci->slots[slot_id].ep_rings[ep_id];
        uint32_t *out = (uint32_t *)xhci->slots[slot_id].dev_ctx;
        uint32_t *ep0 = xhci_output_ep_ctx(xhci, out, 1);
        uint32_t n = ring->enqueue;
        uint32_t i0 = (n + ring->size - 3) % ring->size;
        uint32_t i1 = (n + ring->size - 2) % ring->size;
        uint32_t i2 = (n + ring->size - 1) % ring->size;
        xhci_status_regs("xhci EP0 submit", "enq=%x ccs=%x deq=%x",
                         n, ring->ccs, ep0[2]);
        xhci_status_regs("xhci EP0 TRBs", "setup=%x data=%x status=%x",
                         ring->ring[i0].control,
                         ring->ring[i1].control,
                         ring->ring[i2].control);
        xhci_dump_trb("xhci SETUP TRB 16B", &ring->ring[i0]);
        xhci_dump_trb("xhci DATA TRB 16B", &ring->ring[i1]);
        xhci_dump_trb("xhci STATUS TRB 16B", &ring->ring[i2]);
    }

    xhci_mmio_barrier();
    if (transfer->type == USB_TRANSFER_INTERRUPT) {
        struct xhci_ring_t *ring =
            &xhci->slots[slot_id].ep_rings[ep_id];
        usb_printk("xhci: HID doorbell slot=%d dci=%d db=%d "
                   "enq=%d deq=%d ccs=%d ring=%x\n",
                   slot_id, ep_id, db_val,
                   ring->enqueue, ring->dequeue, ring->ccs,
                   (uint32_t)ring->phys);
    }
    mmio_write32(xhci->db_base + (slot_id * 4), db_val);
    /* Flush: read back the same doorbell register we just wrote */
    mmio_read32(xhci->db_base + (slot_id * 4));
}

static void xhci_wait_transfer(struct usb_transfer_t *transfer)
{
    struct xhci_dev_t *xhci = (struct xhci_dev_t *)transfer->dev->priv;

    /* SET_ADDRESS already handled via Address Device command �?instant success */
    if (transfer->trans_head && transfer->trans_head->data == (void *)0x1) {
        transfer->success = 1;
        usb_printk("xhci: SET_ADDRESS already done via AddrDevice, success\n");
        return;
    }

    /* Compute the DCI we're waiting for.
     * CRITICAL: only accept Transfer Events for this endpoint.
     * Without this filter, a leftover event from a previous transfer
     * (e.g. CSW on EP3) would be consumed by a new transfer on EP4,
     * causing the new transfer to falsely appear complete.              */
    int my_ep = xhci_transfer_ep_id(transfer);
    int endpoint_retries = 0;

    struct xhci_trb_t event;
    volatile int timeout = 1000000;  /* 1 second */

    while (timeout--) {
        /* Keep HID interrupt pipes serviced while synchronous transfers wait. */
        usb_poll_interrupts();
        /* The event ring is shared with already-registered HID transfers.
         * Do not consume a foreign mouse event and then discard it while a
         * synchronous MSC/control transfer is waiting.  Let the normal HID
         * poller consume and re-arm its own event first. */
        {
            struct xhci_trb_t *pending =
                &xhci->event_ring.ring[xhci->event_ring.dequeue];
            if ((pending->control & 0x1) == xhci->event_ring.ccs) {
                uint8_t pending_type =
                    (pending->control >> TRB_TRB_TYPE_SHIFT) & 0x3F;
                if (pending_type == TRB_TYPE_TRANSFER_EVENT) {
                    uint8_t pending_slot = (pending->control >> 24) & 0xFF;
                    uint8_t pending_ep = (pending->control >> 16) & 0x1F;
                    if (pending_slot != (uint8_t)transfer->dev->num ||
                        pending_ep != (uint8_t)my_ep) {
                        uint32_t before = xhci->event_ring.dequeue;
                        usb_poll_interrupts();
                        if (xhci->event_ring.dequeue != before) {
                            tick_delay(1);
                            continue;
                        }
                    }
                }
            }
        }
        if (xhci_get_event(xhci, &event) > 0) {
            uint8_t trb_type = (event.control >> TRB_TRB_TYPE_SHIFT) & 0x3F;
            if (trb_type == TRB_TYPE_TRANSFER_EVENT) {
                uint8_t  cc    = (event.status >> 24) & 0xFF;
                uint8_t  ep_id = (event.control >> 16) & 0x1F;
                uint8_t  event_slot = (event.control >> 24) & 0xFF;
                uint32_t residual = event.status & 0x00FFFFFF;
                (void)event_slot;
                (void)residual;
                /* Ignore events for other endpoints �?they belong to a
                 * different (or leftover) transfer.  Do NOT return
                 * success/failure; just keep waiting.                    */
                if (ep_id != (uint8_t)my_ep ||
                    event_slot != (uint8_t)transfer->dev->num) {
                    continue;
                }

                /* A Slot/Endpoint match is not sufficient: EP0 is shared by
                 * every control request on the device and stale completion
                 * events can otherwise make a new request look successful.
                 * The event pointer must identify this transaction's final
                 * TRB (normally the Status TRB). */
                uint32_t expected_ptr = transfer->trans_tail
                    ? (uint32_t)transfer->trans_tail->hc_trb_phys : 0;
                int event_ptr_in_transaction = 0;
                struct usb_transaction_t *trb = transfer->trans_head;
                while (trb) {
                    if ((uint32_t)event.param == (uint32_t)trb->hc_trb_phys) {
                        event_ptr_in_transaction = 1;
                        break;
                    }
                    if (trb == transfer->trans_tail)
                        break;
                    trb = trb->next;
                }
                /* Some physical xHCI implementations report a zero TRB
                 * pointer.  In that case slot + DCI are the only usable
                 * ownership keys.  Keep strict pointer matching whenever
                 * hardware supplies a nonzero pointer. */
                if (transfer->type == USB_TRANSFER_CTRL &&
                    expected_ptr && event.param &&
                    !event_ptr_in_transaction) {
                    xhci_error_regs("xhci transfer event mismatch",
                                    "ptr=%x expect=%x slot=%x",
                                    (uint32_t)event.param, expected_ptr,
                                    (uint32_t)event_slot);
                    continue;
                }

                /* Completion Code 0 is reserved.  Never treat it as a
                 * successful transfer: accepting it can expose an untouched
                 * zero-filled DMA buffer as a USB descriptor. */
                if (cc == 0) {
                    xhci_error_regs("xhci invalid completion",
                                    "cc=%x remain=%x slot=%x",
                                    (uint32_t)cc, residual,
                                    (uint32_t)event_slot);
                    transfer->success = 0;
                    return;
                }

                if (cc == CC_SUCCESS || cc == CC_SHORT_PACKET) {
                    if (transfer->type == USB_TRANSFER_BULK &&
                        transfer->trans_head) {
                        uint32_t requested = transfer->trans_head->len;
                        uint32_t actual = residual <= requested
                            ? requested - residual : 0;
                        transfer->actual_length = (uint16_t)actual;
                    }
                    /* Successful transfers are extremely frequent during
                     * USB FAT scans; logging each one floods the terminal
                     * and makes the write path look much slower than it is. */
                    transfer->success = 1;
                    return;
                }

                /* Non-success: fail */
                if (cc == CC_ENDPOINT_NOT_ENABLED && my_ep > 1 &&
                    endpoint_retries++ == 0 && transfer->trans_head) {
                    volatile struct usb_transaction_t *trans =
                        transfer->trans_head;
                    struct xhci_ring_t *ring =
                        &xhci->slots[transfer->dev->num].ep_rings[my_ep];
                    struct xhci_trb_t retry;
                    uint8_t ep_addr = transfer->endpoint->addr;
                    uint32_t data_phys = trans->data &&
                        trans->data != (void *)0x1
                        ? (uint32_t)virt_to_phys(trans->data) : 0;

                    if (transfer->endpoint->direction == USB_ENDPOINT_IN)
                        ep_addr |= 0x80;
                    A_memset(ring->ring, 0,
                             ring->size * sizeof(struct xhci_trb_t));
                    ring->ring[ring->size - 1].param = ring->phys;
                    ring->ring[ring->size - 1].control =
                        TRB_TYPE(TRB_TYPE_LINK) | TRB_TC | TRB_CYCLE;
                    ring->enqueue = 0;
                    ring->dequeue = 0;
                    ring->ccs = 1;
                    xhci_mmio_barrier();

                    usb_printk("xhci: sync EP%d disabled; reconfigure and "
                               "retry transfer\n", my_ep);
                    if (xhci_configure_endpoint(
                            xhci, transfer->dev->num, ep_addr,
                            transfer->endpoint->type,
                            transfer->endpoint->mps,
                            transfer->endpoint->interval) == 0 &&
                        data_phys) {
                        A_memset(&retry, 0, sizeof(retry));
                        retry.param = data_phys;
                        retry.status = trans->len;
                        retry.control = TRB_TYPE(TRB_TYPE_NORMAL) | TRB_IO;
                        {
                            int idx = ring_enqueue(ring, &retry);
                            if (idx >= 0) {
                                trans->hc_trb_phys = ring->phys +
                                    (uintptr_t)idx * sizeof(struct xhci_trb_t);
                                mmio_write32(xhci->db_base +
                                             transfer->dev->num * 4, my_ep);
                                mmio_read32(xhci->db_base +
                                            transfer->dev->num * 4);
                                continue;
                            }
                        }
                    }
                }
                xhci_error_regs("xhci transfer failed", "cc=%x ep=%x remain=%x",
                                (uint32_t)cc, (uint32_t)ep_id, residual);
                xhci_status_regs("xhci transfer event", "ptr=%x expect=%x slot=%x",
                                (uint32_t)event.param,
                                transfer->trans_tail
                                    ? (uint32_t)transfer->trans_tail->hc_trb_phys
                                    : 0,
                                (uint32_t)event_slot);
                if (cc == CC_STALL_ERROR && my_ep == 1) {
                    int reset_res = xhci_reset_endpoint_ring(
                        xhci, transfer->dev->num, 1);
                    if (reset_res < 0)
                        xhci_error_regs("xhci EP0 recovery failed",
                                        "slot=%x res=%x state=%x",
                                        (uint32_t)transfer->dev->num,
                                        (uint32_t)reset_res, 0);
                    else
                        xhci_status_regs("xhci EP0 recovered",
                                         "slot=%x cc=%x ring=%x",
                                         (uint32_t)transfer->dev->num,
                                         (uint32_t)cc,
                                         (uint32_t)xhci->slots[
                                             transfer->dev->num].ep_rings[1].phys);
                }
                transfer->success = 0;
                return;
            } else if (trb_type == TRB_TYPE_CMD_COMP_EVENT) {
                /* Command completions during transfer �?ignore */
            } else if (trb_type == TRB_TYPE_PORT_STATUS_EVENT) {
                usb_printk("xhci: port status event during transfer\n");
            }
        }
        tick_delay(1);
    }

    xhci_error_regs("xhci transfer timeout", "slot=%x ep=%x ERDP=%x",
                    (uint32_t)transfer->dev->num, (uint32_t)my_ep,
                    (uint32_t)(xhci->event_ring.phys +
                        xhci->event_ring.dequeue * sizeof(struct xhci_trb_t)));
    transfer->success = 0;
}

int xhci_poll_transfer(struct usb_transfer_t *transfer)
{
    struct xhci_dev_t *xhci = (struct xhci_dev_t *)transfer->dev->priv;

    /* STALL recovery: sync CLEAR_FEATURE + re-arm, all in one go.
     * Bypasses the heavy control-transfer API to stay within
     * 7 KB stack.  Writes CLEAR_FEATURE TRBs directly to EP0,
     * waits for its Transfer Event, THEN re-arms EP3.
     * No race: EP3 IN token only goes out AFTER device halt is cleared. */
    if (transfer->need_clear_halt == 1) {
        int slot_id = transfer->dev->num;
        uint16_t ep_addr = transfer->endpoint->addr;
        if (transfer->endpoint->direction == USB_ENDPOINT_IN)
            ep_addr |= 0x80;
        uint32_t ep_val = 1;
        if (transfer->endpoint->addr != 0) {
            ep_val = transfer->endpoint->addr * 2;
            if (transfer->endpoint->direction == USB_ENDPOINT_IN)
                ep_val |= 1;
        }

        usb_printk("xhci: EP%d STALL recovery (sync CLEAR_FEATURE)\n",
                   ep_val);

        /* ---- Phase 1: CLEAR_FEATURE on EP0 (lightweight) ---- */
        {
            struct xhci_ring_t *ep0 = &xhci->slots[slot_id].ep_rings[1];

            if (ep0->ring) {
                struct xhci_trb_t setup, status;
                uint8_t *b;
                A_memset(&setup, 0, sizeof(setup));
                b = (uint8_t *)&setup.param;
                b[0] = 0x02; b[1] = 1;        /* CLEAR_FEATURE */
                b[4] = ep_addr & 0xFF;          /* wIndex = EP addr */
                setup.status  = 8;
                setup.control = TRB_TYPE(TRB_TYPE_SETUP_STAGE)
                              | TRB_IDT;

                A_memset(&status, 0, sizeof(status));
                status.control = TRB_TYPE(TRB_TYPE_STATUS_STAGE)
                               | TRB_DIR_IN | TRB_IO;

                ring_enqueue(ep0, &setup);
                ring_enqueue(ep0, &status);
                mmio_write32(xhci->db_base + (slot_id * 4), 1);
                mmio_read32(xhci->db_base + (slot_id * 4));

                /* Wait for EP0 Transfer Event �?minimal loop, no printf spam */
                {
                    volatile int to = 500000;
                    while (to--) {
                        struct xhci_trb_t *trb =
                            &xhci->event_ring.ring[xhci->event_ring.dequeue];
                        if ((trb->control & 0x1) != xhci->event_ring.ccs) {
                            tick_delay(1);
                            continue;
                        }
                        uint8_t typ = (trb->control >> 10) & 0x3F;
                        uint8_t cc2 = (trb->status >> 24) & 0xFF;
                        uint8_t ep2 = (trb->control >> 16) & 0x1F;
                        /* Advance dequeue */
                        xhci->event_ring.dequeue++;
                        if (xhci->event_ring.dequeue >= xhci->event_ring.size)
                            { xhci->event_ring.dequeue = 0; xhci->event_ring.ccs ^= 1; }
                        uint64_t erdp = xhci->event_ring.phys +
                            xhci->event_ring.dequeue * sizeof(struct xhci_trb_t);
                        xhci_write64(xhci, XHCI_RT_ERDP_LO(xhci->rt_base - xhci->mmio_base), erdp | (1<<3));
                        if (typ == 32 && ep2 == 1) {
                            usb_printk("xhci: CLEAR_FEATURE done cc=%d\n", cc2);
                            break;
                        }
                        /* else: other event (CmdComp etc.) �?consumed, ignore */
                        tick_delay(1);
                    }
                }
            }
        }

        /* ---- Phase 2: re-arm after CLEAR_FEATURE (HC side) ---- */
        usb_printk("xhci: EP 0x%02x re-arm after CLEAR_FEATURE\n", ep_addr);
        struct xhci_ring_t *tr = &xhci->slots[slot_id].ep_rings[ep_val];
        if (tr->ring && xhci_reset_endpoint_ring(xhci, slot_id, ep_val) == 0) {
            uint32_t ilen = transfer->endpoint->mps;
            if (ilen < 3) ilen = 3;
            uint32_t dp; void *db = dma_alloc_coherent(ilen, &dp);
            if (db) {
                volatile struct usb_transaction_t *trans = transfer->trans_head;
                if (trans)
                    trans->data = db;
                A_memset(db, 0, ilen);
                struct xhci_trb_t dt;
                A_memset(&dt, 0, sizeof(dt));
                dt.param = dp;
                dt.status = TRB_XFER_LEN(ilen) |
                            TRB_TD_SIZE(0) |
                            TRB_INTR_TARGET(0);
                dt.control = TRB_TYPE(TRB_TYPE_NORMAL) | TRB_IO | TRB_ISP;
                {
                    int idx = ring_enqueue(tr, &dt);
                    if (idx >= 0 && trans)
                        trans->hc_trb_phys =
                            tr->phys + (uintptr_t)idx * sizeof(struct xhci_trb_t);
                }
                usb_printk("xhci: INT re-arm slot=%d ep=%d mps=%d\n",
                    slot_id, ep_val, ilen);
                mmio_write32(xhci->db_base + (slot_id * 4), ep_val);
                mmio_read32(xhci->db_base + (slot_id * 4));
            }
        }
        transfer->need_clear_halt = 0;
        return 0;
    }

    struct xhci_trb_t event;

    /* The event ring is shared by every transfer on this controller, while
     * usb_poll_interrupts() visits transfers one at a time. Do not consume a
     * Transfer Event until the owning transfer is being polled; otherwise an
     * earlier list entry can steal the mouse event from the EP3 transfer. */
    {
        struct xhci_trb_t *pending =
            &xhci->event_ring.ring[xhci->event_ring.dequeue];
        uint8_t cycle = pending->control & 0x1;

        if (cycle == xhci->event_ring.ccs) {
            uint8_t type =
                (pending->control >> TRB_TRB_TYPE_SHIFT) & 0x3F;

            if (type == TRB_TYPE_TRANSFER_EVENT) {
                uint8_t event_slot = (pending->control >> 24) & 0xFF;
                uint8_t event_ep = (pending->control >> 16) & 0x1F;
                uint8_t transfer_ep = 1;
                volatile struct usb_transaction_t *trans =
                    transfer->trans_head;
                static uint32_t runtime_event_lines;

                if (transfer->endpoint && transfer->endpoint->addr != 0) {
                    transfer_ep = transfer->endpoint->addr * 2;
                    if (transfer->endpoint->direction == USB_ENDPOINT_IN)
                        transfer_ep |= 1;
                }

                if (runtime_event_lines < 32) {
                    xhci_status_regs("xhci runtime event",
                                     "slot=%x ep=%x cc=%x",
                                     event_slot, event_ep,
                                     (pending->status >> 24) & 0xFF);
                    runtime_event_lines++;
                }

                if (event_slot != (uint8_t)transfer->dev->num ||
                    event_ep != transfer_ep ||
                    (transfer->type == USB_TRANSFER_CTRL &&
                     pending->param && trans && trans->hc_trb_phys &&
                     (pending->param & ~0xFULL) !=
                     ((uint64_t)trans->hc_trb_phys & ~0xFULL))) {
                    return 0;
                }
            }
        }
    }

    if (xhci_get_event(xhci, &event) > 0) {
        uint8_t trb_type = (event.control >> TRB_TRB_TYPE_SHIFT) & 0x3F;
        if (trb_type == TRB_TYPE_TRANSFER_EVENT) {
            uint8_t cc = (event.status >> 24) & 0xFF;
            uint8_t  ep  = (event.control >> 16) & 0x1F;
            {
                uint32_t my_ep = 1;
                if (transfer->endpoint && transfer->endpoint->addr != 0) {
                    my_ep = transfer->endpoint->addr * 2;
                    if (transfer->endpoint->direction == USB_ENDPOINT_IN)
                        my_ep |= 1;
                }
                if (ep != (uint8_t)my_ep) {
                    cc = 0xFF; /* sentinel: skip processing below */
                }
            }

            if (cc == CC_SUCCESS || cc == CC_SHORT_PACKET ||
                cc == CC_STALL_ERROR || cc == CC_ENDPOINT_NOT_ENABLED) {
                /* For interrupt transfers: copy DMA鈫抲ser buffer (on success),
                 * then re-submit the TRB for the next poll interval.
                 * CC_STALL_ERROR = device NAK'd or STALL'd �?re-submit anyway
                 * to keep the polling pipe alive. */
                if (transfer->type == USB_TRANSFER_INTERRUPT &&
                    transfer->trans_head && transfer->endpoint) {
                    volatile struct usb_transaction_t *trans = transfer->trans_head;
                    {
                        static uint32_t hid_event_log;
                        if (hid_event_log < 24) {
                            usb_printk("xhci: HID event slot=%d ep=%d cc=%d "
                                       "ptr=%x remain=%d\n",
                                       transfer->dev->num, ep, cc,
                                       (uint32_t)event.param,
                                       event.status & 0xFFFFFF);
                            hid_event_log++;
                        }
                    }

                    if (transfer->type == USB_TRANSFER_CTRL &&
                        event.param && trans->hc_trb_phys &&
                        (event.param & ~0xFULL) !=
                        ((uint64_t)trans->hc_trb_phys & ~0xFULL)) {
                        return 0;
                    }

                    uint32_t intr_len = transfer->endpoint->mps;
                    if (intr_len < 3) intr_len = 3;
                    int report_ready = 0;

                    if (cc == CC_SUCCESS || cc == CC_SHORT_PACKET) {
                        uint32_t rem  = event.status & 0xFFFFFF;
                        uint32_t rx   = (rem <= intr_len) ? (intr_len - rem) : 0;
                        transfer->actual_length = (uint16_t)rx;
                        if (trans->data && trans->data != (void *)0x1 &&
                            trans->buf && rx > 0) {
                            static uint32_t runtime_report_lines;
                            A_memcpy(trans->buf, (void *)trans->data, rx);
                            report_ready = 1;
                            {
                                static uint32_t runtime_report_log;
                                if (runtime_report_log < 16) {
                                    usb_printk("xhci: HID report slot=%d ep=%d "
                                               "bytes=%d\n",
                                               transfer->dev->num, ep, rx);
                                    runtime_report_log++;
                                }
                            }
                            if (runtime_report_lines < 32) {
                                xhci_status_regs("xhci mouse report",
                                                 "slot=%x ep=%x bytes=%x",
                                                 (uint32_t)transfer->dev->num,
                                                 (uint32_t)ep, rx);
                                runtime_report_lines++;
                            }
                        }
                        transfer->success = 1;
                    }

                    /* Re-submit: enqueue new Normal TRB + ring doorbell */
                    int slot_id = transfer->dev->num;
                    uint32_t ep_val = 1;
                    if (transfer->endpoint->addr != 0) {
                        ep_val = transfer->endpoint->addr * 2;
                        if (transfer->endpoint->direction == USB_ENDPOINT_IN)
                            ep_val |= 1;
                    }

                    /* If STALL: mark for deferred recovery.
                     *   need_clear_halt: 0=normal, 1=need CLEAR_FEATURE,
                     *   2=need HC re-arm (set by deferred handler).
                     * We CANNOT call CLEAR_FEATURE synchronously from
                     * here �?it would nest a full control transfer inside
                     * the event-poll context and overflow the 7 KB stack.
                     * Instead we set a flag and return; the actual
                     * CLEAR_FEATURE is done by usb_process_deferred_clears()
                     * at a shallower stack depth (from the scheduler idle
                     * loop in kernel.c). */
                    if (cc == CC_STALL_ERROR) {
                        transfer->actual_length = 0;
                        transfer->need_clear_halt = 1;
                        return 0;  /* no data; callback NOT called */
                    }

                    struct xhci_ring_t *tr =
                        &xhci->slots[slot_id].ep_rings[ep_val];
                    void *data_buf = (void *)trans->data;
                    uint32_t data_phys = data_buf && data_buf != (void *)0x1
                        ? (uint32_t)virt_to_phys(data_buf) : 0;

                    if (cc == CC_ENDPOINT_NOT_ENABLED) {
                        static uint32_t endpoint_disabled_lines;
                        uint8_t ep_addr = transfer->endpoint->addr;
                        if (transfer->endpoint->direction == USB_ENDPOINT_IN)
                            ep_addr |= 0x80;

                        if (endpoint_disabled_lines < 16) {
                            xhci_error_regs("xhci endpoint disabled",
                                            "slot=%x ep=%x remain=%x",
                                            (uint32_t)slot_id, ep_val,
                                            event.status & 0xFFFFFF);
                            endpoint_disabled_lines++;
                        }

                        /* A disabled endpoint cannot accept Reset Endpoint.
                         * Rebuild its transfer ring before Configure Endpoint,
                         * whose dequeue pointer starts at the ring base. */
                        A_memset(tr->ring, 0,
                                 tr->size * sizeof(struct xhci_trb_t));
                        tr->ring[tr->size - 1].param = tr->phys;
                        tr->ring[tr->size - 1].control =
                            TRB_TYPE(TRB_TYPE_LINK) | TRB_TC | TRB_CYCLE;
                        tr->enqueue = 0;
                        tr->dequeue = 0;
                        tr->ccs = 1;
                        xhci_mmio_barrier();

                        if (xhci_configure_endpoint(
                                xhci, slot_id, ep_addr,
                                transfer->endpoint->type,
                                transfer->endpoint->mps,
                                transfer->endpoint->interval) < 0) {
                            xhci_error_regs("xhci EP configure failed",
                                            "slot=%x ep=%x cc=%x",
                                            (uint32_t)slot_id, ep_val,
                                            (uint32_t)cc);
                            transfer->success = 0;
                            return 0;
                        }
                        xhci_status_regs("xhci EP reconfigured",
                                         "slot=%x ep=%x ring=%x",
                                         (uint32_t)slot_id, ep_val,
                                         (uint32_t)tr->phys);
                    }

                    if (data_phys && tr->ring) {
                        /* A short/NAK completion must not expose stale bytes
                         * as a relative mouse displacement. */
                        A_memset(data_buf, 0, intr_len);
                        struct xhci_trb_t data_trb;
                        A_memset(&data_trb, 0, sizeof(data_trb));
                        data_trb.param   = (uint64_t)data_phys;
                        data_trb.status  = TRB_XFER_LEN(intr_len) |
                                            TRB_TD_SIZE(0) |
                                            TRB_INTR_TARGET(0);
                        data_trb.control = TRB_TYPE(TRB_TYPE_NORMAL) |
                                            TRB_IO | TRB_ISP;
                        {
                            int idx = ring_enqueue(tr, &data_trb);
                            if (idx >= 0)
                                trans->hc_trb_phys =
                                    tr->phys + (uintptr_t)idx * sizeof(struct xhci_trb_t);
                        }

                        mmio_write32(xhci->db_base + (slot_id * 4), ep_val);
                        mmio_read32(xhci->db_base + (slot_id * 4));
                    } else {
                        xhci_error_regs("xhci INT rearm failed",
                                        "slot=%x ep=%x dma=%x",
                                        (uint32_t)slot_id, ep_val, data_phys);
                    }

                    return (report_ready &&
                            (cc == CC_SUCCESS || cc == CC_SHORT_PACKET)) ? 1 : 0;
                }

                /* Control and bulk transfers only succeed on a normal or
                 * short completion.  A STALL or disabled endpoint is a real
                 * failure here; treating it as success feeds zero-filled DMA
                 * buffers into the MSC state machine as valid SCSI data. */
                if (cc != CC_SUCCESS && cc != CC_SHORT_PACKET) {
                    char error[96];
                    transfer->actual_length = 0;
                    transfer->success = 0;
                    snprintf(error, sizeof(error),
                             "slot=%x ep=%x cc=%x remain=%x",
                             (uint32_t)transfer->dev->num,
                             (uint32_t)ep, (uint32_t)cc,
                             event.status & 0xFFFFFF);
                    kernel_usb_error_line("xhci transfer failed", error);
                    return 1;
                }

                {
                    volatile struct usb_transaction_t *trans =
                        transfer->trans_head;
                    uint32_t requested = trans ? trans->len : 0;
                    uint32_t remain = event.status & 0xFFFFFF;
                    uint32_t actual = remain <= requested ?
                        requested - remain : 0;
                    transfer->actual_length = (uint16_t)actual;
                }
                transfer->success = 1;
                return 1;
            }

        }
    }
    return 0;
}

static void xhci_delete_transfer(struct usb_transfer_t *transfer)
{
    /* For now: nothing to free. DMA pool is bump-allocated. */
    transfer->data = NULL;
}

static void xhci_free_transaction_data(volatile struct usb_transaction_t *transaction)
{
    /* Copy IN data from DMA buffer to user buffer ONLY on success.
     * On STALL / error the DMA buffer may contain stale data �?
     * copying it would pass garbage as valid payload. */
    struct usb_transfer_t *transfer = (struct usb_transfer_t *)transaction->transfer;
    if (transfer && transfer->success &&
        transaction->data && transaction->data != (void *)0x1) {
        if (transaction->buf && transaction->len > 0) {
            uint32_t copy_len = transaction->len;
            if (transfer->type == USB_TRANSFER_BULK &&
                transfer->actual_length < copy_len)
                copy_len = transfer->actual_length;
            A_memcpy(transaction->buf, (void *)transaction->data, copy_len);
        }
    }
    transaction->data = NULL;
}

/* ================================================================
 * Install and Poll
 * ================================================================ */

/* Non-blocking controller stop/reset front-end. One register operation is
 * performed per call; no tick_delay() or wait loop is used here. */
int xhci_async_reset_step(pci_compat_t *pci, struct pci_bar_t *bar)
{
    static uintptr_t mmio_base;
    static uintptr_t op_base;
    static int state;

    if (state < 0) return state;

    if (state == 0) {
        if (!pci || !bar || bar->iotype != PCI_IOTYPE_MMIO || !bar->base) {
            state = -EINVAL;
            return state;
        }
        uintptr_t base = bar->base & ~0xF;
        mmio_base = (uintptr_t)mmio_map(base, base + bar->iosize);
        if (!mmio_base) { state = -EIO; return state; }
        uint8_t caplen = mmio_read8(mmio_base + XHCI_CAP_CAPLENGTH);
        if (caplen < 0x20) { state = -EIO; return state; }
        op_base = mmio_base + caplen;
        pci_enable_busmastering(pci);
        pci_enable_memoryspace(pci);
        usb_printk("xhci-async: MMIO=%x caplen=%x warm start\n",
                   mmio_base, caplen);
        state = 1;
        return 0;
    }

    if (state == 1) {
        (void)op_base;
        state = 2;
        return 1;
    }
    return 1;
}

static void xhci_free_install_state(struct xhci_dev_t *xhci)
{
    int s;

    if (!xhci) return;
    if (xhci->slots) {
        for (s = 0; s <= XHCI_MAX_SLOTS; s++) {
            if (xhci->slots[s].ep_rings)
                kfree(xhci->slots[s].ep_rings);
        }
        kfree(xhci->slots);
    }
    kfree(xhci);
}

int xhci_install(pci_compat_t *pci, struct pci_bar_t *bar)
{
    struct xhci_dev_t *xhci;
    int res = 0;
    uintptr_t base;

    if (first_xhci &&
        first_xhci->pci.bus_id == pci->bus_id &&
        first_xhci->pci.dev_id == pci->dev_id &&
        first_xhci->pci.fn_id == pci->fn_id &&
        (first_xhci->flags & XHCI_FLAG_RUN)) {
        usb_printk("xhci: controller %02x:%02x.%x already installed, reusing\n",
                   pci->bus_id, pci->dev_id, pci->fn_id);
        return 0;
    }

    if (!pci || !bar || bar->iotype != PCI_IOTYPE_MMIO ||
        !bar->base || !bar->iosize || bar->base + bar->iosize <= bar->base) {
        usb_printk("xhci: invalid PCI or MMIO BAR\n");
        return -EINVAL;
    }

    /* Memory decoding must be enabled before the first MMIO access. */
    pci_enable_memoryspace(pci);
    pci_enable_busmastering(pci);

    xhci_status_regs("xhci pci", "ven=%x dev=%x bdf=%x",
                     pci->vendor_id, pci->device_id,
                     ((uint32_t)pci->bus_id << 16) |
                     ((uint32_t)pci->dev_id << 8) | pci->fn_id);
    xhci_status_regs("xhci pci", "bar=%x size=%x type=%x",
                     bar->base, bar->iosize, bar->iotype);
    xhci_status_regs("xhci pci", "cmd=%x sts=%x bar0=%x",
                     pci_config_read_word(pci, 0x04),
                     pci_config_read_word(pci, 0x06),
                     pci_read_config_dword(pci->bus_id, pci->dev_id,
                                           pci->fn_id, 0x10));
    {
        uint16_t pci_cmd = pci_config_read_word(pci, 0x04);
        xhci_status_regs("xhci pci", "mem=%x busmaster=%x reserved=%x",
                         !!(pci_cmd & 0x2), !!(pci_cmd & 0x4),
                         pci_cmd & ~0x7U);
    }
    xhci_status_regs("xhci pci", "bar1=%x base=%x size=%x",
                     pci_read_config_dword(pci->bus_id, pci->dev_id,
                                           pci->fn_id, 0x14),
                     bar->base, bar->iosize);

    if (!(xhci = kmalloc(sizeof(struct xhci_dev_t)))) {
        usb_printk("%s: insufficient memory\n", "xhci");
        return -ENOMEM;
    }
    A_memset(xhci, 0, sizeof(struct xhci_dev_t));

    /* Allocate per-slot data array (small top-level, ep_rings per slot) */
    {
        int slot_size = (XHCI_MAX_SLOTS + 1) * sizeof(xhci->slots[0]);
        xhci->slots = kmalloc(slot_size);
        if (!xhci->slots) { kfree(xhci); return -ENOMEM; }
        A_memset(xhci->slots, 0, slot_size);
        for (int s = 0; s <= XHCI_MAX_SLOTS; s++) {
            int ring_size = XHCI_MAX_ENDPOINTS * sizeof(struct xhci_ring_t);
            xhci->slots[s].ep_rings = kmalloc(ring_size);
            if (!xhci->slots[s].ep_rings) {
                xhci_free_install_state(xhci);
                return -ENOMEM;
            }
            A_memset(xhci->slots[s].ep_rings, 0, ring_size);
        }
    }

    /* Reserve slot 0 */
    xhci->addr_bitmap[0] |= 1;
    xhci->pci = *pci;

    /* Map MMIO */
    base = bar->base & ~0xF;
    xhci->mmio_phys_base = base;
    xhci->mmio_base = (uintptr_t)mmio_map(base, base + bar->iosize);
    if (!xhci->mmio_base) {
        usb_printk("xhci: MMIO mapping failed\n");
        xhci_free_install_state(xhci);
        return -EIO;
    }
    xhci->mmio = 1;

    usb_printk("%s: base 0x%x, mmio 0x%x, size 0x%x\n",
        "xhci", bar->base, xhci->mmio_base, bar->iosize);
    xhci_status_regs("xhci pci", "ven=%x dev=%x bar=%x",
                     pci->vendor_id, pci->device_id, bar->base);
    xhci_status_regs("xhci pci", "phys=%x virt=%x size=%x",
                     (uint32_t)xhci->mmio_phys_base,
                     (uint32_t)xhci->mmio_base, bar->iosize);
    kernel_usb_status_line("xhci", "installing controller");

    /* Initialize controller */
    if ((res = xhci_start(xhci)) < 0) {
        usb_printk("%s: start failed\n", "xhci");
        xhci_free_install_state(xhci);
        return res;
    }

    /* Publish only controllers that completed initialization. */
    if (!first_xhci) {
        first_xhci = xhci;
    } else {
        volatile struct xhci_dev_t *tmp = first_xhci;
        while (tmp->next) tmp = tmp->next;
        tmp->next = xhci;
    }

    /* Kernel boot uses a synchronous init path: enumerate the initial root
     * ports here before usb_init() returns.  Runtime hotplug is still handled
     * later from xhci_poll(). */
    xhci->init_port_cursor = 0;
    xhci_enable_ports(xhci);

    usb_printk("%s: init OK\n", "xhci");
    kernel_usb_status_line("xhci", "controller and root ports ready");
    return 0;
}

void xhci_poll(void)
{
    volatile struct xhci_dev_t *xhci = first_xhci;
    volatile uint32_t port;
    uint32_t ready;

    while (xhci) {
        if (!(xhci->flags & XHCI_FLAG_RUN)) {
            xhci = xhci->next;
            continue;
        }

        if (!(xhci->flags & XHCI_FLAG_PORTSENABLED)) {
            xhci_enable_ports(xhci);
        }

        ready = xhci_event_ready_count(xhci);
        if (ready >= (xhci->event_ring.size * 3) / 4) {
            uint32_t before;
            int budget = 32;
            usb_printk("xhci: event high water ready=%d size=%d dequeue=%d\n",
                       ready, xhci->event_ring.size,
                       xhci->event_ring.dequeue);
            xhci_status_regs("xhci event high water",
                             "ready=%x size=%x dequeue=%x",
                             ready, xhci->event_ring.size,
                             xhci->event_ring.dequeue);
            do {
                before = xhci->event_ring.dequeue;
                usb_poll_interrupts();
            } while (--budget > 0 &&
                     xhci->event_ring.dequeue != before);
        }

        /* Check ports for connect/disconnect events */
        for (port = 0; port < xhci->port_count; port++) {
            uint32_t reg = xhci->op_base + xhci_port_offset(xhci, port);
            uint32_t portsc = mmio_read32(reg);

            /* Connect Status Change */
            if (portsc & XHCI_PORT_CSC) {
                /* Clear CSC �?mask PED */
                mmio_write32(reg, (portsc & ~XHCI_PORT_PED) | XHCI_PORT_CSC);

                if (portsc & XHCI_PORT_CCS) {
                    usb_printk("xhci: device connected on port %d\n", port);
                    if (port < 64)
                        xhci->port_attempted &= ~(1ULL << port);
                    xhci_reset_port(xhci, port);
                } else {
                    usb_printk("xhci: device removed from port %d\n", port);
                    xhci_remove_port_device(xhci, port);
                    if (port < 64)
                        xhci->port_attempted &= ~(1ULL << port);
                }
            }
        }

        /* Drain Port Status Change events only (leave Transfer events
         * for xhci_poll_transfer / xhci_wait_transfer to consume). */
        {
            struct xhci_trb_t event;
            int peek = 0;
            while (peek < 16) {
                /* Non-destructive check: peek at next event without consuming */
                struct xhci_trb_t *trb = &xhci->event_ring.ring[xhci->event_ring.dequeue];
                uint8_t trb_cycle = trb->control & 0x1;
                if (trb_cycle != xhci->event_ring.ccs) break;
                uint8_t trb_type = (trb->control >> TRB_TRB_TYPE_SHIFT) & 0x3F;
                if (trb_type != TRB_TYPE_PORT_STATUS_EVENT) {
                    break;  /* Leave it for the proper handler */
                }
                /* Consume only port status events */
                xhci_get_event(xhci, &event);
                uint32_t port_id = (event.param >> 24) & 0xFF;
                usb_printk("xhci: port %d status change event\n", port_id);
                peek++;
            }
        }

        /* Poll interrupt transfers (replaces usb_task_func which
         * never runs because start_kernel_task is a stub). */
        usb_poll_interrupts();

        xhci = xhci->next;
    }
}

int xhci_all_ports_enabled(void)
{
    volatile struct xhci_dev_t *xhci = first_xhci;
    if (!xhci) return 0;
    while (xhci) {
        if (!(xhci->flags & XHCI_FLAG_PORTSENABLED)) return 0;
        xhci = xhci->next;
    }
    return 1;
}

/* ================================================================
 * Fire-and-forget CLEAR_FEATURE(ENDPOINT_HALT)
 *
 * Writes the Setup+Status TRBs directly onto EP0's transfer ring
 * and rings the doorbell WITHOUT waiting for completion.
 *
 * This avoids the full synchronous control-transfer call chain
 * (usb_ctrl_set �?�?�?xhci_wait_transfer), which overflows the
 * 7 KB kernel stack when called from the polling path.
 *
 * The Transfer Event for this control transfer will be consumed
 * naturally by the next xhci_poll() �?xhci_poll_transfer cycle.
 * ================================================================ */
void xhci_clear_feature_halt_async(struct usb_dev_t *dev,
                                   struct usb_endpoint_t *ep)
{
    volatile struct xhci_dev_t *xhci = (volatile struct xhci_dev_t *)dev->priv;
    int slot_id = dev->num;
    struct xhci_ring_t *ep0 = &xhci->slots[slot_id].ep_rings[1];

    if (!ep0->ring) {
        usb_printk("xhci: CLEAR_FEATURE async �?no EP0 ring!\n");
        return;
    }

    /* EP address with direction bit (IN �?bit 7 set) */
    uint16_t ep_addr = ep->addr;
    if (ep->direction == USB_ENDPOINT_IN)
        ep_addr |= 0x80;

    usb_printk("xhci: CLEAR_FEATURE async slot=%d ep=0x%02x\n",
               slot_id, ep_addr);

    /* Build Setup Stage TRB (IDT=1: embed 8-byte setup packet) */
    struct xhci_trb_t setup;
    A_memset(&setup, 0, sizeof(setup));
    {
        uint8_t *b = (uint8_t *)&setup.param;
        b[0] = 0x02;            /* bmRequestType: OUT, Standard, Endpoint */
        b[1] = 1;               /* bRequest: CLEAR_FEATURE */
        b[2] = 0;               /* wValue: ENDPOINT_HALT = 0 */
        b[3] = 0;
        b[4] = ep_addr & 0xFF;  /* wIndex: endpoint address */
        b[5] = (ep_addr >> 8);
        b[6] = 0;               /* wLength = 0 (no data stage) */
        b[7] = 0;
    }
    setup.status  = 8;
    setup.control = TRB_TYPE(TRB_TYPE_SETUP_STAGE) | TRB_IDT;

    /* Build Status Stage TRB (IN, zero-length, IOC) */
    struct xhci_trb_t status;
    A_memset(&status, 0, sizeof(status));
    status.param   = 0;
    status.status  = 0;
    status.control = TRB_TYPE(TRB_TYPE_STATUS_STAGE) | TRB_DIR_IN | TRB_IO;

    /* Enqueue both TRBs */
    ring_enqueue(ep0, &setup);
    ring_enqueue(ep0, &status);

    /* Ring EP0 doorbell �?HC will process immediately */
    mmio_write32(xhci->db_base + (slot_id * 4), 1);
    mmio_read32(xhci->db_base + (slot_id * 4));
}

/* ================================================================
 * usb_ops_t vtable
 * ================================================================ */
static struct usb_ops_t xhci_ops = {
    .setup_transfer        = xhci_setup_transfer,
    .schedule_transfer     = xhci_schedule_transfer,
    .wait_transfer         = xhci_wait_transfer,
    .poll_transfer         = xhci_poll_transfer,
    .delete_transfer       = xhci_delete_transfer,
    .setup_transaction     = xhci_setup_transaction,
    .in_transaction        = xhci_in_transaction,
    .out_transaction       = xhci_out_transaction,
    .free_transaction_data = xhci_free_transaction_data,
    .get_next_addr         = xhci_get_next_addr,
    .free_addr             = xhci_free_addr,
};
