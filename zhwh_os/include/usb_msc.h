#ifndef USB_MSC_H
#define USB_MSC_H
#include <stdint.h>
int  usb_msc_init(uint8_t dev_addr, uint8_t ep_in, uint8_t ep_out);
int  usb_msc_read(uint32_t lba, uint8_t *buf, uint16_t nsec);
int  usb_msc_write(uint32_t lba, const uint8_t *buf, uint16_t nsec);
int  usb_msc_is_ok(void);
void usb_msc_save_log(void);
#endif
