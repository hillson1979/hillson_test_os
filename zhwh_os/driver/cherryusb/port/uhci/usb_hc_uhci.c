/**
 * @file usb_hc_uhci.c
 * @brief CherryUSB UHCI Host Controller Driver (x86 32-bit)
 *
 * Minimal UHCI driver for QEMU piix3-usb-uhci.
 * Frame list based, TD chain for control transfers.
 */
#include <stdint.h>
#include "string.h"
#include "printf.h"
#include "usb_hc_uhci.h"
#include "x86/io.h"

static uint16_t g_io_base = 0;
uint16_t g_uhci_io = 0;  /* 公开给 port 适配层 */
static uint32_t g_frame_list[UHCI_FRAME_COUNT] __attribute__((aligned(4096)));
static uhci_td_t  g_td[4] __attribute__((aligned(16)));
static uhci_qh_t  g_qh __attribute__((aligned(16)));
static uint8_t    g_data_buf[64] __attribute__((aligned(4)));

static uint16_t rd16(uint16_t reg) { return inw(g_io_base + reg); }
static void    wr16(uint16_t reg, uint16_t v) { outw(g_io_base + reg, v); }
static void    wr32(uint16_t reg, uint32_t v) { outl(g_io_base + reg, v); }

int uhci_hc_init(uint16_t io_base) {
    g_io_base = io_base;
    g_uhci_io = io_base;
    printf("[UHCI] CherryUSB init at I/O 0x%04x\n", io_base);

    /* Reset */
    wr16(UHCI_USBCMD, UHCI_CMD_GRESET);
    for (volatile int i = 0; i < 50000; i++) __asm__("nop");
    wr16(UHCI_USBCMD, 0);

    /* Build frame list (all entries point to QH) */
    uint32_t qh_phys = (uint32_t)&g_qh;
    for (int i = 0; i < UHCI_FRAME_COUNT; i++)
        g_frame_list[i] = (qh_phys & ~0xF) | 0x2; /* QH type */

    memset(&g_qh, 0, sizeof(g_qh));
    g_qh.head_ptr = 1;    /* Terminate */
    g_qh.element_ptr = 1; /* Terminate */

    wr32(UHCI_FRBASEADD, (uint32_t)g_frame_list);
    wr16(UHCI_SOFMOD, 64); /* Full speed */
    wr16(UHCI_USBCMD, UHCI_CMD_MAXP | UHCI_CMD_CONFIGURE | UHCI_CMD_RUN);

    /* Enable port */
    uint16_t psc = rd16(UHCI_PORTSC1);
    if (psc & UHCI_PORT_CONNECT) {
        /* Reset port */
        wr16(UHCI_PORTSC1, psc | UHCI_PORT_RESET);
        for (volatile int i = 0; i < 100000; i++) __asm__("nop");
        wr16(UHCI_PORTSC1, psc & ~UHCI_PORT_RESET);
        for (volatile int i = 0; i < 50000; i++) __asm__("nop");
        wr16(UHCI_PORTSC1, rd16(UHCI_PORTSC1) | UHCI_PORT_ENABLE);
    }

    printf("[UHCI] Ready, frame list at 0x%x\n", (uint32_t)g_frame_list);
    return 0;
}

int uhci_hc_control_xfer(uint8_t addr, uint8_t ep, void *setup_pkt,
                          void *data, int len, int dir_in) {
    uint8_t *sp = (uint8_t *)setup_pkt;
    memcpy(g_data_buf, sp, 8); /* Setup packet = 8 bytes */

    /* Build 3-stage TD chain: SETUP → DATA → STATUS */
    memset(g_td, 0, sizeof(g_td));

    /* TD0: SETUP (8 bytes, OUT) */
    g_td[0].link_ptr = (uint32_t)&g_td[1] | 0x4; /* Depth-first */
    g_td[0].ctrl_status = UHCI_TD_ACTIVE | (7 << 21) | (3 << 27); /* maxlen=7, CERR=3 */
    g_td[0].token = (0x2D << 8) | (addr << 15) | (0 << 18); /* SETUP, addr, EP0 */
    g_td[0].buffer = (uint32_t)g_data_buf;

    /* TD1: DATA (IN or OUT) */
    g_td[1].link_ptr = (uint32_t)&g_td[2] | 0x4;
    g_td[1].ctrl_status = UHCI_TD_ACTIVE | ((len - 1) << 21) | (3 << 27) | UHCI_TD_IOC;
    if (dir_in) {
        g_td[1].token = (0x69 << 8) | (addr << 15) | (0 << 18) | (1 << 19); /* IN, DATA1 */
        g_td[1].buffer = (uint32_t)data;
    } else {
        g_td[1].token = (0xE1 << 8) | (addr << 15) | (0 << 18) | (1 << 19); /* OUT, DATA1 */
        if (len > 0 && data) memcpy(g_data_buf + 8, data, len);
        g_td[1].buffer = (uint32_t)(g_data_buf + 8);
    }

    /* TD2: STATUS (opposite direction) */
    g_td[2].link_ptr = 1; /* Terminate */
    g_td[2].ctrl_status = UHCI_TD_ACTIVE | (0xFF << 21) | (3 << 27) | UHCI_TD_IOC;
    g_td[2].token = ((dir_in ? 0xE1 : 0x69) << 8) | (addr << 15) | (0 << 18) | (0 << 19);
    g_td[2].buffer = 0;

    /* Link to QH and wait */
    g_qh.element_ptr = (uint32_t)&g_td[0];
    for (volatile int to = 0; to < 500000; to++) {
        if (!(g_td[2].ctrl_status & UHCI_TD_ACTIVE)) {
            if (g_td[2].ctrl_status & UHCI_TD_ERROR) return -1;
            return len;
        }
        __asm__("nop");
    }
    return -1; /* Timeout */
}

int uhci_hc_intr_xfer(uint8_t addr, uint8_t ep, void *data, int len) {
    /* Interrupt transfer — use existing UHCI2 polling */
    (void)addr; (void)ep; (void)data; (void)len;
    return -1; /* Use HillsonOS usb_mouse_periodic_poll instead */
}
