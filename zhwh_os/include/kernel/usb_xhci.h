/**
 * @file usb_xhci.h
 * @brief XHCI (USB 3.0+) Host Controller Driver definitions
 *
 * Based on eXtensible Host Controller Interface spec rev 1.2.
 * 32-bit only — 64-bit registers are accessed as two 32-bit halves.
 */
#ifndef KERNEL_USB_XHCI_H
#define KERNEL_USB_XHCI_H

#include "usb.h"

/* ================================================================
 * XHCI Register Offsets (from MMIO base)
 * ================================================================ */

/* Capability Registers (offset 0x00) */
#define XHCI_CAP_CAPLENGTH      0x00   /* Capability Register Length */
#define XHCI_CAP_RSVD           0x01   /* Reserved */
#define XHCI_CAP_HCIVERSION     0x02   /* Interface Version (16-bit) */
#define XHCI_CAP_HCSPARAMS1     0x04   /* Structural Parameters 1 */
#define XHCI_CAP_HCSPARAMS2     0x08   /* Structural Parameters 2 */
#define XHCI_CAP_HCSPARAMS3     0x0C   /* Structural Parameters 3 */
#define XHCI_CAP_HCCPARAMS1     0x10   /* Capability Parameters 1 */
#define XHCI_CAP_DBOFF          0x14   /* Doorbell Offset */
#define XHCI_CAP_RTSOFF         0x18   /* Runtime Register Space Offset */
#define XHCI_CAP_HCCPARAMS2     0x1C   /* Capability Parameters 2 */

/* HCSPARAMS1 bitfields */
#define HCSPARAMS1_MAXSLOTS(x)     ((x) & 0xFF)
#define HCSPARAMS1_MAXINTRS(x)    (((x) >> 8) & 0x7FF)
#define HCSPARAMS1_MAXPORTS(x)    (((x) >> 24) & 0xFF)

/* HCSPARAMS2 bitfields */
#define HCSPARAMS2_IST(x)         ((x) & 0xF)
#define HCSPARAMS2_ERSTMAX(x)    (((x) >> 4) & 0xF)
#define HCSPARAMS2_MAXSCRPBUF(x) (((x) >> 21) & 0x1F)

/* HCSPARAMS3 bitfields */
#define HCSPARAMS3_U1DEL(x)       ((x) & 0xFF)
#define HCSPARAMS3_U2DEL(x)      (((x) >> 16) & 0xFFFF)

/* HCCPARAMS1 bitfields */
#define HCCPARAMS1_AC64(x)        ((x) & 0x1)            /* 64-bit addressing capable */
#define HCCPARAMS1_BNC(x)        (((x) >> 1) & 0x1)     /* BW Negotiation */
#define HCCPARAMS1_CSZ(x)        (((x) >> 2) & 0x1)     /* Context Size */
#define HCCPARAMS1_PPC(x)        (((x) >> 3) & 0x1)     /* Port Power Control */
#define HCCPARAMS1_PIND(x)       (((x) >> 4) & 0x1)     /* Port Indicators */
#define HCCPARAMS1_LHRC(x)       (((x) >> 5) & 0x1)     /* Light HC Reset */
#define HCCPARAMS1_LTC(x)        (((x) >> 6) & 0x1)     /* Latency Tolerance Msg */
#define HCCPARAMS1_NSS(x)        (((x) >> 7) & 0x1)     /* No Secondary SID */
#define HCCPARAMS1_PAE(x)        (((x) >> 8) & 0x1)     /* Parse All Event Data */
#define HCCPARAMS1_SPC(x)        (((x) >> 9) & 0x1)     /* Stopped - Short Packet */
#define HCCPARAMS1_SEC(x)        (((x) >> 10) & 0x1)    /* Stopped EDTLA */
#define HCCPARAMS1_CFC(x)        (((x) >> 11) & 0x1)    /* Contiguous Frame ID */
#define HCCPARAMS1_MAXPSASIZE(x) (((x) >> 12) & 0xF)    /* Max PSA Size */
#define HCCPARAMS1_XECP(x)       (((x) >> 16) & 0xFFFF) /* xECP offset */

/* Operational Registers (offset = CAPLENGTH) */
#define XHCI_OP_USBCMD           0x00   /* USB Command */
#define XHCI_OP_USBSTS           0x04   /* USB Status */
#define XHCI_OP_PAGESIZE         0x08   /* Page Size */
#define XHCI_OP_RESERVED1        0x0C
#define XHCI_OP_RESERVED2        0x10
#define XHCI_OP_DNCTRL           0x14   /* Device Notification Control */
#define XHCI_OP_CRCR_LO          0x18   /* Command Ring Control (low 32) */
#define XHCI_OP_CRCR_HI          0x1C   /* Command Ring Control (high 32) */
#define XHCI_OP_RESERVED3        0x20   /* 4 reserved dwords */
#define XHCI_OP_DCBAAP_LO        0x30   /* Device Context Base Address Array (low 32) */
#define XHCI_OP_DCBAAP_HI        0x34   /* Device Context Base Address Array (high 32) */
#define XHCI_OP_CONFIG           0x38   /* Configure */

/* USBCMD bits */
#define USBCMD_RS                (1 << 0)   /* Run/Stop */
#define USBCMD_HCRST             (1 << 1)   /* Host Controller Reset */
#define USBCMD_INTE              (1 << 2)   /* Interrupter Enable */
#define USBCMD_HSEE              (1 << 3)   /* Host System Error Enable */
#define USBCMD_LHCRST            (1 << 7)   /* Light HC Reset */
#define USBCMD_CSS               (1 << 8)   /* Controller Save State */
#define USBCMD_CRS               (1 << 9)   /* Controller Restore State */
#define USBCMD_EWE               (1 << 10)  /* Enable Wrap Event */

/* USBSTS bits */
#define USBSTS_HCH               (1 << 0)   /* HC Halted */
#define USBSTS_HSE               (1 << 2)   /* Host System Error */
#define USBSTS_EINT              (1 << 3)   /* Event Interrupt */
#define USBSTS_PCD               (1 << 4)   /* Port Change Detect */
#define USBSTS_SSS               (1 << 8)   /* Save State Status */
#define USBSTS_RSS               (1 << 9)   /* Restore State Status */
#define USBSTS_SRE               (1 << 10)  /* Save/Restore Error */
#define USBSTS_CNR               (1 << 11)  /* Controller Not Ready */
#define USBSTS_HCE               (1 << 12)  /* Host Controller Error */

/* CONFIG bits */
#define CONFIG_MAXSLOTSEN(x)     ((x) & 0xFF)

/* Runtime Registers (offset = RTSOFF) */
/* Interrupter 0 registers */
#define XHCI_RT_IMAN(x)          ((x) + 0x20)  /* Interrupter Management */
#define XHCI_RT_IMOD(x)          ((x) + 0x24)  /* Interrupter Moderation */
#define XHCI_RT_ERSTSZ(x)        ((x) + 0x28)  /* Event Ring Segment Table Size */
#define XHCI_RT_ERSTBA_LO(x)     ((x) + 0x30)  /* Event Ring Segment Table Base Address (low) */
#define XHCI_RT_ERSTBA_HI(x)     ((x) + 0x34)  /* Event Ring Segment Table Base Address (high) */
#define XHCI_RT_ERDP_LO(x)       ((x) + 0x38)  /* Event Ring Dequeue Pointer (low) */
#define XHCI_RT_ERDP_HI(x)       ((x) + 0x3C)  /* Event Ring Dequeue Pointer (high) */

/* IMAN bits */
#define IMAN_IP                  (1 << 0)   /* Interrupt Pending */
#define IMAN_IE                  (1 << 1)   /* Interrupt Enable */

/* Doorbell Array (offset = DBOFF) */
#define XHCI_DB_TARGET(x)         ((x) & 0xFF)
#define XHCI_DB_STREAM(x)        (((x) >> 16) & 0xFFFF)

/* Port Status Registers (offset from Operational base) */
/* USB2 protocol ports start at 0x400, USB3 at offset 0x400 + (USB2_port_count * 0x10) */
/* But we read port info from HCSPARAMS1 to calculate offsets dynamically */

/* ================================================================
 * XHCI Port Register bit definitions
 * Bit layout differs from EHCI! PE=bit1, CSC=bit17.
 * ================================================================ */
#define XHCI_PORT_CCS             (1 << 0)    /* Current Connect Status */
#define XHCI_PORT_PED             (1 << 1)    /* Port Enabled/Disabled */
#define XHCI_PORT_OCA             (1 << 2)    /* Over-current Active (USB2) */
#define XHCI_PORT_OCC             (1 << 3)    /* Over-current Change (USB2) */
#define XHCI_PORT_PR              (1 << 4)    /* Port Reset */
#define XHCI_PORT_PEC             (1 << 5)    /* Port Enable Change (USB2) */
#define XHCI_PORT_PLS_SHIFT       5
#define XHCI_PORT_PLS_MASK        (0xF << XHCI_PORT_PLS_SHIFT)
#define XHCI_PORT_PP              (1 << 9)    /* Port Power */
#define XHCI_PORT_SPEED_SHIFT     10
#define XHCI_PORT_SPEED_MASK      (0xF << XHCI_PORT_SPEED_SHIFT)
#define XHCI_PORT_WCE             (1 << 20)   /* Wake on Connect Enable */
#define XHCI_PORT_WDE             (1 << 19)   /* Wake on Disconnect Enable */
#define XHCI_PORT_CSC             (1 << 17)   /* Connect Status Change */
#define XHCI_PORT_PRC             (1 << 21)   /* Port Reset Change */
#define XHCI_PORT_PLC             (1 << 22)   /* Port Link State Change */
#define XHCI_PORT_CEC             (1 << 23)   /* Config Error Change */
#define XHCI_PORT_CAS             (1 << 24)   /* Cold Attach Status */
#define XHCI_PORT_WPR             (1 << 31)   /* Warm Port Reset (USB3) */

/* Port Speed IDs (from bits 13:10) */
#define XHCI_PORT_SPEED_FULL      1
#define XHCI_PORT_SPEED_LOW       2
#define XHCI_PORT_SPEED_HIGH      3
#define XHCI_PORT_SPEED_SUPER     4

/* ================================================================
 * TRB (Transfer Request Block) — 16 bytes, 16-byte aligned
 * ================================================================ */
struct xhci_trb_t {
    uint64_t param;       /* Data pointer or command-specific */
    uint32_t status;      /* Completion code + transfer length */
    uint32_t control;     /* TRB type + flags + cycle bit */
};

/* TRB Types (TRB[10:0] in control DWORD) */
#define TRB_TYPE_RESERVED             0
#define TRB_TYPE_NORMAL               1
#define TRB_TYPE_SETUP_STAGE          2
#define TRB_TYPE_DATA_STAGE           3
#define TRB_TYPE_STATUS_STAGE         4
#define TRB_TYPE_ISOCH                5
#define TRB_TYPE_LINK                 6
#define TRB_TYPE_EVENT_DATA           7
#define TRB_TYPE_NOOP                 8
#define TRB_TYPE_ENABLE_SLOT          9
#define TRB_TYPE_DISABLE_SLOT        10
#define TRB_TYPE_ADDRESS_DEVICE      11
#define TRB_TYPE_CONFIGURE_EP        12
#define TRB_TYPE_EVALUATE_CTX        13
#define TRB_TYPE_RESET_EP            14
#define TRB_TYPE_STOP_EP             15
#define TRB_TYPE_SET_TR_DEQUEUE      16
#define TRB_TYPE_RESET_DEVICE        17
#define TRB_TYPE_FORCE_EVENT         18
#define TRB_TYPE_NEGOTIATE_BW        19
#define TRB_TYPE_SET_LAT_TOL         20
#define TRB_TYPE_GET_PORT_BW         21
#define TRB_TYPE_FORCE_HEADER        22
#define TRB_TYPE_NOOP_CMD            23
/* Event TRB types (32-43) */
#define TRB_TYPE_TRANSFER_EVENT      32
#define TRB_TYPE_CMD_COMP_EVENT      33
#define TRB_TYPE_PORT_STATUS_EVENT   34
#define TRB_TYPE_BANDWIDTH_EVENT     35
#define TRB_TYPE_DOORBELL_EVENT      36
#define TRB_TYPE_HOST_EVENT          37
#define TRB_TYPE_DEVICE_NOTIF        38
#define TRB_TYPE_MFINDEX_WRAP_EVENT  39

/* TRB Control DWORD bitfields */
#define TRB_CYCLE                   (1 << 0)    /* Cycle bit (toggle per wraparound) */
#define TRB_ENT                     (1 << 1)    /* Evaluate Next TRB */
#define TRB_TOGGLE_CYCLE(x)        (((x) & 0x1) << 0)
#define TRB_TC                     (1 << 1)    /* Toggle Cycle (for Link TRB) */
#define TRB_CHAIN                  (1 << 4)    /* Chain bit */
#define TRB_IO                     (1 << 5)    /* Interrupt On Completion */
#define TRB_ISP                    (1 << 2)    /* Interrupt-on-Short-Packet */
#define TRB_IDT                    (1 << 6)    /* Immediate Data */
#define TRB_DIR_IN                 (1 << 16)   /* Transfer direction (Data Stage) */
#define TRB_TRB_TYPE_SHIFT         10
#define TRB_TRB_TYPE_MASK          (0x3F << TRB_TRB_TYPE_SHIFT)
#define TRB_TYPE(x)                (((x) & 0x3F) << TRB_TRB_TYPE_SHIFT)
#define TRB_LINK_TOGGLE            (1 << 1)

/* Transfer Event TRB Completion Codes */
#define CC_SUCCESS                  1
#define CC_DATA_BUFFER_ERROR        2
#define CC_BABBLE_DETECTED          3
#define CC_USB_TRANSACTION_ERROR    4
#define CC_TRB_ERROR                5
#define CC_STALL_ERROR              6
#define CC_RESOURCE_ERROR           7
#define CC_BANDWIDTH_ERROR          8
#define CC_NO_SLOTS_ERROR           9
#define CC_INVALID_STREAM_TYPE     10
#define CC_SLOT_NOT_ENABLED        11
#define CC_ENDPOINT_NOT_ENABLED    12
#define CC_SHORT_PACKET            13
#define CC_RING_UNDERRUN           14
#define CC_RING_OVERRUN            15
#define CC_VF_EVENT_RING_FULL      16
#define CC_PARAMETER_ERROR         17
#define CC_BANDWIDTH_OVERRUN       18
#define CC_CONTEXT_STATE_ERROR     19
#define CC_NO_PING_RESPONSE        20
#define CC_EVENT_RING_FULL         21
#define CC_INCOMPATIBLE_DEVICE     22
#define CC_MISSED_SERVICE          23
#define CC_COMMAND_RING_STOPPED    24
#define CC_COMMAND_ABORTED         25
#define CC_STOPPED                 26
#define CC_STOPPED_LENGTH_INVALID  27
#define CC_STOPPED_SHORT_PACKET    28
#define CC_MAX_EXIT_LAT_TOO_LARGE  29
#define CC_ISOCH_BUFFER_OVERRUN    31
#define CC_EVENT_LOST              32
#define CC_UNDEFINED               33
#define CC_INVALID_SID             34
#define CC_SEC_BANDWIDTH_OVERRUN   35
#define CC_SPLIT_XACT_ERROR        36

/* ================================================================
 * Event Ring Segment Table Entry — 16 bytes
 * ================================================================ */
struct xhci_erst_entry_t {
    uint32_t addr_lo;      /* Segment base address (low 32) */
    uint32_t addr_hi;      /* Segment base address (high 32, always 0 for us) */
    uint32_t size;         /* Number of TRBs in segment (max 65536) */
    uint32_t rsvd;         /* Reserved */
};

/* ================================================================
 * XHCI Context Structures (32 bytes each, 64-byte aligned)
 * ================================================================ */

/* Slot Context (32 bytes) — xHCI spec rev 1.2, Table 6-7 */
struct xhci_slot_ctx_t {
    uint32_t dw0;          /* Route String (19:0) | Speed (23:20) | RSVDZ (26:24)
                               | Context Entries (31:27): last valid context index */
    uint32_t dw1;          /* RSVDZ (7:0) | Root Hub Port Num (15:8) | RSVDZ (31:16) */
    uint32_t dw2;          /* TT Hub Slot ID (7:0) | TT Port Num (15:8)
                               | RSVDZ (25:16) | Hub (26) | MTT (27) | RSVDZ (31:28) */
    uint32_t dw3;          /* RSVDZ (9:0) | Interrupter Target (21:10) | RSVDZ (31:22) */
    uint32_t dw4;          /* USB Device Address (7:0) | RSVDZ (23:8)
                               | Slot State (27:24) | RSVDZ (31:28) */
    uint32_t dw5;
    uint32_t dw6;
    uint32_t dw7;
};

/* Slot Context field masks (dw0) */
#define SLOT_CTX_CTXENT_SHIFT      27      /* Context Entries is in dw0 [31:27] */
#define SLOT_CTX_CTXENT_MASK       (0x1F << SLOT_CTX_CTXENT_SHIFT)
#define SLOT_CTX_SPEED_SHIFT       20      /* Speed is in dw0 [23:20] */
#define SLOT_CTX_SPEED_MASK        (0xF << SLOT_CTX_SPEED_SHIFT)
#define SLOT_CTX_ROUTE_SHIFT       0
#define SLOT_CTX_ROUTE_MASK        0xFFFFF
#define SLOT_CTX_PORT_SHIFT        8
#define SLOT_CTX_PORT_MASK         (0xFF << SLOT_CTX_PORT_SHIFT)
#define SLOT_CTX_ADDR_SHIFT        0
#define SLOT_CTX_ADDR_MASK         0xFF
#define SLOT_CTX_STATE_SHIFT       24      /* Slot State is in dw4 [27:24] */
#define SLOT_CTX_STATE_MASK        (0xF << SLOT_CTX_STATE_SHIFT)
#define SLOT_CTX_MTT               (1 << 25)
#define SLOT_CTX_HUB               (1 << 26)

/* Slot States */
#define SLOT_STATE_DISABLED        0
#define SLOT_STATE_DEFAULT         1
#define SLOT_STATE_ADDRESSED       2
#define SLOT_STATE_CONFIGURED      3

/* Endpoint Context (32 bytes) — xHCI spec rev 1.2, Table 6-12 */
struct xhci_ep_ctx_t {
    uint32_t dw0;          /* EP State (2:0) | Mult (9:8) | MaxPStreams (14:10) | LSA (15)
                               | Interval (23:16) | Max ESIT Payload Hi (31:24) */
    uint32_t dw1;          /* EP Type (2:0) | CErr (7:4) | Max Burst (15:8) | MPS (31:16) */
    uint32_t dw2;          /* TR Dequeue Pointer (low 32) */
    uint32_t dw3;          /* TR Dequeue Pointer (high 32) */
    uint32_t dw4;          /* Average TRB Length (15:0) | Max ESIT Payload Lo (31:16) */
    uint32_t dw5;
    uint32_t dw6;
    uint32_t dw7;
};

/* EP Context field masks */
#define EP_CTX_STATE_MASK          0x7
#define EP_CTX_MPS_SHIFT           16      /* MPS is in dw1 [31:16] */
#define EP_CTX_MPS_MASK            (0xFFFF << EP_CTX_MPS_SHIFT)
#define EP_CTX_INTERVAL_MASK       (0xFF << 8)
#define EP_CTX_TYPE_SHIFT          0       /* EP Type is in dw1 [2:0] */
#define EP_CTX_TYPE_MASK           (0x7 << EP_CTX_TYPE_SHIFT)
#define EP_CTX_CERR_SHIFT          4       /* CErr is in dw1 [7:4] */
#define EP_CTX_CERR_MASK           (0xF << EP_CTX_CERR_SHIFT)
#define EP_CTX_CERR_DEFAULT        (3 << EP_CTX_CERR_SHIFT)  /* Max 3 errors before halt */
#define EP_CTX_MAXB_SHIFT          8       /* Max Burst is in dw1 [15:8] */
#define EP_CTX_MAXP_SHIFT          10      /* MaxPStreams is in dw0 [14:10] */
#define EP_CTX_LSA                 (1 << 15)  /* Linear Stream Array in dw0 */
#define EP_CTX_DCS                 (1 << 0)   /* Dequeue Cycle State */
#define EP_CTX_MAX_ESIT_SHIFT      24      /* Max ESIT Payload Hi is in dw0 [31:24] */

/* EP Types */
#define EP_TYPE_NOT_VALID          0
#define EP_TYPE_ISOCH_OUT          1
#define EP_TYPE_BULK_OUT           2
#define EP_TYPE_INTERRUPT_OUT      3
#define EP_TYPE_CONTROL            4
#define EP_TYPE_ISOCH_IN           5
#define EP_TYPE_BULK_IN            6
#define EP_TYPE_INTERRUPT_IN       7

/* EP States */
#define EP_STATE_DISABLED          0
#define EP_STATE_RUNNING           1
#define EP_STATE_HALTED            2
#define EP_STATE_STOPPED           3
#define EP_STATE_ERROR             4

/* Input Control Context (32 bytes) */
struct xhci_input_ctrl_ctx_t {
    uint32_t drop_flags;   /* Drop Context flags */
    uint32_t add_flags;    /* Add Context flags (bit 0=Slot, bit 1=EP0, bit 2+ = EP1-31) */
    uint32_t rsvd[6];
};

/* Device Context Structure: Input Control + Slot + EP0..EP30 (or EP0-Out + EP0-In + EP1..) */
#define XHCI_CTX_SIZE              32
#define XHCI_CTX_ALIGN             64

/* ================================================================
 * Command TRB parameter encoding
 * ================================================================ */

/* Enable Slot: no parameters (TRB type 9) */
/* Disable Slot: slot_id in bits [31:24] of DWORD 3 */
#define DISABLE_SLOT_SLOTID(x)     (((x) & 0xFF) << 24)

/* Address Device: bits [7:0] = slot_id in DWORD 3, input context ptr in param */
/* Configure Endpoint: slot_id + deconfigure flag */
#define CONFIGURE_EP_SLOT(x)       (((x) & 0xFF) << 24)
#define CONFIGURE_EP_DC            (1 << 9)
/* Evaluate Context: slot_id */
#define EVAL_CTX_SLOT(x)           (((x) & 0xFF) << 24)

/* ================================================================
 * XHCI Device Structure
 * ================================================================ */
#define XHCI_MAX_SLOTS             16
#define XHCI_MAX_ENDPOINTS         32
#define XHCI_CMD_RING_SIZE         256     /* TRBs */
#define XHCI_EVENT_RING_SIZE       256     /* TRBs */
#define XHCI_TRANSFER_RING_SIZE    64      /* TRBs per endpoint */

struct xhci_ring_t {
    struct xhci_trb_t  *ring;       /* Virtual address */
    uint64_t            phys;       /* Physical address (lower 32 bits relevant) */
    uint32_t            size;       /* Number of TRBs in ring */
    uint32_t            enqueue;    /* Producer index */
    uint32_t            dequeue;    /* Consumer index (software copy) */
    uint8_t             ccs;        /* Current Cycle State */
};

struct xhci_dev_t {
    pci_compat_t       *pci;
    struct xhci_dev_t  *next;
    uint32_t            flags;
    uint32_t            port_count;     /* Total ports (USB2 + USB3) */
    uint32_t            usb2_ports;     /* Number of USB2 protocol ports */
    uint32_t            usb3_ports;     /* Number of USB3 protocol ports */
    uint32_t            max_slots;
    uint8_t             caplen;
    uintptr_t           mmio_base;      /* Virtual address of MMIO base */
    uintptr_t           cap_base;       /* Capability regs (same as mmio_base) */
    uintptr_t           op_base;        /* Operational regs (mmio_base + caplen) */
    uintptr_t           rt_base;        /* Runtime regs (mmio_base + RTSOFF) */
    uintptr_t           db_base;        /* Doorbell array (mmio_base + DBOFF) */
    uint32_t            page_size;
    int                 mmio;           /* Always 1 for XHCI */

    /* DCBAA: array of 64-bit pointers to Device Contexts (max_slots + 1) */
    uint64_t           *dcbaa;          /* Virtual */
    uint64_t            dcbaa_phys;

    /* Command Ring */
    struct xhci_ring_t  cmd_ring;

    /* Event Ring */
    struct xhci_ring_t  event_ring;
    struct xhci_erst_entry_t *erst;     /* Event Ring Segment Table (1 segment) */
    uint64_t            erst_phys;

    /* Scratchpad Buffers (for page-aligned memory) */
    uint32_t            scratchpad_count;
    uint64_t           *sp_array;       /* Array of phys addresses for scratchpads */
    uint64_t            sp_array_phys;

    /* Per-slot state — dynamically allocated array */
    struct {
        uint8_t           enabled;
        uint8_t           addressed;
        uint8_t           configured;
        uint8_t           speed;
        uint8_t           port;
        struct usb_dev_t *usb;
        void             *dev_ctx;
        uint64_t          dev_ctx_phys;
        void             *input_ctx;
        uint64_t          input_ctx_phys;
        struct xhci_ring_t *ep_rings;  /* [XHCI_MAX_ENDPOINTS] — dyn alloc */
    } *slots;                            /* [XHCI_MAX_SLOTS + 1] */

    /* Device address bitmap */
    uint32_t            addr_bitmap[(XHCI_MAX_SLOTS / 32) + 1];
};

/* Flags */
#define XHCI_FLAG_RUN            0x01
#define XHCI_FLAG_PORTSENABLED   0x02

/* ================================================================
 * Function declarations
 * ================================================================ */
int  xhci_install(pci_compat_t *pci, struct pci_bar_t *bar);
void xhci_poll(void);
struct usb_dev_t *xhci_get_dev_struct(pci_compat_t *bus, uint8_t num);

#endif /* KERNEL_USB_XHCI_H */
