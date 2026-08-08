#ifndef USB_LOG_H
#define USB_LOG_H
void usb_log_clear(void);
void usb_log_append(const char *s);
void usb_log_set_console(int enabled);
int  usb_log_get_console(void);
void usb_log_snapshot(void);
void *usb_log_get_buf(void);
int  usb_log_get_size(void);
#endif
