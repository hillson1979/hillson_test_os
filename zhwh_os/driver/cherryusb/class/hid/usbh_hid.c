#include <stdint.h>
#include "string.h"
#include "printf.h"
#include "usbh_hid.h"
#include "../../port/hillson_hcd.h"
static usbh_hid_device_t g_devs[USBH_HID_MAX_DEVICES];
static int g_count = 0;

int usbh_hid_set_protocol(uint8_t a, uint8_t f, uint8_t p) {
    usb_setup_packet_t s; memset(&s,0,sizeof(s));
    s.bmRequestType=0x21; s.bRequest=0x0B; s.wValue=p; s.wIndex=f;
    return usbh_port_control_xfer(a,&s,NULL);
}
int usbh_hid_register(uint8_t a, uint8_t f, uint8_t e, uint8_t m, uint8_t i, uint8_t p) {
    if(g_count>=USBH_HID_MAX_DEVICES) return -1;
    usbh_hid_device_t *d=&g_devs[g_count]; memset(d,0,sizeof(*d));
    d->dev_addr=a; d->iface=f; d->ep_in=e; d->ep_in_max=m; d->ep_in_interval=i; d->protocol=p;
    /* SET_PROTOCOL done by USB enum, skip here to avoid EHCI conflict */
    d->ready=1; printf("[CherryUSB HID] dev%d addr=%d ep=0x%02x\n",g_count,a,e);
    return g_count++;
}
int usbh_hid_read(int i, void *r, int x) {
    if(i<0||i>=g_count||!g_devs[i].ready) return -1;
    uint8_t b[8]={0}; int l=x<8?x:8;
    if(l>g_devs[i].ep_in_max) l=g_devs[i].ep_in_max;
    int n=usbh_port_intr_xfer(g_devs[i].dev_addr,g_devs[i].ep_in&0x7F,b,l);
    if(n<0)return n; memcpy(r,b,n); return n;
}
int usbh_hid_count(void){return g_count;}
