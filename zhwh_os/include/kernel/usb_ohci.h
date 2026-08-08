/**
 * @file usb_ohci.h
 */
#ifndef KERNEL_USB_OHCI_H
#define KERNEL_USB_OHCI_H

#include "usb.h"

/* ================================================================
 * OHCI Register Offsets (MMIO)
 * ================================================================ */
#define OHCI_REG_CTRL           0x04   /* HC Control */
#define OHCI_REG_CMD_STS        0x08   /* HC Command & Status */
#define OHCI_REG_INT_STS        0x0C   /* HC Interrupt Status */
#define OHCI_REG_INT_EN         0x10   /* HC Interrupt Enable */
#define OHCI_REG_INT_DIS        0x14   /* HC Interrupt Disable */
#define OHCI_REG_HCCA           0x18   /* HCCA Address */
#define OHCI_REG_CTRL_HEAD_ED   0x20   /* Control List Head ED */
#define OHCI_REG_CTRL_CUR_ED    0x24   /* Control List Current ED */
#define OHCI_REG_BULK_HEAD_ED   0x28   /* Bulk List Head ED */
#define OHCI_REG_BULK_CUR_ED    0x2C   /* Bulk List Current ED */
#define OHCI_REG_FRAME_INTRVL   0x34   /* Frame Interval */
#define OHCI_REG_PERIOD_START   0x40   /* Periodic Start */
#define OHCI_REG_ROOTHUB_DESCA  0x48   /* Root Hub Descriptor A */
#define OHCI_REG_ROOTHUB_DESCB  0x4C   /* Root Hub Descriptor B */
#define OHCI_REG_ROOTHUB_STS    0x50   /* Root Hub Status */
#define OHCI_REG_ROOTHUB_PRSTS  0x54   /* Root Hub Port Status (base, each port +4) */

/* ================================================================
 * CTRL register bits
 * ================================================================ */
#define OHCI_CTRL_INTREDIR       (1 << 0)   /* Interrupt Routing (0=SMI, 1=IRQ) */
#define OHCI_CTRL_PERIOD_EN      (1 << 2)   /* Periodic List Enable */
#define OHCI_CTRL_CTRL_EN        (1 << 4)   /* Control List Enable */
#define OHCI_CTRL_BULK_EN        (1 << 5)   /* Bulk List Enable */
#define OHCI_CTRL_ISOC_EN        (1 << 3)   /* Isochronous List Enable */
#define OHCI_CTRL_REMWAKEUP_EN   (1 << 9)   /* Remote Wakeup Enable */
#define OHCI_CTRL_HCFS           (3 << 6)   /* Host Controller Functional State */
#define OHCI_HC_RESET            0x00
#define OHCI_HC_RESUME           0x40
#define OHCI_HC_OPERATIONAL      0x80
#define OHCI_HC_SUSPEND          0xC0

/* ================================================================
 * CMD_STS register bits
 * ================================================================ */
#define OHCI_CMDSTS_RESET   (1 << 0)   /* Host Controller Reset */
#define OHCI_CMDSTS_CLF     (1 << 1)   /* Control List Filled */
#define OHCI_CMDSTS_BLF     (1 << 2)   /* Bulk List Filled */
#define OHCI_CMDSTS_OCR     (1 << 3)   /* Ownership Change Request */

/* ================================================================
 * INT_STS/INT_EN/INT_DIS register bits
 * ================================================================ */
#define OHCI_INTSTS_SOF          (1 << 0)   /* Scheduling Overrun / Start of Frame */
#define OHCI_INTSTS_WR_DONE      (1 << 1)   /* Write-back Done Head */
#define OHCI_INTSTS_SCHED_OVRRN  (1 << 2)   /* Scheduling Overrun */
#define OHCI_INTSTS_RESUME_DET   (1 << 4)   /* Resume Detected */
#define OHCI_INTSTS_ERR          (1 << 5)   /* Unrecoverable Error */
#define OHCI_INTSTS_FRAME_OVRFL  (1 << 6)   /* Frame Number Overflow */
#define OHCI_INTSTS_RHSC         (1 << 7)   /* Root Hub Status Change */
#define OHCI_INTSTS_OWN_CHG      (1 << 30)  /* Ownership Change */
#define OHCI_INTSTS_MAST_INT_EN  (1 << 31)  /* Master Interrupt Enable */

/* ================================================================
 * Root Hub Descriptor A bits
 * ================================================================ */
#define OHCI_RHA_DEV_TYPE        (1 << 12)  /* Device Type */
#define OHCI_RHA_NO_PWR_SWITCH   (1 << 8)   /* No Power Switching */
#define OHCI_RHA_PORT_MASK       0xFF       /* Number of downstream ports */

/* ================================================================
 * Root Hub Status bits
 * ================================================================ */
#define OHCI_RHS_PWRSTAT_CHG     (1 << 16)  /* Port Power Status Change */

/* ================================================================
 * Root Hub Port Status bits (per-port)
 * ================================================================ */
#define OHCI_RHP_CUR_CONN_STS       (1 << 0)   /* Current Connect Status */
#define OHCI_RHP_PORTEN_STS         (1 << 1)   /* Port Enable Status */
#define OHCI_RHP_PORTSUSPND_STS     (1 << 2)   /* Port Suspend Status */
#define OHCI_RHP_PORTOVRCUR_CHG     (1 << 4)   /* Port Overcurrent Indicator Change */
#define OHCI_RHP_PORTRST_STS        (1 << 5)   /* Port Reset Status Change */
#define OHCI_RHP_PORTPWR_STS        (1 << 8)   /* Port Power Status */
#define OHCI_RHP_LOSPEED            (1 << 9)   /* Low Speed Device Attached */
#define OHCI_RHP_CONN_STS_CHG       (1 << 16)  /* Connect Status Change */
#define OHCI_RHP_PORTEN_STS_CHG     (1 << 17)  /* Port Enable Status Change */
#define OHCI_RHP_PORTSUSPND_STS_CHG (1 << 18)  /* Port Suspend Status Change */
#define OHCI_RHP_PORTRST_STS_CHG    (1 << 20)  /* Port Reset Status Change */

/* ================================================================
 * Pool sizes
 * ================================================================ */
#define OHCI_MAX_TD          128
#define OHCI_MAX_ED          128
#define OHCI_MAX_TDBUF       128
#define OHCI_TDBUF_SIZE      1024
#define OHCI_TDBUF_POOL_SIZE (OHCI_MAX_TDBUF * OHCI_TDBUF_SIZE)
#define OHCI_MAX_INT_INDEX   32

/* ================================================================
 * Flags
 * ================================================================ */
#define OHCI_FLAG_RUN          0x01
#define OHCI_FLAG_PORTENABLED  0x02

#define OHCI_PORT_FLAG_CONNECTED  0x01

/* ================================================================
 * OHCI Data Structures
 * ================================================================ */

/* HCCA (Host Controller Communication Area — 256 bytes, 256-byte aligned) */
struct ohci_hcca_t {
    uint32_t int_table[32];       /* Interrupt ED table (32 entries) */
    uint16_t frame_no;
    uint16_t pad1;
    uint32_t done_head;
    uint8_t  reserved[116];
};

/* Transfer Descriptor */
struct ohci_td_t {
    uintptr_t self_phys;
    uint32_t  next;        /* Next TD */
    uint32_t  dword0;      /* Control field */
    uint32_t  curbuf;      /* Current Buffer Pointer */
    uint32_t  bufend;      /* Buffer End */
    void     *virtbuf;
    int       alloced;
};

/* Endpoint Descriptor */
struct ohci_ed_t {
    uintptr_t  self_phys;
    uint32_t   dword0;      /* Control */
    uint32_t   td_qtail;    /* Tail TD */
    uint32_t   td_qhead;    /* Head TD */
    uint32_t   next;        /* Next ED */
    int        alloced;
    int        skip;
    uint8_t    endpoint;
    uint8_t    dev;
    uint16_t   mps;
    uint8_t    direction;
    uint8_t    speed;
    uint8_t    format;
    struct ohci_td_t *td_dummy;
};

/* Per-port state */
struct ohci_port_t {
    uint8_t           port;
    struct usb_dev_t *usb;
    uint32_t          flags;
    void             *hc;
};

/* Transaction descriptor */
struct ohci_transaction_t {
    struct ohci_td_t *tdvirt;
    struct ohci_td_t *tdcopyvirt;
    void             *tdbuf;
    void             *inbuf;
    size_t            inlen;
};

/* OHCI Host Controller device */
struct ohci_dev_t {
    pci_compat_t *pci;
    struct ohci_dev_t *next;
    uint32_t           flags;
    uint32_t           port_count;
    uintptr_t          iobase;
    uint32_t           iosize;
    int                mmio;
    struct ohci_port_t *ports;

    /* HCCA */
    struct ohci_hcca_t *hcca;
    uintptr_t           hcca_phys;

    /* TD Pool */
    struct ohci_td_t   *tdpool;
    uintptr_t           tdpool_phys;

    /* ED Pool */
    struct ohci_ed_t   *edpool;
    uintptr_t           edpool_phys;

    /* TD Buffer Pool */
    uintptr_t           tdbufpool;
    uintptr_t           tdbufpool_phys;
    uint8_t             tdbuf_used[OHCI_MAX_TDBUF];

    /* List heads */
    struct ohci_ed_t   *ed_ctrl_head;
    struct ohci_ed_t   *ed_bulk_head;
    struct ohci_ed_t   *ed_int_head[OHCI_MAX_INT_INDEX];

    /* Device address bitmap */
    uint32_t            addr_bitmap[MAX_DEV_PER_HC / 32];

    /* Synchronization */
    kernel_mutex_t      lock;
};

/* ================================================================
 * Function declarations (implemented in usb_ohci.c)
 * ================================================================ */
struct usb_dev_t *ohci_get_dev_struct(pci_compat_t *bus, uint8_t num);
int ohci_install(pci_compat_t *pci, struct pci_bar_t *bar);

#endif /* KERNEL_USB_OHCI_H */
