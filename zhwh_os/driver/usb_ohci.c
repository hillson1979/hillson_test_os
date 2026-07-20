/**
 * @file usb_ohci.c
 * @brief Minimal OHCI driver — enough to get Dell MS116 mouse working
 */
#include <stdint.h>
#include "string.h"
#include "printf.h"
#include "pci.h"
#include "page.h"

// OHCI MMIO registers
#define OREV    0x00
#define OCTRL   0x04
#define OCMD    0x08
#define OINT    0x0C
#define OINTE   0x10
#define OHCCA   0x18
#define OCTRLHD 0x20
#define OBCKHD  0x24
#define OBCKTAIL 0x28
#define OINTTB  0x30
#define OFMINT  0x34
#define OPRDST  0x40

// Control
#define OCTRL_FS_MASK  3
#define OCTRL_OPER     (2<<0)
#define OCTRL_PLE      (1<<2)   // Periodic List Enable
#define OCTRL_CLE      (1<<4)   // Control List Enable
#define OCTRL_BLE      (1<<5)   // Bulk List Enable
// CMD
#define OCMD_HCR       (1<<0)   // Host Controller Reset
// Int
#define OINT_WDH       (1<<2)   // Writeback Done Head

#define OED_SKIP       0x02
#define OED_LS         (1<<13)

typedef struct { uint32_t tbl[32]; uint32_t frm; uint32_t rsv; uint32_t dh; } __attribute__((aligned(256))) ohci_hcca_t;
typedef struct { uint32_t ctrl; uint32_t ttail; uint32_t thead; uint32_t next; } __attribute__((packed)) ohci_ed_t;
typedef struct { uint32_t ctrl; uint32_t cbuf; uint32_t next; uint32_t bend; } __attribute__((packed)) ohci_td_t;

#define OTD_CC_SHIFT 28
#define OTD_CC_MASK  0xF
#define OTD_RND      (1<<18)
#define OTD_DI_SHIFT 19
#define OTD_DI_MASK  7
#define OTD_TGL0     (0<<24)
#define OTD_TGL1     (1<<24)
#define OTD_TGL2     (2<<24)
#define OTD_TGL_MASK (3<<24)

static struct {
    uint32_t mmio;           // virtual address
    ohci_hcca_t *hcca;       uint32_t hcca_phys;
    ohci_ed_t *ed;           uint32_t ed_phys;
    ohci_td_t *td[2];        uint32_t td_phys[2];
    uint8_t *buf;            uint32_t buf_phys;
    int toggle;
    int active;
} ohci;

static inline uint32_t or32(int off) { return *(volatile uint32_t*)(ohci.mmio + off); }
static inline void ow32(int off, uint32_t v) { *(volatile uint32_t*)(ohci.mmio + off) = v; }

extern void *dma_alloc_coherent(size_t sz, uint32_t *phys);
extern void map_4k_page(uint32_t p, uint32_t v, uint32_t f);

int ohci_init(pci_dev_t *d) {
    printf("[OHCI] Found but skipped — driver incomplete\n");
    return -1;  // FIXME: implement properly
    // Read BAR0 (MMIO physical)
    extern uint32_t pci_read_config_dword(unsigned b, unsigned de, unsigned f, unsigned r);
    uint32_t mp = pci_read_config_dword(d->bus_id, d->dev_id, d->fn_id, 0x10) & ~0xFFF;
    printf("[OHCI] MMIO phys=0x%x\n", mp);
    if (!mp) return -1;

    uint32_t mv = 0xF1000000;
    map_4k_page(mp, mv, 0x3);  // Present|Write
    ohci.mmio = mv;
    printf("[OHCI] MMIO mapped to 0x%x\n", mv);

    // Reset
    ow32(OCMD, OCMD_HCR);
    for (int i=0; i<100000; i++) if (!(or32(OCMD)&OCMD_HCR)) break;
    printf("[OHCI] Reset done\n");

    // HCCA
    ohci.hcca = dma_alloc_coherent(256, &ohci.hcca_phys);
    memset(ohci.hcca, 0, 256);
    ow32(OHCCA, ohci.hcca_phys);

    // Frame interval (default 12000 frames/sec)
    ow32(OFMINT, (12000-1) | (0x2EDF<<16));
    ow32(OPRDST, 0x2A2F);

    // Go operational
    ow32(OCTRL, OCTRL_OPER | OCTRL_PLE | OCTRL_CLE | OCTRL_BLE);
    printf("[OHCI] Started, CTRL=0x%x\n", or32(OCTRL));

    ohci.active = 1;
    return 0;
}

int ohci_mouse_init(uint8_t addr, uint8_t ep, int ls) {
    if (!ohci.active) return -1;

    // ED
    ohci.ed = dma_alloc_coherent(32, &ohci.ed_phys);
    memset(ohci.ed, 0, 16);
    uint8_t en = ep & 0xF;
    ohci.ed->ctrl = (ls ? OED_LS : 0) | (addr << 7) | (en << 15) | (8 << 16);
    ohci.ed->next = 0;

    // DMA buffer
    ohci.buf = dma_alloc_coherent(8, &ohci.buf_phys);
    memset(ohci.buf, 0, 8);

    // 2 TDs
    for (int i=0; i<2; i++) {
        ohci.td[i] = dma_alloc_coherent(32, &ohci.td_phys[i]);
        memset(ohci.td[i], 0, 16);
        ohci.td[i]->cbuf = ohci.buf_phys;
        ohci.td[i]->bend = ohci.buf_phys + 7;
        ohci.td[i]->ctrl = OTD_RND | (7<<OTD_DI_SHIFT) | (i ? OTD_TGL1 : OTD_TGL0);
    }
    ohci.td[0]->next = ohci.td_phys[1];
    ohci.td[1]->next = ohci.td_phys[0];

    ohci.ed->thead = ohci.td_phys[0];
    ohci.ed->ttail = ohci.td_phys[1];

    // Insert into interrupt table, slot 0 (1ms)
    for (int i=0; i<32; i++)
        ohci.hcca->tbl[i] = ohci.ed_phys;
    ow32(OCTRL, or32(OCTRL) | OCTRL_PLE);

    ohci.toggle = 0;
    printf("[OHCI] Mouse intr: addr=%d ep=%d ed=0x%x\n", addr, en, ohci.ed_phys);
    return 0;
}

int ohci_poll(uint8_t *rpt) {
    if (!ohci.active || !ohci.ed) return 0;

    // Check donhead
    uint32_t dh = ohci.hcca->dh;
    if (!dh) return 0;

    // Find which TD completed
    for (int i=0; i<2; i++) {
        if (ohci.td_phys[i] == dh) {
            ohci_td_t *t = ohci.td[i];
            uint32_t cc = (t->ctrl >> OTD_CC_SHIFT) & OTD_CC_MASK;
            if (cc == 0) { // NoError
                int len = 3; // HID boot mouse report
                memcpy(rpt, ohci.buf, len);
                // Save to global for editor display
                extern uint8_t g_dma_bytes[8];
                for (int j=0; j<8; j++) g_dma_bytes[j] = ohci.buf[j];
                memset(ohci.buf, 0, 8);
                // Re-arm TD
                t->ctrl = OTD_RND | (7<<OTD_DI_SHIFT) | (ohci.toggle ? OTD_TGL1 : OTD_TGL0);
                t->cbuf = ohci.buf_phys;
                ohci.toggle ^= 1;
                ohci.ed->thead = ohci.td_phys[i];
                ohci.hcca->dh = 0;
                static int pc=0;
                if (++pc<=5) printf("[OHCI] POLL#%d: %02x %02x %02x\n", pc, rpt[0],rpt[1],rpt[2]);
                return len;
            }
            // Re-arm on error too
            t->ctrl = OTD_RND | (7<<OTD_DI_SHIFT) | (ohci.toggle ? OTD_TGL1 : OTD_TGL0);
            t->cbuf = ohci.buf_phys;
            ohci.ed->thead = ohci.td_phys[i];
            ohci.hcca->dh = 0;
        }
    }
    return 0;
}
