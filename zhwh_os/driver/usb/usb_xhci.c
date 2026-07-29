/**
 * @file usb_xhci.c
 * @brief XHCI (USB 3.0+) Host Controller Driver
 *
 * Implements the usb_ops_t vtable for the XHCI controller.
 * Based on eXtensible Host Controller Interface spec rev 1.2.
 * 32-bit only — upper 32 bits of 64-bit registers are always 0.
 */
#include <kernel/errno.h>
#include <kernel/pciio.h>
#include <kernel/asm.h>
#include <kernel/usb.h>
#include <kernel/usb_xhci.h>
#include <kernel/hillsonos.h>
#include <mm/kheap.h>
#include <string.h>

/* ================================================================
 * 64-bit MMIO helpers (32-bit system: high dword always 0)
 * ================================================================ */
STATIC_INLINE uint64_t xhci_read64(volatile struct xhci_dev_t *xhci, uint32_t reg)
{
    uint32_t lo = mmio_read32(xhci->mmio_base + reg);
    uint32_t hi = mmio_read32(xhci->mmio_base + reg + 4);
    return ((uint64_t)hi << 32) | lo;
}

STATIC_INLINE void xhci_write64(volatile struct xhci_dev_t *xhci, uint32_t reg, uint64_t val)
{
    mmio_write32(xhci->mmio_base + reg, (uint32_t)(val & 0xFFFFFFFF));
    mmio_write32(xhci->mmio_base + reg + 4, (uint32_t)(val >> 32));
}

/* ================================================================
 * Forward declarations
 * ================================================================ */
static int xhci_enable_slot(volatile struct xhci_dev_t *xhci);
static int xhci_address_device(volatile struct xhci_dev_t *xhci, int slot_id, uint8_t port, uint8_t speed);
static int xhci_alloc_transfer_ring(volatile struct xhci_dev_t *xhci, int slot_id, int ep_id);
static void *xhci_alloc_dev_ctx(volatile struct xhci_dev_t *xhci, uint64_t *phys_out);

/* ================================================================
 * Global linked list
 * ================================================================ */
static volatile struct xhci_dev_t *first_xhci = NULL;
static struct usb_ops_t xhci_ops;

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
    if (slot_id < 1 || slot_id > (unsigned int)xhci->max_slots) return;
    xhci->addr_bitmap[slot_id / 32] &= ~(1 << (slot_id % 32));
    /* Clear slot state to prevent stale context on reuse */
    A_memset(&xhci->slots[slot_id], 0, sizeof(xhci->slots[0]));
    /* Re-allocate ep_rings (freed by memset above) */
    xhci->slots[slot_id].ep_rings = kmalloc(XHCI_MAX_ENDPOINTS * sizeof(struct xhci_ring_t));
    if (xhci->slots[slot_id].ep_rings)
        A_memset(xhci->slots[slot_id].ep_rings, 0, XHCI_MAX_ENDPOINTS * sizeof(struct xhci_ring_t));
}

struct usb_dev_t *xhci_get_dev_struct(pci_compat_t *bus, uint8_t num)
{
    volatile struct xhci_dev_t *xhci = first_xhci;
    if (!bus || num < 1) return NULL;
    while (xhci) {
        if (xhci->pci == bus) {
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
     * when following the link. Without it, HC cycle ≠ our ccs → HC skips
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
 * The last slot (ring[size-1]) is a permanent Link TRB — never overwrite it. */
static int ring_enqueue(struct xhci_ring_t *ring, struct xhci_trb_t *trb)
{
    /* If we're at the Link TRB slot, wrap to beginning and toggle cycle */
    if (ring->enqueue == ring->size - 1) {
        ring->enqueue = 0;
        ring->ccs ^= 1;
    }

    /* Full check: next slot must not be dequeue */
    uint32_t next = ring->enqueue + 1;
    if (next == ring->dequeue)
        return -1;

    int idx = ring->enqueue;
    A_memcpy(&ring->ring[idx], trb, sizeof(struct xhci_trb_t));
    ring->ring[idx].control |= (ring->ccs & 0x1);  /* Set Cycle Bit */
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

    printf("EVENT dequeue=%d ccs=%d trb=%p\n",
       xhci->event_ring.dequeue,
       xhci->event_ring.ccs,
       trb);

    /* Check if HC has written a valid TRB (cycle bit matches CCS) */
    uint8_t trb_cycle = trb->control & 0x1;
    if (trb_cycle != xhci->event_ring.ccs) return 0;

    A_memcpy(event, (void *)trb, sizeof(struct xhci_trb_t));

    {
        uint32_t *dw = (uint32_t *)event;

        printf("EVENT TRB\n");
        printf(" DW0=%08x\n", dw[0]);
        printf(" DW1=%08x\n", dw[1]);
        printf(" DW2=%08x\n", dw[2]);
        printf(" DW3=%08x\n", dw[3]);

        printf(" type=%d\n",
            (dw[3] >> 10) & 0x3f);

        printf(" cc=%d\n",
            (dw[2] >> 24) & 0xff);
    }

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

/* Wait for command completion, return completion code */
static int xhci_wait_cmd(volatile struct xhci_dev_t *xhci, int timeout_ms)
{
    volatile int timeout = timeout_ms * 1000;  /* Convert to tick_delay units */
    struct xhci_trb_t event;

    while (timeout--) {
        if (xhci_get_event(xhci, &event) > 0) {
            uint8_t trb_type = (event.control >> TRB_TRB_TYPE_SHIFT) & 0x3F;
            if (trb_type == TRB_TYPE_CMD_COMP_EVENT) {
                uint8_t cc = (event.status >> 24) & 0xFF;
                return cc;
            }
            /* Other events (transfer, port status) — ignore for now */
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
    ring_enqueue(&xhci->cmd_ring, cmd);

    /* Ring Command Doorbell (DB offset 0, target 0) */
    mmio_write32(xhci->db_base, 0);
    mmio_read32(xhci->db_base);  /* flush */

    return 0;
}

/* ================================================================
 * Controller Initialization
 * ================================================================ */

static int xhci_reset(volatile struct xhci_dev_t *xhci)
{
    volatile uint32_t dword;
    volatile int timeout;

    /* Stop the HC */
    dword = mmio_read32(xhci->op_base + XHCI_OP_USBCMD);
    dword &= ~USBCMD_RS;
    mmio_write32(xhci->op_base + XHCI_OP_USBCMD, dword);

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
    dword = mmio_read32(xhci->op_base + XHCI_OP_USBCMD);
    dword |= USBCMD_HCRST;
    mmio_write32(xhci->op_base + XHCI_OP_USBCMD, dword);

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
    usb_printk("%s: reset done\n", "xhci");

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
    xhci->db_base     = xhci->mmio_base + dboff;
    xhci->max_slots   = HCSPARAMS1_MAXSLOTS(hcsparams1);
    xhci->port_count  = HCSPARAMS1_MAXPORTS(hcsparams1);
    /* USB2/USB3 port split is in xECP, not HCSPARAMS1.
     * HCSPARAMS1[7:0] is MaxSlots, not USB2 ports!
     * For now, treat all ports as capable — use PORTSC speed to determine type. */
    xhci->usb2_ports  = xhci->port_count;  /* conservative: all could be USB2 */
    xhci->usb3_ports  = 0;                 /* will be detected per-port */
    xhci->page_size   = mmio_read32(xhci->op_base + XHCI_OP_PAGESIZE);

    usb_printk("%s: version 0x%x, slots=%u, ports=%u (USB2=%u, USB3=%u)\n",
        "xhci", hciver, xhci->max_slots, xhci->port_count,
        xhci->usb2_ports, xhci->usb3_ports);
    usb_printk("%s: caplen=0x%x, dboff=0x%x, rtsoff=0x%x, 64bit=%d, CSZ=%d\n",
        "xhci", caplen, dboff, rtsoff,
        HCCPARAMS1_AC64(hccparams1), HCCPARAMS1_CSZ(hccparams1));

    /* Warn if 64-bit addressing — we only support 32-bit */
    if (HCCPARAMS1_AC64(hccparams1))
        usb_printk("%s: HC supports 64-bit, using 32-bit mode\n", "xhci");

    /* Cap max slots to our array size */
    if (xhci->max_slots > XHCI_MAX_SLOTS) {
        usb_printk("%s: capping slots %u -> %u\n", "xhci",
            xhci->max_slots, XHCI_MAX_SLOTS);
        xhci->max_slots = XHCI_MAX_SLOTS;
    }

    /* Reset HC */
    if ((res = xhci_reset(xhci)) < 0) return res;

    /* Program Max Slots Enabled */
    dword = mmio_read32(xhci->op_base + XHCI_OP_CONFIG);
    dword &= ~0xFF;
    dword |= xhci->max_slots;
    mmio_write32(xhci->op_base + XHCI_OP_CONFIG, dword);

    /* Allocate DCBAA (max_slots + 1 entries of 8 bytes, 64-byte aligned) */
    {
        uint32_t dcbaa_size = (xhci->max_slots + 1) * sizeof(uint64_t);
        uint32_t dcbaa_phys;
        void *dcbaa_virt = dma_alloc_coherent(dcbaa_size, &dcbaa_phys);
        if (!dcbaa_virt) { res = -ENOMEM; goto err; }
        A_memset(dcbaa_virt, 0, dcbaa_size);
        xhci->dcbaa = (uint64_t *)dcbaa_virt;
        xhci->dcbaa_phys = (uint64_t)dcbaa_phys;

        /* Pre-allocate Output Device Context for every slot now.
         * The HC writes to DCBAA[slot_id] during Enable Slot —
         * if any entry is NULL, that slot goes bad (cc=11). */
        for (i = 1; i <= (int)xhci->max_slots; i++) {
            xhci->slots[i].dev_ctx = xhci_alloc_dev_ctx(xhci,
                &xhci->slots[i].dev_ctx_phys);
            if (!xhci->slots[i].dev_ctx) {
                usb_printk("%s: failed to alloc dev_ctx for slot %d\n",
                    "xhci", i);
                res = -ENOMEM; goto err;
            }
            xhci->dcbaa[i] = xhci->slots[i].dev_ctx_phys;
        }
    }

    /* Allocate Scratchpad Buffers */
    {
        uint32_t sp_count = HCSPARAMS2_MAXSCRPBUF(hcsparams2);
        xhci->scratchpad_count = sp_count;
        if (sp_count > 0) {
            /* Allocate array of phys addresses (sp_count * 8 bytes) */
            uint32_t sp_arr_size = sp_count * sizeof(uint64_t);
            uint32_t sp_arr_phys;
            void *sp_arr_virt = dma_alloc_coherent(sp_arr_size, &sp_arr_phys);
            if (!sp_arr_virt) { res = -ENOMEM; goto err; }
            A_memset(sp_arr_virt, 0, sp_arr_size);
            xhci->sp_array = (uint64_t *)sp_arr_virt;
            xhci->sp_array_phys = (uint64_t)sp_arr_phys;

            /* Allocate each scratchpad page */
            for (i = 0; i < (int)sp_count; i++) {
                uint32_t sp_phys;
                void *sp_virt = dma_alloc_coherent(xhci->page_size, &sp_phys);
                if (!sp_virt) { res = -ENOMEM; goto err; }
                xhci->sp_array[i] = (uint64_t)sp_phys;
            }
            xhci->dcbaa[0] = xhci->sp_array_phys;
        }
    }

    /* Program DCBAAP */
    xhci_write64(xhci, xhci->caplen + XHCI_OP_DCBAAP_LO, xhci->dcbaa_phys);

    /* Allocate Command Ring */
    if ((res = xhci_ring_init(&xhci->cmd_ring, XHCI_CMD_RING_SIZE)) < 0) goto err;
    usb_printk("%s: cmd ring virt=%p phys=%x\n", "xhci",
        xhci->cmd_ring.ring, xhci->cmd_ring.phys);

    /* Program CRCR (Command Ring Control Register) */
    {
        uint64_t crcr = xhci->cmd_ring.phys;
        /* CRCR: bits [5:0] = RCS (Ring Cycle State = 1), CA (Command Abort) */
        crcr |= 0x1;  /* RCS = 1 */
        xhci_write64(xhci, xhci->caplen + XHCI_OP_CRCR_LO, crcr);
    }

    /* Allocate Event Ring Segment Table (1 segment) */
    {
        uint32_t erst_phys;
        void *erst_virt = dma_alloc_coherent(sizeof(struct xhci_erst_entry_t), &erst_phys);
        if (!erst_virt) { res = -ENOMEM; goto err; }
        A_memset(erst_virt, 0, sizeof(struct xhci_erst_entry_t));
        xhci->erst = (struct xhci_erst_entry_t *)erst_virt;
        xhci->erst_phys = (uint64_t)erst_phys;

        /* Allocate Event Ring */
        if ((res = xhci_ring_init(&xhci->event_ring, XHCI_EVENT_RING_SIZE)) < 0) goto err;
        usb_printk("%s: event ring virt=%p phys=%x\n", "xhci",
            xhci->event_ring.ring, xhci->event_ring.phys);

        /* Fill ERST entry */
        xhci->erst->addr_lo = (uint32_t)(xhci->event_ring.phys & 0xFFFFFFFF);
        xhci->erst->addr_hi = (uint32_t)(xhci->event_ring.phys >> 32);
        xhci->erst->size    = XHCI_EVENT_RING_SIZE;
        xhci->erst->rsvd    = 0;
    }

    /* Initialize Interrupter 0 */
    {
        uint32_t rt_off = xhci->rt_base - xhci->mmio_base;

        /* ERSTSZ = 1 (one segment) */
        mmio_write32(xhci->mmio_base + XHCI_RT_ERSTSZ(rt_off), 1);

        /* ERSTBA = physical address of ERST */
        xhci_write64(xhci, XHCI_RT_ERSTBA_LO(rt_off), xhci->erst_phys);

        /* ERDP = start of event ring */
        xhci_write64(xhci, XHCI_RT_ERDP_LO(rt_off), xhci->event_ring.phys);

        /* IMAN: clear pending, disable interrupt (we poll) */
        uint32_t iman = mmio_read32(xhci->mmio_base + XHCI_RT_IMAN(rt_off));
        iman |= IMAN_IP;   /* Clear pending by writing 1 */
        iman &= ~IMAN_IE;  /* Disable interrupt */
        mmio_write32(xhci->mmio_base + XHCI_RT_IMAN(rt_off), iman);

        /* IMOD: set to 0 (no interrupt moderation — we poll anyway) */
        mmio_write32(xhci->mmio_base + XHCI_RT_IMOD(rt_off), 0);
    }

    /* Start the Host Controller */
    usb_printk("%s: starting HC...\n", "xhci");
    dword = mmio_read32(xhci->op_base + XHCI_OP_USBCMD);
    dword |= USBCMD_RS;
    mmio_write32(xhci->op_base + XHCI_OP_USBCMD, dword);

    /* Wait for HCHalted to clear */
    {
        volatile int timeout = 200;
        while (timeout--) {
            dword = mmio_read32(xhci->op_base + XHCI_OP_USBSTS);
            if (!(dword & USBSTS_HCH)) break;
            tick_delay(100);
        }
        if (timeout <= 0) {
            usb_printk("%s: start timeout (HCH still set)\n", "xhci");
            res = -ETIMEDOUT; goto err;
        }
    }

    usb_printk("%s: HC started, status=0x%x\n", "xhci",
        mmio_read32(xhci->op_base + XHCI_OP_USBSTS));

    /* Issue No-Op command to verify Command Ring works */
    {
        struct xhci_trb_t cmd;
        A_memset(&cmd, 0, sizeof(cmd));
        cmd.control = TRB_TYPE(TRB_TYPE_NOOP_CMD) ;
        xhci_post_cmd(xhci, &cmd);
        int cc = xhci_wait_cmd(xhci, 500);
        if (cc != CC_SUCCESS) {
            usb_printk("%s: No-Op command returned cc=%d\n", "xhci", cc);
            /* Non-fatal on some QEMU versions */
        } else {
            usb_printk("%s: No-Op command succeeded\n", "xhci");
        }
    }

    xhci->flags |= XHCI_FLAG_RUN;
    return 0;

err:
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

/* Determine port speed from PORTSC [13:10] */
static uint8_t xhci_port_speed(uint32_t portsc, int is_usb3)
{
    /* XHCI speed IDs: 1=Full, 2=Low, 3=High, 4=SuperSpeed.
     * QEMU NEC XHCI may report speed=4 on USB3-capable ports even
     * with USB2 devices. If no USB3 ports, cap speed=4 to High. */
    uint32_t speed_id = (portsc >> 10) & 0xF;
    if (!is_usb3 && speed_id >= 4)
        speed_id = 3;
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
    uint32_t reg = xhci->op_base + xhci_port_offset(xhci, port);
    uint32_t portsc = mmio_read32(reg);
    int is_usb3 = (port >= xhci->usb2_ports);
    volatile int timeout;

    usb_printk("xhci: port %d reset: PORTSC=0x%x (USB%s)\n",
        port, portsc, is_usb3 ? "3" : "2");

    /* Power on port if needed.
     * CRITICAL: always mask out PED in writes.  PED is write-1-to-disable
     * in xHCI — writing 1 disables the port instead of enabling it. */
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
        if (portsc & XHCI_PORT_PED) {
            uint8_t speed = xhci_port_speed(portsc, is_usb3);
            usb_printk("xhci: port %d enabled, PORTSC=0x%x speed=%d\n",
                port, portsc, speed);
            /* Clear CSC — mask PED to avoid disabling the port */
            mmio_write32(reg, (portsc & ~XHCI_PORT_PED) | XHCI_PORT_CSC);

            /* Setup device using XHCI flow:
             * 1. Enable slot → get slot_id
             * 2. Set dev->num = slot_id (usb_setup_device preserves for XHCI)
             * 3. Address device → assigns USB address
             * 4. usb_setup_device → enumerates with control transfers */
            struct usb_dev_t *usb;
            int slot_id;

            usb = usb_create_dev(xhci->pci->unit, port, speed);
            if (!usb) {
                usb_printk("%s: failed to create USB device\n", "xhci");
                return;
            }
            usb->type = USB_TYPE_XHCI;
            usb->priv = (void *)xhci;
            usb->ops  = &xhci_ops;

            /* DMA coherency test: write magic pattern to ALL output contexts.
             * After Enable Slot, HC should overwrite the assigned slot's context.
             * If magic persists → HC didn't write. If zeros → cache coherency issue. */
            for (int s = 1; s <= (int)xhci->max_slots; s++) {
                uint32_t *out = (uint32_t *)xhci->slots[s].dev_ctx;
                out[0] = 0xAAAAAAAA;  /* magic */
            }

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
            {
                uint32_t *out = (uint32_t *)xhci->slots[slot_id].dev_ctx;
                A_memset(out, 0, XHCI_CTX_SIZE);  /* 32 bytes Slot Context */
                out[3] = (1 << 27);  /* Slot State = Enabled, bits[31:27]=1 */
            }

            /* Mark slot in bitmap */
            xhci->addr_bitmap[slot_id / 32] |= (1 << (slot_id % 32));
            printf("A: dcbaa[%d]=%x (ptr=%p)\n", slot_id,
                (uint32_t)xhci->dcbaa[slot_id], &xhci->dcbaa[slot_id]);

            /* Allocate transfer ring for EP0 */
            xhci_alloc_transfer_ring(xhci, slot_id, 1);
            printf("B: dcbaa[%d]=%x\n", slot_id,
                (uint32_t)xhci->dcbaa[slot_id]);

            /* Set slot_id as the USB address (preserved by usb_setup_device for XHCI) */
            usb->num = slot_id;
            printf("C: dcbaa[%d]=%x\n", slot_id,
                (uint32_t)xhci->dcbaa[slot_id]);

            /* Step 1: Address Device — validate context, enable slot/EP0.
             * BSR=0: full address assignment (BSR=1 is context-only test). */
            if (xhci_address_device(xhci, slot_id, port, speed) < 0) {
                usb_printk("%s: Address Device failed for slot %d\n", "xhci", slot_id);
                xhci_free_addr((void *)xhci, slot_id);
                usb_destroy_dev(usb);
                return;
            }

            /* Step 2: Enumerate (GetDescriptor at address 0 through TRB ring).
             * usb_setup_device will:
             *   a) Read 8-byte device descriptor at addr 0 → learn real MPS
             *   b) Call usb_set_device_addr (SET_ADDRESS) — for XHCI this
             *      is intercepted: we do Address Device BSR=0 instead.
             *   c) Read full descriptor + config at new address */
            if (usb_setup_device(usb, slot_id) < 0) {
                usb_printk("%s: failed to set up device on port %d\n",
                    "xhci", port);
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
            return;
        }
        tick_delay(500);
    }
    usb_printk("%s: port %d enable timeout\n", "xhci", port);
}

static void xhci_enable_ports(volatile struct xhci_dev_t *xhci)
{
    volatile uint32_t i;

    usb_printk("%s: enabling %u ports...\n", "xhci", xhci->port_count);

    for (i = 0; i < xhci->port_count; i++) {
        uint32_t reg = xhci->op_base + xhci_port_offset(xhci, i);
        uint32_t portsc = mmio_read32(reg);

        usb_printk("xhci: port %d PORTSC=0x%x\n", i, portsc);

        /* Power on port — mask PED to avoid disabling */
        if (!(portsc & XHCI_PORT_PP)) {
            mmio_write32(reg, (portsc & ~XHCI_PORT_PED) | XHCI_PORT_PP);
            tick_delay(10000);
        }

        /* Clear CSC (write-1-to-clear) — mask PED */
        if (portsc & XHCI_PORT_CSC) {
            mmio_write32(reg, (portsc & ~XHCI_PORT_PED) | XHCI_PORT_CSC);
        }

        portsc = mmio_read32(reg);
        usb_printk("xhci: port %d after power PORTSC=0x%x\n", i, portsc);

        /* Check if device connected */
        if (portsc & XHCI_PORT_CCS) {
            usb_printk("xhci: device on port %d, resetting...\n", i);
            xhci_reset_port(xhci, i);
        }
    }

    xhci->flags |= XHCI_FLAG_PORTSENABLED;
}

/* ================================================================
 * Device Management Commands
 * ================================================================ */

/* Returns a device context pointer.
 * xHCI spec: each context is 32 bytes, packed contiguously.
 * Input Context: InputCtrl (32B) + Slot (32B) + EP0..EP31 (32 × 32B)
 * = 34 × 32 = 1088 bytes. Allocate with 64-byte alignment for base. */
static void *xhci_alloc_dev_ctx(volatile struct xhci_dev_t *xhci, uint64_t *phys_out)
{
    uint32_t phys;
    int size = 34 * XHCI_CTX_SIZE;   /* 34 contexts × 32 bytes each */
    void *virt = dma_alloc_coherent(size, &phys);
    if (!virt) return NULL;
    A_memset(virt, 0, size);
    *phys_out = (uint64_t)phys;
    return virt;
}

/* Enable Slot — returns slot_id (positive) or negative error */
static int xhci_enable_slot(volatile struct xhci_dev_t *xhci)
{
    struct xhci_trb_t cmd;
    struct xhci_trb_t event;
    volatile int timeout = 100000;

    A_memset(&cmd, 0, sizeof(cmd));
    cmd.control = TRB_TYPE(TRB_TYPE_ENABLE_SLOT) ;
    xhci_post_cmd(xhci, &cmd);

    while (timeout--) {
        if (xhci_get_event(xhci, &event) > 0) {
            uint8_t trb_type = (event.control >> TRB_TRB_TYPE_SHIFT) & 0x3F;
            if (trb_type == TRB_TYPE_CMD_COMP_EVENT) {
                uint8_t cc = (event.status >> 24) & 0xFF;
                uint8_t sid = (event.control >> 24) & 0xFF;
                if (cc == CC_SUCCESS && sid > 0)
                    return (int)sid;
                usb_printk("%s: Enable Slot failed, cc=%d\n", "xhci", cc);
                return -EIO;
            }
        }
        tick_delay(1);
    }
    usb_printk("%s: Enable Slot timeout\n", "xhci");
    return -ETIMEDOUT;
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

/* Address Device — assign USB address, set slot context + EP0 context
 * dev_ctx + DCBAA[slot_id] are pre-allocated in xhci_start. */
static int xhci_address_device(volatile struct xhci_dev_t *xhci,
                               int slot_id, uint8_t port, uint8_t speed)
{
    struct xhci_trb_t cmd;
    uint32_t *ctx;

    printf("ENTER addr_dev: dcbaa[%d]=%x\n", slot_id,
        (uint32_t)xhci->dcbaa[slot_id]);

    /* Allocate Input Context on first use */
    if (!xhci->slots[slot_id].input_ctx) {
        xhci->slots[slot_id].input_ctx = xhci_alloc_dev_ctx(xhci,
            &xhci->slots[slot_id].input_ctx_phys);
        if (!xhci->slots[slot_id].input_ctx) return -ENOMEM;
    }

    /* Flat dword array: each context is 32 bytes = 8 dwords.
     * Input Context layout (xHCI spec rev 1.2 §6.2.5.1):
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
    A_memset(ctx, 0, 64 * sizeof(uint32_t));

    uint16_t mps = (speed == USB_SPEED_SUPER) ? 512 :
                    (speed == USB_SPEED_HIGH)  ? 64  : 8;

             
    uint32_t speed_code = (speed == USB_SPEED_SUPER) ? 4 :
                          (speed == USB_SPEED_LOW)   ? 2 :
                          (speed == USB_SPEED_FULL)  ? 1 : 3;


    //mps=8;speed_code=1;
    /* ---- Input Control ---- */
    ctx[0] = 0;   /* drop_flags */
    ctx[1] = 3;   /* add_flags: Slot (bit 0) + EP0 (bit 1) */

    /* ---- Slot Context (ctx[8..15]) ----
     * dw0: Route String[19:0]=0 | Speed[23:20] | Context Entries[31:27]
     * dw1: Root Hub Port Number[15:8]
     * dw2: TT/Hub — 0 for root-hub devices
     * dw3: Interrupter Target[21:10]=0
     */
    ctx[8]  =(1 << 27) | (speed_code << 20);  /* Context Entries=1 → Slot+EP0 */

    printf("Slot DW0=%08x\n",ctx[8]);
    printf("CE=%d speed=%d\n",
       ctx[8]>>27,
       (ctx[8]>>20)&0xf);

    ctx[9]  = ((port + 1) << 16);                      /* Root Hub Port Number (1-indexed) */
    ctx[10] = 0;                                 /* Not a hub, no TT */
    ctx[11] = 0;                                 /* Interrupter 0 */

    /* ---- EP0 Context (ctx[16..23]) ----
     * dw0: EP State=0
     * dw1: MPS[31:16] | CErr[7:4]=3 | EP Type[2:0]=4 (Control)
     * dw2: TR Dequeue Pointer Lo | DCS (bit 0)=1
     * dw3: TR Dequeue Pointer Hi
     * dw4: Average TRB Length = 8
     */
    ctx[16] = 0;   /* State=0 (disabled — HC sets RUNNING after Address Device) */
    ctx[17] = (mps << 16) | (3 << 3) | 4;       /* MPS | CErr=3 | EPType=Control */
    
    printf("EP0 DW1=%08x\n",ctx[17]);
    printf("EP type=%d\n",
       ctx[17] & 7);

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
        tr &= ~0xF;   // 强制 16 字节对齐
        tr |= 1;      // DCS = 1

        ctx[18] = (uint32_t)(tr & 0xFFFFFFFF);
        ctx[19] = (uint32_t)(tr >> 32);

    }

    ctx[20] = 8;   /* Average TRB Length (minimum for control EP) */

    /* Dump all 24 dwords for debug */
    for (int i = 0; i < 24; i++)
        printf("ctx[%02d] = %08x\n", i, ctx[i]);

    /* Prepare Address Device command (BSR=0 — full address assignment) */
    A_memset(&cmd, 0, sizeof(cmd));
    cmd.param  = xhci->slots[slot_id].input_ctx_phys;
    cmd.control = TRB_TYPE(TRB_TYPE_ADDRESS_DEVICE) | ((slot_id & 0xFF) << 24)| TRB_CYCLE;

    //cmd.control = (1 << 9);      // Address Device

    dump_trb_raw(&cmd);

    usb_printk("%s: Address Device: slot=%d ctx=0x%x speed=%d port=%d mps=%d\n",
        "xhci", slot_id, (uint32_t)xhci->slots[slot_id].input_ctx_phys,
        (int)speed, (int)port, (int)mps);
    usb_printk("%s:   InputCtrl: drop=%x add=%x\n", "xhci", ctx[0], ctx[1]);
    usb_printk("%s:   SlotCtx:   dw0=%x dw1=%x dw2=%x dw3=%x\n",
        "xhci", ctx[8], ctx[9], ctx[10], ctx[11]);
    usb_printk("%s:   EP0Ctx:    dw0=%x dw1=%x dw2=%x dw3=%x dw4=%x\n",
        "xhci", ctx[16], ctx[17], ctx[18], ctx[19], ctx[20]);

    xhci_post_cmd(xhci, &cmd);

    int cc = xhci_wait_cmd(xhci, 500);
    if (cc != CC_SUCCESS) {
        usb_printk("%s: Address Device  returned cc=%d\n", "xhci", cc);
        return -EIO;
    }
    usb_printk("%s:  Address Device succeeded\n", "xhci");
    xhci->slots[slot_id].addressed = 1;
    usb_printk("%s: Address Device slot %d OK\n", "xhci", slot_id);
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
     * SET_ADDRESS is a no-op here — just return success. */
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

    /* Build Setup Stage TRB */
    struct xhci_trb_t setup_trb;
    struct usb_request_t req;
    struct xhci_ring_t *tr = &xhci->slots[slot_id].ep_rings[ep_id];

    req.type  = transaction->type;
    req.req   = transaction->req;
    req.hival = transaction->hival;
    req.loval = transaction->loval;
    req.index = transaction->index;
    req.len   = transaction->len;

    /* Copy setup packet into DMA buffer (pointer-based, NOT IDT).
     * IDT+CHAIN has compatibility issues with some XHCI controllers. */
    uint32_t setup_phys;
    void *setup_buf = dma_alloc_coherent(8, &setup_phys);
    if (!setup_buf) return -ENOMEM;
    A_memcpy(setup_buf, &req, 8);

    A_memset(&setup_trb, 0, sizeof(setup_trb));
    setup_trb.param   = (uint64_t)setup_phys;       /* pointer to setup packet */
    setup_trb.status  = 8;                           /* transfer length = 8 */
    {
        /* TRT[17:16]: 0=NoData, 2=OUT, 3=IN */
        uint32_t trt = 0;
        if (transaction->len > 0) {
            trt = (transaction->type & 0x80) ? (3 << 16) : (2 << 16);
        }
        setup_trb.control = TRB_TYPE(TRB_TYPE_SETUP_STAGE) | TRB_CHAIN | trt;
    }

    /* Store the transfer context pointer for later stages */
    /* (We store the TRB index for completion tracking) */
    transaction->data = (void *)(uintptr_t)tr->enqueue;

    /* Enqueue */
    if (ring_enqueue(tr, &setup_trb) < 0) {
        usb_printk("%s: setup ring full\n", "xhci");
        return -ENOMEM;
    }

    return 0;
}

static void xhci_in_transaction(struct usb_transaction_t *transaction)
{
    struct usb_transfer_t *transfer = (struct usb_transfer_t *)transaction->transfer;
    /* Skip if SET_ADDRESS already handled via Address Device command */
    if (transfer->trans_head && transfer->trans_head->data == (void *)0x1) return;
    struct usb_dev_t      *usb      = transaction->dev;
    struct xhci_dev_t     *xhci     = (struct xhci_dev_t *)usb->priv;
    int slot_id = usb->num;
    int ep_id   = 1;
    struct xhci_ring_t *tr = &xhci->slots[slot_id].ep_rings[ep_id];

    if (!tr->ring) {
        usb_printk("%s: no transfer ring for IN\n", "xhci");
        return;
    }

    /* Build Data Stage TRB (IN direction) */
    struct xhci_trb_t data_trb;
    A_memset(&data_trb, 0, sizeof(data_trb));

    if (transaction->len > 0 && transaction->buf) {
        /* Allocate DMA buffer for the data */
        uint32_t data_phys;
        void *data_buf = dma_alloc_coherent(transaction->len, &data_phys);
        if (!data_buf) {
            usb_printk("%s: failed to alloc IN buffer\n", "xhci");
            return;
        }
        data_trb.param   = (uint64_t)data_phys;
        data_trb.status  = transaction->len;
        /* Save buffer pointer for copying result later */
        transaction->data = data_buf;  /* Stores both the buffer and hint */
    } else {
        data_trb.param   = 0;
        data_trb.status  = 0;
    }

    data_trb.control = TRB_TYPE(TRB_TYPE_DATA_STAGE) | TRB_DIR_IN | TRB_CHAIN;

    ring_enqueue(tr, &data_trb);
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
    struct xhci_ring_t *tr = &xhci->slots[slot_id].ep_rings[ep_id];

    if (!tr->ring) {
        usb_printk("%s: no transfer ring for OUT\n", "xhci");
        return;
    }

    /* Build Data Stage TRB (OUT direction) or Status Stage */
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
        out_trb.status  = transaction->len;
        out_trb.control = TRB_TYPE(TRB_TYPE_DATA_STAGE) | TRB_CHAIN;
    } else {
        /* Status Stage (zero-length) */
        out_trb.param   = 0;
        out_trb.status  = 0;
        out_trb.control = TRB_TYPE(TRB_TYPE_STATUS_STAGE);  /* No CHAIN — last */
    }

    ring_enqueue(tr, &out_trb);
}

static void xhci_setup_transfer(struct usb_transfer_t *transfer)
{
    struct xhci_dev_t *xhci = (struct xhci_dev_t *)transfer->dev->priv;
    int slot_id = transfer->dev->num;
    int ep_id   = 1;  /* Default: EP0 */

    if (transfer->type == USB_TRANSFER_ISOCHRONOUS) {
        usb_printk("%s: isochronous not implemented\n", "xhci");
        return;
    }

    /* For non-control endpoints, use the endpoint address */
    if (transfer->endpoint && transfer->endpoint->addr != 0) {
        ep_id = transfer->endpoint->addr * 2;
        if (transfer->endpoint->direction == USB_ENDPOINT_IN)
            ep_id |= 1;
    }

    /* Allocate transfer ring if needed */
    if (!xhci->slots[slot_id].ep_rings[ep_id].ring) {
        xhci_ring_init(&xhci->slots[slot_id].ep_rings[ep_id],
                       XHCI_TRANSFER_RING_SIZE);
    }

    transfer->data = (void *)&xhci->slots[slot_id].ep_rings[ep_id];
}

static void xhci_schedule_transfer(struct usb_transfer_t *transfer)
{
    struct xhci_dev_t *xhci = (struct xhci_dev_t *)transfer->dev->priv;
    int slot_id = transfer->dev->num;

    /* Check if SET_ADDRESS was already handled via Address Device command */
    if (transfer->trans_head && transfer->trans_head->data == (void *)0x1) {
        return;  /* Already done — no TRBs to queue */
    }

    /* Ring doorbell for the slot's EP0 (endpoint 1)
     * Doorbell target = slot_id, stream = 0
     * DB register value = slot_id | (endpoint_id << 0)?
     * Actually: DB target = endpoint_id (1 for EP0), DB stream = 0
     *
     * Wait — the doorbell format is:
     *   DB[slot_id] = endpoint_id | (stream_id << 16)
     * For EP0, endpoint_id = 1, stream_id = 0
     */
    uint32_t db_val = 1;  /* EP0 = endpoint 1 */
    mmio_write32(xhci->db_base + (slot_id * 4), db_val);
    mmio_read32(xhci->db_base);  /* Flush */
}

static void xhci_wait_transfer(struct usb_transfer_t *transfer)
{
    struct xhci_dev_t *xhci = (struct xhci_dev_t *)transfer->dev->priv;

    /* SET_ADDRESS already handled via Address Device command — instant success */
    if (transfer->trans_head && transfer->trans_head->data == (void *)0x1) {
        transfer->success = 1;
        usb_printk("xhci: SET_ADDRESS already done via AddrDevice, success\n");
        return;
    }

    struct xhci_trb_t event;
    volatile int timeout = 5000000;  /* 5 seconds */

    usb_printk("xhci: waiting transfer slot=%d...\n", transfer->dev->num);

    while (timeout--) {
        if (xhci_get_event(xhci, &event) > 0) {
            uint8_t trb_type = (event.control >> TRB_TRB_TYPE_SHIFT) & 0x3F;
            if (trb_type == TRB_TYPE_TRANSFER_EVENT) {
                uint8_t cc = (event.status >> 24) & 0xFF;
                uint32_t len = (event.status & 0xFFFFFF);
                uint8_t ep_id = (event.control >> 16) & 0x1F;
                usb_printk("xhci: Transfer Event ep=%d cc=%d len=%d\n",
                    ep_id, cc, len);

                if (cc == CC_SUCCESS || cc == CC_SHORT_PACKET) {
                    transfer->success = 1;
                    return;
                }
                if (cc != CC_SUCCESS) {
                    usb_printk("xhci: transfer error cc=%d\n", cc);
                    transfer->success = 0;
                    return;
                }
            } else if (trb_type == TRB_TYPE_CMD_COMP_EVENT) {
                /* Command completions during transfer — ignore */
            } else if (trb_type == TRB_TYPE_PORT_STATUS_EVENT) {
                usb_printk("xhci: port status event during transfer\n");
            }
        }
        tick_delay(1);
    }

    usb_printk("%s: transfer timeout slot=%d\n", "xhci", transfer->dev->num);
    transfer->success = 0;
}

static int xhci_poll_transfer(struct usb_transfer_t *transfer)
{
    /* Non-blocking poll — check Event Ring for transfer events */
    struct xhci_dev_t *xhci = (struct xhci_dev_t *)transfer->dev->priv;
    struct xhci_trb_t event;

    if (xhci_get_event(xhci, &event) > 0) {
        uint8_t trb_type = (event.control >> TRB_TRB_TYPE_SHIFT) & 0x3F;
        if (trb_type == TRB_TYPE_TRANSFER_EVENT) {
            uint8_t cc = (event.status >> 24) & 0xFF;
            if (cc == CC_SUCCESS || cc == CC_SHORT_PACKET) {
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
    /* DMA pool is bump-allocated — data not freed */
    transaction->data = NULL;
}

/* ================================================================
 * Install and Poll
 * ================================================================ */

int xhci_install(pci_compat_t *pci, struct pci_bar_t *bar)
{
    struct xhci_dev_t *xhci;
    int res = 0;

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
            if (!xhci->slots[s].ep_rings) { kfree(xhci->slots); kfree(xhci); return -ENOMEM; }
            A_memset(xhci->slots[s].ep_rings, 0, ring_size);
        }
    }

    /* Reserve slot 0 */
    xhci->addr_bitmap[0] |= 1;
    xhci->pci = pci;

    /* Map MMIO */
    if (bar->iotype == PCI_IOTYPE_MMIO) {
        uintptr_t base = bar->base & ~0xF;
        xhci->mmio_base = (uintptr_t)mmio_map(base, base + bar->iosize);
        xhci->mmio = 1;
    } else {
        usb_printk("%s: XHCI requires MMIO, got I/O BAR\n", "xhci");
        kfree(xhci->slots); kfree(xhci);
        return -EINVAL;
    }

    usb_printk("%s: base 0x%x, mmio 0x%x, size 0x%x\n",
        "xhci", bar->base, xhci->mmio_base, bar->iosize);

    /* Link into global list */
    if (!first_xhci) {
        first_xhci = xhci;
    } else {
        volatile struct xhci_dev_t *tmp = first_xhci;
        while (tmp->next) tmp = tmp->next;
        tmp->next = xhci;
    }

    pci_enable_busmastering(pci);
    pci_enable_memoryspace(pci);

    /* Initialize controller */
    if ((res = xhci_start(xhci)) < 0) {
        usb_printk("%s: start failed\n", "xhci");
        return res;
    }

    /* Enable ports */
    xhci_enable_ports(xhci);

    usb_printk("%s: init OK\n", "xhci");
    return 0;
}

void xhci_poll(void)
{
    volatile struct xhci_dev_t *xhci = first_xhci;
    volatile uint32_t port;

    while (xhci) {
        if (!(xhci->flags & XHCI_FLAG_RUN)) {
            xhci = xhci->next;
            continue;
        }

        /* Check ports for connect/disconnect events */
        for (port = 0; port < xhci->port_count; port++) {
            uint32_t reg = xhci->op_base + xhci_port_offset(xhci, port);
            uint32_t portsc = mmio_read32(reg);

            /* Connect Status Change */
            if (portsc & XHCI_PORT_CSC) {
                /* Clear CSC — mask PED */
                mmio_write32(reg, (portsc & ~XHCI_PORT_PED) | XHCI_PORT_CSC);

                if (portsc & XHCI_PORT_CCS) {
                    /* Device connected */
                    usb_printk("xhci: device connected on port %d\n", port);
                    xhci_reset_port(xhci, port);
                } else {
                    /* Device disconnected */
                    usb_printk("xhci: device removed from port %d\n", port);
                    /* TODO: cleanup slot */
                }
            }
        }

        /* Drain any pending events */
        {
            struct xhci_trb_t event;
            while (xhci_get_event(xhci, &event) > 0) {
                uint8_t trb_type = (event.control >> TRB_TRB_TYPE_SHIFT) & 0x3F;
                if (trb_type == TRB_TYPE_PORT_STATUS_EVENT) {
                    uint32_t port_id = (event.param >> 24) & 0xFF;
                    usb_printk("xhci: port %d status change event\n", port_id);
                }
            }
        }

        xhci = xhci->next;
    }
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
