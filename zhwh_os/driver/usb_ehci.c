/**
 * @file usb_ehci.c — EHCI driver, periodic-only (no async)
 * Control transfers + interrupt IN both on periodic schedule
 */
#include <stdint.h>
#include <stdarg.h>
#include "string.h"
#include "printf.h"
#include "pci.h"
#include "usb.h"

#define OP_USBCMD       0x00
#define OP_USBSTS       0x04
#define OP_FRINDEX      0x0C
#define OP_PERIODICLIST 0x14

#define CMD_RUN  (1<<0)
#define CMD_PSE  (1<<4)
#define STS_HALT (1<<12)
#define STS_PS   (1<<4)

#define T_TERM  1
#define T_QH    (1<<1)
#define T_ACT   (1<<7)
#define T_IOC   (1<<15)
#define T_IN    (1<<8)
#define T_OUT   0
#define T_SETUP (2<<8)
#define T_CERR  (3<<10)
#define T_TOG0  0
#define T_TOG1  (1<<31)

// === QH capability bits ===
#define QH_CAP_EPS  (1<<12)
#define QH_CAP_DTC  (1<<14)
#define QH_CAP_H    (1<<15)

typedef struct { uint32_t hl,caps,caps2,cur,nxt,alt,tok,b0,b1,b2,b3,b4; } __attribute__((aligned(64))) QH;
typedef struct { uint32_t nxt,alt,tok,b0,b1,b2,b3,b4; } __attribute__((aligned(32))) TD;

static struct {
    uint32_t mmio,op;
    QH *qh;  uint32_t qhp;   // periodic QH (shared)
    TD *td[3]; uint32_t tdp[3]; // 3 TDs (0,1=mouse IN, 2=ctrl STATUS)
    uint8_t *bf; uint32_t bfp;  // buffer (first 16 bytes for mouse TD0/TD1, rest for ctrl)
    int cur,tog,ok,mouse_port,mouse_ls;
    uint32_t flp;

    QH *async_dummy, *async_real;
    uint32_t async_dummy_phys, async_real_phys;
} e;

#define OP_ASYNCLISTADDR 0x18

#define CMD_ASE  (1<<5)
#define STS_AS   (1<<5)



static int init_done=0;
static inline uint32_t orr(int o){return *(volatile uint32_t*)(e.op+o);}
static inline void orw(int o,uint32_t v){*(volatile uint32_t*)(e.op+o)=v;}
extern void *dma_alloc_coherent(unsigned int,uint32_t*);
extern void map_page(uint32_t,uint32_t,uint32_t,uint32_t);
extern uint32_t kernel_page_directory_phys;

// === ehci_log → klog (via printf) ===
char *g_ehci_debug_buf=0; int g_ehci_debug_len=0;
static int _vsnprint(char *b,int sz,const char *f,va_list va){
    int p=0;
    while(*f&&p<sz-1){if(*f!='%'){b[p++]=*f++;continue;}f++;
        switch(*f++){
        case 's':{char*s=va_arg(va,char*);if(!s)s=(char*)"?";while(*s&&p<sz-1)b[p++]=*s++;break;}
        case 'd':{int n=va_arg(va,int);if(n<0){b[p++]='-';n=-n;}char t[16];int i=0;
            if(!n)t[i++]='0';else while(n){t[i++]='0'+n%10;n/=10;}while(i)b[p++]=t[--i];break;}
        case 'x':{unsigned n=va_arg(va,unsigned);char t[16];int i=0;
            if(!n)t[i++]='0';else while(n){t[i++]="0123456789ABCDEF"[n&0xF];n>>=4;}while(i)b[p++]=t[--i];break;}
        case 'c':b[p++]=(char)va_arg(va,int);break;
        default:b[p++]='?';
        }}
    b[p]=0;return p;
}
void ehci_log(const char*f,...){
    char b[256]; va_list v; va_start(v,f);
    _vsnprint(b,sizeof(b),f,v); va_end(v);
    printf("%s\n",b);
}

// === EHCI enumeration display buffer (F1 screen) ===
#define EHCI_DISPLAY_SZ 8192
static char g_ehci_display[EHCI_DISPLAY_SZ];
static int  g_ehci_display_len = 0;

void ehci_display_clear(void) {
    g_ehci_display_len = 0;
    g_ehci_display[0] = 0;
}

void ehci_display_append(const char *s) {
    int i = 0;
    while (s[i] && g_ehci_display_len < EHCI_DISPLAY_SZ - 1) {
        g_ehci_display[g_ehci_display_len++] = s[i++];
    }
    g_ehci_display[g_ehci_display_len] = 0;
}

void ehci_display_hex(const uint8_t *data, int len) {
    for (int i = 0; i < len && g_ehci_display_len < EHCI_DISPLAY_SZ - 4; i++) {
        uint8_t hi = (data[i] >> 4) & 0xF;
        uint8_t lo = data[i] & 0xF;
        g_ehci_display[g_ehci_display_len++] = hi < 10 ? '0' + hi : 'A' + hi - 10;
        g_ehci_display[g_ehci_display_len++] = lo < 10 ? '0' + lo : 'A' + lo - 10;
        g_ehci_display[g_ehci_display_len++] = ' ';
    }
    g_ehci_display[g_ehci_display_len] = 0;
}

// Convenience: append a formatted string
void ehci_display_sprintf(const char *fmt, ...) {
    char tmp[256];
    va_list v;
    va_start(v, fmt);
    int n = _vsnprint(tmp, sizeof(tmp), fmt, v);
    va_end(v);
    ehci_display_append(tmp);
}

const char *ehci_display_get(void) { return g_ehci_display; }

static void dump_qh(const char*t,QH*q,uint32_t p){
    volatile uint32_t*w=(volatile uint32_t*)q;
    ehci_log("[EHCI] %s QH v=0x%x p=0x%x",t,(uint32_t)q,p);
    for(int i=0;i<12;i++)ehci_log("[EHCI] Q[%d]=0x%x",i,w[i]);
    ehci_log("[EHCI] caps: A=%d E=%d S=%d D=%d H=%d M=%d",w[1]&0x7F,(w[1]>>8)&0xF,!!(w[1]&0x1000),!!(w[1]&0x4000),!!(w[1]&0x8000),(w[1]>>16)&0xF);
    ehci_log("[EHCI] caps2: S=0x%x C=0x%x Hub=%d Port=%d",w[2]&0xFF,(w[2]>>8)&0xFF,(w[2]>>16)&0x7F,(w[2]>>23)&0x7F);
    ehci_log("[EHCI] ovl: tok=0x%x ACT=%d HALT=%d PID=%d CERR=%d Total=%d TOG=%d",
        w[6],!!(w[6]&0x80),!!(w[6]&0x40),(w[6]>>8)&3,(w[6]>>10)&3,(w[6]>>16)&0x7FFF,w[6]>>31);
}



// === Periodic-only control transfer ===
// Uses e.qh + e.td[0..2]. Saves/restores nothing — caller reconfigures after.
static int ehci_control_transfer_async(uint8_t dev, uint8_t *setup8, uint8_t *data, int dlen, int is_in) {
    int cmask = e.mouse_ls ? 0x1C : 0x04;
    // Config QH for this device
    //e.qh->hl    = T_TERM | (1<<15);
    //e.qh->hl = T_TERM ; // periodic QH: terminate list
    uint32_t qh_phys = e.qhp;

    e.qh->hl = qh_phys | T_QH;
    e.qh->caps  = (8<<16) | (1<<12) | (0<<8) | (dev & 0x7f); // EPS=FS/LS, DTC=1, H=1
    e.qh->caps2 = (e.mouse_port<<23) | (cmask<<8) ;
    e.qh->cur   = 0; e.qh->alt = T_TERM; e.qh->tok = 0;
    // SETUP TD
    e.td[0]->nxt=T_TERM; e.td[0]->alt=T_TERM;
    e.td[0]->tok=T_ACT|T_SETUP|T_CERR|(8<<16)|T_TOG0;
    e.td[0]->b0=e.bfp; e.td[0]->b1=e.td[0]->b2=e.td[0]->b3=e.td[0]->b4=0;
    memcpy(e.bf, setup8, 8);
    if(dlen>0){
        // DATA TD
        if(!is_in) memcpy(e.bf+8, data, dlen);
        e.td[1]->nxt=T_TERM; e.td[1]->alt=T_TERM;
        e.td[1]->tok=T_ACT|(is_in?T_IN:T_OUT)|T_CERR|(dlen<<16)|T_TOG1;
        e.td[1]->b0=e.bfp+8; e.td[1]->b1=e.td[1]->b2=e.td[1]->b3=e.td[1]->b4=0;
        e.td[0]->nxt=e.tdp[1];
        // STATUS TD (opposite dir)
        e.td[2]->nxt=T_TERM; e.td[2]->alt=T_TERM;
        e.td[2]->tok=T_ACT|(is_in?T_OUT:T_IN)|T_CERR|(0<<16)|T_TOG1;
        e.td[2]->b0=0; e.td[2]->b1=e.td[2]->b2=e.td[2]->b3=e.td[2]->b4=0;
        e.td[1]->nxt=e.tdp[2];
    } else {
        // No data phase: STATUS IN
        e.td[1]->nxt=T_TERM; e.td[1]->alt=T_TERM;
        e.td[1]->tok=T_ACT|T_IN|T_CERR|(0<<16)|T_TOG1;
        e.td[1]->b0=0; e.td[1]->b1=e.td[1]->b2=e.td[1]->b3=e.td[1]->b4=0;
        e.td[0]->nxt=e.tdp[1];
    }
    e.qh->nxt=e.tdp[0]&~0x1F;
    asm volatile("mfence":::"memory");
    // Wait for last TD (STATUS or DATA if no data phase)
    TD *last = (dlen>0) ? e.td[2] : e.td[1];
    int r=-1;
    for(int t=0;t<50000;t++){
        if(!(last->tok&T_ACT)){
            int halt=!!(last->tok&0x40);
            if(!halt&&is_in&&dlen>0) memcpy(data,e.bf+8,dlen);
            r=halt?-1:dlen;
            ehci_log("[EHCI] ctrl: dev=%d %s r=%d tok=0x%x",dev,is_in?"IN":"OUT",r,last->tok);
            break;
        }
        for(volatile int d=0;d<50;d++);
    }
    if(r<0) ehci_log("[EHCI] ctrl timeout tok=0x%x",last->tok);
    ehci_log(
            "CTRL dev=%d QH=%x TD0=%x TD1=%x TD2=%x",
            dev,
            e.qh->caps,
            e.td[0]->tok,
            e.td[1]->tok,
            e.td[2]->tok);
    return r;
}
// === Public API: periodic-only control transfer ===
// 自动构造 SETUP + DATA + STATUS，使用 periodic QH
int ehci_control_transfer_periodic(
        uint8_t addr,
        uint8_t bmRequestType,
        uint8_t bRequest,
        uint16_t wValue,
        uint16_t wIndex,
        uint16_t wLength,
        void *buf)
{
    uint8_t setup[8];
    setup[0] = bmRequestType;
    setup[1] = bRequest;
    setup[2] = wValue & 0xFF;
    setup[3] = wValue >> 8;
    setup[4] = wIndex & 0xFF;
    setup[5] = wIndex >> 8;
    setup[6] = wLength & 0xFF;
    setup[7] = wLength >> 8;

    int is_in = bmRequestType & 0x80;

    // 使用你已经验证可工作的 ctrl_xfer_periodic()
    return ehci_control_transfer_async(
        addr,
        setup,
        buf,
        wLength,
        is_in
    );
}



int ehci_control_transfer(uint8_t addr, uint8_t bmRequestType,
                          uint8_t bRequest, uint16_t wValue,
                          uint16_t wIndex, uint16_t wLength,
                          void *buf)
{
    // --- Build SETUP packet ---
    uint32_t setup_phys, data_phys, status_phys;
    TD *td_setup  = dma_alloc_coherent(sizeof(TD), &setup_phys);
    TD *td_data   = dma_alloc_coherent(sizeof(TD), &data_phys);
    TD *td_status = dma_alloc_coherent(sizeof(TD), &status_phys);

    uint32_t setup_bfp, data_bfp;
    uint8_t *setup = dma_alloc_coherent(8, &setup_bfp);
    uint8_t *data  = dma_alloc_coherent(wLength ? wLength : 8, &data_bfp);

    setup[0] = bmRequestType;
    setup[1] = bRequest;
    setup[2] = wValue & 0xFF;
    setup[3] = wValue >> 8;
    setup[4] = wIndex & 0xFF;
    setup[5] = wIndex >> 8;
    setup[6] = wLength & 0xFF;
    setup[7] = wLength >> 8;

    int is_in = bmRequestType & 0x80;

    if (!is_in && wLength)
        memcpy(data, buf, wLength);

    // --- SETUP TD ---
    memset(td_setup, 0, sizeof(TD));
    td_setup->nxt = data_phys & ~0x1F;
    td_setup->alt = T_TERM;
    td_setup->tok = T_ACT | T_SETUP | T_CERR | (8 << 16) | T_TOG0;
    td_setup->b0  = setup_bfp;

    // --- DATA TD ---
    memset(td_data, 0, sizeof(TD));
    td_data->nxt = status_phys & ~0x1F;
    td_data->alt = T_TERM;
    td_data->tok = T_ACT |
                   (is_in ? T_IN : T_OUT) |
                   T_CERR |
                   (wLength << 16) |
                   T_TOG1;
    td_data->b0  = data_bfp;

    // --- STATUS TD ---
    memset(td_status, 0, sizeof(TD));
    td_status->nxt = T_TERM;
    td_status->alt = T_TERM;
    td_status->tok = T_ACT |
                     T_IOC |
                     (is_in ? T_OUT : T_IN) |
                     (0 << 16) |
                     T_TOG1;
    td_status->b0  = 0;

    // --- QH overlay ---
    QH *qh = e.async_real;
    qh->nxt = setup_phys & ~0x1F;
    qh->alt = T_TERM;
    qh->tok = 0;
    qh->caps  = (8 << 16) | addr | QH_CAP_DTC;
    qh->caps2 = 0;

    asm volatile("mfence");

    // --- Wait for STATUS TD ---
    int timeout = 2000000;
    while (timeout--) {
        if (!(td_status->tok & T_ACT))
            break;
    }

    if (td_status->tok & T_ACT)
        return -1;

    if (td_status->tok & 0x40)
        return -1;

    if (is_in && wLength)
        memcpy(buf, data, wLength);

    return 0;
}

void dump_usb_config(uint8_t *p, int len)
{
    int pos = 0;

    while(pos < len)
    {
        uint8_t l = p[pos];
        uint8_t t = p[pos+1];

        if(l == 0)
            break;


        if(t == 0x04)   // Interface Descriptor
        {
            ehci_log(
                "[USB] Interface class=%x subclass=%x protocol=%x",
                p[pos+5],
                p[pos+6],
                p[pos+7]
            );
        }


        if(t == 0x05)   // Endpoint Descriptor
        {
            uint8_t ep =
                p[pos+2];

            uint8_t attr =
                p[pos+3];

            uint16_t maxpkt =
                p[pos+4] |
                (p[pos+5]<<8);

            uint8_t interval =
                p[pos+6];


            ehci_log(
                "[USB] EP addr=0x%x attr=0x%x maxpkt=%d interval=%d",
                ep,
                attr,
                maxpkt,
                interval
            );
        }


        pos += l;
    }
}
static int enumerate_mouse(void) {
    uint8_t buf[64]; int r;

    // Clear F1 display buffer and write header
    ehci_display_clear();
    ehci_display_append("=== EHCI MOUSE ENUM ===\n");
    ehci_display_sprintf("port=%d ls=%d\n", e.mouse_port, e.mouse_ls);

    ehci_log("[EHCI] Port reset...");
    ehci_port_reset(e.mouse_port);
    ehci_log("[EHCI] Reset done");

    // GET_DESCRIPTOR(Device, 8 bytes, addr 0)
    r = ehci_control_transfer_periodic(
            0, 0x80, 0x06, 0x0100, 0x0000, 8, buf);
    ehci_log("[EHCI] GET_DESC8: r=%d", r);
    ehci_display_sprintf("GET_DESC8 r=%d\n", r);
    if (r > 0) {
        ehci_display_append("  DATA: "); ehci_display_hex(buf, r<8?r:8); ehci_display_append("\n");
    } else {
        ehci_display_append("  FAILED\n");
    }

    // SET_ADDRESS (addr 0 -> 1)
    r = ehci_control_transfer_periodic(
            0, 0x00, 0x05, 0x0001, 0x0000, 0, NULL);
    ehci_log("[EHCI] SET_ADDR: r=%d", r);
    ehci_display_sprintf("SET_ADDR r=%d\n", r);
    for (volatile int i=0;i<10000;i++);

    // GET_DESCRIPTOR(Device, 18 bytes, addr 1)
    r = ehci_control_transfer_periodic(
            0, 0x80, 0x06, 0x0100, 0x0000, 18, buf);
    ehci_log("[EHCI] GET_DESC18: r=%d", r);
    ehci_display_sprintf("GET_DESC18 r=%d\n", r);
    if (r > 0) {
        ehci_display_append("  DATA: "); ehci_display_hex(buf, r<18?r:18); ehci_display_append("\n");
    } else {
        ehci_display_append("  FAILED\n");
    }

    // GET CONFIGURATION descriptor
    uint8_t cfg[256];
    r = ehci_control_transfer_periodic(
            1, 0x80, 0x06, 0x0200, 0x0000, 9, cfg);
    ehci_log("[EHCI] GET_CFG9: r=%d", r);
    ehci_display_sprintf("GET_CFG9 r=%d\n", r);
    if (r > 0) {
        ehci_display_append("  DATA: "); ehci_display_hex(cfg, r<9?r:9); ehci_display_append("\n");
        ehci_display_sprintf("  wTotalLength=%d bNumIf=%d bmAttr=%02x bMaxPower=%d\n",
            cfg[2]|(cfg[3]<<8), cfg[4], cfg[7], cfg[8]);
    } else {
        ehci_display_append("  FAILED\n");
    }

    if(r > 0) {
        uint16_t total = cfg[2] | (cfg[3] << 8);
        ehci_log("[USB] config total length=%d", total);
        ehci_display_sprintf("GET_CFG_FULL total=%d\n", total);

        r = ehci_control_transfer_periodic(
                1, 0x80, 0x06, 0x0200, 0x0000, total, cfg);
        ehci_log("[EHCI] GET_CFG_FULL: r=%d", r);
        ehci_display_sprintf("GET_CFG_FULL r=%d\n", r);
        if(r > 0) {
            ehci_display_append("  DATA: "); ehci_display_hex(cfg, r<256?r:256); ehci_display_append("\n");
            dump_usb_config(cfg, r);
        } else {
            ehci_display_append("  FAILED\n");
        }
    }

    // SET_CONFIGURATION(1)
    r = ehci_control_transfer_periodic(
            1, 0x00, 0x09, 0x0001, 0x0000, 0, NULL);
    ehci_log("[EHCI] SET_CFG: r=%d", r);
    ehci_display_sprintf("SET_CFG r=%d\n", r);

    // SET_PROTOCOL(boot)
    r = ehci_control_transfer_periodic(
            1, 0x21, 0x0B, 0x0000, 0x0000, 0, NULL);
    ehci_log("[EHCI] SET_PROTO: r=%d", r);
    ehci_display_sprintf("SET_PROTO r=%d\n", r);

    // SET_IDLE
    r = ehci_control_transfer_periodic(
            1, 0x21, 0x0A, 0x0000, 0x0000, 0, NULL);
    ehci_log("[EHCI] SET_IDLE: r=%d", r);
    ehci_display_sprintf("SET_IDLE r=%d\n", r);

    ehci_display_append("=== ENUM DONE ===\n");
    return 0;
}

// 建立最小 async list：dummy_qh <-> e.qh(real_qh)
void ehci_async_init()
{
    uint32_t dqp;

    // 分配 dummy QH
    QH *dq = dma_alloc_coherent(sizeof(QH), &dqp);
    memset(dq, 0, sizeof(QH));


    /*
     * 使用已经分配好的 e.qh 作为 async real QH
     *
     * e.qh:
     *      phys = e.qhp
     */
    uint32_t rqp = e.qhp;
    QH *rq = e.qh;


    memset(rq, 0, sizeof(QH));


    /*
     * dummy -> real
     */
    dq->hl = (rqp & ~0x1F) | T_QH;


    /*
     * real -> dummy
     */
    rq->hl = (dqp & ~0x1F) | T_QH;


    /*
     * 初始化 real QH
     * 让它成为 async head
     */
    rq->cur = 0;
    rq->alt = T_TERM;
    rq->tok = 0;


    /*
     * Async List Address 指向 dummy
     */
    orw(OP_ASYNCLISTADDR, dqp & ~0x1F);


    /*
     * 开启 async schedule
     */
    uint32_t cmd = orr(OP_USBCMD);

    cmd |= CMD_RUN | CMD_ASE;

    orw(OP_USBCMD, cmd);


    /*
     * 等待 Async Schedule Active
     */
    int active = 0;

    for(int i=0;i<100000;i++)
    {
        if(orr(OP_USBSTS) & STS_AS)
        {
            active = 1;
            break;
        }
    }


    e.async_dummy = dq;
    e.async_real  = rq;

    e.async_dummy_phys = dqp;
    e.async_real_phys  = rqp;


    ehci_log(
        "[EHCI] async list: dummy=0x%x real(e.qh)=0x%x AS=%d",
        dqp,
        rqp,
        active
    );
}


// === Init ===
int ehci_init(pci_dev_t *d) {
    if(init_done) return -1; init_done=1;
    extern uint32_t pci_read_config_dword(unsigned,unsigned,unsigned,unsigned);
    extern void pci_write_config_dword(unsigned,unsigned,unsigned,unsigned,uint32_t);
    ehci_log("[EHCI] Init");

    uint32_t pc=pci_read_config_dword(d->bus_id,d->dev_id,d->fn_id,0x04);
    if(!(pc&0x06)) pci_write_config_dword(d->bus_id,d->dev_id,d->fn_id,0x04,pc|0x06);

    uint32_t bar0=pci_read_config_dword(d->bus_id,d->dev_id,d->fn_id,0x10);
    uint32_t mp=bar0&~0xFFF;
    if(!mp||mp==0xFFFFFFFF){ehci_log("[EHCI] BAR0 bad");return -1;}

    uint32_t sup=pci_read_config_dword(d->bus_id,d->dev_id,d->fn_id,0xC0);
    if(sup&1){
        uint32_t sctl=pci_read_config_dword(d->bus_id,d->dev_id,d->fn_id,0xC4);
        sctl&=~0x000F; sup&=~1; sup|=(1<<16);
        pci_write_config_dword(d->bus_id,d->dev_id,d->fn_id,0xC4,sctl);
        pci_write_config_dword(d->bus_id,d->dev_id,d->fn_id,0xC0,sup);
        for(volatile int i=0;i<10000;i++){sup=pci_read_config_dword(d->bus_id,d->dev_id,d->fn_id,0xC0);if(!(sup&1))break;}
        ehci_log("[EHCI] SMI cleared");
    }

    uint32_t mv=0xD0000000;
    for(int i=0;i<2;i++) map_page(kernel_page_directory_phys,mv+i*0x1000,mp+i*0x1000,0x13);
    uint8_t cl=*(volatile uint8_t*)(mv+0); e.mmio=mv; e.op=mv+cl;
    ehci_log("[EHCI] MMIO=0x%x Op=0x%x",mv,mv+cl);

    int np=orr(0x04)&0xF,found=0;
    for(int i=0;i<np;i++){
        uint32_t ps=*(volatile uint32_t*)(e.op+0x44+i*4);
        int conn=ps&1,en=!!(ps&4),owner=(ps>>13)&1,ls=!!(ps&0x100);
        ehci_log("[EHCI] P%d: c=%d e=%d o=%d l=%d",i+1,conn,en,owner,ls);
        if(conn){
            extern uint32_t g_ehci_cmd;
            g_ehci_cmd=(ls<<3)|(owner<<2)|(en<<1)|conn|((i+1)<<24);
            if(owner){ps&=~(1<<13);*(volatile uint32_t*)(e.op+0x44+i*4)=ps;}
            if(!found){found=i+1;e.mouse_ls=ls;}
        }
    }
    e.mouse_port=found?found:1;
    extern int g_ehci_mouse_port;
    g_ehci_mouse_port = e.mouse_port;
    ehci_log("[EHCI] port=%d ls=%d",e.mouse_port,e.mouse_ls);

    // DMA: FL + 1 QH + 3 TD + 1 buffer (64 bytes for ctrl data)
    uint32_t frp; uint8_t *fr=dma_alloc_coherent(8192,&frp);
    uint32_t flp=(frp+0xFFF)&~0xFFF; e.flp=flp;
    uint32_t *fl=(uint32_t*)(fr+(flp-frp));
    memset(fl,0,4096);
    e.qh=dma_alloc_coherent(sizeof(QH),&e.qhp);
    e.td[0]=dma_alloc_coherent(sizeof(TD),&e.tdp[0]);
    e.td[1]=dma_alloc_coherent(sizeof(TD),&e.tdp[1]);
    e.td[2]=dma_alloc_coherent(sizeof(TD),&e.tdp[2]);


    e.bf=dma_alloc_coherent(64,&e.bfp);
    if(!e.qh||!e.td[0]||!e.td[1]||!e.td[2]||!e.bf){ehci_log("[EHCI] DMA fail");return -1;}
    ehci_log("[EHCI] qhp=0x%x tdp=0x%x,0x%x,0x%x bfp=0x%x",e.qhp,e.tdp[0],e.tdp[1],e.tdp[2],e.bfp);

    // 建立最小 async list
    ehci_async_init();

    // Frame list → QH
    for(int i=0;i<1024;i++) fl[i]=T_QH|(e.qhp&~0x1F);

    

    // Setup: stop → PERIODICLISTBASE → CONFIGFLAG → start with PSE
    uint32_t cmd=orr(OP_USBCMD);
    if(cmd&CMD_RUN){cmd&=~CMD_RUN;orw(OP_USBCMD,cmd);for(volatile int i=0;i<10000;i++)if(orr(OP_USBSTS)&STS_HALT)break;}
    orw(OP_PERIODICLIST, flp);
    orw(0x40,1);
    cmd=orr(OP_USBCMD);
    cmd&=~(3<<2);cmd|=(0<<2);
    cmd|=CMD_RUN|CMD_PSE|CMD_ASE;
    orw(OP_USBCMD,cmd);
    for(volatile int i=0;i<10000;i++)if(!(orr(OP_USBSTS)&STS_HALT))break;
    uint32_t s=orr(OP_USBSTS); uint32_t fr0=orr(OP_FRINDEX);
    for(volatile int i=0;i<200000;i++); // wait ~2ms for FRINDEX change
    uint32_t fr1=orr(OP_FRINDEX);
    ehci_log("[EHCI] STS=0x%x PS=%d HALT=%d FR=%x->%x %s",s,!!(s&STS_PS),!!(s&STS_HALT),fr0,fr1,(fr0!=fr1)?"RUN":"STUCK");

    
    extern uint32_t g_ehci_qh_phys,g_ehci_cmd,g_ehci_sts;
    g_ehci_qh_phys=e.qhp; g_ehci_cmd=orr(OP_USBCMD); g_ehci_sts=s;

    

    e.ok=1; return 0;
}

// === Mouse setup — do enumeration then configure QH for IN polling ===
int ehci_mouse_setup(uint8_t addr, uint8_t ep, int ls) {
    if(!e.ok) return -1;
    // Enumerate
    enumerate_mouse();
    // Now reconfigure QH for interrupt IN (addr=1 from enum, or caller's addr)
    uint8_t en=ep&0xF; ls=e.mouse_ls;
    int cmask=ls?0x1C:0x04;
    uint8_t dev_addr=1; // after SET_ADDRESS
    //e.qh->hl=T_TERM|(1<<15);
    //e.qh->hl = T_TERM ; // periodic QH: terminate list
    // 保留 async ring
    e.qh->hl = (e.async_dummy_phys & ~0x1F) | T_QH;

    //e.qh->caps=(8<<16)|(1<<12)|(0<<8)|(dev_addr & 0x7f);
    uint8_t epnum = ep & 0x0f;

    e.qh->caps =
          (dev_addr & 0x7f)
        | (epnum << 8)
        | (1<<12)
        | (8<<16);
    //e.qh->caps2=(e.mouse_port<<23)|(cmask<<8);
    e.qh->caps2 =
          (e.mouse_port << 23)
        | 0x01
        | (0x1c << 8);
    e.qh->cur=0; e.qh->alt=T_TERM;// e.qh->tok=0;

    // Set up 2 TDs for alternating IN polling (TD0, TD1)
    // for(int i=0;i<2;i++){
    //     e.td[i]->nxt=T_TERM; e.td[i]->alt=T_TERM;
    //     e.td[i]->tok=T_ACT|T_IOC|T_IN|T_CERR|(8<<16)|T_TOG0;
    //     e.td[i]->b0=e.bfp+(i*8);
    //     e.td[i]->b1=e.td[i]->b2=e.td[i]->b3=e.td[i]->b4=0;
    // }
    // TD0 <-> TD1 circular interrupt queue

    for(int i=0;i<2;i++)
    {
        // e.td[i]->alt=T_TERM;

        // e.td[i]->tok =
        //       T_ACT
        //     | T_IOC
        //     | T_IN
        //     | T_CERR
        //     | (8<<16)
        //     |T_TOG0;
        //     //| ((i==0)?T_TOG0:T_TOG1);

        // e.td[i]->b0=e.bfp+(i*8);

        // e.td[i]->b1=0;
        // e.td[i]->b2=0;
        // e.td[i]->b3=0;
        // e.td[i]->b4=0;
        e.qh->nxt=e.tdp[0]&~0x1f;


        e.td[0]->nxt=T_TERM;
        e.td[0]->alt=T_TERM;

        e.td[0]->tok=
              T_ACT
            | T_IOC
            | T_IN
            | T_CERR
            | (8<<16)
            | T_TOG0;

        e.td[0]->b0=e.bfp;
    }


    // TD0 -> TD1
    // e.td[0]->nxt =
    //     e.tdp[1] & ~0x1F;


    // // TD1 -> TD0
    // e.td[1]->nxt =
    //     e.tdp[0] & ~0x1F;

    // e.qh->nxt=e.tdp[0]&~0x1F;
    // 单 TD 测试
    e.td[0]->nxt = T_TERM;
    e.td[0]->alt = T_TERM;

    e.td[0]->tok =
          T_ACT
        | T_IOC
        | T_IN
        | T_CERR
        | (8 << 16)
        | T_TOG0;

    e.td[0]->b0 = e.bfp;
    e.td[0]->b1 = 0;
    e.td[0]->b2 = 0;
    e.td[0]->b3 = 0;
    e.td[0]->b4 = 0;


e.qh->nxt = e.tdp[0] & ~0x1F;
    e.cur=0; e.tog=0;
    asm volatile("mfence":::"memory");

    // No stop/start — just let HC pick up QH changes on next microframe
    asm volatile("mfence":::"memory");
    uint32_t s=orr(OP_USBSTS); uint32_t fr0=orr(OP_FRINDEX);
    for(volatile int i=0;i<200000;i++);
    uint32_t fr1=orr(OP_FRINDEX);
    ehci_log("[EHCI] MSETUP: caps=0x%x caps2=0x%x STS=0x%x CMD=0x%x FR=%x->%x %s",
        e.qh->caps,e.qh->caps2,s,orr(OP_USBCMD),fr0,fr1,(fr0!=fr1)?"RUN":"STUCK");
    dump_qh("MSETUP",e.qh,e.qhp);
    return 0;
}

// === Poll ===
int ehci_poll(uint8_t *r) {
    if(!e.ok) return 0;
    TD *td=e.td[e.cur];
    uint32_t sts=orr(OP_USBSTS); static int dc=0; dc++;
    if(dc==1){ehci_log("[EHCI] POLL1: STS=0x%x CMD=0x%x FR=0x%x",sts,orr(OP_USBCMD),orr(OP_FRINDEX));dump_qh("POLL1",e.qh,e.qhp);}
    if(dc<=5)ehci_log("[EHCI] POLL#%d STS=0x%x PS=%d tok=0x%x ACT=%d",dc,sts,!!(sts&STS_PS),td->tok,!!(td->tok&T_ACT));
    extern uint32_t g_ehci_fl_phys; g_ehci_fl_phys=td->tok;
    if(td->tok&T_ACT) return 0;
    // TD completed
    int len=(td->tok>>16)&0x7FFF; if(len>8)len=8; if(len<3)len=3;
    memcpy(r,e.bf+(e.cur*8),len);
    extern uint8_t g_dma_bytes[8]; for(int j=0;j<8;j++)g_dma_bytes[j]=e.bf[(e.cur*8)+j];
    static int hc=0;
    if(++hc<=20)ehci_log("[EHCI] DATA#%d: %x %x %x %x len=%d",hc,e.bf[e.cur*8],e.bf[e.cur*8+1],e.bf[e.cur*8+2],e.bf[e.cur*8+3],len);
    // Re-arm: toggle + switch to other TD
    memset(e.bf+(e.cur*8),0,len);
    e.tog^=1; int n=e.cur^1;
    e.td[n]->tok=T_ACT|T_IOC|T_IN|T_CERR|(8<<16)|(e.tog?T_TOG1:T_TOG0);
    e.td[n]->b0=e.bfp+(n*8);
    e.qh->nxt=e.tdp[n]&~0x1F; e.qh->cur=0;
    e.cur=n;
    asm volatile("mfence":::"memory");
    return len;
}

// Port reset helper
void ehci_port_reset(int port) {
    uint32_t off=0x44+(port-1)*4;
    uint32_t ps=*(volatile uint32_t*)(e.op+off);
    ps|=(1<<8); *(volatile uint32_t*)(e.op+off)=ps;
    for(volatile int i=0;i<100000;i++){ps=*(volatile uint32_t*)(e.op+off);if(!(ps&(1<<8)))break;}
}
