/* 
/**
 *    Copyright 2025, 2026 (c)
 * 
 *    file: usb_uhci.c
 *

/**
 *  \file usb_uhci.c
 *
 *  The Universal Serial Bus (USB) driver code is split into several files:
 *    - usb.c       => main entry point and general functions
 *    - usb_msd.c   => functions to handle Mass Storage Devices (MSD)
 *    - usb_hid.c   => functions to handle Human Interaction Devices (HID)
 *    - usb_hub.c   => functions to handle USB hubs
 *    - usb_ioctl.c => functions to handle ioctl() calls
 *    - usb_ohci.c  => OHCI layer
 *    - usb_uhci.c  => UHCI layer
 *    - usb_ehci.c  => EHCI layer
 */

//#define __DEBUG

#include <kernel/errno.h>
#include <kernel/pciio.h>
#include <kernel/asm.h>
#include <kernel/usb.h>
#include <kernel/usb_uhci.h>
#include <kernel/pic.h>
#include <kernel/hillsonos.h>
#include <mm/kheap.h>


static volatile struct uhci_dev_t *first_uhci = NULL;
static struct usb_ops_t uhci_ops;

struct regs { int dummy; };
#define PROPERTY_SELECT_EVENT  1
int uhci_intr(struct regs *r, void *arg);
static int uhci_setup_device(volatile struct uhci_dev_t *uhci, unsigned int port, uint8_t speed);
static void uhci_free_usb_transaction_data(volatile struct usb_transaction_t *usbtrans);


struct usb_dev_t *uhci_get_dev_struct(pci_compat_t *bus, uint8_t num)
{
    volatile struct uhci_dev_t *uhci = first_uhci;

    if(!bus || num < 1)
    {
        return NULL;
    }

    while(uhci)
    {
        if(uhci->pci == bus)
        {
            if(num > uhci->port_count)
            {
                return NULL;
            }

            return uhci->ports[num - 1].usb;
        }

        uhci = uhci->next;
    }

    return NULL;
}


static void print_port_state(struct uhci_dev_t *uhci, unsigned int port)
{
    int first = 1;
    int reg = UHCI_REG_PORTSC1 + (2 * port);
    uint16_t word = pcidev_inw(uhci, reg);

    usb_printk("uhci: port %u: status 0x%x (", port, word);

    if(word & UHCI_PORTSC_RST)
    {
        usb_printk("%sreset", first ? "" : ", ");
        first = 0;
    }

    if(word & UHCI_PORTSC_SUSPEND)
    {
        usb_printk("%ssuspend", first ? "" : ", ");
        first = 0;
    }

    if(word & UHCI_PORTSC_RES_DET)
    {
        usb_printk("%sresume detect", first ? "" : ", ");
        first = 0;
    }

    if(word & UHCI_PORTSC_LOSPEED)
    {
        usb_printk("%slowspeed device", first ? "" : ", ");
        first = 0;
    }
    else
    {
        usb_printk("%sfullspeed device", first ? "" : ", ");
        first = 0;
    }

    if(word & (1 << 5))
    {
        usb_printk("%sline D-", first ? "" : ", ");
        first = 0;
    }

    if(word & (1 << 4))
    {
        usb_printk("%sline D+", first ? "" : ", ");
        first = 0;
    }

    if(word & UHCI_PORTSC_EN_CHG)
    {
        usb_printk("%senable change", first ? "" : ", ");
        first = 0;
    }

    if(word & UHCI_PORTSC_EN)
    {
        usb_printk("%senabled", first ? "" : ", ");
        first = 0;
    }

    if(word & UHCI_PORTSC_CS_CHG)
    {
        usb_printk("%sdevice change", first ? "" : ", ");
        first = 0;
    }

    if(word & UHCI_PORTSC_CS)
    {
        usb_printk("%sdevice attached", first ? "" : ", ");
        first = 0;
    }
    else
    {
        usb_printk("%sno device attached", first ? "" : ", ");
        first = 0;
    }

    usb_printk(")\n");
}


static unsigned int uhci_get_next_addr(void *__uhci)
{
    struct uhci_dev_t *uhci = __uhci;
    volatile unsigned int i, j;

    for(i = 0; i < (MAX_DEV_PER_HC / sizeof(uint32_t)); i++)
    {
        for(j = 0; j < 32; j++)
        {
            if(!(uhci->addr_bitmap[i] & (1 << j)))
            {
                uhci->addr_bitmap[i] |= (1 << j);
                return (i * 32) + j;
            }
        }
    }

    return 0;
}


static void uhci_free_addr(void *__uhci, unsigned int i)
{
    struct uhci_dev_t *uhci = __uhci;

    if(i < 1 || i >= MAX_DEV_PER_HC)
    {
        return;
    }

    uhci->addr_bitmap[i / 32] &= ~(1 << (i % 32));
}


static void free_qh(struct uhci_qh_t *qh)
{
    __sync_bool_compare_and_swap(&qh->alloced, 1, 0);
}


static struct uhci_qh_t *alloc_qh(struct uhci_dev_t *uhci)
{
    struct uhci_qh_t *qh, *qhl = uhci->qhpool + UHCI_MAX_QH;

    for(qh = uhci->qhpool; qh != qhl; qh++)
    {
        if(__sync_bool_compare_and_swap(&qh->alloced, 0, 1))
        {
            return qh;
        }
    }

    usb_printk("uhci: failed to alloc HQ\n");

    return NULL;
}


static void free_td(struct uhci_td_t *td)
{
    __sync_bool_compare_and_swap(&td->alloced, 1, 0);
}


static struct uhci_td_t *alloc_td(struct uhci_dev_t *uhci, uintptr_t *tdphys)
{
    struct uhci_td_t *td, *tdl = uhci->tdpool + UHCI_MAX_TD;
    uintptr_t tdpool_phys = uhci->tdpool_phys;

    *tdphys = 0;

    for(td = uhci->tdpool; td != tdl; td++)
    {
        if(__sync_bool_compare_and_swap(&td->alloced, 0, 1))
        {
            *tdphys = tdpool_phys;
            return td;
        }

        tdpool_phys += sizeof(struct uhci_td_t);
    }

    usb_printk("%s: failed to alloc TD\n", "uhci");

    return NULL;
}


/*
static void print_buf_usage(struct uhci_dev_t *uhci, const char *caller)
{
    volatile int i;

    usb_printk("%s: uhci 0x%lx ", caller, uhci);
    for(i = 0; i < UHCI_MAX_TDBUF; i++)
    {
        usb_printk("%d ", uhci->tdbuf_used[i]);
    }
    usb_printk("\n");
}
*/


static void free_tdbuf(struct uhci_dev_t *uhci, uintptr_t tdbuf)
{
    volatile int i = (tdbuf - uhci->tdbufpool) / UHCI_TDBUF_SIZE;

    __sync_bool_compare_and_swap(&uhci->tdbuf_used[i], 1, 0);
    //print_buf_usage(uhci, __func__);
}


static uintptr_t alloc_tdbuf(struct uhci_dev_t *uhci, uintptr_t *tdbufphys)
{
    volatile int i;

    *tdbufphys = 0;

    for(i = 0; i < UHCI_MAX_TDBUF; i++)
    {
        if(__sync_bool_compare_and_swap(&uhci->tdbuf_used[i], 0, 1))
        {
            *tdbufphys = uhci->tdbufpool_phys + (i * UHCI_TDBUF_SIZE);
            return uhci->tdbufpool + (i * UHCI_TDBUF_SIZE);
        }
    }

    //print_buf_usage(uhci, __func__);
    usb_printk("%s: failed to alloc TD buffer\n", "uhci");

    return 0;
}


static void init_qhpool(struct uhci_dev_t *uhci, uintptr_t qhphys)
{
    struct uhci_qh_t *qh, *qhl = uhci->qhpool + UHCI_MAX_QH;

    A_memset(uhci->qhpool, 0, PAGE_SIZE);

    for(qh = uhci->qhpool; qh != qhl; qh++)
    {
        qh->self_phys = qhphys;
        qhphys += sizeof(struct uhci_qh_t);
    }
}


static inline void uhci_fill_qh(struct uhci_dev_t *uhci, struct uhci_qh_t *qh,
                                uintptr_t tdphys)
{
    struct uhci_qh_t *prev;

    switch(qh->freq)
    {
        case 32:
            prev = uhci->qh_32ms;
            break;

        case 16:
            prev = uhci->qh_16ms;
            break;

        case 8:
            prev = uhci->qh_8ms;
            break;

        case 4:
            prev = uhci->qh_4ms;
            break;

        case 2:
            prev = uhci->qh_2ms;
            break;

        default:
            prev = uhci->qh_1ms;
            break;
    }

    // mark as invalid to ensure controller is not processing this queue
    qh->transfer = (tdphys & 0xFFFFFFF0) | (1 << 0);

    kernel_mutex_lock(&uhci->qh_lock);

    __atomic_store_n(&qh->next, prev->next, __ATOMIC_SEQ_CST);
    __atomic_store_n(&qh->nextvirt, prev->nextvirt, __ATOMIC_SEQ_CST);
    __atomic_store_n(&prev->next, qh->self_phys | (1 << 1), __ATOMIC_SEQ_CST);
    __atomic_store_n(&prev->nextvirt, qh, __ATOMIC_SEQ_CST);

    kernel_mutex_unlock(&uhci->qh_lock);

    // now set the right pointer
    qh->transfer &= ~(1 << 0);
}


static void uhci_reset_port(volatile struct uhci_dev_t *uhci, unsigned int port)
{
    uint16_t word;
    int timeout;
    int reg = UHCI_REG_PORTSC1 + (2 * port);

    pcidev_outw(uhci, reg, UHCI_PORTSC_RST);
    tick_delay(1);

    // clear reset bit
    word = pcidev_inw(uhci, reg);
    word &= ~UHCI_PORTSC_RST;
    pcidev_outw(uhci, reg, word);

    // wait for reset bit to clear
    timeout = 20;

    while((pcidev_inw(uhci, reg) & UHCI_PORTSC_RST) && timeout--)
    {
        tick_delay(1);
    }

    if(timeout <= 0)
    {
        usb_printk("%s: reset timeout for port %u\n", "uhci", port);
        return;
    }

    // enable port
    pcidev_outw(uhci, reg, UHCI_PORTSC_CS_CHG|UHCI_PORTSC_EN_CHG|UHCI_PORTSC_EN);
    tick_delay(1);
    usb_printk("%s: port %u reset: status 0x%x\n", "uhci", port, pcidev_inw(uhci, reg));
}


static int uhci_enable_ports(struct uhci_dev_t *uhci)
{
    volatile unsigned int i;
    uint16_t word;

    if(!(uhci->ports = kmalloc(sizeof(struct uhci_port_t) * uhci->port_count)))
    {
        usb_printk("%s: insufficienct memory to enable ports\n", "uhci");
        return -ENOMEM;
    }

    A_memset((void *)uhci->ports, 0, sizeof(struct uhci_port_t) * uhci->port_count);

    for(i = 0; i < uhci->port_count; i++)
    {
        uhci_reset_port(uhci, i);
        uhci->ports[i].port = i;
        uhci->ports[i].uhci = uhci;
    }

    for(i = 0; i < uhci->port_count; i++)
    {
        print_port_state(uhci, i);
    }

    // check for devices that are attached on boot
    for(i = 0; i < uhci->port_count; i++)
    {
        word = pcidev_inw(uhci, UHCI_REG_PORTSC1 + (2 * i));

        if(word & UHCI_PORTSC_CS)
        {
            uhci_setup_device(uhci, i, (word & UHCI_PORTSC_LOSPEED) ?
                                          USB_SPEED_LOW : USB_SPEED_FULL);
        }
    }

    uhci->flags |= UHCI_FLAG_PORTENABLED;

    return 0;
}


static void uhci_setup_transfer(struct usb_transfer_t *transfer)
{
    struct uhci_dev_t *uhci = (struct uhci_dev_t *)transfer->dev->priv;
    struct uhci_qh_t *qh;

    transfer->data = alloc_qh(uhci);
    qh = transfer->data;

    if(transfer->type == USB_TRANSFER_INTERRUPT)
    {
        qh->freq = transfer->freq;

        if(qh->freq < 2)
        {
            qh->freq = 1;
        }
        else if(qh->freq < 4)
        {
            qh->freq = 2;
        }
        else if(qh->freq < 8)
        {
            qh->freq = 4;
        }
        else if(qh->freq < 16)
        {
            qh->freq = 8;
        }
        else if(qh->freq < 32)
        {
            qh->freq = 16;
        }
        else
        {
            qh->freq = 32;
        }
    }
    else
    {
        qh->freq = 1;
    }
}


static void uhci_delete_transfer(struct usb_transfer_t *transfer)
{
    struct uhci_dev_t *uhci = (struct uhci_dev_t *)transfer->dev->priv;
    volatile struct uhci_qh_t *prev, *qh = transfer->data;

    if(!qh)
    {
        return;
    }

    // mark as invalid to ensure controller is not processing this queue
    qh->transfer |= (1 << 0);

    switch(qh->freq)
    {
        case 32:
            prev = uhci->qh_32ms;
            break;

        case 16:
            prev = uhci->qh_16ms;
            break;

        case 8:
            prev = uhci->qh_8ms;
            break;

        case 4:
            prev = uhci->qh_4ms;
            break;

        case 2:
            prev = uhci->qh_2ms;
            break;

        default:
            prev = uhci->qh_1ms;
            break;
    }

    kernel_mutex_lock(&uhci->qh_lock);

    while(prev)
    {
        if(prev->nextvirt == qh)
        {
            __atomic_store_n(&prev->next, qh->next, __ATOMIC_SEQ_CST);
            __atomic_store_n(&prev->nextvirt, qh->nextvirt, __ATOMIC_SEQ_CST);
            break;
        }

        prev = prev->nextvirt;
    }

    kernel_mutex_unlock(&uhci->qh_lock);

    free_qh(transfer->data);
    transfer->data = NULL;
}


static void uhci_schedule_transfer(struct usb_transfer_t *transfer)
{
    struct uhci_dev_t *uhci = (struct uhci_dev_t *)transfer->dev->priv;
    struct uhci_transaction_t *first = transfer->trans_head->data;
    struct uhci_qh_t *qh = transfer->data;
    volatile uint16_t word;

    uhci_fill_qh(uhci, qh, (uintptr_t)first->tdphys);

    /* Debug: verify controller state and QH chain */
    usb_printk("uhci: CMD=0x%x STS=0x%x FRNUM=0x%x FRBASE=0x%x\n",
        pcidev_inw(uhci, UHCI_REG_CMD), pcidev_inw(uhci, UHCI_REG_STS),
        pcidev_inw(uhci, UHCI_REG_FRNUM), pcidev_inl(uhci, UHCI_REG_FRBASEADDR));
    usb_printk("uhci: QH phys=0x%x next=0x%x transfer=0x%x freq=%d\n",
        qh->self_phys, qh->next, qh->transfer, qh->freq);
    usb_printk("uhci: qh_1ms phys=0x%x next=0x%x transfer=0x%x\n",
        uhci->qh_1ms->self_phys, uhci->qh_1ms->next, uhci->qh_1ms->transfer);

    /* Debug: dump TD chain before starting */
    {
        volatile struct usb_transaction_t *ut = transfer->trans_head;
        int tn = 0;
        while (ut) {
            struct uhci_transaction_t *uht = ut->data;
            struct uhci_td_t *td = uht->tdvirt;
            usb_printk("uhci: TD%d: buf=0x%x maxlen=%d pktid=%d toggle=%d dev=%d ep=%d "
                       "dword1=0x%x dword2=0x%x next=0x%x lospeed=%d\n",
                tn, td->buf, td->maxlen, td->pktid, td->toggle, td->devaddr,
                td->endpoint, td->dword1, td->dword2, td->next, td->lospeed);
            ut = ut->next;
            tn++;
        }
    }

    transfer->success = 1;

    // start scheduler if not running
    if((pcidev_inw(uhci, UHCI_REG_STS) & UHCI_STS_HCHALTED))
    {
        uhci->flags |= UHCI_FLAG_RUN;
        //pcidev_outw(uhci, UHCI_REG_FRNUM, 0);
        word = pcidev_inw(uhci, UHCI_REG_CMD);
        word |= UHCI_CMD_RS;
        pcidev_outw(uhci, UHCI_REG_CMD, word);
    }
}


static inline int transaction_success(struct uhci_td_t *td)
{
    return !(td->dword1 & 0xFE0000);
}


static int uhci_poll_transfer(struct usb_transfer_t *transfer)
{
    //struct uhci_dev_t *uhci = (struct uhci_dev_t *)transfer->dev->priv;
    struct uhci_qh_t *qh = transfer->data;
    struct uhci_transaction_t *first;
    volatile struct usb_transaction_t *usbtrans;
    int done = 1;

    transfer->success = 1;

    // check for completion
    for(usbtrans = transfer->trans_head; usbtrans != NULL; usbtrans = usbtrans->next)
    {
        struct uhci_transaction_t *uhcitrans = usbtrans->data;
        transfer->success = (transfer->success && transaction_success(uhcitrans->tdvirt));
        done = done && !(uhcitrans->tdvirt->dword1 & 0x800000);

        if(transfer->success)
        {
            if(uhcitrans->inbuf && uhcitrans->inlen)
            {
                A_memcpy(uhcitrans->inbuf, uhcitrans->tdbuf, uhcitrans->inlen);
            }
        }
    }

    if(!done)
    {
        return 0;
    }

    // re-schedule
    for(usbtrans = transfer->trans_head; usbtrans != NULL; usbtrans = usbtrans->next)
    {
        struct uhci_transaction_t *uhcitrans = usbtrans->data;

        uhcitrans->tdvirt->toggle = transfer->endpoint->toggle;
        transfer->endpoint->toggle = !(transfer->endpoint->toggle);
        uhcitrans->tdvirt->len = 0;
        uhcitrans->tdvirt->active = 1;
    }

    // The UHCI protocol says the host controller overwrites the QH's
    // Queue Element Link Pointer after processing a TD, so we need to 
    // re-wire our QH. It seems the host controller leaves the Queue
    // Head Link Pointer alone, so we don't need to fix this one
    first = transfer->trans_head->data;
    qh->transfer = ((uintptr_t)first->tdphys & 0xFFFFFFF0);

    return transfer->success;
}


static void uhci_wait_transfer(struct usb_transfer_t *transfer)
{
    //struct uhci_dev_t *uhci = (struct uhci_dev_t *)transfer->dev->priv;
    volatile struct usb_transaction_t *usbtrans = transfer->trans_head;
    volatile int timeout = 150000;

    while(timeout--)
    {
        struct uhci_transaction_t *uhcitrans = usbtrans->data;
        struct uhci_td_t *td = uhcitrans->tdvirt;

        // while not active
        while(!(td->dword1 & 0x800000))
        {
            if((usbtrans = usbtrans->next) == NULL)
            {
                break;
            }

            uhcitrans = usbtrans->data;
            td = uhcitrans->tdvirt;
        }

        if(usbtrans == NULL)
        {
            break;
        }

        /*
        __asm__ __volatile__("pause":::);
        __asm__ __volatile__("pause":::);
        __asm__ __volatile__("pause":::);
        //scheduler();
        //tick_delay(2);
        */
        set_task_waking_signal(this_core->cur_task, 0);
        // __sync_and_and_fetch(&this_core->cur_task->properties, ~PROPERTY_SELECT_EVENT);
        block_task_timeout(this_core->cur_task, 1);
    }

    if(!timeout)
    {
        usb_printk("%s: transfer timed out\n", "uhci");
    }

    // check results
    transfer->success = 1;

    for(usbtrans = transfer->trans_head; usbtrans != NULL; usbtrans = usbtrans->next)
    {
        struct uhci_transaction_t *uhcitrans = usbtrans->data;
        struct uhci_td_t *td = uhcitrans->tdvirt;
        transfer->success = (transfer->success && transaction_success(uhcitrans->tdvirt));
        usb_printk("uhci: post-TD dword1=0x%x dword2=0x%x success=%d\n",
            td->dword1, td->dword2, transfer->success);

        if(transfer->success)
        {
            if(uhcitrans->inbuf && uhcitrans->inlen)
            {
                usb_printk("uhci: tdbuf raw: %02x%02x%02x%02x%02x%02x%02x%02x\n",
                    ((uint8_t*)uhcitrans->tdbuf)[0], ((uint8_t*)uhcitrans->tdbuf)[1],
                    ((uint8_t*)uhcitrans->tdbuf)[2], ((uint8_t*)uhcitrans->tdbuf)[3],
                    ((uint8_t*)uhcitrans->tdbuf)[4], ((uint8_t*)uhcitrans->tdbuf)[5],
                    ((uint8_t*)uhcitrans->tdbuf)[6], ((uint8_t*)uhcitrans->tdbuf)[7]);
                A_memcpy(uhcitrans->inbuf, uhcitrans->tdbuf, uhcitrans->inlen);
                usb_printk("uhci: IN copy %dB: %02x%02x%02x%02x%02x%02x%02x%02x\n",
                    uhcitrans->inlen,
                    ((uint8_t*)uhcitrans->inbuf)[0], ((uint8_t*)uhcitrans->inbuf)[1],
                    ((uint8_t*)uhcitrans->inbuf)[2], ((uint8_t*)uhcitrans->inbuf)[3],
                    ((uint8_t*)uhcitrans->inbuf)[4], ((uint8_t*)uhcitrans->inbuf)[5],
                    ((uint8_t*)uhcitrans->inbuf)[6], ((uint8_t*)uhcitrans->inbuf)[7]);
            }
        }
    }
}


static struct uhci_td_t *uhci_alloc_td(struct usb_dev_t *usb, uintptr_t next, uintptr_t *tdphys)
{
    struct uhci_td_t *td;

    if(!(td = alloc_td(usb->priv, tdphys)))
    {
        return NULL;
    }

    // zero manually so we don't overwrite the 'active' bit
    td->next = 0;
    td->buf = 0;
    td->dword1 = 0;
    td->dword2 = 0;
    td->virtbuf = 0;

    if(next != (1 << 0))
    {
        // XXX: our calls never come here
        td->next = get_phys_addr(next & 0xFFFFFFF0) | (1 << 2);
    }
    else
    {
        td->next = (1 << 0);
    }

    td->errcnt = 3;
    td->lospeed = (usb->speed == USB_SPEED_LOW);
    td->active = 1;
    td->intoncomp = 0;

    return td;
}


static int uhci_alloc_tdbuf(struct usb_dev_t *usb, struct uhci_td_t *td, size_t len)
{
    if(len > UHCI_TDBUF_SIZE)
    {
        kpanic("uhci: TD buffer length > 1024\n");
    }

    if(len)
    {
        uintptr_t tdphys;

        if(!(td->virtbuf = (void *)alloc_tdbuf(usb->priv, &tdphys)))
        {
            return -ENOMEM;
        }

        td->buf = tdphys;
    }
    else
    {
        td->virtbuf = 0;
        td->buf = 0;
    }

    return 0;
}


static int uhci_td_setup(struct usb_transaction_t *transaction, 
                         struct uhci_transaction_t *ut, uintptr_t next)
{
    struct usb_dev_t *usb = transaction->dev;
    struct usb_transfer_t *transfer = (struct usb_transfer_t *)transaction->transfer;
    struct uhci_td_t *td;
    volatile struct usb_request_t *req;
    uintptr_t tdphys;

    if(!(td = uhci_alloc_td(usb, next, &tdphys)))
    {
        usb_printk("%s: failed to set up transaction descriptor\n", "uhci");
        return -ENOMEM;
    }

    td->pktid = UHCI_TD_PKTID_SETUP;
    td->toggle = transaction->toggle;
    td->devaddr = usb->num;
    td->endpoint = transfer->endpoint->addr;
    td->maxlen = 8 - 1;

    if(uhci_alloc_tdbuf(usb, td, sizeof(struct usb_request_t)) < 0)
    {
        usb_printk("%s: failed to alloc TD buffer\n", "uhci");
        free_td(td);
        return -ENOMEM;
    }

    req = td->virtbuf;
    req->type = transaction->type;
    req->req = transaction->req;
    req->hival = transaction->hival;
    req->loval = transaction->loval;
    req->index = transaction->index;
    req->len = transaction->len;

    ut->tdbuf = (void *)td->virtbuf;
    ut->tdvirt = td;
    ut->tdphys = (void *)tdphys;

    /* Pack software fields into hardware dword2 (SETUP TD) */
    td->dword2 = ((td->maxlen & 0x7FF) << 21) |
                 ((td->toggle & 1) << 19) |
                 ((td->pktid & 0xF) << 15) |
                 ((td->devaddr & 0x7F) << 8) |
                 ((td->endpoint & 0xF) << 4);
    td->dword1 = (td->active ? (1 << 23) : 0) |
                 ((td->errcnt & 3) << 27) |
                 (td->intoncomp ? (1 << 24) : 0);

    return 0;
}


static int uhci_td_setup_io(struct usb_transaction_t *transaction, 
                            struct uhci_transaction_t *ut, 
                            uintptr_t next, uint8_t pktid)
{
    struct usb_dev_t *usb = transaction->dev;
    struct usb_transfer_t *transfer = (struct usb_transfer_t *)transaction->transfer;
    struct uhci_td_t *td;
    uintptr_t tdphys;

    if(!(td = uhci_alloc_td(usb, next, &tdphys)))
    {
        usb_printk("%s: failed to set up transaction descriptor\n", "uhci");
        return -ENOMEM;
    }

    if(transaction->len)
    {
        td->maxlen = (transaction->len - 1) & 0x07FF;
    }
    else
    {
        td->maxlen = 0x07FF;
    }

    td->pktid = pktid;
    td->toggle = transaction->toggle;
    td->devaddr = usb->num;
    td->endpoint = transfer->endpoint->addr;

    if(uhci_alloc_tdbuf(usb, td, transaction->len) < 0)
    {
        usb_printk("%s: failed to alloc TD buffer\n", "uhci");
        free_td(td);
        return -ENOMEM;
    }

    ut->tdbuf = (void *)td->virtbuf;
    ut->tdvirt = td;
    ut->tdphys = (void *)tdphys;

    /* Pack software fields into hardware dword2 */
    td->dword2 = ((td->maxlen & 0x7FF) << 21) |
                 ((td->toggle & 1) << 19) |
                 ((td->pktid & 0xF) << 15) |
                 ((td->devaddr & 0x7F) << 8) |
                 ((td->endpoint & 0xF) << 4) |
                 (td->lospeed ? (1 << 3) : 0);
    /* dword1: active + error count */
    td->dword1 = (td->active ? (1 << 23) : 0) |
                 ((td->errcnt & 3) << 27) |
                 (td->intoncomp ? (1 << 24) : 0);

    return 0;
}


#define APPEND_TD(tranfer, ut)                                          \
    if(transfer && transfer->trans_tail) {                              \
        struct uhci_transaction_t *last = transfer->trans_tail->data;   \
        last->tdvirt->next = ((uintptr_t)ut->tdphys & 0xFFFFFFF0) | (1 << 2); \
    }


static int uhci_setup_transaction(struct usb_transaction_t *transaction)
{
    struct uhci_transaction_t *ut;
    struct usb_transfer_t *transfer = (struct usb_transfer_t *)transaction->transfer;

    if(!(ut = kmalloc(sizeof(struct uhci_transaction_t))))
    {
        usb_printk("%s: failed to set up transaction\n", "uhci");
        return -ENOMEM;
    }

    A_memset(ut, 0, sizeof(struct uhci_transaction_t));
    transaction->data = ut;
    ut->inbuf = NULL;
    ut->inlen = 0;

    if(uhci_td_setup(transaction, ut, 1) < 0)
    {
        kfree(ut);
        return -ENOMEM;
    }

    APPEND_TD(tranfer, ut);

    return 0;
}


static void uhci_in_transaction(struct usb_transaction_t *transaction)
{
    struct uhci_transaction_t *ut;
    struct usb_transfer_t *transfer = (struct usb_transfer_t *)transaction->transfer;

    if(!(ut = kmalloc(sizeof(struct uhci_transaction_t))))
    {
        usb_printk("%s: failed to set up IN transaction\n", "uhci");
        return;
    }

    A_memset(ut, 0, sizeof(struct uhci_transaction_t));
    transaction->data = ut;
    ut->inbuf = transaction->buf;
    ut->inlen = transaction->len;

    if(uhci_td_setup_io(transaction, ut, 1, UHCI_TD_PKTID_IN) < 0)
    {
        kfree(ut);
        return;
    }

    APPEND_TD(tranfer, ut);
}


static void uhci_out_transaction(struct usb_transaction_t *transaction)
{
    struct uhci_transaction_t *ut;
    struct usb_transfer_t *transfer = (struct usb_transfer_t *)transaction->transfer;

    if(!(ut = kmalloc(sizeof(struct uhci_transaction_t))))
    {
        usb_printk("%s: failed to set up OUT transaction\n", "uhci");
        return;
    }

    A_memset(ut, 0, sizeof(struct uhci_transaction_t));
    transaction->data = ut;
    ut->inbuf = NULL;
    ut->inlen = 0;

    if(uhci_td_setup_io(transaction, ut, 1, UHCI_TD_PKTID_OUT) < 0)
    {
        kfree(ut);
        return;
    }

    if(transaction->buf && transaction->len)
    {
        A_memcpy(ut->tdbuf, transaction->buf, transaction->len);
    }

    APPEND_TD(tranfer, ut);
}

#undef APPEND_TD


static void uhci_free_usb_transaction_data(volatile struct usb_transaction_t *usbtrans)
{
    struct usb_transfer_t *transfer = (struct usb_transfer_t *)usbtrans->transfer;
    struct uhci_dev_t *uhci = (struct uhci_dev_t *)transfer->dev->priv;
    struct uhci_transaction_t *ut = usbtrans->data;

    if(!ut)
    {
        return;
    }

    if(ut->tdvirt)
    {
        free_td(ut->tdvirt);
    }

    if(ut->tdbuf)
    {
        free_tdbuf(uhci, (uintptr_t)ut->tdbuf);
    }

    ut->tdvirt = 0;
    ut->tdphys = 0;
    ut->tdbuf = 0;

    kfree(ut);
    usbtrans->data = NULL;
}


static int uhci_setup_device(volatile struct uhci_dev_t *uhci, unsigned int port, uint8_t speed)
{
    struct usb_dev_t *usb;
    int res;

    if(!(usb = usb_create_dev(uhci->pci->unit, port, speed)))
    {
        usb_printk("%s: failed to create USB device\n", "uhci");
        return -ENOMEM;
    }

    usb->type = USB_TYPE_UHCI;
    usb->priv = (void *)uhci;
    usb->ops = &uhci_ops;

    // TODO: check the result of uhci_get_next_addr() != 0
    if((res = usb_setup_device(usb, uhci_get_next_addr((void *)uhci) /* port + 1 */)) < 0)
    {
        usb_printk("%s: failed to set up USB device\n", "uhci");
        uhci_free_addr((void *)uhci, usb->num);
        usb_destroy_dev(usb);
        return res;
    }

    uhci->ports[port].usb = usb;
    uhci->ports[port].flags |= UHCI_PORT_FLAG_CONNECTED;

    return 0;
}


int uhci_install(pci_compat_t *pci, struct pci_bar_t *bar)
{
    struct uhci_dev_t *uhci;
    char buf[8];
    //uintptr_t phys, virt;
    int timeout;
    int res = 0;
    uintptr_t base = bar->base;
    volatile uint16_t legacy_support, word;
    volatile unsigned int i;

    if(!(uhci = kmalloc(sizeof(struct uhci_dev_t))))
    {
        usb_printk("%s: insufficient memory to init device\n", "uchi");
        return -ENOMEM;
    }
    
    A_memset(uhci, 0, sizeof(struct uhci_dev_t));

    // entry 0 in the device address bitmap is always used so that we
    // don't use device address 0 by mistake, as it is reserved
    uhci->addr_bitmap[0] = 1;

    //pci->unit = last_unit++;

    uhci->pci = pci;
    uhci->iosize = bar->iosize;

    // check whether I/O is memory-mapped or normal I/O
    if(bar->iotype == PCI_IOTYPE_MMIO)
    {
        // MMIO
        base &= ~0xf;
        uhci->iobase = mmio_map(base, base + bar->iosize);
        uhci->mmio = 1;
    }
    else
    {
        // I/O
        uhci->iobase = base & ~0x3;
    }

    usb_printk("%s: base " _XPTR_ ", iobase " _XPTR_ ", iosize " _XPTR_ " (%s)\n", 
           "uhci", base, uhci->iobase, uhci->iosize, uhci->mmio ? "MMIO" : "I/O");

    if(!first_uhci)
    {
        first_uhci = uhci;
    }
    else
    {
        volatile struct uhci_dev_t *tmp = first_uhci;

        while(tmp->next)
        {
            tmp = tmp->next;
        }

        tmp->next = uhci;
    }

    // register IRQ handler
    ksprintf(buf, 8, "uhci%d", pci->unit);
    pci_register_irq_handler(pci, uhci_intr, buf);

    pci_enable_busmastering(pci);
    pci_enable_interrupts(pci);
    pci_enable_memoryspace(pci);

    // reset controller
    legacy_support = pci_config_read(pci, 0xC0);
    pcidev_outw(uhci, UHCI_REG_CMD, UHCI_CMD_GRST);

    // should wait 50 msec
    tick_delay(1);
    pcidev_outw(uhci, UHCI_REG_CMD, 0);

    // get root port count -- each port has 2-byte PORTSC register
    uhci->port_count = (uhci->iosize - UHCI_REG_PORTSC1) / 2;

    for(i = 2; i < uhci->port_count; i++)
    {
        word = pcidev_inw(uhci, UHCI_REG_PORTSC1 + (i * 2));

        if(!(word & UHCI_PORTSC_VALID) || (word == 0xFFFF))
        {
            uhci->port_count = i;
            break;
        }
    }

    usb_printk("%s: root ports %u\n", "uhci", uhci->port_count);

    word = pcidev_inw(uhci, UHCI_REG_CMD);

    if((legacy_support & ~(UHCI_PCI_LEGACY_PIRQ|UHCI_PCI_LEGACY_NOCHG|UHCI_PCI_LEGACY_STS)) ||
       (word & UHCI_CMD_RS) ||
       (word & UHCI_CMD_CF) ||
       !(word & UHCI_CMD_EGSM) ||
       (pcidev_inw(uhci, UHCI_REG_INT) & UHCI_INT_MASK))
    {
        // reset status bits
        pcidev_outw(uhci, UHCI_REG_STS, UHCI_STS_MASK);

        // should wait 1 frame (1 ms)
        tick_delay(1);

        // reset legacy support status
        pci_config_write(pci, 0xC0, UHCI_PCI_LEGACY_STS);

        // reset controller
        pcidev_outw(uhci, UHCI_REG_CMD, UHCI_CMD_HCRST);

        timeout = 50;

        while((pcidev_inw(uhci, UHCI_REG_CMD) & UHCI_CMD_HCRST) && timeout--)
        {
            tick_delay(1);
        }

        if(timeout <= 0)
        {
            usb_printk("uhci: device reset timeout (cmd 0x%x)\n", pcidev_inw(uhci, UHCI_REG_CMD));
            res = -ETIMEDOUT;
            goto err;
        }

        // turn off interrupts and the host controller
        pcidev_outw(uhci, UHCI_REG_INT, 0);
        pcidev_outw(uhci, UHCI_REG_CMD, 0);

        // turn off valid root ports
        for(i = 0; i < uhci->port_count; i++)
        {
            pcidev_outw(uhci, UHCI_REG_PORTSC1 + (i * 2), 0);
        }
    }

    // alloc memory for frame list
   	if(!(uhci->framelist_phys = pmmngr_alloc_block()))
    {
        res = -ENOMEM;
        goto err;
    }

    uhci->framelist = (struct uhci_framelist_t *)
                            mmio_map((physical_addr)uhci->framelist_phys, 
                                     (physical_addr)uhci->framelist_phys + PAGE_SIZE);

    // alloc memory for transfer descriptor pool
   	if(!(uhci->tdpool_phys = (uintptr_t)pmmngr_alloc_block()))
    {
        res = -ENOMEM;
        goto err;
    }

    uhci->tdpool = (struct uhci_td_t *)mmio_map(uhci->tdpool_phys, 
                                                uhci->tdpool_phys + PAGE_SIZE);

    A_memset(uhci->tdpool, 0, PAGE_SIZE);

    usb_printk("%s: tdpool virt " _XPTR_ ", phys " _XPTR_ "\n", 
                "uhci", uhci->tdpool, uhci->tdpool_phys);

    // alloc memory for transfer descriptor buffer pool
    if(!(uhci->tdbufpool_phys = 
            (uintptr_t)pmmngr_alloc_blocks(UHCI_TDBUF_POOL_SIZE / PAGE_SIZE)))
    {
        res = -ENOMEM;
        goto err;
    }

    uhci->tdbufpool = mmio_map(uhci->tdbufpool_phys, uhci->tdbufpool_phys + UHCI_TDBUF_POOL_SIZE);

    usb_printk("%s: tdbufpool virt " _XPTR_ ", phys " _XPTR_ "\n", 
                "uhci", uhci->tdbufpool, uhci->tdbufpool_phys);

    // alloc memory for queue header pool
   	if(!(uhci->qhpool_phys = pmmngr_alloc_block()))
    {
        res = -ENOMEM;
        goto err;
    }

    uhci->qhpool = (struct uhci_qh_t *)
                        mmio_map((physical_addr)uhci->qhpool_phys, 
                                 (physical_addr)uhci->qhpool_phys + PAGE_SIZE);
    init_qhpool(uhci, (uintptr_t)uhci->qhpool_phys);

    usb_printk("%s: qhpool virt " _XPTR_ ", phys " _XPTR_ "\n", 
                "uhci", uhci->qhpool, uhci->qhpool_phys);

    // prep queue heads
    uhci->qh_32ms = alloc_qh(uhci);
    uhci->qh_16ms = alloc_qh(uhci);
    uhci->qh_8ms = alloc_qh(uhci);
    uhci->qh_4ms = alloc_qh(uhci);
    uhci->qh_2ms = alloc_qh(uhci);
    uhci->qh_1ms = alloc_qh(uhci);

    uhci->qh_32ms->next = (uhci->qh_16ms->self_phys | (1 << 2) | (1 << 1));
    uhci->qh_32ms->transfer = (1 << 0);  // Terminate (no valid queue entries)
    uhci->qh_32ms->nextvirt = uhci->qh_16ms;

    uhci->qh_16ms->next = (uhci->qh_8ms->self_phys | (1 << 2) | (1 << 1));
    uhci->qh_16ms->transfer = (1 << 0);  // Terminate (no valid queue entries)
    uhci->qh_16ms->nextvirt = uhci->qh_8ms;

    uhci->qh_8ms->next = (uhci->qh_4ms->self_phys | (1 << 2) | (1 << 1));
    uhci->qh_8ms->transfer = (1 << 0);   // Terminate (no valid queue entries)
    uhci->qh_8ms->nextvirt = uhci->qh_4ms;

    uhci->qh_4ms->next = (uhci->qh_2ms->self_phys | (1 << 2) | (1 << 1));
    uhci->qh_4ms->transfer = (1 << 0);   // Terminate (no valid queue entries)
    uhci->qh_4ms->nextvirt = uhci->qh_2ms;

    uhci->qh_2ms->next = (uhci->qh_1ms->self_phys | (1 << 2) | (1 << 1));
    uhci->qh_2ms->transfer = (1 << 0);   // Terminate (no valid queue entries)
    uhci->qh_2ms->nextvirt = uhci->qh_1ms;

    uhci->qh_1ms->next = (1 << 0);       // Terminate (last pointer)
    uhci->qh_1ms->transfer = (1 << 0);   // Terminate (no valid queue entries)
    uhci->qh_1ms->nextvirt = NULL;

    for(i = 0; i < 1024; i++)
    {
        // Reset all frame list entries to point to the Queue Head.
        // Set bit 1 (QH) and clear bit 0 (pointer is valid)
        //uhci->framelist->fr[i] = phys | (1 << 1);

        if((i % 32) == 0)
        {
            uhci->framelist->fr[i] = (uhci->qh_32ms->self_phys | (1 << 2) | (1 << 1));
        }
        else if((i % 16) == 0)
        {
            uhci->framelist->fr[i] = (uhci->qh_16ms->self_phys | (1 << 2) | (1 << 1));
        }
        else if((i % 8) == 0)
        {
            uhci->framelist->fr[i] = (uhci->qh_8ms->self_phys | (1 << 2) | (1 << 1));
        }
        else if((i % 4) == 0)
        {
            uhci->framelist->fr[i] = (uhci->qh_4ms->self_phys | (1 << 2) | (1 << 1));
        }
        else if((i % 2) == 0)
        {
            uhci->framelist->fr[i] = (uhci->qh_2ms->self_phys | (1 << 2) | (1 << 1));
        }
        else
        {
            uhci->framelist->fr[i] = (uhci->qh_1ms->self_phys | (1 << 2) | (1 << 2) | (1 << 1));
        }
    }

    // SOF cycle time = 11936 + UHCI_REG_SOFMOD
    // For 1ms frames, we need to write the value 0x40 (64 decimal), which
    // gives a cycle time of 12000, producing a 1ms frame period
    pcidev_outb(uhci, UHCI_REG_SOFMOD, 0x40);

    // set framelist addr
    pcidev_outl(uhci, UHCI_REG_FRBASEADDR, (uintptr_t)uhci->framelist_phys);

    // start at frame 0
    pcidev_outw(uhci, UHCI_REG_FRNUM, 0);

    // set PIRQ
    pcidev_outw(uhci, 0xC0, UHCI_PCI_LEGACY_PIRQ);

    // start controller with 64-byte max packets
    pcidev_outw(uhci, UHCI_REG_CMD, UHCI_CMD_RS|UHCI_CMD_CF|UHCI_CMD_MAXP);

    // turn interrupts on
    pcidev_outw(uhci, UHCI_REG_INT, 0 /* UHCI_INT_MASK */);

    // turn on valid root ports
    for(i = 0; i < uhci->port_count; i++)
    {
        pcidev_outw(uhci, UHCI_REG_PORTSC1 + (i * 2), UHCI_PORTSC_CS_CHG);
    }

    pcidev_outw(uhci, UHCI_REG_CMD, UHCI_CMD_RS|UHCI_CMD_CF|UHCI_CMD_MAXP|UHCI_CMD_FGR);
    tick_delay(1);
    pcidev_outw(uhci, UHCI_REG_CMD, UHCI_CMD_RS|UHCI_CMD_CF|UHCI_CMD_MAXP);
    tick_delay(1);

    usb_printk("uhci: PORTSC1 0x%x, PORTSC2 0x%x\n", 
            pcidev_inw(uhci, UHCI_REG_PORTSC1),
            pcidev_inw(uhci, UHCI_REG_PORTSC2));

    if((pcidev_inw(uhci, UHCI_REG_CMD) & UHCI_CMD_RS))
    {
        uhci->flags |= UHCI_FLAG_RUN;
    }

    if((pcidev_inw(uhci, UHCI_REG_STS) & UHCI_STS_HCHALTED))
    {
        usb_printk("uhci: controller halted\n");
        usb_printk("uhci: flags 0x%x, frame 0x%x\n", uhci->flags, pcidev_inw(uhci, UHCI_REG_FRNUM));
    }
    else
    {
        usb_printk("%s: controller running, enabling ports\n", "uhci");

        if((res = uhci_enable_ports(uhci)) < 0)
        {
            goto err;
        }
    }

    usb_printk("%s: setup done\n", "uhci");

    return 0;


err:

    if(uhci->mmio)
    {
        vmmngr_free_pages(uhci->iobase, uhci->iosize);
        uhci->iobase = 0;
    }

    if(uhci->framelist_phys)
    {
        vmmngr_free_pages((virtual_addr)uhci->framelist, PAGE_SIZE);
        uhci->framelist = 0;
        uhci->framelist_phys = 0;
    }

    if(uhci->tdpool_phys)
    {
        vmmngr_free_pages((virtual_addr)uhci->tdpool, PAGE_SIZE);
        uhci->tdpool_phys = 0;
        uhci->tdpool = 0;
    }

    if(uhci->tdbufpool_phys)
    {
        vmmngr_free_pages(uhci->tdbufpool, UHCI_TDBUF_POOL_SIZE);
        uhci->tdbufpool_phys = 0;
        uhci->tdbufpool = 0;
    }

    if(uhci->qhpool_phys)
    {
        vmmngr_free_pages((virtual_addr)uhci->qhpool, PAGE_SIZE);
        uhci->qhpool_phys = 0;
        uhci->qhpool = 0;
    }

#if 0

#define FREE_QH(m) \
    if(uhci->qh_ ## m ## ms) { \
        vmmngr_free_page(get_page_entry((void *)uhci->qh_ ## m ## ms)); \
        uhci->qh_ ## m ## ms = 0; \
    }

    FREE_QH(32);
    FREE_QH(16);
    FREE_QH(8);
    FREE_QH(4);
    FREE_QH(2);
    FREE_QH(1);

#undef FREE_QH

#endif

    return res;
}


void uhci_poll(void)
{
    volatile struct uhci_dev_t *uhci = first_uhci;
    struct usb_dev_t *usb;
    volatile unsigned int i;
    uint16_t word;

    //usb_printk("uhci_poll:\n");

    while(uhci)
    {
        if(!(uhci->flags & UHCI_FLAG_RUN) ||
           !(uhci->flags & UHCI_FLAG_PORTENABLED))
        {
            uhci = uhci->next;
            continue;
        }

        // for each UHCI port
        for(i = 0; i < uhci->port_count; i++)
        {
            // check port status
            word = pcidev_inw(uhci, UHCI_REG_PORTSC1 + (i * 2));

            // and if there is a device change
            if(word & UHCI_PORTSC_CS_CHG)
            {
                // acknowledge that
                pcidev_outw(uhci, UHCI_REG_PORTSC1 + (i * 2), UHCI_PORTSC_CS_CHG);

                if(word & UHCI_PORTSC_LOSPEED)
                {
                    uhci->ports[i].flags |= UHCI_PORT_FLAG_LOSPEED;
                }
                else
                {
                    uhci->ports[i].flags &= ~UHCI_PORT_FLAG_LOSPEED;
                }

                // and if there is a new device attached, set it up
                if(word & UHCI_PORTSC_CS)
                {
                    if(!(uhci->ports[i].flags & UHCI_PORT_FLAG_CONNECTED))
                    {
                        usb_printk("uhci: device connected to port %d\n", i);
                        uhci_reset_port(uhci, i);
                        uhci_setup_device(uhci, i, 
                                          (word & UHCI_PORTSC_LOSPEED) ?
                                            USB_SPEED_LOW : USB_SPEED_FULL);
                    }
                }
                // and if a device was removed, delete it
                else
                {
                    usb_printk("uhci: device removed from port %d\n", i);
                    usb = uhci->ports[i].usb;

                    if(usb && usb->type == USB_TYPE_UHCI)
                    {
                        uhci->ports[i].usb = NULL;
                        uhci_free_addr((void *)uhci, usb->num);
                        usb_destroy_dev(usb);
                        uhci->ports[i].flags &= ~UHCI_PORT_FLAG_CONNECTED;
                    }
                }
            }
        }

        uhci = uhci->next;
    }
}


/*
 * IRQ callback function.
 */
int uhci_intr(struct regs *r, void *arg)
{
    UNUSED(r);

    //KDEBUG("uhci_intr:\n");
    //screen_refresh(NULL);
    //__asm__ __volatile__("xchg %%bx, %%bx"::);

    pci_compat_t *pci = arg;
    volatile struct uhci_dev_t *uhci = first_uhci;
    volatile uint16_t word;

    while(uhci)
    {
        if(uhci->pci == pci)
        {
            break;
        }

        uhci = uhci->next;
    }

    if(!uhci)
    {
        usb_printk("uhci_intr: not from here\n");
        return 0;
    }

    word = pcidev_inw(uhci, UHCI_REG_STS);

    if(!word)
    {
        // IRQ not from this device
        //usb_printk("uhci_intr: word 0\n");
        return 0;
    }

    //usb_printk("uhci_intr: word 0x%x ", word);

    if(word & UHCI_STS_INT)
    {
        KDEBUG("uhci_intr: transaction complete for frame %u\n", pcidev_inw(uhci, UHCI_REG_FRNUM));
        pcidev_outw(uhci, UHCI_REG_STS, UHCI_STS_INT);

        /*
        if(waiting_task)
        {
            unblock_task(waiting_task);
        }
        */
    }

    if(word & UHCI_STS_RESDET)
    {
        KDEBUG("uhci_intr: resume detect\n");
        pcidev_outw(uhci, UHCI_REG_STS, UHCI_STS_RESDET);
    }

    if(word & UHCI_STS_HCHALTED)
    {
        KDEBUG("uhci_intr: host controller halted\n");
        pcidev_outw(uhci, UHCI_REG_STS, UHCI_STS_HCHALTED);
    }

    if(word & UHCI_STS_PROCERR)
    {
        KDEBUG("uhci_intr: host controller process error\n");
        pcidev_outw(uhci, UHCI_REG_STS, UHCI_STS_PROCERR);
    }

    if(word & UHCI_STS_ERR)
    {
        KDEBUG("uhci_intr: USB error\n");
        pcidev_outw(uhci, UHCI_REG_STS, UHCI_STS_ERR);
    }

    if(word & UHCI_STS_HOSTERR)
    {
        KDEBUG("uhci_intr: host system error\n");
        pcidev_outw(uhci, UHCI_REG_STS, UHCI_STS_HOSTERR);
    }

    pic_send_eoi(uhci->pci->irq[0]);

    return 1;
}


static struct usb_ops_t uhci_ops =
{
    .setup_transfer = uhci_setup_transfer,
    .schedule_transfer = uhci_schedule_transfer,
    .wait_transfer = uhci_wait_transfer,
    .poll_transfer = uhci_poll_transfer,
    .delete_transfer = uhci_delete_transfer,
    .setup_transaction = uhci_setup_transaction,
    .in_transaction = uhci_in_transaction,
    .out_transaction = uhci_out_transaction,
    .free_transaction_data = uhci_free_usb_transaction_data,
    .get_next_addr = uhci_get_next_addr,
    .free_addr = uhci_free_addr,
};

