/**
 * @file usb_msc.c
 * @brief USB Mass Storage Class wrapper for Hillson OS
 *
 * Thin wrapper around usb_msd.c providing the public API
 * declared in include/usb_msc.h.
 */
#include <string.h>
#include <kernel/hillsonos.h>
#include <kernel/usb.h>
#include <usb_msc.h>
#include <mm/kheap.h>

/* MSD driver internals — real read function */
struct ata_dev_s;
extern long usb_msd_read(struct ata_dev_s *dev, size_t lba,
                         int sectors, uintptr_t buf);

static int msc_initialized = 0;
static uint8_t msc_dev_addr = 0;
static uint8_t msc_ep_in    = 0;
static uint8_t msc_ep_out   = 0;

/* Pointer to the first MSD device (set by init_msd via usb_msc_set_dev) */
static struct ata_dev_s *msc_dev = NULL;

void usb_msc_set_dev(void *dev)
{
    msc_dev = (struct ata_dev_s *)dev;
}

int usb_msc_init(uint8_t dev_addr, uint8_t ep_in, uint8_t ep_out)
{
    msc_dev_addr = dev_addr;
    msc_ep_in    = ep_in;
    msc_ep_out   = ep_out;
    msc_initialized = 1;
    usb_printk("usb_msc: initialized dev=%d ep_in=0x%02x ep_out=0x%02x\n",
           dev_addr, ep_in, ep_out);
    return 0;
}

int usb_msc_is_ok(void)
{
    return msc_initialized;
}

int usb_msc_read(uint32_t lba, uint8_t *buf, uint16_t nsec)
{
    if (!msc_initialized || !msc_dev) return -1;

    long ret = usb_msd_read(msc_dev, lba, nsec, (uintptr_t)buf);
    if (ret < 0) {
        usb_printk("usb_msc_read: error lba=%u nsec=%u\n", lba, nsec);
        return -1;
    }
    return 0;
}

int usb_msc_write(uint32_t lba, const uint8_t *buf, uint16_t nsec)
{
    (void)lba; (void)buf; (void)nsec;
    if (!msc_initialized) return -1;
    return -1;
}

void usb_msc_save_log(void)
{
    usb_printk("usb_msc_save_log: stub\n");
}
