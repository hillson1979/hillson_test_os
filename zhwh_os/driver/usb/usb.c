/* 
/**
 *    Copyright 2025, 2026 (c)
 * 
 *    file: usb.c
 *

/**
 *  \file usb.c
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

#include <string.h>
#include <kernel/errno.h>
#include <kernel/hillsonos.h>
#include <kernel/task.h>
#include <kernel/dev.h>
#include <kernel/pciio.h>
#include <kernel/usb.h>
#include <kernel/usb_uhci.h>
#include <kernel/usb_ohci.h>
#include <kernel/usb_ehci.h>
#include <kernel/usb_hid.h>
#include <kernel/usb_hub.h>
#include <kernel/usb_xhci.h>
#include <mm/kheap.h>

extern void kernel_usb_status_line(const char *stage, const char *detail);
extern void kernel_usb_error_line(const char *stage, const char *detail);
extern int snprintf(char *str, unsigned int size, const char *fmt, ...);


static volatile int last_bus = 0;
pci_compat_t *usbbus[MAX_USB_BUSES + 1];

static struct usb_transfer_t inttransfer_head;
static kernel_mutex_t inttransfer_list_lock;


static void print_device(struct usb_dev_t *dev)
{
    usb_printk("usb: USB v%x.%x ", BYTE2(dev->spec), BYTE1(dev->spec));

    if(dev->spec != 0x0100 && dev->spec != 0x0110 &&
       dev->spec != 0x0200 && dev->spec != 0x0300)
    {
        usb_printk("(INVALID) ");
    }

    if(dev->class == 0x09)
    {
        if(dev->protocol == 0)
        {
            usb_printk("- full speed USB hub");
        }
        else if(dev->protocol == 1)
        {
            usb_printk("- Hi-speed USB hub with single TT");
        }
        else if(dev->protocol == 2)
        {
            usb_printk("- Hi-speed USB hub with multiple TTs");
        }
    }

    usb_printk("\n     endpoint 0:    mps %u bytes\n", dev->endpoints->mps);
    usb_printk(  "     class:         0x%x\n", dev->class);
    usb_printk(  "     subclass:      0x%x\n", dev->subclass);
    usb_printk(  "     vendor:        0x%x\n", dev->vendor);
    usb_printk(  "     product:       0x%x\n", dev->product);
    usb_printk(  "     release:       %u.%u\n", BYTE2(dev->release), BYTE1(dev->release));
    usb_printk(  "     manufacturer:  0x%x\n", dev->manufacturerid);
    usb_printk(  "     productid:     0x%x\n", dev->productid);
    usb_printk(  "     serial:        0x%x\n", dev->serialid);
    usb_printk(  "     configs:       %u\n", dev->configs);
}


#ifdef __DEBUG

static void print_config_descriptor(struct usb_config_descriptor_t *desc)
{
    if(desc->len)
    {
        usb_printk("usb: config descriptor:\n");
        usb_printk("     len:             %u\n", desc->len);
        usb_printk("     desc type:       %u\n", desc->type);
        usb_printk("     total len:       %u\n", desc->totlen);
        usb_printk("     interfaces:      %u\n", desc->interfaces);
        usb_printk("     config id:       0x%x\n", desc->configval);
        usb_printk("     config name id:  0x%x\n", desc->config);
        usb_printk("     attribs:         0x%x\n", desc->attribs);
        usb_printk("     max power (mA):  %u\n", desc->maxpower);
    }
}


static void print_endpoint_descriptor(struct usb_endpoint_descriptor_t *desc)
{
    if(desc->len)
    {
        usb_printk("usb: endpoint descriptor:\n");
        usb_printk("     len:             %u\n", desc->len);
        usb_printk("     desc type:       %u\n", desc->type);
        usb_printk("     endpoint %u:     %s\n", 
                (desc->addr & 0xF),
                (desc->addr & 0x80) ? "IN" : "OUT");
        usb_printk("     attribs:         0x%x%s\n", 
                desc->attribs,
                (desc->attribs == 2) ? "(bulk data)" : " ");
        usb_printk("     mps:             %u bytes\n", desc->mps);
        usb_printk("     interval:        %u\n", desc->interval);
    }
}


static void print_interface_descriptor(struct usb_interface_descriptor_t *desc)
{
    if(desc->len)
    {
        usb_printk("usb: interface descriptor:\n");
        usb_printk("     len:             %u\n", desc->len);
        usb_printk("     desc type:       %u\n", desc->type);
        usb_printk("     interface num:   %u\n", desc->interfacenum);
        usb_printk("     endpoints:       %u\n", desc->endpoints);
        usb_printk("     altsetting:      %u\n", desc->altsetting);
        usb_printk("     class:           0x%x\n", desc->class);
        usb_printk("     subclass:        0x%x\n", desc->subclass);
        usb_printk("     protocol:        0x%x\n", desc->protocol);
        usb_printk("     interface:       0x%x\n", desc->interface);
    }
}


static void print_hid_descriptor(struct usb_hid_descriptor_t *desc)
{
    int i;
    uint16_t desclen;
    uint8_t *descriptors;

    if(desc->len)
    {
        usb_printk("usb: Human Interface Device (HID) descriptor:\n");
        usb_printk("     len:               %u\n", desc->len);
        usb_printk("     desc type:         %u\n", desc->type);
        usb_printk("     class spec:        %u.%u\n", BYTE2(desc->hid_bcd), BYTE1(desc->hid_bcd));
        usb_printk("     country code:      %u\n", desc->country_code);
        usb_printk("     descriptor count:  %u\n", desc->descriptor_count);

        descriptors = (uint8_t *)desc->descriptors;

        for(i = 0; i < desc->descriptor_count; i++)
        {
            desclen = descriptors[1] | (descriptors[2] << 8);
            usb_printk("       [%d] type 0x%x, len %u\n", i, descriptors[0], desclen);
            descriptors += 3;
        }
    }
}


static void print_string_descriptor(struct usb_string_descriptor_t *desc)
{
    int i;

    if(desc->len)
    {
        usb_printk("usb: string descriptor:\n");
        usb_printk("     len:             %u\n", desc->len);
        usb_printk("     desc type:       %u\n", desc->type);
        usb_printk("     languages:       ");

        for(i = 0; i < 10; i++)
        {
            if(desc->langid[i] >= 0x0400 && desc->langid[i] <= 0x0465)
            {
                usb_printk("0x%x ", desc->langid[i]);
            }
        }

        usb_printk("\n");
    }
}


static void print_unistring_descriptor(struct usb_dev_t *dev,
                                       struct usb_unistring_descriptor_t *desc,
                                       uint32_t strindex)
{
    int i;
    char ascii[32] = { 0, };

    if(desc->len)
    {
        usb_printk("usb: Unicode string descriptor:\n");
        usb_printk("     len:             %u\n", desc->len);
        usb_printk("     desc type:       %u\n", desc->type);

        for(i = 0; i < MIN(64, (desc->len - 2)); i += 2)
        {
            if(desc->wch[i])
            {
                ascii[i / 2] = desc->wch[i];
            }
        }

        ascii[31] = '\0';

        if(strindex == 2)
        {
            A_memcpy(dev->product_name, ascii, 32);
            usb_printk("     product name:    %s\n", dev->product_name);
        }
        else if(strindex == 3)
        {
            A_memcpy(dev->serial, ascii, 32);
            usb_printk("     serial:          %s\n", dev->serial);
        }
        else
        {
            usb_printk("     strindex:        %u\n", strindex);
        }
    }
}

#endif      /* __DEBUG */


struct usb_dev_t *usb_create_dev(uint8_t bus, unsigned int port, uint8_t speed)
{
    struct usb_dev_t *dev;

    if(!(dev = kmalloc(sizeof(struct usb_dev_t))))
    {
        return NULL;
    }

    A_memset(dev, 0, sizeof(struct usb_dev_t));

    dev->speed = speed;
    dev->port = port;
    dev->bus = bus;

    // alloc the first endpoint
    if(!(dev->endpoints = kmalloc(sizeof(struct usb_endpoint_t))))
    {
        kfree(dev);
        return NULL;
    }

    A_memset(dev->endpoints, 0, sizeof(struct usb_endpoint_t));

    if(speed == USB_SPEED_LOW || speed == USB_SPEED_FULL)
    {
        dev->endpoints->mps = 8;
    }
    else if(speed == USB_SPEED_HIGH)
    {
        dev->endpoints->mps = 64;
    }
    else if(speed == USB_SPEED_SUPER)
    {
        dev->endpoints->mps = 512;
    }

    dev->endpoints->direction = USB_ENDPOINT_BI;
    dev->endpoints->type = USB_ENDPOINT_CONTROL;

    return dev;
}


void usb_destroy_dev(struct usb_dev_t *dev)
{
    volatile struct usb_interface_t *iface, *niface;
    volatile struct usb_endpoint_t *endpoint, *nendpoint;

    if(!dev)
    {
        return;
    }

    // remove USB hub
    if(dev->class == 0x09)
    {
        usb_hub_remove_dev(dev);
    }

    // remove the interfaces
    for(iface = dev->interfaces; iface != NULL; iface = iface->next)
    {
        // if this is a mass storage device, remove it from the device list and
        // delete it from /dev tree
        if(iface->desc.class == 0x08 && iface->desc.subclass == 0x06)
        {
            usb_msd_remove((struct usb_interface_t *)iface);
        }
        // remove HID
        else if(iface->desc.class == 0x03)
        {
            usb_hid_remove((struct usb_interface_t *)iface);
        }
    }

    // free interfaces
    for(iface = dev->interfaces; iface != NULL; )
    {
        niface = iface->next;
        kfree((void *)iface);
        iface = niface;
    }

    // remove endpoint devices from /dev
    for(endpoint = dev->endpoints; endpoint != NULL; endpoint = endpoint->next)
    {
        remove_dev_node(USB_MAKE_DEVID(dev->bus, dev->num, endpoint->addr));
    }

    // free endpoints
    for(endpoint = dev->endpoints; endpoint != NULL; )
    {
        nendpoint = endpoint->next;
        kfree((void *)endpoint);
        endpoint = nendpoint;
    }

    dev->interfaces = NULL;
    dev->endpoints = NULL;

    kfree(dev);
}


void remove_interrupt_transfer(struct usb_transfer_t *transfer)
{
    volatile struct usb_transfer_t *t, *prev = &inttransfer_head;

    kernel_mutex_lock(&inttransfer_list_lock);

    for(t = inttransfer_head.next_inttransfer; t != NULL; t = t->next_inttransfer)
    {
        if(t == transfer)
        {
            prev->next_inttransfer = t->next_inttransfer;
            break;
        }

        prev = t;
    }

    kernel_mutex_unlock(&inttransfer_list_lock);
}


void usb_schedule_inttransfer(struct usb_dev_t *usb, struct usb_endpoint_t *endpoint,
                              struct usb_transfer_t *transfer, 
                              void *buf, size_t bufsz,
                              void (*callback)(void *), void *callback_arg,
                              uint8_t freq)
{
    usb_setup_transfer(usb, endpoint, transfer, USB_TRANSFER_INTERRUPT);

    transfer->callback = callback;
    transfer->callback_arg = callback_arg;
    transfer->freq = freq;

    usb_in_transaction(transfer, 0, buf, bufsz);
    usb_schedule_transfer(transfer);

    kernel_mutex_lock(&inttransfer_list_lock);
    transfer->next_inttransfer = inttransfer_head.next_inttransfer;
    inttransfer_head.next_inttransfer = transfer;
    kernel_mutex_unlock(&inttransfer_list_lock);
}


void usb_setup_transfer(struct usb_dev_t *dev, struct usb_endpoint_t *endpoint,
                        struct usb_transfer_t *transfer, uint8_t type)
{
    A_memset(transfer, 0, sizeof(struct usb_transfer_t));
    transfer->dev = dev;
    transfer->endpoint = endpoint;
    transfer->type = type;
    transfer->pktsz = endpoint->mps;

    if(!dev->ops || !dev->ops->setup_transfer)
    {
        usb_printk("usb: device with NULL setup_transfer() function\n");
    }
    else
    {
        dev->ops->setup_transfer(transfer);
    }
}


void usb_schedule_transfer(struct usb_transfer_t *transfer)
{
    if(!transfer->dev || !transfer->dev->ops || !transfer->dev->ops->schedule_transfer)
    {
        usb_printk("usb: device with NULL schedule_transfer() function\n");
    }
    else
    {
        transfer->dev->ops->schedule_transfer(transfer);
    }
}


int usb_poll_transfer(struct usb_transfer_t *transfer)
{
    static int cnt = 0;
    if (++cnt <= 3)
        usb_printk("usb_poll_transfer: dev=%p ops=%p poll=%p\n",
                   transfer->dev,
                   transfer->dev ? transfer->dev->ops : NULL,
                   (transfer->dev && transfer->dev->ops) ?
                       (void*)transfer->dev->ops->poll_transfer : NULL);

    /* Bypass vtable for XHCI — linker sometimes corrupts the function pointer */
    if (transfer->dev && transfer->dev->type == USB_TYPE_XHCI) {
        extern int xhci_poll_transfer(struct usb_transfer_t *transfer);
        if (cnt <= 3)
            usb_printk("usb_poll_transfer: XHCI bypass, calling directly\n");
        return xhci_poll_transfer(transfer);
    }

    if(!transfer->dev || !transfer->dev->ops || !transfer->dev->ops->poll_transfer)
    {
        usb_printk("usb: device with NULL poll_transfer() function\n");
        return 0;
    }
    else
    {
        int ret = transfer->dev->ops->poll_transfer(transfer);
        if (cnt <= 3)
            usb_printk("usb_poll_transfer: returned %d\n", ret);
        return ret;
    }
}


void usb_wait_transfer(struct usb_transfer_t *transfer)
{
    if(!transfer->dev || !transfer->dev->ops || !transfer->dev->ops->wait_transfer)
    {
        usb_printk("usb: device with NULL wait_transfer() function\n");
    }
    else
    {
        transfer->dev->ops->wait_transfer(transfer);
    }
}


void usb_delete_transfer(struct usb_transfer_t *transfer)
{
    struct usb_dev_t *dev = transfer->dev;
    volatile struct usb_transaction_t *trans, *next;

    if(!dev || !dev->ops || !dev->ops->delete_transfer)
    {
        usb_printk("usb: device with NULL delete_transfer() function\n");
    }
    else
    {
        dev->ops->delete_transfer(transfer);
    }

    if(transfer->type == USB_TRANSFER_INTERRUPT)
    {
        remove_interrupt_transfer(transfer);
    }

    for(trans = transfer->trans_head; trans != NULL; )
    {
        next = trans->next;

        if(dev && trans->data)
        {
            dev->ops->free_transaction_data(trans);
            trans->data = NULL;
        }

        kfree((void *)trans);
        trans = next;
    }

    transfer->trans_head = NULL;
    transfer->trans_tail = NULL;
}


#define APPEND_TRANSACTION(transfer, transaction)   \
    if(transfer->trans_head == NULL) {              \
        transfer->trans_head = transaction;         \
        transfer->trans_tail = transaction;         \
    } else {                                        \
        transfer->trans_tail->next = transaction;   \
        transfer->trans_tail = transaction;         \
    }


uint8_t usb_setup_transaction(struct usb_transfer_t *transfer, 
                              uint8_t type, uint8_t req,
                              uint8_t hival, uint8_t loval,
                              uint16_t index, uint16_t len)
{
    struct usb_transaction_t *transaction;
    uint8_t res = 0;

    if(!(transaction = kmalloc(sizeof(struct usb_transaction_t))))
    {
        return 0;
    }

    A_memset(transaction, 0, sizeof(struct usb_transaction_t));

    transaction->dev = transfer->dev;
    transaction->type = USB_TRANS_SETUP;
    transaction->transfer = transfer;
    transaction->type = type;
    transaction->req = req;
    transaction->hival = hival;
    transaction->loval = loval;
    transaction->index = index;
    transaction->len = len;
    transaction->toggle = 0;
    transfer->endpoint->toggle = 1; /* SETUP=DATA0, next phase=DATA1 */

    if(!transfer->dev || !transfer->dev->ops || !transfer->dev->ops->setup_transaction)
    {
        usb_printk("usb: device with NULL setup_transaction() function\n");
    }
    else
    {
        if(transfer->dev->ops->setup_transaction(transaction) == 0)
        {
            res = loval;
        }

        APPEND_TRANSACTION(transfer, transaction);
        transfer->endpoint->toggle = 1;
    }

    KDEBUG("usb_setup_transaction: res %d\n", res);

    return res;
}


void usb_in_transaction(struct usb_transfer_t *transfer, 
                        int ctrl_handshake, void *buf, size_t len)
{
    struct usb_transaction_t *transaction;
    size_t minlen;
    int remaining;

    /* xHCI transfer rings describe buffers, not individual USB packets.
     * Queue one Data Stage TRB for the whole control payload and let the
     * controller split it according to EP0 MPS on the wire. */
    if(transfer->type == USB_TRANSFER_CTRL && transfer->dev &&
       transfer->dev->type == USB_TYPE_XHCI)
    {
        minlen = len;
        len = 0;
        remaining = 0;
    }
    else if(transfer->type == USB_TRANSFER_ISOCHRONOUS)
    {
        minlen = len;
        len = 0;
        remaining = 0;
    }
    else
    {
        minlen = MIN(transfer->pktsz, len);
        len -= minlen;
        remaining = len / transfer->pktsz;

        if(len % transfer->pktsz)
        {
            remaining++;
        }
    }

    if(!(transaction = kmalloc(sizeof(struct usb_transaction_t))))
    {
        return;
    }

    A_memset(transaction, 0, sizeof(struct usb_transaction_t));

    transaction->dev = transfer->dev;
    transaction->type = USB_TRANS_IN;
    transaction->transfer = transfer;
    transaction->buf = buf;
    transaction->len = minlen;

    if(ctrl_handshake)
    {
        transfer->endpoint->toggle = 1;
    }

    transaction->toggle = transfer->endpoint->toggle;

    if(!transfer->dev || !transfer->dev->ops || !transfer->dev->ops->in_transaction)
    {
        usb_printk("usb: device with NULL in_transaction() function\n");
    }
    else
    {
        transfer->dev->ops->in_transaction(transaction);
        APPEND_TRANSACTION(transfer, transaction);
        transfer->endpoint->toggle = !(transfer->endpoint->toggle);

        if(remaining)
        {
            //usb_printk("usb_in_transaction: remaining %d, pktsz %d, len %ld, minlen %ld\n", remaining, transfer->pktsz, len, minlen);
            usb_in_transaction(transfer, transfer->endpoint->toggle,
                                            (char *)buf + minlen, len);
        }
    }
}


void usb_out_transaction(struct usb_transfer_t *transfer, 
                         int ctrl_handshake, void *buf, size_t len)
{
    struct usb_dev_t *dev = transfer->dev;
    struct usb_transaction_t *transaction;
    size_t minlen;
    int remaining;

    if(transfer->type == USB_TRANSFER_CTRL && dev &&
       dev->type == USB_TYPE_XHCI)
    {
        minlen = len;
        len = 0;
        remaining = 0;
    }
    else
    {
        minlen = MIN(transfer->pktsz, len);
        len -= minlen;
        remaining = len / transfer->pktsz;

        if(len % transfer->pktsz)
            remaining++;
    }

    if(!(transaction = kmalloc(sizeof(struct usb_transaction_t))))
    {
        return;
    }

    A_memset(transaction, 0, sizeof(struct usb_transaction_t));

    transaction->dev = transfer->dev;
    transaction->type = USB_TRANS_OUT;
    transaction->transfer = transfer;
    transaction->buf = buf;
    transaction->len = minlen;

    if(ctrl_handshake)
    {
        transfer->endpoint->toggle = 1;
    }

    transaction->toggle = transfer->endpoint->toggle;

    if(!dev || !dev->ops || !dev->ops->out_transaction)
    {
        usb_printk("usb: device with NULL out_transaction() function\n");
    }
    else
    {
        dev->ops->out_transaction(transaction);
        APPEND_TRANSACTION(transfer, transaction);
        transfer->endpoint->toggle = !(transfer->endpoint->toggle);

        if(remaining)
        {
            //usb_printk("usb_out_transaction: remaining %d\n", remaining);
            usb_out_transaction(transfer, transfer->endpoint->toggle,
                                            (char *)buf + minlen, len);
        }
    }
}

#undef APPEND_TRANSACTION


int usb_ctrl_in(struct usb_dev_t *dev, void *buf,
                uint8_t type, uint8_t req,
                uint8_t hival, uint8_t loval,
                uint16_t index, uint16_t len)
{
    struct usb_transfer_t transfer;

    usb_setup_transfer(dev, dev->endpoints, &transfer, USB_TRANSFER_CTRL);
    usb_setup_transaction(&transfer, type, req, hival, loval, index, len);
    usb_in_transaction(&transfer, 0, buf, len);
    usb_out_transaction(&transfer, 1, 0, 0);
    usb_schedule_transfer(&transfer);
    usb_wait_transfer(&transfer);
    usb_delete_transfer(&transfer);

    return transfer.success;
}


int usb_ctrl_out(struct usb_dev_t *dev, void *buf,
                 uint8_t type, uint8_t req,
                 uint8_t hival, uint8_t loval,
                 uint16_t index, uint16_t len)
{
    struct usb_transfer_t transfer;

    usb_setup_transfer(dev, dev->endpoints, &transfer, USB_TRANSFER_CTRL);
    usb_setup_transaction(&transfer, type, req, hival, loval, index, len);
    usb_out_transaction(&transfer, 0, buf, len);
    usb_in_transaction(&transfer, 1, 0, 0);
    usb_schedule_transfer(&transfer);
    usb_wait_transfer(&transfer);
    usb_delete_transfer(&transfer);

    return transfer.success;
}


int usb_ctrl_set(struct usb_dev_t *dev,
                 uint8_t type, uint8_t req,
                 uint8_t hival, uint8_t loval, uint16_t index)
{
    struct usb_transfer_t transfer;

    usb_setup_transfer(dev, dev->endpoints, &transfer, USB_TRANSFER_CTRL);
    usb_setup_transaction(&transfer, type, req, hival, loval, index, 0);
    usb_in_transaction(&transfer, 1, 0, 0);
    usb_schedule_transfer(&transfer);
    usb_wait_transfer(&transfer);
    usb_delete_transfer(&transfer);

    return transfer.success;
}


int usb_get_device_descriptor(struct usb_dev_t *dev, uint8_t len)
{
    struct usb_device_descriptor_t desc;
    int res = -EIO;

    if(usb_ctrl_in(dev, &desc, 0x80, 6, 1, 0, 0, len))
    {
        {
            char raw[96];
            uint8_t *b = (uint8_t *)&desc;
            unsigned int raw_len = len;
            unsigned int pos = 0;
            unsigned int i;
            pos += snprintf(raw + pos, sizeof(raw) - pos,
                            "raw(%u):", raw_len);
            for (i = 0; i < raw_len && i < sizeof(desc) && pos + 4 < sizeof(raw); i++)
                pos += snprintf(raw + pos, sizeof(raw) - pos,
                                " %02x", b[i]);
            kernel_usb_status_line("usb desc raw", raw);
            if (b[0] != 18 || b[1] != 1) {
                kernel_usb_error_line("usb desc invalid", "bad device descriptor header");
                return -EIO;
            }
        }
        uint16_t real_mps;

        /* Decode bMaxPacketSize0:
         *   USB 1.x / 2.0 (bcdUSB < 0x0300): literal value (8, 16, 32, 64)
         *   USB 3.0  (bcdUSB >= 0x0300): exponent — real MPS = 2^exp (9 => 512)
         */
        if (desc.bcd_usb >= 0x0300 && desc.mps >= 7 && desc.mps <= 12) {
            real_mps = 1u << desc.mps;   /* 9 -> 512, 10 -> 1024, etc. */
        } else {
            real_mps = desc.mps;          /* pass through for USB 1/2 */
        }

        if (real_mps == 0 || real_mps > 1024) {
            usb_printk("usb: invalid descriptor MPS=%d (raw=%d), retrying\n",
                real_mps, desc.mps);
            return -EIO;
        }

        dev->spec = desc.bcd_usb;
        dev->class = desc.class;
        dev->subclass = desc.subclass;
        dev->protocol = desc.protocol;

        if(dev->endpoints->mps != real_mps)
        {
            dev->endpoints->mps = real_mps;
            if (dev->type == USB_TYPE_XHCI &&
                xhci_update_ep0_mps(dev, real_mps) < 0) {
                kernel_usb_error_line("usb enum", "EP0 MPS update failed");
                return -EIO;
            }
        }

        if(len > 8)
        {
            dev->vendor = desc.vendorid;
            dev->product = desc.productid;
            dev->release = desc.bcd_dev;
            dev->manufacturerid = desc.manufacturer;
            dev->productid = desc.product;
            dev->serialid = desc.serial;
            dev->configs = desc.configs;
        }

        print_device(dev);
        res = 0;
    }

    return res;
}


int usb_get_config_descriptor(struct usb_dev_t *dev)
{
    struct usb_config_descriptor_t desc;
    struct usb_interface_t *current_iface = NULL;

    if(!usb_ctrl_in(dev, &desc, 0x80, 6, 2, 0, 0, sizeof(struct usb_config_descriptor_t)))
    {
        return -EIO;
    }

    uint16_t bufsz = desc.totlen;
    char buf[bufsz];

    if(!usb_ctrl_in(dev, buf, 0x80, 6, 2, 0, 0, bufsz))
    {
        return -EIO;
    }

    uint16_t confsz = MIN(bufsz, *(uint16_t *)(buf + 2));
    uintptr_t addr = (uintptr_t)buf;
    uintptr_t laddr = addr + confsz;
    uintptr_t config_base = addr;

    {
        char raw[160];
        unsigned int pos = 0;
        unsigned int j;
        pos += snprintf(raw + pos, sizeof(raw) - pos,
                        "raw(%u):", confsz);
        for (j = 0; j < confsz && pos + 4 < sizeof(raw); j++)
            pos += snprintf(raw + pos, sizeof(raw) - pos,
                            " %02x", (uint8_t)buf[j]);
        kernel_usb_status_line("usb config raw", raw);
    }

    // read the descriptors
    while(addr + 2 <= laddr)
    {
        uint8_t type = *(uint8_t *)(addr + 1);
        uint8_t len = *(uint8_t *)(addr);

        if (len < 2 || addr + len > laddr) {
            char detail[96];
            snprintf(detail, sizeof(detail),
                     "off=%u type=%u len=%u remain=%u",
                     (unsigned int)(addr - config_base), type, len,
                     (unsigned int)(laddr - addr));
            usb_printk("usb: malformed config descriptor %s\n", detail);
            kernel_usb_error_line("usb config malformed", detail);
            return -EIO;
        }

        if(len == 9 && type == 2)           // config descriptor
        {
#ifdef __DEBUG
            struct usb_config_descriptor_t *desc = 
                            (struct usb_config_descriptor_t *)addr;

            print_config_descriptor(desc);
#endif
        }
        else if(len == 9 && type == 4)      // interface descriptor
        {
            struct usb_interface_t *iface, *tmp;
            struct usb_interface_descriptor_t *desc = 
                            (struct usb_interface_descriptor_t *)addr;

            current_iface = NULL;

#ifdef __DEBUG
            print_interface_descriptor(desc);
#endif

            if(!(iface = kmalloc(sizeof(struct usb_interface_t))))
            {
                usb_printk("usb: failed to alloc interface struct\n");
            }
            else
            {
                A_memset(iface, 0, sizeof(*iface));
                A_memcpy(&iface->desc, desc, sizeof(struct usb_interface_descriptor_t));
                iface->bytes_per_sector = 512;      // for MSDs
                iface->usb = dev;
                //iface->data = NULL;
                iface->next = NULL;

                if(dev->interfaces == NULL)
                {
                    dev->interfaces = iface;
                }
                else
                {
                    for(tmp = dev->interfaces; tmp->next != NULL; tmp = tmp->next)
                    {
                        ;
                    }

                    tmp->next = iface;
                }
                current_iface = iface;
            }
        }
        else if(len == 7 && type == 5)  // endpoint descriptor
        {
            struct usb_endpoint_t *endpoint, *tmp;
            struct usb_endpoint_descriptor_t *desc =
                            (struct usb_endpoint_descriptor_t *)addr;

            /* Always print non-EP0 endpoint info for debugging */
            if ((desc->addr & 0xF) != 0) {
                usb_printk("usb: EP%d %s mps=%d interval=%d attribs=0x%02x\n",
                    desc->addr & 0xF,
                    (desc->addr & 0x80) ? "IN" : "OUT",
                    desc->mps, desc->interval, desc->attribs);
            }

            if(!(endpoint = kmalloc(sizeof(struct usb_endpoint_t))))
            {
                usb_printk("usb: failed to alloc endpoint struct\n");
            }
            else
            {
                endpoint->addr = (desc->addr & 0xF);
                endpoint->mps = desc->mps & 0x07FF;
                endpoint->interval = desc->interval;
                endpoint->toggle = 0;
                endpoint->type = (desc->attribs & 0x03);
                endpoint->direction = (desc->addr & 0x80) ? 
                                        USB_ENDPOINT_IN : USB_ENDPOINT_OUT;
                endpoint->next = NULL;

                if(dev->endpoints == NULL)
                {
                    dev->endpoints = endpoint;
                }
                else
                {
                    for(tmp = dev->endpoints; tmp->next != NULL; tmp = tmp->next)
                    {
                        ;
                    }

                    tmp->next = endpoint;
                }

                if (current_iface) {
                    if (endpoint->type == USB_ENDPOINT_INTERRUPT)
                        current_iface->endpoint_interrupt = endpoint;
                    else if (endpoint->direction == USB_ENDPOINT_IN)
                        current_iface->endpoint_in = endpoint;
                    else
                        current_iface->endpoint_out = endpoint;
                }
            }
        }
        else if(type == 33)
        {
            /* HID descriptor: byte 6 is the subordinate descriptor type,
             * bytes 7..8 are its little-endian length.  Associate it with
             * the interface descriptor that immediately precedes it. */
            if (current_iface && len >= 9 &&
                *(uint8_t *)(addr + 6) == 0x22) {
                current_iface->hid_report_len =
                    *(uint8_t *)(addr + 7) |
                    ((uint16_t)*(uint8_t *)(addr + 8) << 8);
                usb_printk("usb: HID report descriptor length=%d iface=%d\n",
                    current_iface->hid_report_len,
                    current_iface->desc.interfacenum);
            }
#ifdef __DEBUG
            struct usb_hid_descriptor_t *desc =
                            (struct usb_hid_descriptor_t *)addr;

            print_hid_descriptor(desc);
#endif
        }
        else
        {
            usb_printk("usb: unknown descriptor: type %u, len %u\n", type, len);
        }

        addr += len;
    }

    // XXX: update endpoint info for XHCI devices

    return 0;
}


#ifdef __DEBUG

int usb_get_string_descriptor(struct usb_dev_t *dev)
{
    struct usb_string_descriptor_t desc;

    if(!usb_ctrl_in(dev, &desc, 0x80, 6, 3, 0, 0, sizeof(struct usb_string_descriptor_t)))
    {
        return -EIO;
    }

    print_string_descriptor(&desc);

    return 0;
}


int usb_get_unistring_descriptor(struct usb_dev_t *dev, uint32_t strindex)
{
    char buf[64];

    if(!usb_ctrl_in(dev, buf, 0x80, 6, 3, strindex, 0x0409, sizeof(buf)))
    {
        return -EIO;
    }

    print_unistring_descriptor(dev, (struct usb_unistring_descriptor_t *)buf, strindex);

    return 0;
}

#endif


uint8_t usb_get_iface(struct usb_dev_t *dev, uint16_t iface)
{
    uint8_t alt_iface = 0;

    usb_ctrl_in(dev, &alt_iface, 0x81, 10, 0, 0, iface, 1);

    return alt_iface;
}


void usb_set_iface(struct usb_dev_t *dev, uint16_t iface, uint8_t alt_iface)
{
    usb_ctrl_set(dev, 0x01, 11, 0, alt_iface, iface);
}


uint8_t usb_get_config(struct usb_dev_t *dev)
{
    uint8_t config = 0;

    usb_ctrl_in(dev, &config, 0x80, 8, 0, 0, 0, 1);

    return config;
}


unsigned int usb_set_device_addr(struct usb_dev_t *dev, unsigned int addr)
{
    struct usb_transfer_t transfer;
    unsigned int addr2;

    /* EHCI real HW requires 3-phase: SETUP + OUT(empty) + IN(handshake) */
    usb_setup_transfer(dev, dev->endpoints, &transfer, USB_TRANSFER_CTRL);
    addr2 = usb_setup_transaction(&transfer, 0x00, 5, 0, addr, 0, 0);
    usb_out_transaction(&transfer, 0, NULL, 0);
    usb_in_transaction(&transfer, 1, 0, 0);

    usb_schedule_transfer(&transfer);
    usb_printk("usb: SET_ADDR waiting...\n");
    usb_wait_transfer(&transfer);
    usb_printk("usb: SET_ADDR done, success=%d\n", transfer.success);
    usb_delete_transfer(&transfer);

    /* USB spec: 2ms recovery after SET_ADDRESS */
    tick_delay(2000);

    return addr2;
}


int usb_set_config(struct usb_dev_t *dev, uint32_t config)
{
    if(usb_ctrl_set(dev, 0x00, 9, 0, config, 0))
    {
        dev->cur_config = config;
        return 0;
    }

    return -EIO;
}


int usb_setup_device(struct usb_dev_t *dev, unsigned int addr)
{
    int res;
    unsigned int i;
    struct usb_endpoint_t *endpoint;

    /* For XHCI, dev->num is the slot_id assigned by xhci_enable_slot.
     * For other HCI types, start at default address 0. */
    if (dev->type != USB_TYPE_XHCI)
        dev->num = 0;
    dev->configs = 0;

    // start with 8 bytes as we don't know the mps yet
    /* Always request 8 bytes first to learn MPS, regardless of speed */
    if((res = usb_get_device_descriptor(dev, 8)) < 0)
    {
        res = usb_get_device_descriptor(dev, 8);
    }

    if(res < 0)
    {
        usb_printk("usb: failed to get device descriptor (err %d)\n", res);
        kernel_usb_error_line("usb enum", "device descriptor failed");
        return res;
    }
    kernel_usb_status_line("usb enum", "device descriptor received");
    /* 2nd port reset after learning MPS, before SET_ADDRESS (EHCI only) */
    if (dev->speed == USB_SPEED_HIGH && dev->type == USB_TYPE_EHCI) {
        extern void ehci_reset_port(void *ehci, unsigned int port);
        ehci_reset_port(dev->priv, dev->port);
    }
    tick_delay(15000);
    usb_printk("usb: 2nd reset done, setting addr=%d\n", addr);
    dev->num = usb_set_device_addr(dev, addr);
    usb_printk("usb: addr=%d set\n", dev->num);

    // now get full 18-byte descriptor for all speeds
    usb_get_device_descriptor(dev, 18);

    if((res = usb_get_config_descriptor(dev)) < 0)
    {
        res = usb_get_config_descriptor(dev);
    }

    if(res < 0)
    {
        usb_printk("usb: failed to get config descriptor (err %d)\n", res);
        kernel_usb_error_line("usb enum", "configuration descriptor failed");
        return res;
    }
    kernel_usb_status_line("usb enum", "configuration descriptor received");

#ifdef __DEBUG

    if((res = usb_get_string_descriptor(dev)) < 0)
    {
        usb_printk("usb: failed to get string descriptor (err %d)\n", res);
        return res;
    }

    for(i = 1; i < 4; i++)
    {
        usb_get_unistring_descriptor(dev, i);
    }

#endif

    // set first config
    if((res = usb_set_config(dev, 1)) < 0)
    {
        usb_printk("usb: failed to set config (err %d)\n", res);
        kernel_usb_error_line("usb enum", "SET_CONFIGURATION failed");
        return res;
    }

    if((i = usb_get_config(dev)) != 1)
    {
        usb_printk("usb: failed to get config (expected 1, got %d)\n", i);
        return -EIO;
    }

    if(dev->class == 0x09)
    {
        usb_printk("usb: setting up USB hub\n");
        init_hub(dev);
    }
    else
    {
        struct usb_interface_t *iface;
        int found = 0;

        for(iface = dev->interfaces; iface != NULL; iface = iface->next)
        {
            if(iface->desc.class == 0x08 && iface->desc.subclass == 0x06)   // MSD
            {
                found = 1;
                usb_printk("usb: setting up Mass Storage Device (MSD)\n");
                kernel_usb_status_line("usb class", "mass storage detected");
                init_msd(iface);
            }
            else if(iface->desc.class == 0x03)
            {
                found = 1;
                usb_printk("usb: HID device detected, initializing...\n");
                kernel_usb_status_line("usb class", "HID device detected");
                init_hid(iface);
            }
        }

        if(!found)
        {
            usb_printk("usb: finished unknown device setup\n");
        }
    }

    kernel_usb_status_line("usb enum", "device configured and ready");

    // now create /dev nodes for the device's endpoints
    for(endpoint = dev->endpoints; endpoint != NULL; endpoint = endpoint->next)
    {
        char buf[16];

        ksprintf(buf, 16, "usb%d.%d.%02d", dev->bus, dev->num, endpoint->addr);
        add_dev_node(buf,
                     USB_MAKE_DEVID(dev->bus, dev->num, endpoint->addr),
                     (S_IFCHR | 0666)); // crw-rw-rw-
    }

    return 0;
}


void usb_clear_feature_halt(struct usb_dev_t *dev, struct usb_endpoint_t *endpoint)
{
    /* wIndex must include direction bit (bit 7) per USB spec §9.4.5 */
    uint16_t ep_addr = endpoint->addr;
    if (endpoint->direction == USB_ENDPOINT_IN)
        ep_addr |= 0x80;
    usb_ctrl_set(dev, 0x02, 1, 0, 0, ep_addr);
}


/*
 * Process deferred CLEAR_FEATURE(ENDPOINT_HALT) requests.
 * Called from usb_periodic_poll_callback() at shallow stack depth
 * (scheduler idle loop), BEFORE usb_hcd_poll_hotplug().
 *
 * STALL recovery is a two-phase operation:
 *   Phase 1 (here):     CLEAR_FEATURE on the device side via EP0
 *   Phase 2 (in HC):    xhci_poll_transfer sees need_clear_halt==2,
 *                       re-arms the transfer ring and rings the doorbell.
 *
 * We cannot do CLEAR_FEATURE inside xhci_poll_transfer because the
 * nested control transfer would overflow the 7 KB kernel stack.
 */
void usb_process_deferred_clears(void)
{
    /* STALL recovery is now handled synchronously inside
     * xhci_poll_transfer (see "STALL recovery" block).
     * The HC driver writes CLEAR_FEATURE to EP0 and waits
     * for its completion inline, THEN re-arms EP3.
     * No deferred processing needed — the flag is cleared
     * inside the recovery function itself. */
}

/*
 * Poll interrupt transfers — called from xhci_poll() in the main
 * scheduler idle loop (via usb_periodic_poll_callback → usb_hcd_poll_hotplug).
 * Each HC driver's poll() handles event-ring completions and
 * re-queues interrupt TRBs.
 */
void usb_poll_interrupts(void)
{
    volatile struct usb_transfer_t *t;
    static int call_cnt = 0;

    if (++call_cnt <= 5)
        usb_printk("usb_poll_intr: call #%d head=%p\n",
                   call_cnt, inttransfer_head.next_inttransfer);

    /* Process deferred CLEAR_FEATURE before polling transfers.
     * Uses fire-and-forget (no wait) to avoid stack overflow.
     * Also called from usb_periodic_poll_callback (idle path). */
    usb_process_deferred_clears();

    for (t = inttransfer_head.next_inttransfer; t != NULL;
         t = t->next_inttransfer) {
        if (usb_poll_transfer((struct usb_transfer_t *)t) && t->callback) {
            t->callback(t->callback_arg);
        }
    }
}


static inline char *usbtype(uint8_t prog_if)
{
    switch(prog_if)
    {
        case USB_TYPE_UHCI: return "UHCI";
        case USB_TYPE_OHCI: return "OHCI";
        case USB_TYPE_EHCI: return "EHCI";
        case USB_TYPE_XHCI: return "XHCI";
        case USB_TYPE_NOHCI: return "No HCI";
        case USB_TYPE_ANYHCI: return "Any HCI";
        default: return "Unknown";
    }
}


int usb_init_controller(pci_compat_t *pci)
{
    int res = -EINVAL;
    uint16_t i;
    struct pci_bar_t bar[6];

    pci_get_bar_info(pci, &bar[0]);

    for(i = 0; i < 6; i++)
    {
        if(bar[i].iotype == PCI_IOTYPE_INVALID)
        {
            continue;
        }

        usb_printk("usb: device %x:%x:%x.%x type 0x%x (%s):\n", 
               pci->bus_id, pci->dev_id, pci->fn_id, i, pci->prog_if, usbtype(pci->prog_if));

        usb_printk("usb:    BAR " _XPTR_ ", iosize " _XPTR_ " (%s)\n", 
               bar[i].base, bar[i].iosize,
               (bar[i].iotype == PCI_IOTYPE_MMIO) ? "MMIO" : "I/O");

        if(last_bus >= MAX_USB_BUSES)
        {
            usb_printk("usb: too many buses -- skipping\n");
            return -ENOMEM;
        }

        usbbus[last_bus] = pci;
        pci->unit = last_bus++;

        switch(pci->prog_if)
        {
            case USB_TYPE_EHCI:
                res = ehci_install(pci, &bar[i]);
                break;

            case USB_TYPE_OHCI:
                res = ohci_install(pci, &bar[i]);
                break;

            case USB_TYPE_UHCI:
                res = uhci_install(pci, &bar[i]);
                break;

            default:
                usb_printk("usb: unsupported device type -- skipping\n");
                break;
        }
    }

    // if the bus was set up properly, add a controller device
    if(res == 0)
    {
        char buf[16];

        ksprintf(buf, 16, "usb%d", pci->unit);
        add_dev_node(buf, USB_MAKE_DEVID(pci->unit, 0, 0), (S_IFCHR | 0666)); // crw-rw-rw-
    }

    return 0;
}
