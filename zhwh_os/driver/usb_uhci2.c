/**
 * @file usb_uhci2.c — Clean UHCI for Intel 6-series companion (fn 0-2)
 */
#include <stdint.h>
#include "string.h"
#include "printf.h"
#include "pci.h"
#include "x86/io.h"

#define UHCI_USBCMD    0x00
#define UHCI_USBSTS    0x02
#define UHCI_USBINTR   0x04
#define UHCI_FRNUM     0x06
#define UHCI_FLBASEADD 0x08
#define UHCI_PORTSC1   0x10
#define UHCI_PORTSC2   0x12

#define CMD_RUN        (1<<0)
#define CMD_HCRESET    (1<<1)
#define CMD_MAXP       0x80

#define TD_TERM        0x01
#define TD_QH          0x02
#define TD_ACT         (1<<23)
#define TD_IOC         (1<<24)
#define TD_LS          (1<<26)
#define TD_SPD         (1<<29)
#define TD_CERR_SHIFT  27
#define TD_CERR_MASK   0x03

#define TOKEN_PID_SHIFT   0
#define TOKEN_ADDR_SHIFT  8
#define TOKEN_ENDP_SHIFT  15
#define TOKEN_DT_SHIFT    19
#define TOKEN_MAXLEN_SHIFT 21
#define USB_PID_IN  0x69

typedef struct { uint32_t link, ctrl, token, buf; } __attribute__((packed)) UTD;
typedef struct { uint32_t head, elem; } __attribute__((packed)) UQH;

static struct {
    uint16_t base;
    uint32_t *fl; UTD *td[2]; UQH *qh;
    uint8_t *bf; uint32_t bfp; int cur,tog,ok;
} u;

extern void *dma_alloc_coherent(unsigned int,uint32_t*);
extern void map_page(uint32_t,uint32_t,uint32_t,uint32_t);
extern uint32_t kernel_page_directory_phys;

static uint16_t ur(uint16_t r){return inw(u.base+r);}
static void uw(uint16_t r,uint16_t v){outw(u.base+r,v);}

int uhci2_init(pci_dev_t *d) {
    extern uint32_t pci_read_config_dword(unsigned,unsigned,unsigned,unsigned);
    extern void pci_write_config_dword(unsigned,unsigned,unsigned,unsigned,uint32_t);

    // Read BAR4 — UHCI uses I/O ports
    uint32_t bar4 = pci_read_config_dword(d->bus_id,d->dev_id,d->fn_id,0x20);
    if(!(bar4&1)||bar4==0xFFFFFFFF) return -1;
    u.base = bar4 & ~1;
    printf("[UHCI2] fn%d BAR4=0x%x base=0x%x\n",d->fn_id,bar4,u.base);

    // Enable PCI bus mastering
    uint32_t pci_cmd=pci_read_config_dword(d->bus_id,d->dev_id,d->fn_id,0x04);
    if(!(pci_cmd&0x06)) pci_write_config_dword(d->bus_id,d->dev_id,d->fn_id,0x04,pci_cmd|0x06);

    // Reset controller
    printf("[UHCI2] Reset...\n");
    uw(UHCI_USBCMD, CMD_HCRESET);
    for(volatile int i=0;i<10000;i++){uw(UHCI_USBSTS,0xFFFF); if(!(ur(UHCI_USBCMD)&CMD_HCRESET))break;}
    printf("[UHCI2] Reset done\n");

    // Allocate DMA with PHYSICAL addresses
    uint32_t flp=0, qhp=0, tdp0=0, tdp1=0, bfp=0;
    u.fl = dma_alloc_coherent(4096,&flp);
    u.qh = dma_alloc_coherent(32,&qhp); memset(u.qh,0,16);
    u.td[0] = dma_alloc_coherent(32,&tdp0); memset(u.td[0],0,16);
    u.td[1] = dma_alloc_coherent(32,&tdp1); memset(u.td[1],0,16);
    u.bf = dma_alloc_coherent(8,&bfp); memset(u.bf,0,8); u.bfp=bfp;

    for(int i=0;i<1024;i++) u.fl[i] = TD_TERM;
    uw(UHCI_FLBASEADD, flp & 0xFFFFF000);

    // QH element → TD0 (use physical addr)
    u.qh->head = TD_TERM;
    u.qh->elem = tdp0 | TD_QH;

    // TDs: endpoint 2 (0x82→en=2), addr=1, LS, IN, 8 bytes
    for(int i=0;i<2;i++){
        u.td[i]->link = (i==0)?(tdp1|TD_QH):TD_TERM;
        u.td[i]->ctrl = TD_ACT|TD_IOC|TD_LS|TD_SPD|(TD_CERR_MASK<<TD_CERR_SHIFT);
        u.td[i]->token = USB_PID_IN|(1<<TOKEN_ADDR_SHIFT)|(2<<TOKEN_ENDP_SHIFT)|((8-1)<<TOKEN_MAXLEN_SHIFT);
        u.td[i]->buf = bfp;
    }
    for(int i=0;i<1024;i++) u.fl[i] = qhp | TD_QH;

    // Start controller
    printf("[UHCI2] Starting...\n");
    uw(UHCI_USBCMD, CMD_RUN|CMD_MAXP);
    for(volatile int i=0;i<10000;i++) if(ur(UHCI_USBSTS)&0x20) break;
    printf("[UHCI2] Started\n");

    u.ok=1;
    printf("[UHCI2] Started at I/O 0x%x\n",u.base);
    return 0;
}

int uhci2_poll(uint8_t *r){
    if(!u.ok) return 0;
    UTD *td = u.td[u.cur];
    if(td->ctrl & TD_ACT) return 0;

    int actlen = (td->ctrl & 0x7FF) + 1;
    if(actlen < 3 || actlen > 8) actlen = 3;

    memcpy(r, u.bf, actlen);
    extern uint8_t g_dma_bytes[8];
    for(int j=0; j<8; j++) g_dma_bytes[j] = u.bf[j];
    memset(u.bf, 0, 8);

    // Re-arm with physical buffer address and endpoint 2
    u.tog ^= 1; int n = u.cur ^ 1;
    u.td[n]->ctrl = TD_ACT|TD_IOC|TD_LS|TD_SPD|(TD_CERR_MASK<<TD_CERR_SHIFT);
    u.td[n]->token = USB_PID_IN|(1<<TOKEN_ADDR_SHIFT)|(2<<TOKEN_ENDP_SHIFT)|(u.tog<<TOKEN_DT_SHIFT)|((8-1)<<TOKEN_MAXLEN_SHIFT);
    u.td[n]->buf = u.bfp;
    u.cur = n;
    u.cur = n;

    static int pc=0; if(++pc<=10) printf("[UHCI2] OK#%d %02x %02x %02x len=%d\n",pc,r[0],r[1],r[2],actlen);
    return actlen;
}
