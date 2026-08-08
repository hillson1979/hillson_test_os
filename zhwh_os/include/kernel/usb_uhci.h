/**
 * @file usb_uhci.h
 */
#ifndef KERNEL_USB_UHCI_H
#define KERNEL_USB_UHCI_H

#include "usb.h"

/* ================================================================
 * UHCI Register Offsets (I/O space)
 * ================================================================ */
#define UHCI_REG_CMD         0x00   /* USBCMD — Command */
#define UHCI_REG_STS         0x02   /* USBSTS — Status */
#define UHCI_REG_INT         0x04   /* USBINTR — Interrupt Enable */
#define UHCI_REG_FRNUM       0x06   /* FRNUM — Frame Number */
#define UHCI_REG_FRBASEADDR  0x08   /* FRBASEADD — Frame List Base Address */
#define UHCI_REG_SOFMOD      0x0C   /* SOFMOD — Start Of Frame Modify */
#define UHCI_REG_PORTSC1     0x10   /* PORTSC1 — Port 1 Status/Control */
#define UHCI_REG_PORTSC2     0x12   /* PORTSC2 — Port 2 Status/Control */

/* ================================================================
 * USBCMD register bits
 * ================================================================ */
#define UHCI_CMD_RS     (1 << 0)   /* Run/Stop */
#define UHCI_CMD_HCRST  (1 << 1)   /* Host Controller Reset */
#define UHCI_CMD_GRST   (1 << 2)   /* Global Reset */
#define UHCI_CMD_EGSM   (1 << 3)   /* Enter Global Suspend Mode */
#define UHCI_CMD_FGR    (1 << 4)   /* Force Global Resume */
#define UHCI_CMD_SWDBG  (1 << 5)   /* Software Debug */
#define UHCI_CMD_CF     (1 << 6)   /* Configure Flag */
#define UHCI_CMD_MAXP   (1 << 7)   /* Max Packet (64 bytes) */

/* ================================================================
 * USBSTS register bits
 * ================================================================ */
#define UHCI_STS_INT       (1 << 0)   /* USB Interrupt */
#define UHCI_STS_ERR       (1 << 1)   /* USB Error Interrupt */
#define UHCI_STS_RESDET    (1 << 2)   /* Resume Detect */
#define UHCI_STS_HOSTERR   (1 << 3)   /* Host System Error */
#define UHCI_STS_PROCERR   (1 << 4)   /* Host Controller Process Error */
#define UHCI_STS_HCHALTED  (1 << 5)   /* HC Halted */
#define UHCI_STS_MASK      0x003F

/* ================================================================
 * USBINTR register bits
 * ================================================================ */
#define UHCI_INT_CRC       (1 << 0)   /* CRC/Timeout */
#define UHCI_INT_RESUME    (1 << 1)   /* Resume */
#define UHCI_INT_IOC       (1 << 2)   /* Interrupt on Complete */
#define UHCI_INT_SHORT     (1 << 3)   /* Short Packet */
#define UHCI_INT_MASK      0x000F

/* ================================================================
 * PORTSC register bits
 * ================================================================ */
#define UHCI_PORTSC_CS         (1 << 0)   /* Current Connect Status */
#define UHCI_PORTSC_CS_CHG     (1 << 1)   /* Connect Status Change */
#define UHCI_PORTSC_EN         (1 << 2)   /* Port Enabled */
#define UHCI_PORTSC_EN_CHG     (1 << 3)   /* Port Enable/Disable Change */
#define UHCI_PORTSC_LOSPEED    (1 << 4)   /* Line Status: Low Speed */
#define UHCI_PORTSC_RST        (1 << 9)   /* Port Reset */
#define UHCI_PORTSC_RES_DET    (1 << 6)   /* Resume Detect */
#define UHCI_PORTSC_SUSPEND    (1 << 12)  /* Suspend */
#define UHCI_PORTSC_VALID      0x0080

/* ================================================================
 * PCI Legacy Support Register (offset 0xC0)
 * ================================================================ */
#define UHCI_PCI_LEGACY_STS   (1 << 0)
#define UHCI_PCI_LEGACY_PIRQ  (1 << 2)
#define UHCI_PCI_LEGACY_NOCHG (1 << 8)

/* ================================================================
 * Pool sizes
 * ================================================================ */
#define UHCI_MAX_QH          128
#define UHCI_MAX_TD          128
#define UHCI_MAX_TDBUF       128
#define UHCI_TDBUF_SIZE      1024
#define UHCI_TDBUF_POOL_SIZE (UHCI_MAX_TDBUF * UHCI_TDBUF_SIZE)

/* ================================================================
 * Flags
 * ================================================================ */
#define UHCI_FLAG_RUN          0x01
#define UHCI_FLAG_PORTENABLED  0x02

#define UHCI_PORT_FLAG_CONNECTED  0x01
#define UHCI_PORT_FLAG_LOSPEED   0x02

/* ================================================================
 * UHCI Data Structures
 * ================================================================ */

/* Frame list (1024 entries, each is a 32-bit pointer to QH or TD) */
struct uhci_framelist_t {
    uint32_t fr[1024];
};

/* Queue Head */
struct uhci_qh_t {
    uintptr_t self_phys;
    uint32_t  next;        /* Queue Head Link Pointer */
    uint32_t  transfer;    /* Queue Element Link Pointer */
    struct uhci_qh_t *nextvirt;
    int       alloced;
    int       freq;
};

/* Per-port state */
struct uhci_port_t {
    uint8_t           port;
    struct usb_dev_t *usb;
    uint32_t          flags;
    void             *uhci;      /* back-pointer to uhci_dev_t */
};

/* Transfer Descriptor (with all fields the driver expects) */
struct uhci_td_t {
    /* Hardware-visible */
    uint32_t next;       /* Link Pointer */
    uint32_t dword1;     /* Status (bit23=active, bit0-10=actlen) */
    uint32_t dword2;     /* Control (packed for hardware) */
    uint32_t buf;        /* Buffer physical address */

    /* Software fields */
    void    *virtbuf;    /* Virtual buffer pointer */
    int      alloced;    /* Pool allocation flag */

    /* Named fields — set by driver, packed into dword2 for hardware */
    uint8_t  pktid;      /* PID: SETUP/IN/OUT */
    uint8_t  toggle;     /* Data toggle bit */
    uint8_t  devaddr;    /* USB device address */
    uint8_t  endpoint;   /* Endpoint number */
    uint16_t maxlen;     /* Max length - 1 (11 bits) */
    uint8_t  errcnt;     /* Error counter (3 retries) */
    uint8_t  lospeed;    /* Low speed device flag */
    uint8_t  active;     /* Active bit (bit 23 of dword1) */
    uint8_t  intoncomp;  /* Interrupt on complete */
    uint32_t len;        /* Transfer length */
};

/* TD PID constants */
#define UHCI_TD_PKTID_SETUP  0x2D
#define UHCI_TD_PKTID_IN     0x69
#define UHCI_TD_PKTID_OUT    0xE1

/* Transaction descriptor */
struct uhci_transaction_t {
    struct uhci_td_t *tdvirt;
    void             *tdphys;
    void             *tdbuf;
    void             *inbuf;
    size_t            inlen;
};

/* UHCI Host Controller device */
struct uhci_dev_t {
    pci_compat_t *pci;
    struct uhci_dev_t *next;
    uint32_t           flags;
    uint32_t           port_count;
    uintptr_t          iobase;
    uint32_t           iosize;
    int                mmio;
    struct uhci_port_t *ports;

    /* Frame List */
    struct uhci_framelist_t *framelist;
    uintptr_t          framelist_phys;

    /* TD Pool */
    struct uhci_td_t  *tdpool;
    uintptr_t          tdpool_phys;

    /* TD Buffer Pool */
    uintptr_t          tdbufpool;
    uintptr_t          tdbufpool_phys;
    uint8_t            tdbuf_used[UHCI_MAX_TDBUF];

    /* QH Pool */
    struct uhci_qh_t  *qhpool;
    uintptr_t          qhpool_phys;

    /* Frequency-sorted QH queues */
    struct uhci_qh_t  *qh_1ms;
    struct uhci_qh_t  *qh_2ms;
    struct uhci_qh_t  *qh_4ms;
    struct uhci_qh_t  *qh_8ms;
    struct uhci_qh_t  *qh_16ms;
    struct uhci_qh_t  *qh_32ms;

    /* Device address bitmap */
    uint32_t           addr_bitmap[MAX_DEV_PER_HC / 32];

    /* Synchronization */
    kernel_mutex_t     qh_lock;
};

/* ================================================================
 * Function declarations (implemented in usb_uhci.c)
 * ================================================================ */
struct usb_dev_t *uhci_get_dev_struct(pci_compat_t *bus, uint8_t num);
int uhci_install(pci_compat_t *pci, struct pci_bar_t *bar);
void uhci_poll(void);

#endif /* KERNEL_USB_UHCI_H */
