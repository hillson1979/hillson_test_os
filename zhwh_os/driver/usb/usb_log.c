/**
 * @file usb_log.c
 * @brief USB环形日志缓冲 — USB printk输出专用
 */
#include <string.h>
#include <stdarg.h>
#include <kernel/hillsonos.h>
#include <usb_log.h>
#include <mm/kheap.h>
#include <net.h>

#define USB_LOG_SZ 65536
static char  usb_buf[USB_LOG_SZ];
static int   usb_w = 0;
static char  usb_snap[USB_LOG_SZ];
static int   usb_snap_len = 0;
static int   usb_console_enabled = 0;

void usb_log_write(const char *s, int len) {
    if (len <= 0) return;
    for (int i = 0; i < len; i++) {
        usb_buf[usb_w % USB_LOG_SZ] = s[i];
        usb_w++;
    }
}

void usb_log_snapshot(void) {
    int total = (usb_w < USB_LOG_SZ) ? usb_w : USB_LOG_SZ;
    int start = (usb_w < USB_LOG_SZ) ? 0 : (usb_w % USB_LOG_SZ);
    usb_snap_len = total;
    for (int i = 0; i < total; i++)
        usb_snap[i] = usb_buf[(start + i) % USB_LOG_SZ];

    /* Preserve the complete ring contents for diagnostics export. */
    /* Reserve one byte for the terminator when the ring is full. */
    if (usb_snap_len >= USB_LOG_SZ)
        usb_snap_len = USB_LOG_SZ - 1;
    usb_snap[usb_snap_len] = 0;
}

void *usb_log_get_buf(void) { return usb_snap; }
int   usb_log_get_size(void) { return usb_snap_len; }
void  usb_log_clear(void) { usb_w = 0; }
void  usb_log_append(const char *s) { if(s) usb_log_write(s, strlen(s)); }
void usb_log_set_console(int enabled) { usb_console_enabled = enabled ? 1 : 0; }
int  usb_log_get_console(void) { return usb_console_enabled; }

net_device_t *usb_net_dev = NULL;
void usb_printk_set_dev(net_device_t *d) {
    usb_net_dev = d;
    printf("[usb_log] dev set: %s\n", d ? d->name : "NULL");
}

/* usb_printk — printf + buffer + UDP via eth0 */
int usb_printk(const char *fmt, ...) {
    char line[2048];
    extern int netdebug_vsnprintf(char *buf, int size, const char *f, va_list args);
    va_list ap;
    va_start(ap, fmt);
    int len = netdebug_vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    /* Always terminate before handing the buffer to console/VGA code. */
    line[sizeof(line) - 1] = '\0';
    if (usb_console_enabled)
        printf("%s", line);
    if (len <= 0) return len;
    /* netdebug_vsnprintf may return the would-be length on truncation. */
    if (len >= (int)sizeof(line))
        len = sizeof(line) - 1;
    usb_log_write(line, len);

    return len;
}

/* usb_log_flush_udp — batch send after USB init */
void usb_log_flush_udp(void) {
    net_device_t *dev = net_device_get("eth0");
    if (!dev) { printf("[usb_log] no eth0\n"); return; }
    usb_log_snapshot();
    char *buf = (char*)usb_log_get_buf();
    int len = usb_log_get_size();
    if (len <= 0) return;
    for (int off = 0; off < len; off += 1400) {
        int chunk = (len - off > 1400) ? 1400 : (len - off);
        udp_output(dev, 0xC0A80091, 9999, 50000, (uint8_t*)(buf+off), chunk);
    }
    printf("[usb_log] flushed %d bytes UDP via %s\n", len, dev->name);
}
