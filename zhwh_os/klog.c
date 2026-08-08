/* klog.c — 桩。USB日志在 usb_log.c */
#include "types.h"
void klog_enable_udp(int en) { (void)en; }
void klog_set_dest_str(const char *ip, unsigned short port) { (void)ip; (void)port; }
void klog_init(void) {}
int  klog_flush(void) { return 0; }
