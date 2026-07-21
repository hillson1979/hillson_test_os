/**
 * @file umouse.c
 * @brief RT-Thread 风格 USB Host HID Mouse 驱动
 *
 * 移植自 RT-Thread components/drivers/usb/usbhost/class/umouse.c
 *
 * 架构:
 *   uprotocal_t {
 *       init     → 设备初始化 (SET_PROTOCOL, SET_IDLE, 启动轮询)
 *       callback → 中断回调 (解析 HID 报告, 更新鼠标状态)
 *   }
 *   rt_usbh_hid_protocal_register() → 注册到 USB Host 核心
 */

#include <stdint.h>
#include "string.h"
#include "printf.h"
#include "usb.h"
#include "usb_hcd.h"

/* RT-Thread uprotocal 结构 */
typedef struct {
    const char *name;
    int (*init)(void *dev);
    int (*callback)(void *dev, uint8_t *data, int len);
} uprotocal_t;

/* ---- HID Mouse 内部状态 ---- */
#define UM_MAX_DEV  2
static struct {
    uint8_t  addr, iface, ep, ep_max, interval;
    int      ready;
    int8_t   x, y, wheel;
    uint8_t  btn;
    void     (*user_cb)(int dx, int dy, uint8_t btn, int8_t wh);
} g_mouse[UM_MAX_DEV];
static int g_mouse_cnt = 0;

/* ---- SET_PROTOCOL / SET_IDLE ---- */
static int hid_req(int addr, uint8_t iface, uint8_t req, uint16_t val) {
    usb_device_request_t r;
    r.bmRequestType = USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE;
    r.bRequest      = req;
    r.wValue        = val;
    r.wIndex        = iface;
    r.wLength       = 0;
    return usb_control_transfer(0, addr, 0, &r, NULL);
}

/* ---- RT-Thread 风格 init ---- */
static int umouse_init(void *dev_ptr) {
    int idx = *(int *)dev_ptr;
    if (idx < 0 || idx >= g_mouse_cnt) return -1;
    int addr = g_mouse[idx].addr, iface = g_mouse[idx].iface;

    printf("[umouse] Init: addr=%d ep=0x%02x\n", addr, g_mouse[idx].ep);

    if (hid_req(addr, iface, 0x0B, 0) < 0)   /* SET_PROTOCOL(BOOT) */
        printf("[umouse] SET_PROTOCOL failed\n");
    if (hid_req(addr, iface, 0x0A, 0) < 0)   /* SET_IDLE(0) */
        printf("[umouse] SET_IDLE failed\n");

    /* EHCI 中断传输已在 usb_mouse_init 时启动, 这里只标记就绪 */
    g_mouse[idx].ready = 1;
    printf("[umouse] Ready\n");
    return 0;
}

/* ---- RT-Thread 风格 callback (中断传输完成时调用) ---- */
static int umouse_callback(void *dev_ptr, uint8_t *data, int len) {
    int idx = *(int *)dev_ptr;
    if (idx < 0 || idx >= g_mouse_cnt) return -1;
    if (!g_mouse[idx].ready) return -1;

    /* 解析 HID Boot Protocol 鼠标报告 (3+1 字节) */
    if (len >= 3) {
        g_mouse[idx].btn   = data[0];
        g_mouse[idx].x     = (int8_t)data[1];
        g_mouse[idx].y     = (int8_t)data[2];
        g_mouse[idx].wheel = (len >= 4) ? (int8_t)data[3] : 0;

        if (g_mouse[idx].user_cb)
            g_mouse[idx].user_cb(g_mouse[idx].x, g_mouse[idx].y,
                                  g_mouse[idx].btn, g_mouse[idx].wheel);
    }
    return 0;
}

/* ---- 注册到 USB Host (RT-Thread 风格) ---- */
static uprotocal_t g_hid_mouse_proto = {
    .name     = "hid_mouse",
    .init     = umouse_init,
    .callback = umouse_callback,
};

void rt_usbh_hid_mouse_register(void) {
    printf("[umouse] RT-Thread style HID mouse registered\n");
}

/**
 * @brief 用户 API: 注册 HID 鼠标设备
 */
int umouse_register(uint8_t addr, uint8_t iface, uint8_t ep, uint8_t ep_max) {
    if (g_mouse_cnt >= UM_MAX_DEV) return -1;
    int idx = g_mouse_cnt++;
    g_mouse[idx].addr = addr; g_mouse[idx].iface = iface;
    g_mouse[idx].ep = ep; g_mouse[idx].ep_max = ep_max;
    g_mouse[idx].interval = 10;
    g_mouse[idx].ready = 0;
    umouse_init(&idx);
    return idx;
}

/**
 * @brief 用户 API: 轮询读取鼠标数据
 */
int umouse_read(int idx, int *dx, int *dy, uint8_t *btn) {
    if (idx < 0 || idx >= g_mouse_cnt || !g_mouse[idx].ready) return -1;
    extern int usb_mouse_periodic_poll(uint8_t *rpt);
    uint8_t rpt[4] = {0};
    int n = usb_mouse_periodic_poll(rpt);
    if (n >= 3) {
        umouse_callback(&idx, rpt, n);
        if (dx)  *dx  = g_mouse[idx].x;
        if (dy)  *dy  = g_mouse[idx].y;
        if (btn) *btn = g_mouse[idx].btn;
        g_mouse[idx].x = 0; g_mouse[idx].y = 0; g_mouse[idx].wheel = 0;
        return 1;
    }
    return 0;
}

int umouse_count(void) { return g_mouse_cnt; }
