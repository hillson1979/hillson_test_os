/**
 * @file usb_ehci.h
 */
#ifndef KERNEL_USB_EHCI_H
#define KERNEL_USB_EHCI_H

#include "usb.h"

/* ================================================================
 * EHCI Register Offsets
 * ================================================================ */
#define EHCI_REG_HCCAP   0x00   /* Host Controller Capabilities (base + 0x00) */
#define EHCI_REG_HCOP    0x00   /* Operational regs: add ehci->caplen to iobase after reading caps */

/* --- HCCAP registers (offset from EHCI_REG_HCCAP) --- */
#define HCCAP_CAPLENGTH      0x00   /* Capability Register Length */
#define HCCAP_HCSPARAMS      0x04   /* Structural Parameters */
#define HCCAP_HCCPARAMS      0x08   /* Capability Parameters */

/* --- HCOP registers (offset from EHCI_REG_HCOP) --- */
#define HCOP_USBCMD           0x00   /* USB Command */
#define HCOP_USBSTS           0x04   /* USB Status */
#define HCOP_USBINTR          0x08   /* USB Interrupt Enable */
#define HCOP_FRINDEX          0x0C   /* Frame Index */
#define HCOP_CTRLDSSEGMENT    0x10   /* CTRLDSSEGMENT */
#define HCOP_PERIODICLISTBASE 0x14   /* Periodic Frame List Base Address */
#define HCOP_ASYNCLISTADDR    0x18   /* Async List Address */
#define HCOP_CONFIGFLAG       0x40   /* Configured Flag Register */
#define HCOP_PORTSC           0x44   /* Port Status/Control (base, each port +4) */

/* ================================================================
 * USBCMD register bits
 * ================================================================ */
#define ECHI_USBCMD_ASYNCEN        (1 << 4)   /* Async Schedule Enable */
#define ECHI_USBCMD_PERIODICEN     (1 << 5)   /* Periodic Schedule Enable */
#define ECHI_USBCMD_ASYNCDOORBELL  (1 << 6)   /* Async Schedule Doorbell */
#define ECHI_USBCMD_FRLIST_MASK    0xFFFFFFF3 /* Clear only bits 2-3 (Frame List Size) */
#define EHCI_USBCMD_FRLIST_1024    0x00        /* 1024-element frame list */

/* ================================================================
 * USBSTS register bits
 * ================================================================ */
#define ECHI_USBSTS_ASYNCINT    (1 << 4)   /* Async Schedule Status */
#define ECHI_USBSTS_PERIODSTS   (1 << 5)   /* Periodic Schedule Status */
#define ECHI_USBSTS_HCHALTED    (1 << 12)  /* HC Halted */
#define ECHI_USBSTS_ASYNCSTS    (1 << 15)  /* Async Schedule Status */
#define ECHI_USBSTS_HSERR       (1 << 16)  /* Host System Error */

/* ================================================================
 * PORTSC register bits
 * ================================================================ */
#define EHCI_PORTSC_CONN       (1 << 0)   /* Current Connect Status */
#define EHCI_PORTSC_CONNCHG    (1 << 1)   /* Connect Status Change */
#define EHCI_PORTSC_EN         (1 << 2)   /* Port Enabled */
#define EHCI_PORTSC_ENCHG      (1 << 3)   /* Port Enable Change */
#define EHCI_PORTSC_OWNER      (1 << 13)  /* Port Owner (1=companion HC) */
#define EHCI_PORTSC_POWER      (1 << 12)  /* Port Power */
#define EHCI_PORTSC_RESET      (1 << 8)   /* Port Reset */

/* ================================================================
 * Pool sizes
 * ================================================================ */
#define EHCI_MAX_QH          128
#define EHCI_MAX_TD          256
#define EHCI_MAX_TDBUF       128
#define EHCI_TDBUF_SIZE      4096
#define EHCI_TDBUF_POOL_SIZE (EHCI_MAX_TDBUF * EHCI_TDBUF_SIZE)

/* ================================================================
 * Flags
 * ================================================================ */
#define EHCI_FLAG_RUN          0x01
#define EHCI_FLAG_PORTENABLED  0x02

#define EHCI_PORT_FLAG_CONNECTED  0x01

/* ================================================================
 * EHCI Data Structures
 * ================================================================ */

/* Queue Head — MUST match EHCI hardware layout exactly.
 * 48 bytes total.  Hardware reads fields at fixed offsets. */
struct ehci_qh_t {
    /* ---- Hardware fields (offsets must match EHCI spec) ---- */
    /* 0x00 */ uint32_t next;              /* Horizontal Link Pointer */
    /* 0x04 */ uint32_t ep_char;           /* Endpoint Characteristics */
    /* 0x08 */ uint32_t ep_caps;           /* Endpoint Capabilities */
    /* 0x0C */ uint32_t cur_qtd;           /* Current qTD Pointer (overlay) */
    /* 0x10 */ uint32_t next_qtd;          /* Next qTD (overlay) */
    /* 0x14 */ uint32_t alt_qtd;           /* Alternate Next qTD (overlay) */
    /* 0x18 */ uint32_t token;             /* qTD Token (overlay) */
    /* 0x1C */ uint32_t buf[5];            /* Buffer Page Pointer List (overlay) */
              /* --- end of hardware visible area at 0x30 --- */

    /* ---- Software fields (not read by hardware) ---- */
    uintptr_t self_phys;         /* our physical address */
    void    *nextvirt;           /* virtual ptr for horizontal traversal */
    uint8_t  head_flag;          /* reclamation list head flag */
    uint8_t  devaddr;            /* USB device address (sw copy) */
    uint8_t  endpoint;           /* Endpoint number (sw copy) */
    uint8_t  endpoint_speed;     /* 0=full, 1=low, 2=high (sw copy) */
    uint8_t  toggle;             /* Data toggle control (sw copy) */
    uint16_t mps;                /* Max packet size (sw copy) */
    uint8_t  int_sched_mask;     /* S-mask (sw copy) */
    uint8_t  mult;               /* High-bandwidth multiplier (sw copy) */
    uint32_t next_qtd_alt;       /* Alternate next qTD (sw copy) */
    uint8_t  __pad[24];          /* pad to 96 bytes — EHCI requires 32-byte aligned QH */
} __attribute__((aligned(32)));

/* Transfer Descriptor (qTD) */
struct ehci_td_t {
    /* DWORDs 0-1: Next qTD pointer */
    uint32_t next_qtd;
    uint32_t next_qtd_alt;

    /* DWORDs 2-3: qTD Token — MUST match EHCI hardware layout */
    union {
        uint32_t raw;
        struct {
            uint32_t status  : 8;   /* [7:0]   Status (bit7=Active, 6=Halted, 5=DataBufErr, 4=Babble, 3=XactErr, 2=Missed) */
            uint32_t pid     : 2;   /* [9:8]   PID Code (0=OUT, 1=IN, 2=SETUP) */
            uint32_t cerr    : 2;   /* [11:10] Error Counter */
            uint32_t cpage   : 3;   /* [14:12] Current page */
            uint32_t ioc     : 1;   /* [15]    Interrupt on complete */
            uint32_t bytes   : 15;  /* [30:16] Total bytes to transfer */
            uint32_t toggle  : 1;   /* [31]    Data Toggle */
        } bits;
    } token;

    /* DWORDs 4-7: Buffer Pointers */
    uint32_t buf0;
    uint32_t buf1;
    uint32_t buf2;
    uint32_t buf3;
    uint32_t buf4;
};

/* Per-port state */
struct ehci_port_t {
    uint8_t           port;
    struct usb_dev_t *usb;
    uint32_t          flags;
    void             *ehci;   /* back-pointer to ehci_dev_t */
};

/* Transaction descriptor */
struct ehci_transaction_t {
    struct ehci_td_t *tdvirt;
    void             *tdphys;
    void             *tdbuf;
    void             *inbuf;
    size_t            inlen;
};

/* EHCI Host Controller device */
struct ehci_dev_t {
    pci_compat_t *pci;
    struct ehci_dev_t *next;
    uint32_t           flags;
    uint32_t           port_count;
    uintptr_t          iobase;
    uintptr_t          capbase;   /* saved HCCAP base before caplen shift */
    uint32_t           iosize;
    int                mmio;
    int                caplen;
    struct ehci_port_t *ports;

    /* Frame List (1 page = 1024 entries, 4KB aligned) */
    uint32_t          *framelist;
    uintptr_t          framelist_phys;
    uintptr_t          framelist_raw_phys; /* pre-alignment, for freeing */

    /* TD Pool */
    uintptr_t          tdpool;
    uintptr_t          tdpool_phys;
    uint8_t            td_used[EHCI_MAX_TD];

    /* TD Buffer Pool */
    uintptr_t          tdbufpool;
    uintptr_t          tdbufpool_phys;
    uint8_t            tdbuf_used[EHCI_MAX_TDBUF];

    /* QH Pool */
    uintptr_t          qhpool;
    uintptr_t          qhpool_phys;
    uint8_t            qh_used[EHCI_MAX_QH];

    /* Async Schedule */
    struct ehci_qh_t  *async_qh;
    struct ehci_qh_t  *tail_qh;
    struct ehci_qh_t  *ctrl_qh;   /* Single control QH — reused */
    int                ctrl_linked; /* QH already in async list */

    /* Device address bitmap */
    uint32_t           addr_bitmap[MAX_DEV_PER_HC / 32];
    int                addr_base;   /* per-controller address offset */

    /* Synchronization */
    kernel_mutex_t     qh_lock;
};

/* ================================================================
 * Function declarations (implemented in usb_ehci.c)
 * ================================================================ */
struct usb_dev_t *ehci_get_dev_struct(pci_compat_t *bus, uint8_t num);
int ehci_install(pci_compat_t *pci, struct pci_bar_t *bar);
void ehci_poll(void);

#endif /* KERNEL_USB_EHCI_H */
