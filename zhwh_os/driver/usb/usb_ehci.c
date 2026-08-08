/**
 * @file usb_ehci.c
 * @brief EHCI USB Host Controller driver
 */

//#define __DEBUG

#include <kernel/errno.h>
#include <kernel/pciio.h>
#include <kernel/asm.h>
#include <kernel/usb.h>
#include <kernel/usb_ehci.h>
#include <kernel/hillsonos.h>
#include <mm/kheap.h>


static volatile struct ehci_dev_t *first_ehci = NULL;
static struct usb_ops_t ehci_ops;

static int ehci_setup_device(volatile struct ehci_dev_t *ehci, unsigned int port, uint8_t speed);


struct usb_dev_t *ehci_get_dev_struct(pci_compat_t *bus, uint8_t num)
{
    volatile struct ehci_dev_t *ehci = first_ehci;

    if(!bus || num < 1) return NULL;

    while(ehci) {
        if(ehci->pci == bus) {
            if(num > ehci->port_count) return NULL;
            return (struct usb_dev_t *)(ehci->ports[num - 1].usb);
        }
        ehci = ehci->next;
    }
    return NULL;
}


static unsigned int ehci_get_next_addr(void *__ehci)
{
    struct ehci_dev_t *ehci = __ehci;
    volatile unsigned int i, j;

    for(i = 0; i < (MAX_DEV_PER_HC / sizeof(uint32_t)); i++) {
        for(j = 0; j < 32; j++) {
            if(!(ehci->addr_bitmap[i] & (1 << j))) {
                ehci->addr_bitmap[i] |= (1 << j);
                return (i * 32) + j;
            }
        }
    }
    return 0;
}


static void ehci_free_addr(void *__ehci, unsigned int i)
{
    struct ehci_dev_t *ehci = __ehci;
    if(i < 1 || i >= MAX_DEV_PER_HC) return;
    ehci->addr_bitmap[i / 32] &= ~(1 << (i % 32));
}


static void init_qhpool(struct ehci_dev_t *ehci)
{
    volatile size_t i;
    volatile struct ehci_qh_t *qh = (struct ehci_qh_t *)ehci->qhpool;
    uintptr_t qhphys = (uintptr_t)ehci->qhpool_phys;

    A_memset((void *)ehci->qhpool, 0, PAGE_SIZE);
    A_memset(ehci->qh_used, 0, EHCI_MAX_QH);

    for(i = 0; i < EHCI_MAX_QH; i++) {
        qh[i].self_phys = qhphys;
        qhphys += sizeof(struct ehci_qh_t);
    }
}


static void init_tdpool(struct ehci_dev_t *ehci)
{
    A_memset(ehci->td_used, 0, EHCI_MAX_TD);
    A_memset((void *)ehci->tdpool, 0, PAGE_SIZE);
}


static void free_qh(struct ehci_dev_t *ehci, uintptr_t qh)
{
    volatile int i = (qh - ehci->qhpool) / sizeof(struct ehci_qh_t);
    __sync_bool_compare_and_swap(&ehci->qh_used[i], 1, 0);
}


static struct ehci_qh_t *alloc_qh(volatile struct ehci_dev_t *ehci)
{
    volatile size_t i;
    for(i = 0; i < EHCI_MAX_QH; i++) {
        if(__sync_bool_compare_and_swap(&ehci->qh_used[i], 0, 1))
            return (struct ehci_qh_t *)(ehci->qhpool + (i * sizeof(struct ehci_qh_t)));
    }
    usb_printk("%s: failed to alloc QH\n", "ehci");
    return 0;
}


static void free_td(struct ehci_dev_t *ehci, uintptr_t td)
{
    volatile int i = (td - ehci->tdpool) / sizeof(struct ehci_td_t);
    __sync_bool_compare_and_swap(&ehci->td_used[i], 1, 0);
}


static struct ehci_td_t *alloc_td(struct ehci_dev_t *ehci, uintptr_t *tdphys)
{
    volatile size_t i;
    *tdphys = 0;
    for(i = 0; i < EHCI_MAX_TD; i++) {
        if(__sync_bool_compare_and_swap(&ehci->td_used[i], 0, 1)) {
            *tdphys = ehci->tdpool_phys + (i * sizeof(struct ehci_td_t));
            return (struct ehci_td_t *)(ehci->tdpool + (i * sizeof(struct ehci_td_t)));
        }
    }
    usb_printk("%s: failed to alloc TD\n", "ehci");
    return NULL;
}


static void free_tdbuf(struct ehci_dev_t *ehci, uintptr_t tdbuf)
{
    volatile int i = (tdbuf - ehci->tdbufpool) / EHCI_TDBUF_SIZE;
    __sync_bool_compare_and_swap(&ehci->tdbuf_used[i], 1, 0);
}


static uintptr_t alloc_tdbuf(struct ehci_dev_t *ehci, uintptr_t *tdbufphys)
{
    volatile int i;
    *tdbufphys = 0;
    for(i = 0; i < EHCI_MAX_TDBUF; i++) {
        if(__sync_bool_compare_and_swap(&ehci->tdbuf_used[i], 0, 1)) {
            uintptr_t v = ehci->tdbufpool + (i * EHCI_TDBUF_SIZE);
            *tdbufphys = ehci->tdbufpool_phys + (i * EHCI_TDBUF_SIZE);
            A_memset((void *)v, 0, EHCI_TDBUF_SIZE);
            return v;
        }
    }
    usb_printk("%s: failed to alloc TD buffer\n", "ehci");
    return 0;
}


/* QEMU EHCI note: ASYNCSTS and PERIODSTS status bits are not always
 * set by QEMU's emulated EHCI controller.  We still set the enable bits
 * and give the hardware time to react, but we never treat a timeout as
 * fatal — the schedule usually runs fine anyway. */

STATIC_INLINE void enable_async_sched(volatile struct ehci_dev_t *ehci)
{
    volatile uint32_t dword;
    volatile int timeout;

    pcidev_outl(ehci, EHCI_REG_HCOP + HCOP_ASYNCLISTADDR, ehci->async_qh->self_phys);

    dword = pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBCMD);
    dword |= ECHI_USBCMD_ASYNCEN;
    pcidev_outl(ehci, EHCI_REG_HCOP + HCOP_USBCMD, dword);

    timeout = 20;
    while(!(pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBSTS) & ECHI_USBSTS_ASYNCSTS) && timeout--)
        tick_delay(2);

    if(timeout <= 0)
        tick_delay(50);  /* QEMU: status bit may never set */
}


STATIC_INLINE void disable_async_sched(volatile struct ehci_dev_t *ehci)
{
    volatile uint32_t dword;
    volatile int timeout;

    dword = pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBCMD);
    dword &= ~ECHI_USBCMD_ASYNCEN;
    pcidev_outl(ehci, EHCI_REG_HCOP + HCOP_USBCMD, dword);

    timeout = 20;
    while((pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBSTS) & ECHI_USBSTS_ASYNCSTS) && timeout--)
        tick_delay(2);
    /* QEMU: status may never clear — ignore timeout */
}


STATIC_INLINE void enable_periodic_sched(volatile struct ehci_dev_t *ehci)
{
    volatile uint32_t dword;
    volatile int timeout;

    usb_printk("%s: enabling periodic scheduling\n", "ehci");

    dword = pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBCMD);
    dword |= ECHI_USBCMD_PERIODICEN;
    pcidev_outl(ehci, EHCI_REG_HCOP + HCOP_USBCMD, dword);

    timeout = 100;
    while(!(pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBSTS) & ECHI_USBSTS_PERIODSTS) && timeout--)
        tick_delay(5);

    if(timeout <= 0)
        usb_printk("ehci: periodic STS not set (QEMU ok), continuing\n");
    /* QEMU: schedule usually runs even without the status bit */
}


static void print_port_status(uint32_t status)
{
    int first = 1;
    usb_printk("%s: status 0x%x (", "ehci", status);
    if(status & (1 << 0)) { usb_printk(" connected"); first = 0; }
    if(status & (1 << 1)) { usb_printk("%sconn change", first ? " " : ", "); first = 0; }
    if(status & (1 << 2)) { usb_printk("%senabled", first ? " " : ", "); first = 0; }
    if(status & (1 << 3)) { usb_printk("%senabled change", first ? " " : ", "); first = 0; }
    if(status & (1 << 4)) { usb_printk("%sovercur", first ? " " : ", "); first = 0; }
    if(status & (1 << 5)) { usb_printk("%sovercur change", first ? " " : ", "); first = 0; }
    if(status & (1 << 6)) { usb_printk("%sresumed", first ? " " : ", "); first = 0; }
    if(status & (1 << 7)) { usb_printk("%ssuspended", first ? " " : ", "); first = 0; }
    if(status & (1 << 8)) { usb_printk("%sreset", first ? " " : ", "); first = 0; }
    usb_printk(" )\n");
}


static char *states[] = { "SE0", "K-state", "J-state", "Undefined" };


void ehci_reset_port(volatile struct ehci_dev_t *ehci, unsigned int port)
{
    uint32_t dword, state;
    volatile int timeout;
    int reg = EHCI_REG_HCOP + HCOP_PORTSC + (4 * port);

    dword = pcidev_inl(ehci, reg);
    dword |= EHCI_PORTSC_POWER;
    pcidev_outl(ehci, reg, dword);

    dword = pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBSTS);
    if(dword & ECHI_USBSTS_HCHALTED)
        usb_printk("ehci: port is halted during reset\n");

    // start reset sequence
    dword = pcidev_inl(ehci, reg);
    dword &= ~EHCI_PORTSC_EN;
    dword |= EHCI_PORTSC_RESET;
    pcidev_outl(ehci, reg, dword);
    tick_delay(50);

    // stop reset sequence
    dword = pcidev_inl(ehci, reg);
    dword &= ~EHCI_PORTSC_RESET;
    pcidev_outl(ehci, reg, dword);

    timeout = 50;
    while((pcidev_inl(ehci, reg) & EHCI_PORTSC_RESET) && timeout--)
        tick_delay(5);

    if(timeout <= 0)
        usb_printk("%s: reset timeout for port %u\n", "ehci", port);

    tick_delay(10);
    dword = pcidev_inl(ehci, reg);
    state = (dword >> 10) & 0x03;
    usb_printk("ehci: port %d after reset: status %x - state %d (%s)\n", port, dword, state, states[state]);

    if((dword & EHCI_PORTSC_POWER)) {
        if(dword & EHCI_PORTSC_EN) {
            /* Determine actual device speed from line state */
            uint8_t devspeed;
            if (state == 1)      devspeed = USB_SPEED_LOW;   /* K-state */
            else if (state == 2) devspeed = USB_SPEED_FULL;  /* J-state */
            else                 devspeed = USB_SPEED_HIGH;  /* default/SE0 */
            usb_printk("ehci: port %d device speed=%d (state=%d)\n", port, devspeed, state);
            ehci_setup_device(ehci, port, devspeed);
        } else {
            dword = mmio_read32(ehci->capbase + 0x04); /* HCCAP_HCSPARAMS */
            if(dword & 0xF000) {
                dword = pcidev_inl(ehci, reg);
                dword |= EHCI_PORTSC_OWNER;
                pcidev_outl(ehci, reg, dword);
            }
        }
    }
    usb_printk("%s: port %u reset complete\n", "ehci", port);
    print_port_status(pcidev_inl(ehci, reg));
}


static void ehci_check_port_state(volatile struct ehci_dev_t *ehci, unsigned int port)
{
    uint32_t dword, state;
    int reg = EHCI_REG_HCOP + HCOP_PORTSC + (4 * port);

    dword = pcidev_inl(ehci, reg);
    usb_printk("ehci: port %d PORTSC=0x%x\n", port, dword);
    if(!(dword & EHCI_PORTSC_CONN)) {
        usb_printk("ehci: port %d: no device connected\n", port);
        return;
    }

    tick_delay(20);
    dword = pcidev_inl(ehci, reg);
    state = (dword >> 10) & 0x03;
    usb_printk("ehci: port %d: status %x - state %d (%s)\n", port, dword, state, states[state]);

    if(state == 0 || state == 2 || state == 3)
        ehci_reset_port(ehci, port);
    else if(state == 1) {
        dword = mmio_read32(ehci->capbase + 0x04); /* HCCAP_HCSPARAMS */
        if(dword & 0xF000) {
            dword = pcidev_inl(ehci, reg);
            dword |= EHCI_PORTSC_OWNER;
            pcidev_outl(ehci, reg, dword);
        }
    }
}


static int ehci_enable_ports(volatile struct ehci_dev_t *ehci)
{
    volatile unsigned int i;

    if(!(ehci->ports = kmalloc(sizeof(struct ehci_port_t) * ehci->port_count))) {
        usb_printk("%s: insufficient memory to enable ports\n", "ehci");
        return -ENOMEM;
    }

    A_memset((void *)ehci->ports, 0, sizeof(struct ehci_port_t) * ehci->port_count);

    for(i = 0; i < ehci->port_count; i++) {
        ehci_check_port_state(ehci, i);
        ehci->ports[i].port = i;
        ehci->ports[i].ehci = ehci;
    }

    ehci->flags |= EHCI_FLAG_PORTENABLED;
    return 0;
}


static void ehci_setup_transfer(struct usb_transfer_t *transfer)
{
    struct ehci_dev_t *ehci = (struct ehci_dev_t *)transfer->dev->priv;

    if(transfer->type == USB_TRANSFER_ISOCHRONOUS)
        kpanic("ehci: isochronous transfers not implemented yet!\n\n");
    else
        transfer->data = alloc_qh(ehci);
}


static void link_async_qh(struct usb_transfer_t *transfer)
{
    struct ehci_dev_t *ehci = (struct ehci_dev_t *)transfer->dev->priv;
    volatile struct ehci_qh_t *prev, *qh = transfer->data;
    uint32_t dword;

    kernel_mutex_lock_infinite_wait(&ehci->qh_lock);

    prev = ehci->async_qh;
    usb_printk("ehci: link_async prev=0x%x prev->next=0x%x qh=0x%x\n",
        prev->self_phys, prev->next, qh->self_phys);
    __atomic_store_n(&qh->next, prev->next, __ATOMIC_SEQ_CST);
    __atomic_store_n(&qh->nextvirt, prev->nextvirt, __ATOMIC_SEQ_CST);
    __atomic_store_n(&prev->next, qh->self_phys | (1 << 1), __ATOMIC_SEQ_CST);
    __atomic_store_n(&prev->nextvirt, qh, __ATOMIC_SEQ_CST);
    usb_printk("ehci: link_async DONE prev->next=0x%x qh->next=0x%x\n",
        prev->next, qh->next);

    /* Ring async schedule doorbell so HC re-fetches from ASYNCLISTADDR */
    dword = pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBCMD);
    dword |= ECHI_USBCMD_ASYNCDOORBELL;  /* bit 6 */
    pcidev_outl(ehci, EHCI_REG_HCOP + HCOP_USBCMD, dword);

    kernel_mutex_unlock(&ehci->qh_lock);
}


static void unlink_async_qh(struct usb_transfer_t *transfer)
{
    struct ehci_dev_t *ehci = (struct ehci_dev_t *)transfer->dev->priv;
    volatile struct ehci_qh_t *prev, *qh = transfer->data;
    volatile uint32_t dword;
    volatile int timeout;

    if(!qh) return;

    kernel_mutex_lock_infinite_wait(&ehci->qh_lock);

    qh->next_qtd = 1;
    qh->next_qtd_alt = 1;
    prev = ehci->async_qh;

    while(prev) {
        if(prev->nextvirt == qh) {
            __atomic_store_n(&prev->next, qh->next, __ATOMIC_SEQ_CST);
            __atomic_store_n(&prev->nextvirt, qh->nextvirt, __ATOMIC_SEQ_CST);
            break;
        }
        prev = prev->nextvirt;
    }

    dword = pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBCMD);
    dword |= ECHI_USBCMD_ASYNCDOORBELL;
    pcidev_outl(ehci, EHCI_REG_HCOP + HCOP_USBCMD, dword);

    timeout = 50;
    dword = pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBSTS);
    while(!(dword & ECHI_USBSTS_ASYNCINT) && timeout--) {
        tick_delay(1);
        dword = pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBSTS);
    }
    pcidev_outl(ehci, EHCI_REG_HCOP + HCOP_USBSTS, dword | ECHI_USBSTS_ASYNCINT);

    kernel_mutex_unlock(&ehci->qh_lock);

    if(timeout <= 0)
        usb_printk("%s: timeout waiting for async advance doorbell\n", "ehci");
}


static void ehci_delete_transfer(struct usb_transfer_t *transfer)
{
    struct ehci_dev_t *ehci = (struct ehci_dev_t *)transfer->dev->priv;
    struct ehci_qh_t *qh = transfer->data;
    volatile int i;
    uint32_t dword;

    if(!qh) return;

    if(transfer->type == USB_TRANSFER_ISOCHRONOUS)
        kpanic("ehci: isochronous transfers not implemented yet!\n\n");
    else if(transfer->type == USB_TRANSFER_CTRL || transfer->type == USB_TRANSFER_BULK)
        unlink_async_qh(transfer);
    else if(transfer->type == USB_TRANSFER_INTERRUPT) {
        kernel_mutex_lock_infinite_wait(&ehci->qh_lock);
        dword = (uint32_t)qh->self_phys | (1 << 1);
        for(i = 0; i < 1024; i++) {
            if(ehci->framelist[i] == dword) {
                ehci->framelist[i] = 0x01;
                break;
            }
        }
        kernel_mutex_unlock(&ehci->qh_lock);
        if(i == 1024)
            usb_printk("ehci: transfer not found in framelist\n");
    }

    free_qh(ehci, (uintptr_t)transfer->data);
    transfer->data = NULL;
}


static void __wait_transfer(struct usb_transfer_t *transfer, volatile int timeout)
{
    volatile struct usb_transaction_t *usbtrans = transfer->trans_head;
    struct ehci_dev_t *ehci = (struct ehci_dev_t *)transfer->dev->priv;

    while(timeout--) {
        struct ehci_transaction_t *ehcitrans = usbtrans->data;
        struct ehci_td_t *td = ehcitrans->tdvirt;

        while(!((td->token.raw & 0x80))) {
            if((usbtrans = usbtrans->next) == NULL) break;
            ehcitrans = usbtrans->data;
            td = ehcitrans->tdvirt;
        }
        if(usbtrans == NULL) break;
        /* Read USBSTS to kick QEMU's EHCI device model into processing */
        pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBSTS);
        tick_delay(1);
    }

    if(timeout <= 0) {
        volatile struct usb_transaction_t *dbgtrans;
        uint32_t sts = pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBSTS);
        uint32_t cmd = pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBCMD);
        void *async_head = (void *)(uintptr_t)pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_ASYNCLISTADDR);

        switch_tty(1);
        usb_printk("%s: async transfer timed out\n", "ehci");
        usb_printk("ehci: USBSTS=%x USB_CMD=%x head=%p\n", sts, cmd, async_head);
        /* Dump all TD tokens for debugging */
        for(dbgtrans = transfer->trans_head; dbgtrans; dbgtrans = dbgtrans->next) {
            struct ehci_transaction_t *et = dbgtrans->data;
            if(et && et->tdvirt) {
                uint32_t raw = et->tdvirt->token.raw;
                usb_printk("ehci: TD phys=%x token=%x (status=%x pid=%d cerr=%d bytes=%d toggle=%d active=%d)\n",
                    (unsigned int)(uintptr_t)et->tdphys, raw,
                    (raw & 0xFF), ((raw >> 8) & 3), ((raw >> 10) & 3),
                    ((raw >> 16) & 0x7FFF), (raw >> 31),
                    ((raw & 0x80) ? 1 : 0));
            }
        }
        if(sts & ECHI_USBSTS_HCHALTED) {
            usb_printk("ehci: HC is halted, attempting restart...\n");
            cmd &= ~(1 << 0);       /* clear run/stop to be safe */
            pcidev_outl(ehci, EHCI_REG_HCOP + HCOP_USBCMD, cmd);
            tick_delay(10);
            cmd |= (1 << 0);         /* set run/stop */
            cmd |= ECHI_USBCMD_ASYNCEN;  /* ensure async is enabled */
            pcidev_outl(ehci, EHCI_REG_HCOP + HCOP_USBCMD, cmd);
            pcidev_outl(ehci, EHCI_REG_HCOP + HCOP_USBSTS, 0x3F);
        }
        kpanic("ehci: async transfer timed out\n");
    }
}


static void ehci_add_async(struct usb_transfer_t *transfer)
{
    struct ehci_dev_t *ehci = (struct ehci_dev_t *)transfer->dev->priv;
    volatile uint32_t dword;

    /* Check both status bit (STS.15) and command enable (CMD.4).
     * QEMU's EHCI model does not reliably set ASYNCSTS, so also check
     * if ASYNCEN is set in USBCMD. */
    dword = pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBSTS);
    if(!(dword & ECHI_USBSTS_ASYNCSTS)) {
        dword = pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBCMD);
        if(!(dword & ECHI_USBCMD_ASYNCEN)) {
            enable_async_sched(ehci);
        } else {
            /* ASYNCEN is set but ASYNCSTS is not — QEMU quirk.
             * Write the async list address again as a safety measure. */
            pcidev_outl(ehci, EHCI_REG_HCOP + HCOP_ASYNCLISTADDR, ehci->async_qh->self_phys);
        }
    }

    link_async_qh(transfer);
}


static void ehci_add_periodic(struct usb_transfer_t *transfer)
{
    struct ehci_dev_t *ehci = (struct ehci_dev_t *)transfer->dev->priv;
    struct ehci_qh_t *qh = transfer->data;
    volatile uint32_t dword;
    volatile int i;

    dword = pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBSTS);
    if(!(dword & ECHI_USBSTS_PERIODSTS))
        enable_periodic_sched(ehci);

    kernel_mutex_lock_infinite_wait(&ehci->qh_lock);

    for(i = 0; i < 1024; i++) {
        if(ehci->framelist[i] == 0x01) break;
    }

    if(i == 1024) {
        kernel_mutex_unlock(&ehci->qh_lock);
        usb_printk("ehci: periodic table is full\n");
        return;
    }

    if(transfer->type == USB_TRANSFER_INTERRUPT)
        ehci->framelist[i] = qh->self_phys | (1 << 1);
    else if(transfer->type == USB_TRANSFER_ISOCHRONOUS)
        kpanic("ehci: isochronous transfers not implemented yet!\n\n");

    kernel_mutex_unlock(&ehci->qh_lock);
}


static void ehci_schedule_transfer(struct usb_transfer_t *transfer)
{
    struct usb_dev_t *usb = transfer->dev;
    struct ehci_transaction_t *first = transfer->trans_head->data;
    struct ehci_qh_t *qh = transfer->data;
    uint8_t smask = 0;

    if(transfer->type == USB_TRANSFER_INTERRUPT)
        smask = (1 << 7);

    if(transfer->type != USB_TRANSFER_ISOCHRONOUS) {
        uintptr_t qhphys = qh->self_phys;

        A_memset(qh, 0, sizeof(struct ehci_qh_t));
        qh->self_phys = qhphys;

        qh->next = 1 | 2;
        qh->devaddr = usb->num;
        qh->endpoint = transfer->endpoint->addr;
        qh->endpoint_speed = (usb->speed == USB_SPEED_HIGH) ? 2 :
                             (usb->speed == USB_SPEED_LOW)  ? 1 : 0;
        qh->toggle = 1;
        qh->mps = transfer->pktsz;
        qh->int_sched_mask = smask;
        qh->mult = 1;
        qh->next_qtd_alt = 1;

        /* Pack software fields into hardware QH dwords:
         * ep_char (0x04): mps[15:0]|format[14]|eps[13:12]|ep[10:8]|dev[7:0] */
        qh->ep_char = ((qh->mps & 0x7FF) << 16) |
                      (0 << 14) |  /* format=0 ctrl/bulk/int */
                      ((qh->endpoint_speed & 3) << 12) |
                      ((qh->endpoint & 0xF) << 8) |
                      (qh->devaddr & 0x7F);
        /* ep_caps (0x08): mult[31:30]|port[28:23]|hub[22:16]|c_mask[15:8]|s_mask[7:0] */
        qh->ep_caps = ((qh->mult & 3) << 30) |
                      (0 << 23) |
                      (qh->int_sched_mask & 0xFF);

        if(first->tdphys)
            qh->next_qtd = (uintptr_t)first->tdphys;
        else
            qh->next_qtd = 1;
    }

    if(transfer->type == USB_TRANSFER_CTRL || transfer->type == USB_TRANSFER_BULK)
        ehci_add_async(transfer);
    else if(transfer->type == USB_TRANSFER_INTERRUPT)
        ehci_add_periodic(transfer);
    else if(transfer->type == USB_TRANSFER_ISOCHRONOUS)
        kpanic("ehci: isochronous transfers not implemented yet!\n\n");

    transfer->success = 1;
}


static inline int transaction_success(struct ehci_td_t *td)
{
    return (((td->token.raw & 0xFE) == 0));
}


static int ehci_poll_transfer(struct usb_transfer_t *transfer)
{
    struct ehci_qh_t *qh = transfer->data;
    struct ehci_transaction_t *first;
    volatile struct usb_transaction_t *usbtrans;
    int done = 1;

    transfer->success = 1;

    for(usbtrans = transfer->trans_head; usbtrans != NULL; usbtrans = usbtrans->next) {
        struct ehci_transaction_t *ehcitrans = usbtrans->data;
        transfer->success = (transfer->success && transaction_success(ehcitrans->tdvirt));
        done = done && !(ehcitrans->tdvirt->token.raw & 0x80);

        if(transfer->success) {
            if(ehcitrans->inbuf && ehcitrans->inlen)
                A_memcpy(ehcitrans->inbuf, ehcitrans->tdbuf, ehcitrans->inlen);
        }
    }

    if(!done) return 0;

    for(usbtrans = transfer->trans_head; usbtrans != NULL; usbtrans = usbtrans->next) {
        struct ehci_transaction_t *ehcitrans = usbtrans->data;
        ehcitrans->tdvirt->token.bits.toggle = transfer->endpoint->toggle;
        transfer->endpoint->toggle = !(transfer->endpoint->toggle);
        ehcitrans->tdvirt->token.raw |= 0x80;
    }

    first = transfer->trans_head->data;
    qh->next_qtd = ((uintptr_t)first->tdphys & 0xFFFFFFF0);
    qh->next_qtd_alt = 1;

    return transfer->success;
}


static void ehci_wait_transfer(struct usb_transfer_t *transfer)
{
    volatile struct usb_transaction_t *usbtrans;

    usb_printk("ehci: waiting transfer...\n");
    /* Dump TD buffer info */
    for(usbtrans = transfer->trans_head; usbtrans; usbtrans = usbtrans->next) {
        struct ehci_transaction_t *et = usbtrans->data;
        usb_printk("ehci: TD phys=0x%x buf0=0x%x inbuf=0x%x inlen=%d\n",
            et->tdphys, et->tdvirt->buf0, et->inbuf, et->inlen);
    }
    __wait_transfer(transfer, 5000);
    usb_printk("ehci: transfer done, success=%d\n", transfer->success);

    transfer->success = 1;

    if(transfer->type == USB_TRANSFER_ISOCHRONOUS)
        kpanic("ehci: isochronous transfers not implemented yet!\n\n");

    for(usbtrans = transfer->trans_head; usbtrans != NULL; usbtrans = usbtrans->next) {
        struct ehci_transaction_t *uhcitrans = usbtrans->data;
        transfer->success = (transfer->success && transaction_success(uhcitrans->tdvirt));

        if(transfer->success) {
            if(uhcitrans->inbuf && uhcitrans->inlen) {
                usb_printk("ehci: tdbuf raw: %02x%02x%02x%02x%02x%02x%02x%02x\n",
                    ((uint8_t*)uhcitrans->tdbuf)[0], ((uint8_t*)uhcitrans->tdbuf)[1],
                    ((uint8_t*)uhcitrans->tdbuf)[2], ((uint8_t*)uhcitrans->tdbuf)[3],
                    ((uint8_t*)uhcitrans->tdbuf)[4], ((uint8_t*)uhcitrans->tdbuf)[5],
                    ((uint8_t*)uhcitrans->tdbuf)[6], ((uint8_t*)uhcitrans->tdbuf)[7]);
                A_memcpy(uhcitrans->inbuf, uhcitrans->tdbuf, uhcitrans->inlen);
            }
        }
    }
}


static struct ehci_td_t *ehci_alloc_td(struct usb_dev_t *usb, uintptr_t *tdphys)
{
    struct ehci_td_t *td;

    if(!(td = alloc_td(usb->priv, tdphys))) return NULL;

    A_memset(td, 0, sizeof(struct ehci_td_t));
    td->next_qtd = 0x01;
    td->next_qtd_alt = 0x01;
    td->token.raw |= 0x80;
    td->token.raw |= (3 << 10);

    return td;
}


static int ehci_alloc_tdbuf(struct usb_dev_t *usb, struct ehci_td_t *td,
                            uintptr_t *virt, size_t len)
{
    if(len > EHCI_TDBUF_SIZE)
        kpanic("ehci: TD buffer length > 4096\n");

    if(len) {
        uintptr_t tdvirt, tdphys;
        if(!(tdvirt = alloc_tdbuf(usb->priv, &tdphys)))
            return -ENOMEM;
        td->buf0 = tdphys;
        *virt = tdvirt;
    } else {
        td->buf0 = 0;
        *virt = 0;
    }
    return 0;
}


static int ehci_td_setup(struct usb_transaction_t *transaction,
                         struct ehci_transaction_t *et)
{
    struct usb_dev_t *usb = transaction->dev;
    struct ehci_dev_t *ehci = (struct ehci_dev_t *)usb->priv;
    struct ehci_td_t *td;
    volatile struct usb_request_t *req;
    uintptr_t bufvirt, tdphys;

    if(!(td = ehci_alloc_td(usb, &tdphys))) {
        usb_printk("%s: failed to set up transaction descriptor\n", "ehci");
        return -ENOMEM;
    }

    /* EHCI PID codes are 2-bit: 2 (10b) = SETUP, 1 (01b) = IN, 0 (00b) = OUT */
    td->token.bits.pid = 2;  /* SETUP */
    td->token.bits.bytes = 8;
    td->token.bits.toggle = transaction->toggle;

    if(ehci_alloc_tdbuf(usb, td, &bufvirt, sizeof(struct usb_request_t)) < 0) {
        usb_printk("%s: failed to alloc TD buffer\n", "ehci");
        free_td(ehci, (uintptr_t)td);
        return -ENOMEM;
    }

    req = (void *)bufvirt;
    req->type = transaction->type;
    req->req = transaction->req;
    req->hival = transaction->hival;
    req->loval = transaction->loval;
    req->index = transaction->index;
    req->len = transaction->len;

    et->tdbuf = (void *)bufvirt;
    et->tdvirt = td;
    et->tdphys = (void *)tdphys;

    return 0;
}


static int ehci_td_setup_io(struct usb_transaction_t *transaction,
                            struct ehci_transaction_t *et,
                            uint8_t pktid)
{
    struct usb_dev_t *usb = transaction->dev;
    struct ehci_dev_t *ehci = (struct ehci_dev_t *)usb->priv;
    struct ehci_td_t *td;
    uintptr_t bufvirt, tdphys;
    /* Map internal USB_TRANS_IN/OUT constants to EHCI 2-bit PID codes:
     * 2 (10b) = SETUP, 1 (01b) = IN, 0 (00b) = OUT */
    static const uint8_t ehci_pid_map[] = {
        [USB_TRANS_SETUP] = 2,
        [USB_TRANS_IN]    = 1,
        [USB_TRANS_OUT]   = 0,
    };

    if(!(td = ehci_alloc_td(usb, &tdphys))) {
        usb_printk("%s: failed to set up transaction descriptor\n", "ehci");
        return -ENOMEM;
    }

    td->token.bits.pid = ehci_pid_map[pktid];
    td->token.bits.bytes = transaction->len;
    td->token.bits.toggle = transaction->toggle;

    if(ehci_alloc_tdbuf(usb, td, &bufvirt, transaction->len) < 0) {
        usb_printk("%s: failed to alloc TD buffer\n", "ehci");
        free_td(ehci, (uintptr_t)td);
        return -ENOMEM;
    }

    et->tdbuf = (void *)bufvirt;
    et->tdvirt = td;
    et->tdphys = (void *)tdphys;

    return 0;
}


#define APPEND_TD(xfer, et)                                            \
    if((xfer) && (xfer)->trans_tail) {                                  \
        struct ehci_transaction_t *last = (xfer)->trans_tail->data;     \
        last->tdvirt->next_qtd = ((uintptr_t)(et)->tdphys);             \
        last->tdvirt->next_qtd_alt = 1;                                 \
    }


static int ehci_setup_transaction(struct usb_transaction_t *transaction)
{
    struct ehci_transaction_t *et;
    struct usb_transfer_t *transfer = (struct usb_transfer_t *)transaction->transfer;

    if(!(et = kmalloc(sizeof(struct ehci_transaction_t)))) {
        usb_printk("%s: failed to set up transaction\n", "ehci");
        return -ENOMEM;
    }

    A_memset(et, 0, sizeof(struct ehci_transaction_t));
    transaction->data = et;
    et->inbuf = NULL;
    et->inlen = 0;

    if(ehci_td_setup(transaction, et) < 0) {
        kfree(et);
        return -ENOMEM;
    }

    APPEND_TD(transfer, et);
    return 0;
}


static void ehci_in_transaction(struct usb_transaction_t *transaction)
{
    struct ehci_transaction_t *et;
    struct usb_transfer_t *transfer = (struct usb_transfer_t *)transaction->transfer;

    if(transfer->type == USB_TRANSFER_ISOCHRONOUS)
        kpanic("ehci: isochronous transfers not implemented yet!\n\n");

    if(!(et = kmalloc(sizeof(struct ehci_transaction_t)))) {
        usb_printk("%s: failed to set up IN transaction\n", "ehci");
        return;
    }

    A_memset(et, 0, sizeof(struct ehci_transaction_t));
    transaction->data = et;
    et->inbuf = transaction->buf;
    et->inlen = transaction->len;

    if(ehci_td_setup_io(transaction, et, USB_TRANS_IN) < 0) {
        kfree(et);
        return;
    }

    APPEND_TD(transfer, et);
}


static void ehci_out_transaction(struct usb_transaction_t *transaction)
{
    struct ehci_transaction_t *et;
    struct usb_transfer_t *transfer = (struct usb_transfer_t *)transaction->transfer;

    if(transfer->type == USB_TRANSFER_ISOCHRONOUS)
        kpanic("ehci: isochronous transfers not implemented yet!\n\n");

    if(!(et = kmalloc(sizeof(struct ehci_transaction_t)))) {
        usb_printk("%s: failed to set up OUT transaction\n", "ehci");
        return;
    }

    A_memset(et, 0, sizeof(struct ehci_transaction_t));
    transaction->data = et;
    et->inbuf = NULL;
    et->inlen = 0;

    if(ehci_td_setup_io(transaction, et, USB_TRANS_OUT) < 0) {
        kfree(et);
        return;
    }

    if(transaction->buf && transaction->len)
        A_memcpy(et->tdbuf, transaction->buf, transaction->len);

    APPEND_TD(transfer, et);
}

#undef APPEND_TD


static void ehci_free_usb_transaction_data(volatile struct usb_transaction_t *usbtrans)
{
    struct usb_transfer_t *transfer = (struct usb_transfer_t *)usbtrans->transfer;
    struct ehci_dev_t *ehci = (struct ehci_dev_t *)transfer->dev->priv;
    struct ehci_transaction_t *et = usbtrans->data;

    if(!et) return;

    if(et->tdvirt) free_td(ehci, (uintptr_t)et->tdvirt);
    if(et->tdbuf)  free_tdbuf(ehci, (uintptr_t)et->tdbuf);

    et->tdvirt = 0;
    et->tdphys = 0;
    et->tdbuf = 0;

    kfree(et);
    usbtrans->data = NULL;
}


static int ehci_setup_device(volatile struct ehci_dev_t *ehci, unsigned int port, uint8_t speed)
{
    struct usb_dev_t *usb;
    int res;

    usb_printk("%s: setting up high-speed USB device on port %d\n", "ehci", port);

    if(!(usb = usb_create_dev(ehci->pci->unit, port, speed))) {
        usb_printk("%s: failed to create USB device\n", "ehci");
        return -ENOMEM;
    }

    usb->type = USB_TYPE_EHCI;
    usb->priv = (void *)ehci;
    usb->ops = &ehci_ops;

    if((res = usb_setup_device(usb, ehci_get_next_addr((void *)ehci))) < 0) {
        usb_printk("%s: failed to set up USB device\n", "ehci");
        ehci_free_addr((void *)ehci, usb->num);
        usb_destroy_dev(usb);
        return res;
    }

    ehci->ports[port].usb = usb;
    ehci->ports[port].flags |= EHCI_PORT_FLAG_CONNECTED;

    return 0;
}


static int ehci_start(volatile struct ehci_dev_t *ehci)
{
    pci_compat_t *pci = ehci->pci;
    volatile uint8_t byte;
    volatile uint32_t dword;
    volatile int timeout;

    dword = pcidev_inl(ehci, EHCI_REG_HCCAP + HCCAP_HCCPARAMS);
    byte = BYTE2(dword);

    if(byte >= 0x40) {
        dword = pci_config_read_long(pci, byte);
        if((dword & 0xFF) == 1) {
            dword |= (1 << 24);
            pci_config_write_long(pci, byte, dword);
            timeout = 300;
            while((pci_config_read_long(pci, byte) & (1 << 16)) && timeout--)
                tick_delay(1);
            if(timeout <= 0)
                usb_printk("%s: BIOS semaphore timeout\n", "ehci");
            timeout = 300;
            while(!(pci_config_read_long(pci, byte) & (1 << 24)) && timeout--)
                tick_delay(1);
            if(timeout <= 0)
                usb_printk("%s: OS semaphore timeout\n", "ehci");
            pci_config_write_long(pci, byte + 4, 0x00);
            tick_delay(50);
            usb_printk("%s: finished BIOS handover\n", "ehci");
        }
    }

    ehci->caplen = pcidev_inl(ehci, EHCI_REG_HCCAP + HCCAP_CAPLENGTH) & 0xFF;

    dword = pcidev_inl(ehci, EHCI_REG_HCCAP + HCCAP_HCSPARAMS);
    ehci->port_count = dword & 0xf;
    usb_printk("%s: root ports %u, caplen=0x%x\n", "ehci", ehci->port_count, ehci->caplen);

    /* NOW advance iobase past capability regs to operational regs */
    ehci->capbase = ehci->iobase;  /* save for later HCCAP reads */
    ehci->iobase += ehci->caplen;

    // stop the host controller
    usb_printk("ehci: stopping HC...\n");
    dword = pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBCMD);
    usb_printk("ehci: USBCMD=0x%x\n", dword);
    dword &= ~(1 << 0);
    pcidev_outl(ehci, EHCI_REG_HCOP + HCOP_USBCMD, dword);

    dword = pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBSTS);
    if(!(dword & ECHI_USBSTS_HCHALTED)) {
        usb_printk("ehci: waiting for HCHALTED...\n");
        timeout = 100;
        while(!(dword & ECHI_USBSTS_HCHALTED) && timeout--) {
            tick_delay(2);
            dword = pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBSTS);
        }
        if(timeout <= 0) usb_printk("ehci: HCHALTED timeout!\n");
    }
    usb_printk("ehci: HC stopped\n");

    pcidev_outl(ehci, EHCI_REG_HCOP + HCOP_USBSTS, 0x3F);
    pcidev_outl(ehci, EHCI_REG_HCOP + HCOP_USBINTR, 0);
    pcidev_outl(ehci, EHCI_REG_HCOP + HCOP_CTRLDSSEGMENT, 0);

    // reset the host controller
    usb_printk("ehci: resetting HC...\n");
    dword = pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBCMD);
    dword |= (1 << 1);
    pcidev_outl(ehci, EHCI_REG_HCOP + HCOP_USBCMD, dword);

    timeout = 50;
    while((pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBCMD) & (1 << 1)) && timeout--)
        tick_delay(1);

    if(timeout <= 0) {
        usb_printk("%s: reset timeout\n", "ehci");
        return -ETIMEDOUT;
    }
    usb_printk("ehci: reset done\n");

    pcidev_outl(ehci, EHCI_REG_HCOP + HCOP_FRINDEX, 0x00);

    // init the async list
    usb_printk("ehci: init async list...\n");
    if(!ehci->async_qh) {
        ehci->async_qh = alloc_qh(ehci);
        ehci->tail_qh = ehci->async_qh;

        uintptr_t qhphys = ehci->async_qh->self_phys;
        A_memset((void *)ehci->async_qh, 0, sizeof(struct ehci_qh_t));
        ehci->async_qh->self_phys = qhphys;

        ehci->async_qh->next = ehci->async_qh->self_phys | (1 << 1);
        ehci->async_qh->nextvirt = ehci->async_qh;
        ehci->async_qh->endpoint_speed = 2;
        ehci->async_qh->toggle = 1;
        ehci->async_qh->head_flag = 1;
        ehci->async_qh->mult = 1;
        ehci->async_qh->next_qtd = 1;
        ehci->async_qh->next_qtd_alt = 1;
        /* Pack hardware fields: ep_char/ep_caps for async head (inactive QH) */
        ehci->async_qh->ep_char = (0 << 14) | ((2 & 3) << 12); /* eps=2, dev=0 */
        ehci->async_qh->ep_caps = ((1 & 3) << 30);  /* mult=1 */
    }

    // init the periodic list
    dword = pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBCMD);
    dword &= ECHI_USBCMD_FRLIST_MASK;
    dword |= EHCI_USBCMD_FRLIST_1024;
    pcidev_outl(ehci, EHCI_REG_HCOP + HCOP_USBCMD, dword);

    /* Program list base addresses but defer schedule enable until first transfer.
     * QEMU's EHCI model may halt if it sees an empty (head-only) async list. */
    pcidev_outl(ehci, EHCI_REG_HCOP + HCOP_ASYNCLISTADDR, ehci->async_qh->self_phys);
    pcidev_outl(ehci, EHCI_REG_HCOP + HCOP_PERIODICLISTBASE, (uintptr_t)ehci->framelist_phys);

    pcidev_outl(ehci, EHCI_REG_HCOP + HCOP_USBSTS, 0x3F);

    dword = pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBSTS);
    if(dword & ECHI_USBSTS_HCHALTED) {
        usb_printk("%s: starting controller\n", "ehci");
        dword = pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBCMD);
        dword |= (1 << 0);
        pcidev_outl(ehci, EHCI_REG_HCOP + HCOP_USBCMD, dword);
        tick_delay(50);
        dword = pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBSTS);
        usb_printk("ehci: controller status 0x%x\n", dword);
    }

    pcidev_outl(ehci, EHCI_REG_HCOP + HCOP_CONFIGFLAG, 0x01);
    usb_printk("ehci: CONFIGFLAG set\n");

    /* QEMU EHCI: explicitly power on all ports */
    for(volatile int pi = 0; pi < ehci->port_count; pi++) {
        int preg = EHCI_REG_HCOP + HCOP_PORTSC + (4 * pi);
        dword = pcidev_inl(ehci, preg);
        usb_printk("ehci: port %d PORTSC=0x%x before power\n", pi, dword);
        if(!(dword & EHCI_PORTSC_POWER)) {
            dword |= EHCI_PORTSC_POWER;
            pcidev_outl(ehci, preg, dword);
        }
    }
    tick_delay(50);

    dword = pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBSTS);
    if(dword & ECHI_USBSTS_HCHALTED) {
        usb_printk("ehci: controller halted (0x%x), restarting\n", dword);
        dword = pcidev_inl(ehci, EHCI_REG_HCOP + HCOP_USBCMD);
        dword |= (1 << 0);  /* set run/stop */
        pcidev_outl(ehci, EHCI_REG_HCOP + HCOP_USBCMD, dword);
        tick_delay(50);
    }
    usb_printk("%s: controller running, enabling ports\n", "ehci");
    return ehci_enable_ports(ehci);
}


int ehci_install(pci_compat_t *pci, struct pci_bar_t *bar)
{
    struct ehci_dev_t *ehci;
    int res = 0;
    uintptr_t base = bar->base;
    volatile unsigned int i;

    if(!(ehci = kmalloc(sizeof(struct ehci_dev_t)))) {
        usb_printk("%s: insufficient memory to init device\n", "echi");
        return -ENOMEM;
    }

    A_memset(ehci, 0, sizeof(struct ehci_dev_t));
    ehci->addr_bitmap[0] = 1;
    ehci->pci = pci;
    ehci->iosize = bar->iosize;

    if(bar->iotype == PCI_IOTYPE_MMIO) {
        base &= ~0xf;
        ehci->iobase = mmio_map(base, base + bar->iosize);
        ehci->mmio = 1;
    } else {
        ehci->iobase = base & ~0x3;
    }

    usb_printk("%s: base " _XPTR_ ", iobase " _XPTR_ ", iosize " _XPTR_ " (%s)\n",
           "ehci", base, ehci->iobase, ehci->iosize, ehci->mmio ? "MMIO" : "I/O");

    if(!first_ehci) {
        first_ehci = ehci;
    } else {
        volatile struct ehci_dev_t *tmp = first_ehci;
        while(tmp->next) tmp = tmp->next;
        tmp->next = ehci;
    }

    pci_enable_busmastering(pci);
    pci_enable_memoryspace(pci);

    if(!(ehci->framelist_phys = pmmngr_alloc_block())) { res = -ENOMEM; goto err; }
    ehci->framelist = (uint32_t *)mmio_map((physical_addr)ehci->framelist_phys,
                                           (physical_addr)ehci->framelist_phys + PAGE_SIZE);
    for(i = 0; i < 1024; i++)
        ehci->framelist[i] = (uint32_t)0x01;

    if(!(ehci->tdpool_phys = (uintptr_t)pmmngr_alloc_block())) { res = -ENOMEM; goto err; }
    ehci->tdpool = mmio_map(ehci->tdpool_phys, ehci->tdpool_phys + PAGE_SIZE);
    init_tdpool(ehci);
    usb_printk("%s: tdpool virt " _XPTR_ ", phys " _XPTR_ "\n", "ehci", ehci->tdpool, ehci->tdpool_phys);

    if(!(ehci->tdbufpool_phys = (uintptr_t)pmmngr_alloc_blocks(EHCI_TDBUF_POOL_SIZE / PAGE_SIZE))) {
        res = -ENOMEM; goto err; }
    ehci->tdbufpool = mmio_map(ehci->tdbufpool_phys, ehci->tdbufpool_phys + EHCI_TDBUF_POOL_SIZE);
    usb_printk("%s: tdbufpool virt " _XPTR_ ", phys " _XPTR_ "\n", "ehci", ehci->tdbufpool, ehci->tdbufpool_phys);

    if(!(ehci->qhpool_phys = (uintptr_t)pmmngr_alloc_block())) { res = -ENOMEM; goto err; }
    ehci->qhpool = mmio_map(ehci->qhpool_phys, ehci->qhpool_phys + PAGE_SIZE);
    init_qhpool(ehci);
    usb_printk("%s: qhpool virt " _XPTR_ ", phys " _XPTR_ "\n", "ehci", ehci->qhpool, ehci->qhpool_phys);

    if((uintptr_t)ehci->framelist_phys > 0xffffffff ||
       ehci->tdpool_phys > 0xffffffff ||
       ehci->tdbufpool_phys > 0xffffffff ||
       ehci->qhpool_phys > 0xffffffff) {
        usb_printk("ehci: fr %lx, tdp %lx, td %lx, qh %lx\n",
                ehci->framelist_phys, ehci->tdpool_phys,
                ehci->tdbufpool_phys, ehci->qhpool_phys);
        kpanic("ehci: could not get physical frames < 0xffffffff\n");
        for(;;);
    }

    if((res = ehci_start(ehci)) < 0)
        goto err;

    ehci->flags |= EHCI_FLAG_RUN;
    usb_printk("%s: setup done\n", "ehci");
    return 0;

err:
    if(ehci->mmio) { vmmngr_free_pages(ehci->iobase, ehci->iobase + ehci->iosize); ehci->iobase = 0; }
    if(ehci->framelist_phys) { pmmngr_free_block(ehci->framelist_phys); ehci->framelist = 0; ehci->framelist_phys = 0; }
    if(ehci->tdpool_phys) { pmmngr_free_block((void *)ehci->tdpool_phys); ehci->tdpool_phys = 0; ehci->tdpool = 0; }
    if(ehci->tdbufpool_phys) { pmmngr_free_blocks((void *)ehci->tdbufpool_phys, EHCI_TDBUF_POOL_SIZE / PAGE_SIZE); ehci->tdbufpool_phys = 0; ehci->tdbufpool = 0; }
    if(ehci->qhpool_phys) { pmmngr_free_block((void *)ehci->qhpool_phys); ehci->qhpool_phys = 0; ehci->qhpool = 0; }
    return res;
}


void ehci_poll(void)
{
    volatile struct ehci_dev_t *ehci = first_ehci;
    volatile struct usb_dev_t *usb;
    volatile unsigned int i;
    uint32_t dword;
    int reg;

    while(ehci) {
        if(!(ehci->flags & EHCI_FLAG_RUN) ||
           !(ehci->flags & EHCI_FLAG_PORTENABLED)) {
            ehci = ehci->next;
            continue;
        }

        for(i = 0; i < ehci->port_count; i++) {
            reg = EHCI_REG_HCOP + HCOP_PORTSC + (i * 4);
            dword = pcidev_inl(ehci, reg);

            if(dword & EHCI_PORTSC_CONNCHG) {
                pcidev_outl(ehci, reg, dword);

                if(dword & EHCI_PORTSC_CONN) {
                    if(!(ehci->ports[i].flags & EHCI_PORT_FLAG_CONNECTED)) {
                        usb_printk("ehci: device connected to port %d\n", i);
                        ehci_check_port_state(ehci, i);
                        ehci->ports[i].port = i;
                        ehci->ports[i].ehci = ehci;
                    }
                } else {
                    usb_printk("ehci: device removed from port %d\n", i);
                    usb = ehci->ports[i].usb;
                    if(usb && usb->type == USB_TYPE_EHCI) {
                        ehci->ports[i].usb = NULL;
                        ehci_free_addr((void *)ehci, usb->num);
                        usb_destroy_dev((struct usb_dev_t *)usb);
                        ehci->ports[i].flags &= ~EHCI_PORT_FLAG_CONNECTED;
                    }
                }
            }
        }
        ehci = ehci->next;
    }
}


static struct usb_ops_t ehci_ops =
{
    .setup_transfer = ehci_setup_transfer,
    .schedule_transfer = ehci_schedule_transfer,
    .wait_transfer = ehci_wait_transfer,
    .poll_transfer = ehci_poll_transfer,
    .delete_transfer = ehci_delete_transfer,
    .setup_transaction = ehci_setup_transaction,
    .in_transaction = ehci_in_transaction,
    .out_transaction = ehci_out_transaction,
    .free_transaction_data = ehci_free_usb_transaction_data,
    .get_next_addr = ehci_get_next_addr,
    .free_addr = ehci_free_addr,
};
