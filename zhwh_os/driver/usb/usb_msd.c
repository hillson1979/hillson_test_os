/* 
/**
 *    Copyright 2025, 2026 (c)
 * 
 *    file: usb_msd.c
 *

/**
 *  \file usb_msd.c
 *
 *  The Universal Serial Bus (USB) driver code is split into several files:
 *    - usb.c       => main entry point and general functions
 *    - usb_msd.c   => functions to handle Mass Storage Devices (MSD)
 *    - usb_hid.c   => functions to handle Human Interaction Devices (HID)
 *    - usb_hub.c   => functions to handle USB hubs
 *    - usb_ioctl.c => functions to handle ioctl() calls
 *    - usb_ohci.c  => OHCI layer
 *    - usb_uhci.c  => UHCI layer
 *    - usb_ehci.c  => EHCI layer
 */

//#define __DEBUG

#include <kernel/errno.h>
#include <sys/hdreg.h>
#include <kernel/pciio.h>
#include <kernel/asm.h>
#include <kernel/usb.h>
#include <kernel/ata.h>
#include <kernel/ahci.h>
#include <kernel/dev.h>
#include <kernel/cdrom.h>
#include <kernel/hillsonos.h>
#include <mm/kheap.h>

#define SCSI_CMD_TEST_UNIT_READY        0x00
#define SCSI_CMD_REQUEST_SENSE          0x03
#define SCSI_CMD_INQUIRY                0x12
#define SCSI_CMD_READ_CAPACITY          0x25
#define SCSI_CMD_READ                   0x28
#define SCSI_CMD_WRITE                  0x2A

#define CBW_SIG                         0x43425355
#define CBW_SIG_OK                      0x53425355
#define CBW_SIG_ERR                     0x01010101

#if 0
/* Our master table for USB MSD disks and their partitions */
struct ata_dev_s *usb_disk_dev[MAX_USB_MSD_DEVICES];
struct parttab_s *usb_disk_part[MAX_USB_MSD_DEVICES];
kernel_mutex_t usb_disk_tablock;

void usb_msd_register_dev(void *__dev, struct parttab_s *part, int n);
#endif
int usb_msd_read_sector_direct(void *__dev, uintptr_t phys_buf, uintptr_t virt_buf, uint32_t lba);

// ahci_register_dev / __ahci_remove_dev are defined in kernel/ata.h
// (declarations here removed to avoid conflicting types)


static void prep_scsi_cmd(struct usb_cmd_blk_wrapper_t *cbw, 
                          uint32_t tag, uint8_t cmd, uint32_t lba, uint16_t len, uint32_t blksz)
{
    cbw->sig = CBW_SIG;
    cbw->tag = tag;
    cbw->lun = 0;

#define CMD_BYTES8(a, b, c, d, e, f, g, h)              \
    cbw->cmd[0] = cmd;                                  \
    cbw->cmd[1] = a; cbw->cmd[2] = b; cbw->cmd[3] = c;  \
    cbw->cmd[4] = d; cbw->cmd[5] = e; cbw->cmd[6] = f;  \
    cbw->cmd[7] = g; cbw->cmd[8] = h;                   \
    for(int z = 9; z < 16; z++) cbw->cmd[z] = 0;

    switch(cmd)
    {
        case SCSI_CMD_TEST_UNIT_READY:
            cbw->len = 0;
            cbw->flags = 0;
            cbw->cblen = 6;
            CMD_BYTES8(0, 0, 0, 0, 0, 0, 0, 0);
            break;

        case SCSI_CMD_REQUEST_SENSE:
            cbw->len = 18;
            cbw->flags = 0x80;
            cbw->cblen = 6;
            CMD_BYTES8(0, 0, 0, 18, 0, 0, 0, 0);
            break;

        case SCSI_CMD_INQUIRY:
            cbw->len = 36;
            cbw->flags = 0x80;
            cbw->cblen = 6;
            CMD_BYTES8(0, 0, 0, 36, 0, 0, 0, 0);
            break;

        case SCSI_CMD_READ_CAPACITY:
            cbw->len = 8;
            cbw->flags = 0x80;
            cbw->cblen = 10;
            CMD_BYTES8(0, BYTE4(lba), BYTE3(lba), BYTE2(lba), BYTE1(lba), 0, 0, 0);
            break;

        case SCSI_CMD_READ:
            cbw->len = len * blksz;
            cbw->flags = 0x80;
            cbw->cblen = 10;
            CMD_BYTES8(0, BYTE4(lba), BYTE3(lba), BYTE2(lba), BYTE1(lba), 0, BYTE2(len), BYTE1(len));
            break;

        case SCSI_CMD_WRITE:
            cbw->len = len * blksz;
            cbw->flags = 0;
            cbw->cblen = 10;
            CMD_BYTES8(0, BYTE4(lba), BYTE3(lba), BYTE2(lba), BYTE1(lba), 0, BYTE2(len), BYTE1(len));
            break;
    }

#undef CMD_BYTES8

}


static void usb_bulk_reset(struct usb_interface_t *iface)
{
    usb_ctrl_set(iface->usb, 0x21, 0xFF, 0, 0, iface->desc.interfacenum);
}


static int reset_recovery(struct usb_interface_t *iface)
{
    /*
     * USB Mass Storage Bulk-Only Transport reset recovery:
     * Bulk-Only Reset, then CLEAR_FEATURE(ENDPOINT_HALT) on both
     * bulk pipes.  SET_CONFIGURATION is not part of BOT recovery.
     */
    usb_bulk_reset(iface);
    usb_clear_feature_halt(iface->usb, iface->endpoint_in);
    usb_clear_feature_halt(iface->usb, iface->endpoint_out);

    iface->endpoint_in->toggle = 0;
    iface->endpoint_out->toggle = 0;

    return 0;
}


static int check_scsi_cmd(struct usb_interface_t *iface, uint32_t tag, void *statbuf)
{
    int err = 0;
    volatile uint32_t dword;
    volatile uint8_t byte;

    usb_printk("usb-msd: CSW tag=%x raw=%x %x %x %x %x %x %x %x %x %x %x %x %x\n",
        tag,
        ((uint8_t *)statbuf)[0], ((uint8_t *)statbuf)[1],
        ((uint8_t *)statbuf)[2], ((uint8_t *)statbuf)[3],
        ((uint8_t *)statbuf)[4], ((uint8_t *)statbuf)[5],
        ((uint8_t *)statbuf)[6], ((uint8_t *)statbuf)[7],
        ((uint8_t *)statbuf)[8], ((uint8_t *)statbuf)[9],
        ((uint8_t *)statbuf)[10], ((uint8_t *)statbuf)[11],
        ((uint8_t *)statbuf)[12]);

    // check signature
    dword = *(volatile uint32_t *)statbuf;

    if(dword != CBW_SIG_OK)
    {
        usb_printk("usb: SCSI cmd returned wrong signature (0x%x)\n", dword);
        err = -EIO;
    }

    // check tag
    dword = *(((volatile uint32_t *)statbuf) + 1);

    if(dword != tag)
    {
        usb_printk("usb: SCSI cmd returned wrong tag (0x%x, expected 0x%x)\n", dword, tag);
        err = -EIO;
    }

    // check data residue
    dword = *(((volatile uint32_t *)statbuf) + 2);

    if(dword != 0)
    {
        usb_printk("usb: SCSI cmd returned data residue (0x%x)\n", dword);
        //err = -EIO;
    }

    // check status byte
    byte = *(((volatile uint8_t *)statbuf) + 12);

    switch(byte)
    {
        case 0:
            break;

        case 1:
            usb_printk("usb: SCSI cmd failed\n");
            err = -EIO;
            break;

        case 2:
            usb_printk("usb: SCSI cmd failed - reset recovery\n");
            reset_recovery(iface);
            err = -EIO;
            break;

        default:
            usb_printk("usb: SCSI cmd failed (err %d)\n", byte);
            err = -EIO;
            break;
    }

    //usb_printk("check_scsi_cmd: err %d\n", err);
    return err;
}

static inline int sense_ok(struct usb_interface_t *iface, char *statbuf);

static int send_scsi_cmd(struct usb_interface_t *iface, 
                         uint8_t cmd, uint32_t lba, uint16_t __len,
                         void *buf, void *statbuf)
{
    struct usb_cmd_blk_wrapper_t cbw;
    struct usb_transfer_t transfer;
    char tmp[16];
    size_t len = __len;
    uint32_t tag;
    uint32_t blksz = iface->bytes_per_sector;

    kernel_mutex_lock_infinite_wait(&iface->usb->lock);

    tag = iface->cur_tag++;
    prep_scsi_cmd(&cbw, tag, cmd, lba, len, blksz);

    usb_setup_transfer(iface->usb, iface->endpoint_out, &transfer, USB_TRANSFER_BULK);
    usb_out_transaction(&transfer, 0, &cbw, 31);
    usb_schedule_transfer(&transfer);
    usb_wait_transfer(&transfer);
    usb_delete_transfer(&transfer);

    if(!transfer.success)
    {
        usb_printk("usb: CBW send failed (cmd=0x%x)\n", cmd);
        kernel_mutex_unlock(&iface->usb->lock);
        return -EIO;
    }

    if(cmd == SCSI_CMD_READ || cmd == SCSI_CMD_WRITE)
    {
        len *= blksz;
    }

    if(len)
    {
        usb_setup_transfer(iface->usb, iface->endpoint_in, &transfer, USB_TRANSFER_BULK);
        usb_in_transaction(&transfer, 0, buf, len);
        usb_schedule_transfer(&transfer);
        usb_wait_transfer(&transfer);
        usb_delete_transfer(&transfer);

        if(!transfer.success)
        {
            usb_printk("usb: data read failed (cmd=0x%x)\n", cmd);
            kernel_mutex_unlock(&iface->usb->lock);
            return -EIO;
        }
    }

    statbuf = statbuf ? statbuf : tmp;

    usb_setup_transfer(iface->usb, iface->endpoint_in, &transfer, USB_TRANSFER_BULK);
    usb_in_transaction(&transfer, 0, statbuf, 13);
    usb_schedule_transfer(&transfer);
    usb_wait_transfer(&transfer);
    usb_delete_transfer(&transfer);

    if(!transfer.success)
    {
        usb_printk("usb: CSW read failed (cmd=0x%x)\n", cmd);
        kernel_mutex_unlock(&iface->usb->lock);
        return -EIO;
    }

    kernel_mutex_unlock(&iface->usb->lock);

    return check_scsi_cmd(iface, tag, statbuf);
}


static int send_scsi_cmd_out(struct usb_interface_t *iface, 
                             uint8_t cmd, uint32_t lba, uint16_t __len,
                             void *buf, void *statbuf)
{
    struct usb_cmd_blk_wrapper_t cbw;
    struct usb_transfer_t transfer;
    char tmp[16];
    size_t len = __len;
    uint32_t tag;
    uint32_t blksz = iface->bytes_per_sector;

    kernel_mutex_lock_infinite_wait(&iface->usb->lock);

    tag = iface->cur_tag++;
    prep_scsi_cmd(&cbw, tag, cmd, lba, len, blksz);

    /* BOT phase 1: CBW is its own OUT transfer. */
    usb_setup_transfer(iface->usb, iface->endpoint_out, &transfer, USB_TRANSFER_BULK);
    usb_out_transaction(&transfer, 0, &cbw, 31);
    usb_schedule_transfer(&transfer);
    usb_wait_transfer(&transfer);
    usb_delete_transfer(&transfer);

    if(!transfer.success)
    {
        usb_printk("usb: WRITE CBW send failed (cmd=0x%x lba=%u sectors=%u)\n", cmd, lba, (uint32_t)__len);
        kernel_mutex_unlock(&iface->usb->lock);
        return -EIO;
    }

    if(cmd == SCSI_CMD_WRITE)
        len *= blksz;

    /* BOT phase 2: data OUT. */
    if(len)
    {
        usb_setup_transfer(iface->usb, iface->endpoint_out, &transfer, USB_TRANSFER_BULK);
        usb_out_transaction(&transfer, 0, buf, len);
        usb_schedule_transfer(&transfer);
        usb_wait_transfer(&transfer);
        usb_delete_transfer(&transfer);

        if(!transfer.success)
        {
            usb_printk("usb: WRITE data send failed (cmd=0x%x lba=%u len=%u)\n",
                       cmd, lba, (uint32_t)len);
            kernel_mutex_unlock(&iface->usb->lock);
            return -EIO;
        }
    }

    statbuf = statbuf ? statbuf : tmp;

    /* BOT phase 3: CSW IN. */
    usb_setup_transfer(iface->usb, iface->endpoint_in, &transfer, USB_TRANSFER_BULK);
    usb_in_transaction(&transfer, 0, statbuf, 13);
    usb_schedule_transfer(&transfer);
    usb_wait_transfer(&transfer);
    usb_delete_transfer(&transfer);

    if(!transfer.success)
    {
        usb_printk("usb: WRITE CSW read failed (cmd=0x%x lba=%u sectors=%u)\n", cmd, lba, (uint32_t)__len);
        kernel_mutex_unlock(&iface->usb->lock);
        return -EIO;
    }

    kernel_mutex_unlock(&iface->usb->lock);

    return check_scsi_cmd(iface, tag, statbuf);
}
/*
uint8_t get_max_lun(struct usb_interface_t *iface)
{
    struct usb_transfer_t transfer;
    uint8_t max_lun = -1;

    usb_setup_transfer(iface->usb, iface->usb->endpoints, &transfer, USB_TRANSFER_CTRL);
    usb_setup_transaction(&transfer, 0xA1, 0xFE, 0, 0, iface->desc.interfacenum, 1);
    usb_in_transaction(&transfer, 0, &max_lun, 1);
    usb_out_transaction(&transfer, 1, 0, 0);
    usb_schedule_transfer(&transfer);
    usb_wait_transfer(&transfer);
    usb_delete_transfer(&transfer);

    usb_printk("usb-msd: max_lun %u\n", max_lun);
    kpanic("^^^^^^^^^^\n");

    return max_lun;
}
*/


static inline int sense_ok(struct usb_interface_t *iface, char *statbuf)
{
    struct sense_data_t sense_data;
    char sense_statbuf[16];

    UNUSED(statbuf);
    A_memset(&sense_data, 0, sizeof(struct sense_data_t));
    A_memset(sense_statbuf, 0, sizeof(sense_statbuf));

    if(send_scsi_cmd(iface, SCSI_CMD_REQUEST_SENSE, 0, 
                        sizeof(struct sense_data_t), &sense_data, sense_statbuf) < 0)
    {
        return -EIO;
    }

    /*
     * Fixed-format sense uses response code 0x70/0x71 in bits [6:0].
     * Bit 7 is only the optional VALID bit for the INFORMATION field.
     */
    if((sense_data.err_code & 0x7f) != 0x70 &&
       (sense_data.err_code & 0x7f) != 0x71)
    {
        usb_printk("usb-msd: invalid sense response errcode=0x%x key=0x%x asc=0x%x ascq=0x%x\n",
                sense_data.err_code,
                (sense_data.sense_key & 0x0f),
                sense_data.additional_sense_code,
                sense_data.additional_sense_code_qualifier);
        return -EINVAL;
    }

    sense_data.sense_key &= 0x0f;

    if(sense_data.sense_key == 0x02 ||              // Not Ready
       sense_data.additional_sense_code == 0x30 ||  // Cannot Read Medium
       sense_data.additional_sense_code == 0x3A)    // Medium Not Present
    {
        // no media
        usb_printk("usb-msd: sense data: errcode 0x%x, key 0x%x, asc 0x%x ascq 0x%x\n",
                sense_data.err_code, 
                (sense_data.sense_key & 0x0f), 
                sense_data.additional_sense_code,
                sense_data.additional_sense_code_qualifier);

        return -ENOENT;
    }

    // sense_key == 0 is no error
    return (sense_data.sense_key == 0) ? 0 : -EIO;
}


int test_unit_ready(struct usb_interface_t *iface)
{
    char statbuf[24];
    int res = -EIO;
    volatile int timeout = 50;

    while(timeout--)
    {
        if((res = send_scsi_cmd(iface, SCSI_CMD_TEST_UNIT_READY, 0, 0, 0, statbuf)) < 0)
        {
            usb_printk("usb-msd: TEST_UNIT_READY failed (err %d)\n", res);

            if((res = sense_ok(iface, statbuf)) == -ENOENT)
            {
                return -ENOENT;
            }

            tick_delay(10);
            continue;
            //return res;
        }

        return 0;
    }

    return res;
}


void usb_msd_remove(struct usb_interface_t *iface)
{
    int i;

    for(i = 0; i < MAX_AHCI_DEVICES; i += 16)
    {
        if(ahci_disk_dev[i] && ahci_disk_dev[i]->priv == iface)
        {
            __ahci_remove_dev(TO_DEVID(AHCI_DEV_MAJ, i), 1, 1);
            return;
        }
    }
}


#define SECTORS_TO_DO               4

long usb_msd_read(struct ata_dev_s *dev, size_t lba, int sectors, uintptr_t buf)
{
    struct usb_interface_t *iface = dev->priv;

    if(!dev || !dev->priv)
    {
        return -EINVAL;
    }

    if(sectors <= SECTORS_TO_DO)
    {
        return send_scsi_cmd(iface, SCSI_CMD_READ, lba, sectors, (void *)buf, 0);
    }
    else
    {
        volatile int remaining = sectors, howmany = SECTORS_TO_DO;

        while(remaining > 0)
        {
            //usb_printk("usb_msd_read: sectors %d, remaining %d, howmany %d\n", sectors, remaining, howmany);

            if(send_scsi_cmd(iface, SCSI_CMD_READ, lba, howmany, (void *)buf, 0) < 0)
            {
                return -EIO;
            }

            lba += SECTORS_TO_DO;
            remaining -= SECTORS_TO_DO;
            buf += (iface->bytes_per_sector * SECTORS_TO_DO);

            if(remaining < SECTORS_TO_DO)
            {
                howmany = remaining;
            }
        }

        return 0;
    }
}


long usb_msd_write(struct ata_dev_s *dev, size_t lba, int sectors, uintptr_t buf)
{
    struct usb_interface_t *iface = dev->priv;

    if(!dev || !dev->priv)
    {
        return -EINVAL;
    }

    if(sectors <= SECTORS_TO_DO)
    {
        return send_scsi_cmd_out(iface, SCSI_CMD_WRITE, lba, sectors, (void *)buf, 0);
    }
    else
    {
        volatile int remaining = sectors, howmany = SECTORS_TO_DO;

        while(remaining > 0)
        {
            if(send_scsi_cmd_out(iface, SCSI_CMD_WRITE, lba, howmany, (void *)buf, 0) < 0)
            {
                return -EIO;
            }

            lba += SECTORS_TO_DO;
            remaining -= SECTORS_TO_DO;
            buf += (iface->bytes_per_sector * SECTORS_TO_DO);

            if(remaining < SECTORS_TO_DO)
            {
                howmany = remaining;
            }
        }

        return 0;
    }
}

#undef SECTORS_TO_DO


/*
 * General AHCI Block Read/Write Operations
 */
long usb_msd_strategy(struct disk_req_t *req)
{
    size_t block;
    long res = 0;
    int sectors_per_block, sectors_to_read;
    int min = MINOR(req->dev);
    /*
    struct ata_dev_s *dev = usb_disk_dev[min];
    struct parttab_s *part = usb_disk_part[min];
    */
    struct ata_dev_s *dev = ahci_disk_dev[min];
    struct parttab_s *part = ahci_disk_part[min];
    
    if(!dev || !dev->priv)
    {
        usb_printk("usb_msd_strategy: invalid device 0x%x\n", req->dev);
        return -ENODEV;
    }

    sectors_to_read = req->datasz / dev->bytes_per_sector;
    sectors_per_block = req->fs_blocksz / dev->bytes_per_sector;
    block = req->blockno * sectors_per_block;
    block += part ? part->lba : 0;

    if(!req->write)
    {
        res = usb_msd_read(dev, block, sectors_to_read, (uintptr_t)req->data);
    }
    else
    {
        res = usb_msd_write(dev, block, sectors_to_read, (uintptr_t)req->data);
    }

    return res ? -EIO : (long)(sectors_to_read * dev->bytes_per_sector);
}


int usb_msd_read_sector_direct(void *__dev, uintptr_t phys_buf, uintptr_t virt_buf, uint32_t lba)
{
    UNUSED(phys_buf);

    return usb_msd_read((struct ata_dev_s *)__dev, lba, 1, virt_buf);
}


int init_msd(struct usb_interface_t *iface)
{
    uint8_t buf[36];
    int res;
    uint32_t lba, blksz;
    struct ata_dev_s *dev;
    volatile struct usb_endpoint_t *endpoint;

    extern void kernel_usb_status_line(const char *stage, const char *detail);
    extern void kernel_usb_msc_status_line(const char *stage, const char *detail);
    extern void kernel_usb_error_line(const char *stage, const char *detail);

    kernel_usb_msc_status_line("fat32", "init_msd start");

    if (!iface->usb || !iface->usb->endpoints) {
        kernel_usb_error_line("fat32", "missing USB endpoints");
        return -EINVAL;
    }

    usb_printk("usb-msd: iface class=%x subclass=%x proto=%x\n",
               iface->desc.class, iface->desc.subclass, iface->desc.protocol);

    for (endpoint = iface->usb->endpoints; endpoint != NULL; endpoint = endpoint->next) {
        int dci = endpoint->addr * 2;
        if (endpoint->direction == USB_ENDPOINT_IN)
            dci |= 1;
        if (endpoint->type == USB_ENDPOINT_INTERRUPT) {
            iface->endpoint_interrupt = (struct usb_endpoint_t *)endpoint;
        } else if (endpoint->type == USB_ENDPOINT_BULK) {
            usb_printk("usb-msd: bulk EP addr=0x%02x dir=%s mps=%d interval=%d => DCI=%d\n",
                       endpoint->addr | (endpoint->direction == USB_ENDPOINT_IN ? 0x80 : 0x00),
                       endpoint->direction == USB_ENDPOINT_IN ? "IN" : "OUT",
                       endpoint->mps, endpoint->interval, dci);

            if (endpoint->direction == USB_ENDPOINT_OUT) {
                iface->endpoint_out = (struct usb_endpoint_t *)endpoint;
            } else {
                iface->endpoint_in = (struct usb_endpoint_t *)endpoint;
            }
        }
    }

    if (!iface->endpoint_out || !iface->endpoint_in) {
        usb_printk("usb-msd: mass storage device has invalid IN/OUT endpoints\n");
        kernel_usb_error_line("fat32", "invalid MSD endpoints");
        return -EINVAL;
    }

    iface->endpoint_out->toggle = 0;
    iface->endpoint_in->toggle = 0;

    if ((res = send_scsi_cmd(iface, SCSI_CMD_INQUIRY, 0, 36, buf, 0)) < 0) {
        usb_printk("usb-msd: INQUIRY failed (err %d)\n", res);
        kernel_usb_error_line("fat32", "INQUIRY failed");
        return res;
    }

    if ((res = test_unit_ready(iface)) < 0) {
        usb_printk("usb-msd: failed to test unit ready (err %d)\n", res);
        kernel_usb_error_line("fat32", "TEST UNIT READY failed");
        return res;
    }

    if ((res = send_scsi_cmd(iface, SCSI_CMD_READ_CAPACITY, 0, 8, buf, 0)) < 0) {
        usb_printk("usb-msd: READ_CAPACITY failed (err %d)\n", res);
        kernel_usb_error_line("fat32", "READ_CAPACITY failed");
        return res;
    }

    lba = ((uint32_t)buf[0] << 24) |
          ((uint32_t)buf[1] << 16) |
          ((uint32_t)buf[2] << 8)  |
          ((uint32_t)buf[3]);
    blksz = ((uint32_t)buf[4] << 24) |
            ((uint32_t)buf[5] << 16) |
            ((uint32_t)buf[6] << 8)  |
            ((uint32_t)buf[7]);

    usb_printk("usb-msd: READ_CAPACITY raw=%x %x %x %x %x %x %x %x\n",
               buf[0], buf[1], buf[2], buf[3],
               buf[4], buf[5], buf[6], buf[7]);
    kernel_usb_msc_status_line("fat32", "READ_CAPACITY ok");

    if (blksz == 0 || blksz > 4096) {
        usb_printk("usb-msd: invalid READ_CAPACITY block size %u\n", blksz);
        kernel_usb_error_line("fat32", "invalid block size");
        return -EIO;
    }

    usb_printk("usb-msd: last lba %u, blksz %u, capacity_mb %u\n",
               lba, blksz, (uint32_t)(((uint64_t)lba + 1) * blksz / (1024 * 1024)));

    if (!(dev = kmalloc(sizeof(struct ata_dev_s)))) {
        usb_printk("usb-msd: insufficient memory to init mass storage device\n");
        kernel_usb_error_line("fat32", "alloc ata_dev failed");
        return -ENOMEM;
    }

    A_memset(dev, 0, sizeof(struct ata_dev_s));

    dev->type = IDE_UNKNOWN;
    dev->base = 0;
    dev->priv = iface;
    dev->bytes_per_sector = blksz;
    dev->size = (lba + 1) * blksz;

    iface->bytes_per_sector = blksz;

    usb_printk("usb-msd: registering disk for FAT32 mount\n");
    kernel_usb_msc_status_line("fat32", "register USB disk");
    ahci_register_dev(dev, NULL, 0);

    {
        extern int usb_msc_init(uint8_t dev_addr, uint8_t ep_in, uint8_t ep_out);
        extern void usb_msc_set_dev(void *dev);
        usb_msc_init(iface->usb->num,
                     iface->endpoint_in->addr,
                     iface->endpoint_out->addr);
        usb_msc_set_dev(dev);
    }

    extern int fat32_try_mount_dev(struct ata_dev_s *dev);
    kernel_usb_msc_status_line("fat32", "calling fat32_try_mount_dev");
    fat32_try_mount_dev(dev);
    kernel_usb_msc_status_line("fat32", "fat32_try_mount_dev returned");

    return 0;
}

