#include <stdint.h>
#include <stddef.h>
#include <kernel/errno.h>
#include <kernel/hillsonos.h>
#include <kernel/usb.h>
#include <kernel/rtl8188eu_wifi.h>
#include <usb_log.h>
#include "rtl8188eu_tables.h"

#define RTL8188EU_USB_REQ_REGS  0x05
#define RTL8188EU_USB_REQ_READ  0xC0
#define RTL8188EU_USB_REQ_WRITE 0x40
#define RTL8188EU_REG_SYS_ISO_CTRL 0x0000
#define RTL8188EU_REG_RSV_CTRL  0x001c
#define RTL8188EU_REG_SYS_FUNC_EN 0x0002
#define RTL8188EU_REG_SYS_CLKR    0x0008
#define RTL8188EU_REG_APS_FSMCO 0x0004
#define RTL8188EU_REG_MCUFWDL   0x0080
#define RTL8188EU_REG_CR        0x0100
#define RTL8188EU_REG_PBP       0x0104
#define RTL8188EU_REG_TRXFF_BNDY 0x0114
#define RTL8188EU_REG_TRXDMA_CTRL 0x010c
#define RTL8188EU_REG_RQPN      0x0200
#define RTL8188EU_REG_TDECTRL   0x0208
#define RTL8188EU_REG_RQPN_NPQ  0x0214
#define RTL8188EU_REG_TXPKTBUF_BCNQ_BDNY 0x0424
#define RTL8188EU_REG_TXPKTBUF_MGQ_BDNY  0x0425
#define RTL8188EU_REG_TXPKTBUF_WMAC_LBK_BF_HD 0x045d
#define RTL8188EU_REG_RXDMA_AGG_PG_TH 0x0280
#define RTL8188EU_REG_RXPKT_NUM       0x0284
#define RTL8188EU_REG_RXDMA_STATUS    0x0288
#define RTL8188EU_REG_RXDMA_DPR       0x028c
#define RTL8188EU_REG_RXDMA_MODE_CTRL 0x0290
#define RTL8188EU_REG_HIMR            0x00b0
#define RTL8188EU_REG_HISR            0x00b4
#define RTL8188EU_REG_HIMRE           0x00b8
#define RTL8188EU_REG_HISRE           0x00bc
#define RTL8188EU_REG_RCR       0x0608
#define RTL8188EU_REG_RX_DRVINFO_SZ 0x060f
#define RTL8188EU_REG_RXFLTMAP0 0x06a0
#define RTL8188EU_REG_RXFLTMAP1 0x06a2
#define RTL8188EU_REG_RXFLTMAP2 0x06a4
#define RTL8188EU_REG_RF_CTRL   0x001f
#define RTL8188EU_REG_FPGA0_RFMOD 0x0800
#define RTL8188EU_REG_FPGA1_RFMOD 0x0900
#define RTL8188EU_REG_CCK0_SYSTEM 0x0a00
#define RTL8188EU_REG_OFDM0_TRXPATH 0x0c04
#define RTL8188EU_REG_OFDM1_TRXPATH 0x0d04
#define RTL8188EU_REG_XA_HSSI_PARAM2 0x0824
#define RTL8188EU_REG_XA_HSSI_PARAM1 0x0820
#define RTL8188EU_REG_XA_LSSI_PARAM  0x0840
#define RTL8188EU_REG_XA_LSSI_READBACK 0x08a0
#define RTL8188EU_REG_XA_HSPI_READBACK 0x08b8
#define RTL8188EU_REG_CCK0_CCA       0x0a08
#define RTL8188EU_REG_OFDM0_TRMUX    0x0c08
#define RTL8188EU_REG_OFDM0_RX_DETECTOR1 0x0c30
#define RTL8188EU_REG_OFDM0_XA_AGC_CORE1 0x0c50
#define RTL8188EU_REG_OFDM0_XA_AGC_CORE2 0x0c54
#define RTL8188EU_REG_AGC_TABLE_PORT 0x0c78
#define RTL8188EU_REG_MACID     0x0610
#define RTL8188EU_REG_EFUSE_ACCESS 0x00cf
#define RTL8188EU_REG_EFUSE_CTRL   0x0030
#define RTL8188EU_REG_EFUSE_ADDR_LOW 0x0031
#define RTL8188EU_REG_EFUSE_ADDR_HIGH 0x0032
#define RTL8188EU_REG_EFUSE_STATUS 0x0033
#define RTL8188EU_EFUSE_ADDR_HIGH_MASK 0x03
#define RTL8188EU_EFUSE_VALID      0x80
#define RTL8188EU_EFUSE_ACCESS_ON  0x69
#define RTL8188EU_EFUSE_ACCESS_OFF 0x00
#define RTL8188EU_SYS_FUNC_ELDR    0x10
#define RTL8188EU_SYS_CLK_LOADER   0x20
#define RTL8188EU_SYS_CLK_ANA8M    0x02
#define RTL8188EU_FW_START      0x1000
#define RTL8188EU_FW_HEADER_LEN 32
#define RTL8188EU_FW_PAGE_SIZE  4096
#define RTL8188EU_FW_BLOCK_SIZE 4
#define RTL8188EU_FW_WRITE_RETRIES 3
#define RTL8188EU_FW_SIGNATURE  0x88E1
#define RTL8188EU_MCUFWDL_EN    0x01
#define RTL8188EU_MCUFWDL_RDY   0x02
#define RTL8188EU_FWDL_CHKSUM_RPT 0x04
#define RTL8188EU_MCUFWDL_PAGE_MASK 0x07
#define RTL8188EU_SYS_FUNC_CPU_ENABLE 0x04
#define RTL8188EU_WINTINI_RDY   0x40
#define RTL8188EU_MCUFWDL_RAM_SEL 0x80
#define RTL8188EU_APS_APFM_ONMAC 0x0100
#define RTL8188EU_APS_MAC_OFF    0x0200
#define RTL8188EU_APS_SW_LPS     0x0400
#define RTL8188EU_APS_HW_SUSPEND 0x0800
#define RTL8188EU_APS_PCIE       0x1000
#define RTL8188EU_APS_HW_POWERDOWN 0x8000
#define RTL8188EU_CR_HCI_TXDMA_EN 0x0001
#define RTL8188EU_CR_HCI_RXDMA_EN 0x0002
#define RTL8188EU_CR_TXDMA_EN     0x0004
#define RTL8188EU_CR_RXDMA_EN     0x0008
#define RTL8188EU_CR_PROTOCOL_EN  0x0010
#define RTL8188EU_CR_SCHEDULE_EN  0x0020
#define RTL8188EU_CR_SECURITY_EN  0x0200
#define RTL8188EU_CR_CALTMR_EN    0x0400
#define RTL8188EU_TX_DESC_SIZE     32
#define RTL8188EU_TX_TEST_ENABLE   1
#define RTL8188EU_RX_TEST_ENABLE   1
#define RTL8188EU_PRE_FW_POWER_ON  1
#define RTL8188EU_FW_RAM_VERIFY    0
#define RTL8188EU_RX_TEST_SIZE     512
#define RTL8188EU_TX_TOTAL_PAGES   0xA9
#define RTL8188EU_TX_HPQ_PAGES     0x0C
#define RTL8188EU_TX_LPQ_PAGES     0x02
#define RTL8188EU_TX_NPQ_PAGES     0x02
#define RTL8188EU_RQPN_LOAD         (1u << 31)
#define RTL8188EU_RCR_AAP            (1u << 0)
#define RTL8188EU_RCR_APM            (1u << 1)
#define RTL8188EU_RCR_AM             (1u << 2)
#define RTL8188EU_RCR_AB             (1u << 3)
#define RTL8188EU_RCR_APP_PHYSTS     (1u << 28)
#define RTL8188EU_SYS_FUNC_BBRSTB     (1u << 0)
#define RTL8188EU_SYS_FUNC_BB_GLB_RST (1u << 1)
#define RTL8188EU_RF_EN               (1u << 0)
#define RTL8188EU_RF_RSTB             (1u << 1)
#define RTL8188EU_RF_SDMRSTB          (1u << 2)
#define RTL8188EU_HSSI_READ_EDGE      (1u << 31)
#define RTL8188EU_HSSI_READ_ADDR_MASK 0x7f800000u
#define RTL8188EU_HSSI_READ_ADDR_SHIFT 23
#define RTL8188EU_HSSI_PI_MODE        (1u << 8)
#define RTL8188EU_RF_REG_MASK         0x000fffffu
#define RTL8188EU_RF_CHNLBW           0x18
#define RTL8188EU_RF_CHANNEL_MASK     0x000003ffu
#define RTL8188EU_SCAN_FIRST_CHANNEL  1
#define RTL8188EU_SCAN_LAST_CHANNEL   13
#define RTL8188EU_MAX_SCAN_RESULTS    32
#define RTL8188EU_MAX_SSID_LEN        32
#define RTL8188EU_BB_CCK_ENABLE       (1u << 0)
#define RTL8188EU_BB_OFDM_ENABLE      (1u << 1)
#define RTL8188EU_RXDMA_AGG_ENABLE    (1u << 2)
#define RTL8188EU_TXDW4_HWSEQ_EN       (1u << 7)
#define RTL8188EU_TXDW4_USE_RATE       (1u << 8)
#define RTL8188EU_TXDW4_DISDATAFB      (1u << 10)
#define RTL8188EU_DESC_RATE_1M         0

extern const uint8_t rtl8188eufw[];
extern const uint32_t rtl8188eufw_size;

struct rtl8188eu_device {
    struct usb_dev_t *usb;
    struct usb_interface_t *iface;
    struct usb_endpoint_t *bulk_in;
    struct usb_endpoint_t *bulk_out;
    struct usb_endpoint_t *bulk_out2;
    uint8_t mac_addr[6];
    uint8_t firmware_loaded;
    uint8_t wifi_state;
    int last_error;
    int selected_result;
    unsigned int scan_result_count;
};

struct rtl8188eu_scan_result {
    uint8_t bssid[6];
    uint8_t ssid[RTL8188EU_MAX_SSID_LEN];
    uint8_t ssid_length;
    uint8_t channel;
    uint8_t privacy;
    uint8_t rsn;
};

/* The USB core currently has no generic per-driver binding storage. */
static struct rtl8188eu_device rtl8188eu;
static struct rtl8188eu_scan_result
    rtl8188eu_scan_results[RTL8188EU_MAX_SCAN_RESULTS];
static int rtl8188eu_fw_usb_trace;
static unsigned int rtl8188eu_fw_data_writes;
static unsigned int rtl8188eu_fw_data_total;

static int rtl8188eu_write8(struct usb_dev_t *usb, uint16_t reg,
                            uint8_t value);
static int rtl8188eu_write16(struct usb_dev_t *usb, uint16_t reg,
                             uint16_t value);
static int rtl8188eu_read8(struct usb_dev_t *usb, uint16_t reg,
                           uint8_t *value);
static int rtl8188eu_read16(struct usb_dev_t *usb, uint16_t reg,
                            uint16_t *value);
static int rtl8188eu_read(struct usb_dev_t *usb, uint16_t reg,
                          uint8_t *data, uint16_t length);
static int rtl8188eu_read32(struct usb_dev_t *usb, uint16_t reg,
                            uint32_t *value);
static int rtl8188eu_write32(struct usb_dev_t *usb, uint16_t reg,
                             uint32_t value);

static void rtl8188eu_put_le32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static uint16_t rtl8188eu_txdesc_checksum(uint8_t *desc)
{
    uint16_t checksum = 0;
    unsigned int i;

    /* The checksum occupies the low 16 bits of descriptor dword 7. */
    desc[28] = 0;
    desc[29] = 0;
    for (i = 0; i < RTL8188EU_TX_DESC_SIZE; i += 2)
        checksum ^= (uint16_t)desc[i] | ((uint16_t)desc[i + 1] << 8);
    desc[28] = (uint8_t)checksum;
    desc[29] = (uint8_t)(checksum >> 8);
    return checksum;
}

static int rtl8188eu_txdesc_checksum_valid(const uint8_t *desc)
{
    uint16_t checksum = 0;
    unsigned int i;

    for (i = 0; i < RTL8188EU_TX_DESC_SIZE; i += 2)
        checksum ^= (uint16_t)desc[i] | ((uint16_t)desc[i + 1] << 8);
    return checksum == 0;
}

static uint32_t rtl8188eu_get_le32_early(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void rtl8188eu_dump_bytes(const char *label, const uint8_t *data,
                                 unsigned int length)
{
    unsigned int i;

    usb_printk("rtl8188eu: %s len=%u bytes=", label, length);
    for (i = 0; i < length; i++)
        usb_printk(" %02x", data[i]);
    usb_printk("\n");
}

static void rtl8188eu_parse_management_ies(const uint8_t *frame,
                                            unsigned int frame_length)
{
    unsigned int offset = 24 + 12;
    struct rtl8188eu_scan_result result;
    unsigned int result_index;
    uint8_t subtype;

    if (frame_length < 24)
        return;
    subtype = frame[0] & 0xf0;
    if (subtype != 0x50 && subtype != 0x80) {
        usb_printk("rtl8188eu: RX 802.11 frame control=%02x%02x subtype=%02x\n",
                   frame[1], frame[0], subtype);
        return;
    }
    usb_printk("rtl8188eu: RX management=%s BSSID=%02x:%02x:%02x:%02x:%02x:%02x\n",
               subtype == 0x50 ? "probe-response" : "beacon",
               frame[16], frame[17], frame[18], frame[19], frame[20],
               frame[21]);
    if (frame_length < offset) {
        usb_printk("rtl8188eu: RX management frame truncated before IEs\n");
        return;
    }
    A_memset(&result, 0, sizeof(result));
    for (result_index = 0; result_index < 6; result_index++)
        result.bssid[result_index] = frame[16 + result_index];
    result.privacy = (frame[34] & 0x10) != 0;
    while (offset + 2 <= frame_length) {
        uint8_t id = frame[offset];
        uint8_t length = frame[offset + 1];
        unsigned int i;

        offset += 2;
        if (offset + length > frame_length) {
            usb_printk("rtl8188eu: RX IE id=%u truncated length=%u remaining=%u\n",
                       id, length, frame_length - offset);
            return;
        }
        if (id == 0) {
            result.ssid_length = length > RTL8188EU_MAX_SSID_LEN ?
                                 RTL8188EU_MAX_SSID_LEN : length;
            for (i = 0; i < result.ssid_length; i++)
                result.ssid[i] = frame[offset + i];
            usb_printk("rtl8188eu: RX SSID len=%u value=", length);
            for (i = 0; i < length; i++) {
                uint8_t c = frame[offset + i];
                usb_printk("%c", c >= 0x20 && c <= 0x7e ? c : '.');
            }
            usb_printk("\n");
        } else if (id == 3 && length == 1) {
            result.channel = frame[offset];
            usb_printk("rtl8188eu: RX DS channel=%u\n", frame[offset]);
        } else if (id == 48 && length >= 2) {
            result.rsn = 1;
        }
        offset += length;
    }
    for (result_index = 0; result_index < rtl8188eu.scan_result_count;
         result_index++) {
        unsigned int byte;
        int same = 1;

        for (byte = 0; byte < 6; byte++) {
            if (rtl8188eu_scan_results[result_index].bssid[byte] !=
                result.bssid[byte]) {
                same = 0;
                break;
            }
        }
        if (same) {
            rtl8188eu_scan_results[result_index] = result;
            return;
        }
    }
    if (rtl8188eu.scan_result_count < RTL8188EU_MAX_SCAN_RESULTS)
        rtl8188eu_scan_results[rtl8188eu.scan_result_count++] = result;
}

static void rtl8188eu_parse_rx_packet(const uint8_t *data,
                                      unsigned int actual_length)
{
    uint32_t dw0;
    unsigned int packet_length;
    unsigned int driver_info_length;
    unsigned int shift;
    unsigned int frame_offset;
    unsigned int available;

    rtl8188eu_dump_bytes("RX raw", data, actual_length);
    if (actual_length < 24) {
        usb_printk("rtl8188eu: RX shorter than 24-byte descriptor\n");
        return;
    }
    dw0 = rtl8188eu_get_le32_early(data);
    packet_length = dw0 & 0x3fff;
    driver_info_length = ((dw0 >> 16) & 0x0f) * 8;
    shift = (dw0 >> 24) & 0x03;
    frame_offset = 24 + driver_info_length + shift;
    usb_printk("rtl8188eu: RX desc dw0=%08x pkt_len=%u drvinfo=%u shift=%u "
               "frame_offset=%u crc=%u icv=%u physt=%u swdec=%u\n",
               dw0, packet_length, driver_info_length, shift, frame_offset,
               (dw0 >> 14) & 1, (dw0 >> 15) & 1, (dw0 >> 26) & 1,
               (dw0 >> 27) & 1);
    if (frame_offset > actual_length) {
        usb_printk("rtl8188eu: RX descriptor offset exceeds transfer length\n");
        return;
    }
    available = actual_length - frame_offset;
    if (!packet_length || packet_length > available) {
        usb_printk("rtl8188eu: RX descriptor length invalid available=%u\n",
                   available);
        return;
    }
    rtl8188eu_dump_bytes("RX 802.11 frame", data + frame_offset,
                         packet_length);
    rtl8188eu_parse_management_ies(data + frame_offset, packet_length);
}

static int rtl8188eu_bulk_out(struct rtl8188eu_device *dev,
                              struct usb_endpoint_t *endpoint,
                              uint8_t *buffer, size_t length)
{
    struct usb_transfer_t transfer;

    if (!dev || !dev->usb || !endpoint || !buffer || !length)
        return -EINVAL;
    usb_setup_transfer(dev->usb, endpoint, &transfer, USB_TRANSFER_BULK);
    usb_out_transaction(&transfer, 0, buffer, length);
    usb_schedule_transfer(&transfer);
    usb_wait_transfer(&transfer);
    usb_delete_transfer(&transfer);
    return transfer.success ? 0 : -EIO;
}

static int rtl8188eu_bulk_in_start(struct rtl8188eu_device *dev,
                                   struct usb_transfer_t *transfer,
                                   uint8_t *buffer, size_t length)
{
    if (!dev || !dev->usb || !dev->bulk_in || !transfer ||
        !buffer || !length)
        return -EINVAL;
    A_memset(buffer, 0, length);
    usb_setup_transfer(dev->usb, dev->bulk_in, transfer, USB_TRANSFER_BULK);
    usb_in_transaction(transfer, 0, buffer, length);
    usb_schedule_transfer(transfer);
    return 0;
}

static int rtl8188eu_bulk_in_finish(struct usb_transfer_t *transfer,
                                    uint16_t *actual_length)
{
    int ret;

    if (!transfer)
        return -EINVAL;
    usb_wait_transfer(transfer);
    ret = transfer->success ? 0 : -EIO;
    if (actual_length)
        *actual_length = transfer->actual_length;
    usb_delete_transfer(transfer);
    return ret;
}

static size_t rtl8188eu_build_probe_request(uint8_t *packet, size_t capacity,
                                            const uint8_t mac[6])
{
    uint8_t *frame;
    size_t frame_len = 34;
    uint32_t txdw0;
    uint32_t txdw1;
    uint32_t txdw4;

    if (!packet || !mac || capacity < RTL8188EU_TX_DESC_SIZE + frame_len)
        return 0;
    A_memset(packet, 0, RTL8188EU_TX_DESC_SIZE + frame_len);
    frame = packet + RTL8188EU_TX_DESC_SIZE;
    frame[0] = 0x40; /* Probe Request management frame. */
    frame[1] = 0x00;
    for (unsigned int i = 0; i < 6; i++) {
        frame[4 + i] = 0xff;
        frame[10 + i] = mac[i];
        frame[16 + i] = 0xff;
    }
    frame[24] = 0x00; /* Wildcard SSID element. */
    frame[25] = 0x00;
    frame[26] = 0x01; /* Supported Rates. */
    frame[27] = 0x04;
    frame[28] = 0x82; /* 1 Mbps, basic. */
    frame[29] = 0x84; /* 2 Mbps, basic. */
    frame[30] = 0x8b; /* 5.5 Mbps, basic. */
    frame[31] = 0x96; /* 11 Mbps, basic. */
    frame[32] = 0x32; /* Extended Supported Rates, empty for now. */
    frame[33] = 0x00;

    txdw0 = (uint32_t)frame_len | (RTL8188EU_TX_DESC_SIZE << 16) |
            (1u << 24) | /* Broadcast/multicast frame. */
            (1u << 26) | (1u << 27) | (1u << 31);
    txdw1 = (0x12u << 8); /* Management queue selector. */
    txdw4 = RTL8188EU_DESC_RATE_1M | RTL8188EU_TXDW4_HWSEQ_EN |
            RTL8188EU_TXDW4_USE_RATE | RTL8188EU_TXDW4_DISDATAFB;
    rtl8188eu_put_le32(packet, txdw0);
    rtl8188eu_put_le32(packet + 4, txdw1);
    rtl8188eu_put_le32(packet + 16, txdw4);
    rtl8188eu_txdesc_checksum(packet);
    return RTL8188EU_TX_DESC_SIZE + frame_len;
}

static void rtl8188eu_print_scan_results(void)
{
    unsigned int result_index;

    usb_printk("rtl8188eu: scan complete networks=%u\n",
               rtl8188eu.scan_result_count);
    for (result_index = 0; result_index < rtl8188eu.scan_result_count;
         result_index++) {
        const struct rtl8188eu_scan_result *result =
            &rtl8188eu_scan_results[result_index];
        unsigned int byte;

        usb_printk("rtl8188eu: AP channel=%u security=%s BSSID="
                   "%02x:%02x:%02x:%02x:%02x:%02x SSID=",
                   result->channel,
                   result->rsn ? "WPA2/RSN" :
                   (result->privacy ? "protected" : "open"),
                   result->bssid[0], result->bssid[1], result->bssid[2],
                   result->bssid[3], result->bssid[4], result->bssid[5]);
        for (byte = 0; byte < result->ssid_length; byte++) {
            uint8_t c = result->ssid[byte];
            usb_printk("%c", c >= 0x20 && c <= 0x7e ? c : '.');
        }
        usb_printk("\n");
    }
}

static void rtl8188eu_log_tx_queue_state(struct usb_dev_t *usb)
{
    uint8_t pbp;
    uint8_t boundary;
    uint32_t rqpn;
    uint32_t tdectrl;
    uint8_t npq;

    if (rtl8188eu_read8(usb, RTL8188EU_REG_PBP, &pbp) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_TRXFF_BNDY, &boundary) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_RQPN, &rqpn) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_TDECTRL, &tdectrl) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_RQPN_NPQ, &npq) < 0) {
        usb_printk("rtl8188eu: TX queue register snapshot failed\n");
        return;
    }
    usb_printk("rtl8188eu: TX queue PBP=%02x BNDY=%02x RQPN=%08x TDECTRL=%08x NPQ=%02x\n",
               pbp, boundary, rqpn, tdectrl, npq);
}

static int rtl8188eu_initialize_tx_pages(struct usb_dev_t *usb)
{
    const uint8_t public_pages = RTL8188EU_TX_TOTAL_PAGES -
        RTL8188EU_TX_HPQ_PAGES - RTL8188EU_TX_LPQ_PAGES -
        RTL8188EU_TX_NPQ_PAGES;
    uint32_t rqpn_old;
    uint8_t npq_old;
    uint32_t rqpn;
    uint32_t rqpn_after;
    uint8_t npq_after;

    if (rtl8188eu_read32(usb, RTL8188EU_REG_RQPN, &rqpn_old) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_RQPN_NPQ, &npq_old) < 0)
        return -EIO;
    if (rqpn_old || npq_old) {
        usb_printk("rtl8188eu: TX pages already configured RQPN=%08x NPQ=%08x\n",
                   rqpn_old, (uint32_t)npq_old);
        return 0;
    }
    rqpn = RTL8188EU_RQPN_LOAD | ((uint32_t)public_pages << 16) |
           ((uint32_t)RTL8188EU_TX_LPQ_PAGES << 8) |
           RTL8188EU_TX_HPQ_PAGES;
    if (rtl8188eu_write8(usb, RTL8188EU_REG_RQPN_NPQ,
                         RTL8188EU_TX_NPQ_PAGES) < 0 ||
        rtl8188eu_write32(usb, RTL8188EU_REG_RQPN, rqpn) < 0)
        goto restore;
    if (rtl8188eu_read32(usb, RTL8188EU_REG_RQPN, &rqpn_after) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_RQPN_NPQ, &npq_after) < 0)
        goto restore;
    usb_printk("rtl8188eu: TX pages HPQ=%u LPQ=%u NPQ=%u PUBQ=%u RQPN=%08x NPQREG=%08x\n",
               RTL8188EU_TX_HPQ_PAGES, RTL8188EU_TX_LPQ_PAGES,
               RTL8188EU_TX_NPQ_PAGES, public_pages, rqpn_after,
               (uint32_t)npq_after);
    if ((rqpn_after & 0x00ffffffu) != (rqpn & 0x00ffffffu) ||
        npq_after != RTL8188EU_TX_NPQ_PAGES)
        goto restore;
    return 0;

restore:
    rtl8188eu_write8(usb, RTL8188EU_REG_RQPN_NPQ, npq_old);
    rtl8188eu_write32(usb, RTL8188EU_REG_RQPN, rqpn_old);
    usb_printk("rtl8188eu: TX page initialization failed; registers restored\n");
    return -EIO;
}

static int rtl8188eu_prepare_firmware_buffers(struct usb_dev_t *usb)
{
    uint16_t trxdma;
    uint8_t pbp;
    uint8_t tx_boundary;
    uint16_t rx_boundary;
    uint32_t rqpn;

    /* Linux 3.18/5.10 performs these before rtl88eu_download_fw(). */
    if (rtl8188eu_write16(usb, RTL8188EU_REG_RQPN_NPQ, 0x0000) < 0 ||
        rtl8188eu_write16(usb, RTL8188EU_REG_RQPN_NPQ, 0x000d) < 0 ||
        rtl8188eu_write32(usb, RTL8188EU_REG_RQPN, 0x808e000du) < 0)
        return -EIO;
    if (rtl8188eu_read16(usb, RTL8188EU_REG_TRXDMA_CTRL, &trxdma) < 0)
        return -EIO;
    trxdma = (uint16_t)((trxdma & 0x0007) | 0xfaf0);
    if (rtl8188eu_write16(usb, RTL8188EU_REG_TRXDMA_CTRL, trxdma) < 0 ||
        rtl8188eu_write16(usb, RTL8188EU_REG_TRXFF_BNDY + 2, 0x23ff) < 0 ||
        rtl8188eu_write8(usb, RTL8188EU_REG_PBP, 0x11) < 0 ||
        rtl8188eu_write8(usb, RTL8188EU_REG_TXPKTBUF_BCNQ_BDNY, 0xaa) < 0 ||
        rtl8188eu_write8(usb, RTL8188EU_REG_TXPKTBUF_MGQ_BDNY, 0xaa) < 0 ||
        rtl8188eu_write8(usb, RTL8188EU_REG_TXPKTBUF_WMAC_LBK_BF_HD,
                         0xaa) < 0 ||
        rtl8188eu_write8(usb, RTL8188EU_REG_TRXFF_BNDY, 0xaa) < 0 ||
        rtl8188eu_write8(usb, RTL8188EU_REG_TDECTRL + 1, 0xaa) < 0)
        return -EIO;
    if (rtl8188eu_read8(usb, RTL8188EU_REG_PBP, &pbp) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_TRXFF_BNDY, &tx_boundary) < 0 ||
        rtl8188eu_read16(usb, RTL8188EU_REG_TRXFF_BNDY + 2,
                         &rx_boundary) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_RQPN, &rqpn) < 0)
        return -EIO;
    usb_printk("rtl8188eu: pre-FW buffers PBP=%02x TXBNDY=%02x "
               "RXBNDY=%04x RQPN=%08x TRXDMA=%04x\n",
               pbp, tx_boundary, rx_boundary, rqpn, trxdma);
    if (pbp != 0x11 || tx_boundary != 0xaa || rx_boundary != 0x23ff ||
        (rqpn & 0x00ffffffu) != 0x008e000du)
        return -EIO;
    return 0;
}

static int rtl8188eu_initialize_rx_filter(struct usb_dev_t *usb)
{
    uint32_t rcr;
    uint32_t rcr_after;
    uint16_t map0;
    uint16_t map1;
    uint16_t map2;
    uint8_t drvinfo;

    if (rtl8188eu_read32(usb, RTL8188EU_REG_RCR, &rcr) < 0)
        return -EIO;
    rcr |= RTL8188EU_RCR_AAP | RTL8188EU_RCR_APM | RTL8188EU_RCR_AM |
           RTL8188EU_RCR_AB | RTL8188EU_RCR_APP_PHYSTS;
    if (rtl8188eu_write32(usb, RTL8188EU_REG_RCR, rcr) < 0 ||
        rtl8188eu_write8(usb, RTL8188EU_REG_RX_DRVINFO_SZ, 4) < 0 ||
        rtl8188eu_write16(usb, RTL8188EU_REG_RXFLTMAP0, 0xffff) < 0 ||
        rtl8188eu_write16(usb, RTL8188EU_REG_RXFLTMAP1, 0x0000) < 0 ||
        rtl8188eu_write16(usb, RTL8188EU_REG_RXFLTMAP2, 0x0000) < 0)
        return -EIO;
    if (rtl8188eu_read32(usb, RTL8188EU_REG_RCR, &rcr_after) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_RX_DRVINFO_SZ, &drvinfo) < 0 ||
        rtl8188eu_read16(usb, RTL8188EU_REG_RXFLTMAP0, &map0) < 0 ||
        rtl8188eu_read16(usb, RTL8188EU_REG_RXFLTMAP1, &map1) < 0 ||
        rtl8188eu_read16(usb, RTL8188EU_REG_RXFLTMAP2, &map2) < 0)
        return -EIO;
    usb_printk("rtl8188eu: RX filter RCR=%08x DRVINFO=%02x MAP0=%04x MAP1=%04x MAP2=%04x\n",
               rcr_after, drvinfo, map0, map1, map2);
    if ((rcr_after & (RTL8188EU_RCR_AAP | RTL8188EU_RCR_APM |
                      RTL8188EU_RCR_AM | RTL8188EU_RCR_AB |
                      RTL8188EU_RCR_APP_PHYSTS)) !=
        (RTL8188EU_RCR_AAP | RTL8188EU_RCR_APM | RTL8188EU_RCR_AM |
         RTL8188EU_RCR_AB | RTL8188EU_RCR_APP_PHYSTS) ||
        drvinfo != 4 || map0 != 0xffff || map1 || map2)
        return -EIO;
    return 0;
}

static int rtl8188eu_disable_rx_aggregation(struct usb_dev_t *usb)
{
    uint8_t old_value;
    uint8_t new_value;
    uint8_t readback;

    if (rtl8188eu_read8(usb, RTL8188EU_REG_TRXDMA_CTRL, &old_value) < 0)
        return -EIO;
    new_value = old_value & (uint8_t)~RTL8188EU_RXDMA_AGG_ENABLE;
    if (rtl8188eu_write8(usb, RTL8188EU_REG_TRXDMA_CTRL, new_value) < 0)
        return -EIO;
    if (rtl8188eu_read8(usb, RTL8188EU_REG_TRXDMA_CTRL, &readback) < 0 ||
        (readback & RTL8188EU_RXDMA_AGG_ENABLE)) {
        rtl8188eu_write8(usb, RTL8188EU_REG_TRXDMA_CTRL, old_value);
        usb_printk("rtl8188eu: RX aggregation disable failed; TRXDMA restored=%02x\n",
                   old_value);
        return -EIO;
    }
    usb_printk("rtl8188eu: RX aggregation disabled TRXDMA old=%02x new=%02x readback=%02x\n",
               old_value, new_value, readback);
    return 0;
}

static void rtl8188eu_log_rx_runtime_state(struct usb_dev_t *usb,
                                           const char *stage)
{
    uint8_t raw[20];
    uint16_t cr;
    uint32_t rcr;
    uint32_t rxpkt_num;
    uint32_t rxdma_status;
    uint32_t rxdma_dpr;
    uint32_t rxdma_mode;
    uint32_t himr;
    uint32_t hisr;
    uint32_t himre;
    uint32_t hisre;

    if (rtl8188eu_read16(usb, RTL8188EU_REG_CR, &cr) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_RCR, &rcr) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_RXPKT_NUM, &rxpkt_num) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_RXDMA_STATUS,
                         &rxdma_status) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_RXDMA_DPR, &rxdma_dpr) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_RXDMA_MODE_CTRL,
                         &rxdma_mode) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_HIMR, &himr) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_HISR, &hisr) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_HIMRE, &himre) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_HISRE, &hisre) < 0) {
        usb_printk("rtl8188eu: RX state %s register read failed\n", stage);
        return;
    }
    usb_printk("rtl8188eu: RX state %s CR=%04x RCR=%08x "
               "RXPKT_NUM=%08x RXDMA_STATUS=%08x DPR=%08x MODE=%08x\n",
               stage, cr, rcr, rxpkt_num, rxdma_status, rxdma_dpr,
               rxdma_mode);
    usb_printk("rtl8188eu: RX IRQ %s HIMR=%08x HISR=%08x "
               "HIMRE=%08x HISRE=%08x\n",
               stage, himr, hisr, himre, hisre);
    if (rtl8188eu_read(usb, RTL8188EU_REG_RXDMA_AGG_PG_TH, raw,
                       sizeof(raw)) == 0)
        rtl8188eu_dump_bytes("RXDMA regs 0280-0293", raw, sizeof(raw));
    else
        usb_printk("rtl8188eu: RXDMA raw register block read failed\n");
}

static int rtl8188eu_clear_interrupt_status(struct usb_dev_t *usb)
{
    uint32_t hisr;
    uint32_t hisre;

    /* HISR/HISRE are write-one-to-clear status registers. */
    if (rtl8188eu_write32(usb, RTL8188EU_REG_HISR, 0xffffffffu) < 0 ||
        rtl8188eu_write32(usb, RTL8188EU_REG_HISRE, 0xffffffffu) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_HISR, &hisr) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_HISRE, &hisre) < 0)
        return -EIO;
    usb_printk("rtl8188eu: IRQ status cleared HISR=%08x HISRE=%08x\n",
               hisr, hisre);
    return 0;
}

static void rtl8188eu_log_bb_rf_state(struct usb_dev_t *usb)
{
    uint16_t sys_func;
    uint8_t rf_ctrl;
    uint32_t fpga0;
    uint32_t fpga1;
    uint32_t cck;
    uint32_t ofdm0;
    uint32_t ofdm1;
    uint32_t hssi2;
    uint32_t lssi;
    uint32_t lssi_rb;
    uint32_t hspi_rb;

    if (rtl8188eu_read16(usb, RTL8188EU_REG_SYS_FUNC_EN, &sys_func) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_RF_CTRL, &rf_ctrl) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_FPGA0_RFMOD, &fpga0) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_FPGA1_RFMOD, &fpga1) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_CCK0_SYSTEM, &cck) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_OFDM0_TRXPATH, &ofdm0) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_OFDM1_TRXPATH, &ofdm1) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_XA_HSSI_PARAM2, &hssi2) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_XA_LSSI_PARAM, &lssi) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_XA_LSSI_READBACK, &lssi_rb) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_XA_HSPI_READBACK, &hspi_rb) < 0) {
        usb_printk("rtl8188eu: BB/RF register snapshot failed\n");
        return;
    }
    usb_printk("rtl8188eu: BB/RF SYS_FUNC=%04x RF_CTRL=%02x FPGA0=%08x FPGA1=%08x\n",
               sys_func, rf_ctrl, fpga0, fpga1);
    usb_printk("rtl8188eu: BB/RF CCK=%08x OFDM0_PATH=%08x OFDM1_PATH=%08x\n",
               cck, ofdm0, ofdm1);
    usb_printk("rtl8188eu: RF path A HSSI2=%08x LSSI=%08x LSSI_RB=%08x HSPI_RB=%08x\n",
               hssi2, lssi, lssi_rb, hspi_rb);
}

static void rtl8188eu_log_agc_cca_state(struct usb_dev_t *usb)
{
    uint32_t cck_cca;
    uint32_t trmux;
    uint32_t rx_detector;
    uint32_t agc1;
    uint32_t agc2;
    uint32_t agc_port;

    if (rtl8188eu_read32(usb, RTL8188EU_REG_CCK0_CCA, &cck_cca) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_OFDM0_TRMUX, &trmux) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_OFDM0_RX_DETECTOR1,
                         &rx_detector) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_OFDM0_XA_AGC_CORE1, &agc1) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_OFDM0_XA_AGC_CORE2, &agc2) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_AGC_TABLE_PORT, &agc_port) < 0) {
        usb_printk("rtl8188eu: AGC/CCA register snapshot failed\n");
        return;
    }
    usb_printk("rtl8188eu: AGC/CCA CCK_CCA=%08x TRMUX=%08x RXDET=%08x\n",
               cck_cca, trmux, rx_detector);
    usb_printk("rtl8188eu: AGC/CCA CORE1=%08x CORE2=%08x PORT=%08x IGI=%02x\n",
               agc1, agc2, agc_port, (unsigned int)(agc1 & 0x7f));
}

static int rtl8188eu_enable_bb_rf(struct usb_dev_t *usb)
{
    uint16_t sys_old;
    uint16_t sys_after;
    uint8_t rf_old;
    uint8_t rf_after;
    uint16_t sys_required = RTL8188EU_SYS_FUNC_BBRSTB |
                            RTL8188EU_SYS_FUNC_BB_GLB_RST;
    uint8_t rf_required = RTL8188EU_RF_EN | RTL8188EU_RF_RSTB |
                          RTL8188EU_RF_SDMRSTB;

    if (rtl8188eu_read16(usb, RTL8188EU_REG_SYS_FUNC_EN, &sys_old) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_RF_CTRL, &rf_old) < 0)
        return -EIO;
    if (rtl8188eu_write16(usb, RTL8188EU_REG_SYS_FUNC_EN,
                          sys_old | sys_required) < 0 ||
        rtl8188eu_write8(usb, RTL8188EU_REG_RF_CTRL,
                         rf_old | rf_required) < 0)
        goto restore;
    tick_delay(1000);
    if (rtl8188eu_read16(usb, RTL8188EU_REG_SYS_FUNC_EN, &sys_after) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_RF_CTRL, &rf_after) < 0)
        goto restore;
    usb_printk("rtl8188eu: BB/RF enable SYS_FUNC=%04x RF_CTRL=%02x\n",
               sys_after, rf_after);
    if ((sys_after & sys_required) != sys_required ||
        (rf_after & rf_required) != rf_required)
        goto restore;
    return 0;

restore:
    rtl8188eu_write8(usb, RTL8188EU_REG_RF_CTRL, rf_old);
    rtl8188eu_write16(usb, RTL8188EU_REG_SYS_FUNC_EN, sys_old);
    usb_printk("rtl8188eu: BB/RF enable failed; registers restored\n");
    return -EIO;
}

static int rtl8188eu_rf_read_path_a(struct usb_dev_t *usb, uint8_t reg,
                                    uint32_t *value)
{
    uint32_t hssi1;
    uint32_t hssi2;
    uint32_t command;
    uint32_t readback;
    uint16_t readback_reg;
    int ret = -EIO;

    if (!value)
        return -EINVAL;
    if (rtl8188eu_read32(usb, RTL8188EU_REG_XA_HSSI_PARAM1, &hssi1) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_XA_HSSI_PARAM2, &hssi2) < 0)
        return -EIO;
    command = (hssi2 & ~RTL8188EU_HSSI_READ_ADDR_MASK &
               ~RTL8188EU_HSSI_READ_EDGE) |
              ((uint32_t)reg << RTL8188EU_HSSI_READ_ADDR_SHIFT);
    if (rtl8188eu_write32(usb, RTL8188EU_REG_XA_HSSI_PARAM2,
                          command) < 0)
        goto restore;
    tick_delay(10);
    if (rtl8188eu_write32(usb, RTL8188EU_REG_XA_HSSI_PARAM2,
                          command | RTL8188EU_HSSI_READ_EDGE) < 0)
        goto restore;
    tick_delay(1000);
    readback_reg = (hssi1 & RTL8188EU_HSSI_PI_MODE) ?
                   RTL8188EU_REG_XA_HSPI_READBACK :
                   RTL8188EU_REG_XA_LSSI_READBACK;
    if (rtl8188eu_read32(usb, readback_reg, &readback) < 0)
        goto restore;
    *value = readback & RTL8188EU_RF_REG_MASK;
    ret = 0;
    usb_printk("rtl8188eu: RF read path=A reg=%02x mode=%s raw=%08x value=%05x\n",
               reg, (hssi1 & RTL8188EU_HSSI_PI_MODE) ? "PI" : "LSSI",
               readback, *value);

restore:
    if (rtl8188eu_write32(usb, RTL8188EU_REG_XA_HSSI_PARAM2, hssi2) < 0)
        ret = -EIO;
    return ret;
}

static int rtl8188eu_rf_write_path_a(struct usb_dev_t *usb, uint8_t reg,
                                     uint32_t value)
{
    uint32_t command = ((uint32_t)reg << 20) |
                       (value & RTL8188EU_RF_REG_MASK);

    if (rtl8188eu_write32(usb, RTL8188EU_REG_XA_LSSI_PARAM, command) < 0)
        return -EIO;
    tick_delay(1000);
    return 0;
}

static int rtl8188eu_load_bb_table(struct usb_dev_t *usb, const char *name,
                                   const uint32_t *table,
                                   unsigned int length, int agc)
{
    unsigned int i;

    if (!table || (length & 1))
        return -EINVAL;
    for (i = 0; i < length; i += 2) {
        uint32_t reg = table[i];
        uint32_t value = table[i + 1];

        if (reg >= 0xcdcdcdcdu) {
            usb_printk("rtl8188eu: %s unsupported condition index=%u "
                       "word=%08x\n", name, i, reg);
            return -EINVAL;
        }
        if (rtl8188eu_write32(usb, (uint16_t)reg, value) < 0) {
            usb_printk("rtl8188eu: %s write failed index=%u reg=%04x "
                       "value=%08x\n", name, i / 2, (unsigned int)reg,
                       value);
            return -EIO;
        }
        if (agc)
            tick_delay(1);
    }
    usb_printk("rtl8188eu: %s loaded entries=%u\n", name, length / 2);
    return 0;
}

static int rtl8188eu_load_rf_a_table(struct usb_dev_t *usb)
{
    unsigned int i;

    for (i = 0; i < RTL8188EE_RADIOA_1TARRAYLEN; i += 2) {
        uint32_t reg = RTL8188EE_RADIOA_1TARRAY[i];
        uint32_t value = RTL8188EE_RADIOA_1TARRAY[i + 1];

        if (reg == 0xffe) {
            tick_delay(50000);
            continue;
        }
        if (reg == 0xfd) {
            tick_delay(5000);
            continue;
        }
        if (reg == 0xfc) {
            tick_delay(1000);
            continue;
        }
        if (reg == 0xfb) {
            tick_delay(50);
            continue;
        }
        if (reg == 0xfa) {
            tick_delay(5);
            continue;
        }
        if (reg == 0xf9) {
            tick_delay(1);
            continue;
        }
        if (reg >= 0xcdcdcdcdu || reg > 0xff) {
            usb_printk("rtl8188eu: RF table unsupported index=%u "
                       "word=%08x\n", i, reg);
            return -EINVAL;
        }
        if (rtl8188eu_rf_write_path_a(usb, (uint8_t)reg, value) < 0) {
            usb_printk("rtl8188eu: RF table write failed index=%u reg=%02x "
                       "value=%05x\n", i / 2, (unsigned int)reg,
                       value & RTL8188EU_RF_REG_MASK);
            return -EIO;
        }
    }
    usb_printk("rtl8188eu: RF path A table loaded entries=%u\n",
               RTL8188EE_RADIOA_1TARRAYLEN / 2);
    return 0;
}

static int rtl8188eu_load_phy_tables(struct usb_dev_t *usb)
{
    uint32_t fpga0;
    uint32_t hssi2;
    uint32_t agc1;
    uint32_t agc_port;
    uint32_t rf18;

    if (rtl8188eu_load_bb_table(usb, "PHY BB",
                                RTL8188EEPHY_REG_1TARRAY,
                                RTL8188EEPHY_REG_1TARRAYLEN, 0) < 0 ||
        rtl8188eu_load_bb_table(usb, "AGC",
                                RTL8188EEAGCTAB_1TARRAY,
                                RTL8188EEAGCTAB_1TARRAYLEN, 1) < 0 ||
        rtl8188eu_load_rf_a_table(usb) < 0)
        return -EIO;
    if (rtl8188eu_read32(usb, RTL8188EU_REG_FPGA0_RFMOD, &fpga0) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_XA_HSSI_PARAM2, &hssi2) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_OFDM0_XA_AGC_CORE1, &agc1) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_AGC_TABLE_PORT,
                         &agc_port) < 0 ||
        rtl8188eu_rf_read_path_a(usb, RTL8188EU_RF_CHNLBW, &rf18) < 0)
        return -EIO;
    usb_printk("rtl8188eu: PHY table readback FPGA0=%08x HSSI2=%08x "
               "AGC1=%08x AGC_PORT=%08x RF18=%05x\n",
               fpga0, hssi2, agc1, agc_port, rf18);
    return 0;
}

static int rtl8188eu_load_mac_table(struct usb_dev_t *usb)
{
    unsigned int i;

    for (i = 0; i < RTL8188EEMAC_1T_ARRAYLEN; i += 2) {
        uint32_t reg = RTL8188EEMAC_1T_ARRAY[i];
        uint32_t value = RTL8188EEMAC_1T_ARRAY[i + 1];

        if (reg > 0xffff || value > 0xff) {
            usb_printk("rtl8188eu: MAC table unsupported index=%u "
                       "reg=%08x value=%08x\n", i / 2, reg, value);
            return -EINVAL;
        }
        if (rtl8188eu_write8(usb, (uint16_t)reg, (uint8_t)value) < 0) {
            usb_printk("rtl8188eu: MAC table write failed index=%u "
                       "reg=%04x value=%02x\n", i / 2,
                       (unsigned int)reg, (unsigned int)value);
            return -EIO;
        }
    }
    usb_printk("rtl8188eu: MAC table loaded entries=%u\n",
               RTL8188EEMAC_1T_ARRAYLEN / 2);
    return 0;
}

static int rtl8188eu_set_channel(struct usb_dev_t *usb, uint8_t channel)
{
    uint32_t old_value;
    uint32_t new_value;
    uint32_t readback;

    if (channel < 1 || channel > 14)
        return -EINVAL;
    if (rtl8188eu_rf_read_path_a(usb, RTL8188EU_RF_CHNLBW,
                                 &old_value) < 0)
        return -EIO;
    new_value = (old_value & ~RTL8188EU_RF_CHANNEL_MASK) | channel;
    if (rtl8188eu_rf_write_path_a(usb, RTL8188EU_RF_CHNLBW,
                                  new_value) < 0)
        return -EIO;
    if (rtl8188eu_rf_read_path_a(usb, RTL8188EU_RF_CHNLBW,
                                 &readback) < 0 ||
        (readback & RTL8188EU_RF_CHANNEL_MASK) != channel) {
        rtl8188eu_rf_write_path_a(usb, RTL8188EU_RF_CHNLBW, old_value);
        usb_printk("rtl8188eu: channel setup failed; RF_CHNLBW restored=%05x\n",
                   old_value);
        return -EIO;
    }
    usb_printk("rtl8188eu: channel set old=%05x new=%05x readback=%05x channel=%u\n",
               old_value, new_value, readback, channel);
    return 0;
}

static int rtl8188eu_enable_bb_modulation(struct usb_dev_t *usb)
{
    uint32_t old_value;
    uint32_t new_value;
    uint32_t readback;
    uint32_t required = RTL8188EU_BB_CCK_ENABLE |
                        RTL8188EU_BB_OFDM_ENABLE;

    if (rtl8188eu_read32(usb, RTL8188EU_REG_FPGA0_RFMOD,
                         &old_value) < 0)
        return -EIO;
    new_value = old_value | required;
    if (rtl8188eu_write32(usb, RTL8188EU_REG_FPGA0_RFMOD,
                          new_value) < 0)
        return -EIO;
    tick_delay(1000);
    if (rtl8188eu_read32(usb, RTL8188EU_REG_FPGA0_RFMOD,
                         &readback) < 0 ||
        (readback & required) != required) {
        rtl8188eu_write32(usb, RTL8188EU_REG_FPGA0_RFMOD, old_value);
        usb_printk("rtl8188eu: BB modulation enable failed; FPGA0 restored=%08x\n",
                   old_value);
        return -EIO;
    }
    usb_printk("rtl8188eu: BB modulation old=%08x new=%08x readback=%08x CCK=on OFDM=on\n",
               old_value, new_value, readback);
    return 0;
}

static int rtl8188eu_read8(struct usb_dev_t *usb, uint16_t reg, uint8_t *value)
{
    if (!usb || !value)
        return -EINVAL;

    if (!usb_ctrl_in(usb, value, RTL8188EU_USB_REQ_READ,
                     RTL8188EU_USB_REQ_REGS, (uint8_t)(reg >> 8),
                     (uint8_t)reg, 0, 1))
        return -EIO;
    return 0;
}

static int rtl8188eu_read16(struct usb_dev_t *usb, uint16_t reg,
                            uint16_t *value)
{
    uint8_t data[2];
    int ret;

    if (!value)
        return -EINVAL;
    ret = usb_ctrl_in(usb, data, RTL8188EU_USB_REQ_READ,
                      RTL8188EU_USB_REQ_REGS, (uint8_t)(reg >> 8),
                      (uint8_t)reg, 0, sizeof(data)) ? 0 : -EIO;
    if (ret < 0)
        return ret;
    *value = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
    return 0;
}

static int rtl8188eu_read32(struct usb_dev_t *usb, uint16_t reg,
                            uint32_t *value)
{
    uint8_t data[4];

    if (!value)
        return -EINVAL;
    if (rtl8188eu_read(usb, reg, data, sizeof(data)) < 0)
        return -EIO;
    *value = (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
             ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
    return 0;
}

static int rtl8188eu_read(struct usb_dev_t *usb, uint16_t reg,
                          uint8_t *data, uint16_t length)
{
    int ok;

    if (!usb || !data || !length)
        return -EINVAL;
    ok = usb_ctrl_in(usb, data, RTL8188EU_USB_REQ_READ,
                     RTL8188EU_USB_REQ_REGS, (uint8_t)(reg >> 8),
                     (uint8_t)reg, 0, length);
    if (rtl8188eu_fw_usb_trace) {
        unsigned int i;
        usb_printk("rtl8188eu: FW USB IN type=c0 req=05 reg=%04x "
                   "index=0000 len=%u ok=%d data=", reg, length, ok);
        if (ok) {
            unsigned int shown = length <= 16 ? length : 8;
            for (i = 0; i < shown; i++)
                usb_printk(" %02x", data[i]);
            if (shown < length)
                usb_printk(" ...");
        } else {
            usb_printk(" <none>");
        }
        usb_printk("\n");
    }
    if (!ok)
        return -EIO;
    return 0;
}

static int rtl8188eu_read_mac_address(struct usb_dev_t *usb, uint8_t mac[6])
{
    unsigned int i;
    int all_zero = 1;
    int all_ff = 1;
    int ret;

    ret = rtl8188eu_read(usb, RTL8188EU_REG_MACID, mac, 6);
    if (ret < 0)
        return ret;
    for (i = 0; i < 6; i++) {
        if (mac[i] != 0x00)
            all_zero = 0;
        if (mac[i] != 0xff)
            all_ff = 0;
    }
    if (all_zero || all_ff || (mac[0] & 0x01)) {
        usb_printk("rtl8188eu: invalid MACID %02x:%02x:%02x:%02x:%02x:%02x\n",
                   mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return -EIO;
    }
    usb_printk("rtl8188eu: MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return 0;
}

static void rtl8188eu_log_efuse_status(struct usb_dev_t *usb)
{
    uint8_t value;
    if (rtl8188eu_read8(usb, RTL8188EU_REG_EFUSE_ACCESS, &value) == 0)
        usb_printk("rtl8188eu: EFUSE_ACCESS=0x%02x (diagnostic)\n", value);
    else
        usb_printk("rtl8188eu: EFUSE_ACCESS read failed\n");
}

/* Read one physical EFUSE byte. This is deliberately read-only: no logical
 * map parsing or programming is attempted until the controller response is
 * verified on hardware. */
static int rtl8188eu_efuse_read_byte(struct usb_dev_t *usb, uint16_t addr,
                                     uint8_t *value)
{
    uint8_t addr_high;
    uint8_t status;
    int i;

    if (!value)
        return -EINVAL;
    /* EFUSE_CTRL is a 32-bit register: data is bits 0..7, address is
     * bits 8..17, and bit 31 is cleared to start a read then set by HW. */
    if (rtl8188eu_read8(usb, RTL8188EU_REG_EFUSE_ADDR_HIGH,
                        &addr_high) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_EFUSE_STATUS, &status) < 0)
        return -EIO;
    if (rtl8188eu_write8(usb, RTL8188EU_REG_EFUSE_ADDR_LOW,
                         (uint8_t)addr) < 0)
        return -EIO;
    addr_high = (addr_high & ~RTL8188EU_EFUSE_ADDR_HIGH_MASK) |
                ((addr >> 8) & RTL8188EU_EFUSE_ADDR_HIGH_MASK);
    if (rtl8188eu_write8(usb, RTL8188EU_REG_EFUSE_ADDR_HIGH,
                         addr_high) < 0)
        return -EIO;
    if (rtl8188eu_write8(usb, RTL8188EU_REG_EFUSE_STATUS,
                         status & ~RTL8188EU_EFUSE_VALID) < 0)
        return -EIO;
    for (i = 0; i < 20; i++) {
        tick_delay(10);
        if (rtl8188eu_read8(usb, RTL8188EU_REG_EFUSE_STATUS, &status) < 0)
            return -EIO;
        if (status & RTL8188EU_EFUSE_VALID) {
            if (rtl8188eu_read8(usb, RTL8188EU_REG_EFUSE_CTRL, value) < 0)
                return -EIO;
            return 0;
        }
    }
    return -EIO;
}

static int rtl8188eu_efuse_power_on(struct usb_dev_t *usb,
                                    uint8_t *access_old,
                                    uint8_t *func_old,
                                    uint8_t *clock_old)
{
    uint8_t access;
    uint8_t func;
    uint8_t clock;

    if (rtl8188eu_read8(usb, RTL8188EU_REG_EFUSE_ACCESS, access_old) < 0)
        return -EIO;
    if (rtl8188eu_read8(usb, RTL8188EU_REG_SYS_FUNC_EN + 1, func_old) < 0)
        return -EIO;
    if (rtl8188eu_read8(usb, RTL8188EU_REG_SYS_CLKR, clock_old) < 0)
        return -EIO;
    if (rtl8188eu_write8(usb, RTL8188EU_REG_EFUSE_ACCESS,
                         RTL8188EU_EFUSE_ACCESS_ON) < 0)
        goto restore;
    if (rtl8188eu_write8(usb, RTL8188EU_REG_SYS_FUNC_EN + 1,
                         *func_old | RTL8188EU_SYS_FUNC_ELDR) < 0)
        goto restore;
    if (rtl8188eu_write8(usb, RTL8188EU_REG_SYS_CLKR,
                         *clock_old | RTL8188EU_SYS_CLK_LOADER |
                         RTL8188EU_SYS_CLK_ANA8M) < 0)
        goto restore;
    if (rtl8188eu_read8(usb, RTL8188EU_REG_EFUSE_ACCESS, &access) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_SYS_FUNC_EN + 1, &func) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_SYS_CLKR, &clock) < 0)
        goto restore;
    usb_printk("rtl8188eu: EFUSE power access=%02x func=%02x clock=%02x\n",
               access, func, clock);
    if (access != RTL8188EU_EFUSE_ACCESS_ON ||
        !(func & RTL8188EU_SYS_FUNC_ELDR) ||
        (clock & (RTL8188EU_SYS_CLK_LOADER | RTL8188EU_SYS_CLK_ANA8M)) !=
        (RTL8188EU_SYS_CLK_LOADER | RTL8188EU_SYS_CLK_ANA8M))
        goto restore;
    return 0;

restore:
    rtl8188eu_write8(usb, RTL8188EU_REG_SYS_CLKR, *clock_old);
    rtl8188eu_write8(usb, RTL8188EU_REG_SYS_FUNC_EN + 1, *func_old);
    rtl8188eu_write8(usb, RTL8188EU_REG_EFUSE_ACCESS, *access_old);
    return -EIO;
}

static int rtl8188eu_efuse_power_off(struct usb_dev_t *usb,
                                     uint8_t func_old,
                                     uint8_t clock_old,
                                     uint8_t *access_after)
{
    int ret = 0;

    if (rtl8188eu_write8(usb, RTL8188EU_REG_SYS_CLKR, clock_old) < 0)
        ret = -EIO;
    if (rtl8188eu_write8(usb, RTL8188EU_REG_SYS_FUNC_EN + 1, func_old) < 0)
        ret = -EIO;
    if (rtl8188eu_write8(usb, RTL8188EU_REG_EFUSE_ACCESS,
                         RTL8188EU_EFUSE_ACCESS_OFF) < 0)
        ret = -EIO;
    if (rtl8188eu_read8(usb, RTL8188EU_REG_EFUSE_ACCESS,
                        access_after) < 0)
        ret = -EIO;
    return ret;
}

static void rtl8188eu_log_efuse_sample(struct usb_dev_t *usb)
{
    uint8_t data[128];
    uint8_t map[256];
    uint8_t efuse_mac[6];
    uint8_t macid_after[6];
    uint8_t access_old = 0;
    uint8_t func_old = 0;
    uint8_t clock_old = 0;
    uint8_t access_after = 0xff;
    int console_old;
    unsigned int i;
    unsigned int payload_len;
    unsigned int payload_pos;
    int mac_valid;

    console_old = usb_log_get_console();
    usb_log_set_console(0);
    if (rtl8188eu_efuse_power_on(usb, &access_old, &func_old,
                                 &clock_old) < 0) {
        usb_log_set_console(console_old);
        usb_printk("rtl8188eu: EFUSE power-on failed\n");
        return;
    }
    for (i = 0; i < sizeof(data); i++) {
        if (rtl8188eu_efuse_read_byte(usb, (uint16_t)i, &data[i]) < 0) {
            rtl8188eu_efuse_power_off(usb, func_old, clock_old,
                                      &access_after);
            usb_log_set_console(console_old);
            usb_printk("rtl8188eu: EFUSE read stopped at byte %u\n", i);
            return;
        }
    }
    if (rtl8188eu_efuse_power_off(usb, func_old, clock_old,
                                  &access_after) < 0) {
        usb_log_set_console(console_old);
        usb_printk("rtl8188eu: EFUSE power-off failed access=%02x\n",
                   access_after);
        return;
    }
    usb_log_set_console(console_old);
    usb_printk("rtl8188eu: EFUSE access after read=%02x\n", access_after);
    usb_printk("rtl8188eu: EFUSE[0..127]=");
    for (i = 0; i < sizeof(data); i++)
        usb_printk(" %02x", data[i]);
    usb_printk("\n");
    for (i = 0; i < sizeof(map); i++)
        map[i] = 0xff;
    /* Decode physical packets into the logical EFUSE map. */
    i = 0;
    while (i < sizeof(data) && data[i] != 0xff) {
        unsigned int packet = i;
        uint8_t header = data[i++];
        uint8_t section;
        uint8_t word_en;
        if ((header & 0x1f) == 0x0f) {
            if (i >= sizeof(data))
                break;
            section = (uint8_t)(((header & 0xe0) >> 5) |
                                ((data[i] & 0xf0) >> 1));
            word_en = data[i++] & 0x0f;
        } else {
            section = (header >> 4) & 0x0f;
            word_en = header & 0x0f;
        }
        payload_len = 0;
        for (unsigned int word = 0; word < 4; word++)
            if (!(word_en & (1u << word)))
                payload_len += 2;
        if (i + payload_len > sizeof(data))
            break;
        usb_printk("rtl8188eu: EFUSE packet phys=0x%02x header=0x%02x section=0x%02x word_en=0x%02x payload_len=%u payload=",
                   packet, header, section, word_en, payload_len);
        for (unsigned int n = 0; n < payload_len; n++)
            usb_printk(" %02x", data[i + n]);
        usb_printk("\n");
        payload_pos = i;
        for (unsigned int word = 0; word < 4; word++) {
            unsigned int logical = (unsigned int)section * 8 + word * 2;
            if (!(word_en & (1u << word))) {
                if (payload_pos + 1 >= sizeof(data) ||
                    logical + 1 >= sizeof(map))
                    break;
                map[logical] = data[payload_pos++];
                map[logical + 1] = data[payload_pos++];
            }
        }
        i += payload_len;
    }
    for (i = 0; i < sizeof(efuse_mac); i++)
        efuse_mac[i] = map[0xd7 + i];
    mac_valid = !(efuse_mac[0] & 0x01);
    for (i = 0; i < sizeof(efuse_mac); i++) {
        if (efuse_mac[i] != 0x00)
            break;
    }
    if (i == sizeof(efuse_mac))
        mac_valid = 0;
    for (i = 0; i < sizeof(efuse_mac); i++) {
        if (efuse_mac[i] != 0xff)
            break;
    }
    if (i == sizeof(efuse_mac))
        mac_valid = 0;
    usb_printk("rtl8188eu: EFUSE MAC %02x:%02x:%02x:%02x:%02x:%02x valid=%s\n",
               efuse_mac[0], efuse_mac[1], efuse_mac[2], efuse_mac[3],
               efuse_mac[4], efuse_mac[5], mac_valid ? "yes" : "no");
    if (!mac_valid)
        return;
    for (i = 0; i < sizeof(efuse_mac); i++) {
        if (rtl8188eu_write8(usb, RTL8188EU_REG_MACID + i,
                             efuse_mac[i]) < 0) {
            usb_printk("rtl8188eu: MACID write failed at byte %u\n", i);
            return;
        }
    }
    if (rtl8188eu_read(usb, RTL8188EU_REG_MACID, macid_after,
                       sizeof(macid_after)) < 0) {
        usb_printk("rtl8188eu: MACID readback failed\n");
        return;
    }
    for (i = 0; i < sizeof(efuse_mac); i++)
        rtl8188eu.mac_addr[i] = macid_after[i];
    usb_printk("rtl8188eu: MACID after EFUSE %02x:%02x:%02x:%02x:%02x:%02x\n",
               macid_after[0], macid_after[1], macid_after[2],
               macid_after[3], macid_after[4], macid_after[5]);
}

static void rtl8188eu_initialize_mac(struct usb_dev_t *usb)
{
    uint8_t tx_test[RTL8188EU_TX_DESC_SIZE + 34];
    uint8_t rx_test[RTL8188EU_RX_TEST_SIZE];
    struct usb_transfer_t rx_transfer;
    uint16_t rx_length = 0;
    uint32_t rf_chnlbw;
    size_t tx_length;
    uint8_t channel;

    if (rtl8188eu_read_mac_address(usb, rtl8188eu.mac_addr) == 0) {
        usb_printk("rtl8188eu: MACID shadow valid; EFUSE read skipped\n");
    } else {
        usb_printk("rtl8188eu: MACID shadow invalid; reading EFUSE\n");
        rtl8188eu_log_efuse_status(usb);
        rtl8188eu_log_efuse_sample(usb);
    }
    if (rtl8188eu_load_mac_table(usb) < 0)
        usb_printk("rtl8188eu: MAC table load failed\n");
    rtl8188eu_log_tx_queue_state(usb);
    if (rtl8188eu_initialize_tx_pages(usb) < 0)
        usb_printk("rtl8188eu: TX page setup not ready\n");
    rtl8188eu_log_tx_queue_state(usb);
    if (rtl8188eu_initialize_rx_filter(usb) < 0)
        usb_printk("rtl8188eu: RX filter setup failed\n");
    if (rtl8188eu_disable_rx_aggregation(usb) < 0)
        usb_printk("rtl8188eu: RX aggregation state not ready\n");
    rtl8188eu_log_bb_rf_state(usb);
    if (rtl8188eu_enable_bb_rf(usb) < 0)
        usb_printk("rtl8188eu: BB/RF not ready\n");
    else if (rtl8188eu_load_phy_tables(usb) < 0)
        usb_printk("rtl8188eu: PHY/AGC/RF table load failed\n");
    if (rtl8188eu_enable_bb_modulation(usb) < 0)
        usb_printk("rtl8188eu: BB modulation not ready\n");
    rtl8188eu_log_bb_rf_state(usb);
    if (rtl8188eu_rf_read_path_a(usb, RTL8188EU_RF_CHNLBW,
                                 &rf_chnlbw) == 0)
        usb_printk("rtl8188eu: RF_CHNLBW=%05x channel_field=%u\n",
                   rf_chnlbw,
                   (unsigned int)(rf_chnlbw & RTL8188EU_RF_CHANNEL_MASK));
    else
        usb_printk("rtl8188eu: RF_CHNLBW read failed\n");
    rtl8188eu_log_agc_cca_state(usb);
    tx_length = rtl8188eu_build_probe_request(tx_test, sizeof(tx_test),
                                              rtl8188eu.mac_addr);
    usb_printk("rtl8188eu: TX pipes test=EP%d secondary=EP%d%s\n",
               rtl8188eu.bulk_out->addr,
               rtl8188eu.bulk_out2 ? rtl8188eu.bulk_out2->addr :
                                     rtl8188eu.bulk_out->addr,
               rtl8188eu.bulk_out2 ? "" : " (shared)");
    usb_printk("rtl8188eu: TX probe descriptor prepared len=%u EP%d submit=%s\n",
               (unsigned int)tx_length, rtl8188eu.bulk_out->addr,
               RTL8188EU_TX_TEST_ENABLE ? "yes" : "no");
    usb_printk("rtl8188eu: TX descriptor checksum=%s bytes=",
               rtl8188eu_txdesc_checksum_valid(tx_test) ? "valid" : "invalid");
    for (unsigned int i = 0; i < RTL8188EU_TX_DESC_SIZE; i++)
        usb_printk(" %02x", tx_test[i]);
    usb_printk("\n");
    usb_printk("rtl8188eu: TX probe frame bytes=");
    for (unsigned int i = RTL8188EU_TX_DESC_SIZE; i < tx_length; i++)
        usb_printk(" %02x", tx_test[i]);
    usb_printk("\n");
    rtl8188eu.scan_result_count = 0;
    A_memset(rtl8188eu_scan_results, 0, sizeof(rtl8188eu_scan_results));
    if (RTL8188EU_TX_TEST_ENABLE && tx_length) {
        for (channel = RTL8188EU_SCAN_FIRST_CHANNEL;
             channel <= RTL8188EU_SCAN_LAST_CHANNEL; channel++) {
            int rx_started = 0;
            int ret;

            ret = rtl8188eu_set_channel(usb, channel);
            if (ret < 0) {
                usb_printk("rtl8188eu: scan channel=%u tune failed=%d\n",
                           channel, ret);
                continue;
            }
            tick_delay(2000);
            if (rtl8188eu_clear_interrupt_status(usb) < 0)
                usb_printk("rtl8188eu: scan channel=%u IRQ clear failed\n",
                           channel);
            if (RTL8188EU_RX_TEST_ENABLE) {
                ret = rtl8188eu_bulk_in_start(&rtl8188eu, &rx_transfer,
                                              rx_test, sizeof(rx_test));
                rx_started = ret == 0;
            }
            ret = rtl8188eu_bulk_out(&rtl8188eu, rtl8188eu.bulk_out,
                                     tx_test, tx_length);
            usb_printk("rtl8188eu: scan channel=%u probe=%s rx_arm=%s\n",
                       channel, ret == 0 ? "sent" : "failed",
                       rx_started ? "yes" : "no");
            if (rx_started) {
                ret = rtl8188eu_bulk_in_finish(&rx_transfer, &rx_length);
                if (ret == 0 && rx_length)
                    rtl8188eu_parse_rx_packet(rx_test, rx_length);
                else
                    usb_printk("rtl8188eu: scan channel=%u rx=%s actual=%u\n",
                               channel, ret == 0 ? "empty" : "timeout",
                               rx_length);
            }
        }
        rtl8188eu_print_scan_results();
        rtl8188eu_log_rx_runtime_state(usb, "scan-complete");
    }
}

int rtl8188eu_wifi_scan(void)
{
    if (!rtl8188eu.usb || !rtl8188eu.firmware_loaded)
        return -ENODEV;
    /* Initialization currently performs the same active 1-13 channel scan. */
    rtl8188eu_initialize_mac(rtl8188eu.usb);
    return (int)rtl8188eu.scan_result_count;
}

int rtl8188eu_wifi_connect(const char *ssid, const char *password)
{
    unsigned int ssid_length = 0, password_length = 0, i, j;

    if (!rtl8188eu.usb || !rtl8188eu.firmware_loaded)
        return -ENODEV;
    if (!ssid || !password)
        return -EINVAL;
    while (ssid[ssid_length] && ssid_length <= RTL8188EU_MAX_SSID_LEN)
        ssid_length++;
    while (password[password_length] && password_length < 64)
        password_length++;
    if (!ssid_length || ssid_length > RTL8188EU_MAX_SSID_LEN ||
        password_length < 8 || password_length > 63)
        return -EINVAL;

    rtl8188eu_wifi_scan();
    for (i = 0; i < rtl8188eu.scan_result_count; i++) {
        if (rtl8188eu_scan_results[i].ssid_length != ssid_length)
            continue;
        for (j = 0; j < ssid_length; j++)
            if (rtl8188eu_scan_results[i].ssid[j] != (uint8_t)ssid[j])
                break;
        if (j != ssid_length)
            continue;
        rtl8188eu.selected_result = (int)i;
        rtl8188eu.wifi_state = RTL8188EU_WIFI_FAILED;
        rtl8188eu.last_error = -ENOTSUP;
        usb_printk("rtl8188eu: target found channel=%u; WPA2 handshake and "
                   "key installation are not implemented\n",
                   rtl8188eu_scan_results[i].channel);
        return -ENOTSUP;
    }
    rtl8188eu.wifi_state = RTL8188EU_WIFI_FAILED;
    rtl8188eu.last_error = -ENOENT;
    return -ENOENT;
}

int rtl8188eu_wifi_disconnect(void)
{
    if (!rtl8188eu.usb)
        return -ENODEV;
    rtl8188eu.selected_result = -1;
    rtl8188eu.last_error = 0;
    rtl8188eu.wifi_state = rtl8188eu.firmware_loaded ?
                           RTL8188EU_WIFI_READY : RTL8188EU_WIFI_ABSENT;
    usb_printk("rtl8188eu: disconnected\n");
    return 0;
}

int rtl8188eu_wifi_status(void)
{
    usb_printk("rtl8188eu: status state=%u firmware=%s networks=%u error=%d\n",
               rtl8188eu.wifi_state,
               rtl8188eu.firmware_loaded ? "ready" : "not-ready",
               rtl8188eu.scan_result_count, rtl8188eu.last_error);
    return rtl8188eu.wifi_state;
}

static int rtl8188eu_write(struct usb_dev_t *usb, uint16_t reg,
                           const uint8_t *data, uint16_t length)
{
    int ok;
    int trace_write = rtl8188eu_fw_usb_trace;

    if (!usb || (!data && length))
        return -EINVAL;
    ok = usb_ctrl_out(usb, (void *)data, RTL8188EU_USB_REQ_WRITE,
                      RTL8188EU_USB_REQ_REGS, (uint8_t)(reg >> 8),
                      (uint8_t)reg, 0, length);
    if (rtl8188eu_fw_usb_trace && length == 4 &&
        reg >= RTL8188EU_FW_START &&
        reg < RTL8188EU_FW_START + RTL8188EU_FW_PAGE_SIZE) {
        rtl8188eu_fw_data_writes++;
        trace_write = rtl8188eu_fw_data_writes <= 2 ||
                      reg == RTL8188EU_FW_START ||
                      (rtl8188eu_fw_data_writes & 0xff) == 0 ||
                      rtl8188eu_fw_data_writes == rtl8188eu_fw_data_total;
    }
    if (trace_write) {
        usb_printk("rtl8188eu: FW USB OUT type=40 req=05 reg=%04x "
                   "index=0000 len=%u ok=%d", reg, length, ok);
        if (length && data) {
            usb_printk(" data=");
            if (length <= 8) {
                unsigned int i;
                for (i = 0; i < length; i++)
                    usb_printk(" %02x", data[i]);
            } else {
                usb_printk(" %02x %02x %02x %02x ... %02x %02x %02x %02x",
                           data[0], data[1], data[2], data[3],
                           data[length - 4], data[length - 3],
                           data[length - 2], data[length - 1]);
            }
        }
        usb_printk("\n");
    }
    if (!ok)
        return -EIO;
    return 0;
}

static int rtl8188eu_write8(struct usb_dev_t *usb, uint16_t reg, uint8_t value)
{
    return rtl8188eu_write(usb, reg, &value, 1);
}

static int rtl8188eu_write16(struct usb_dev_t *usb, uint16_t reg,
                             uint16_t value)
{
    uint8_t data[2];

    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
    return rtl8188eu_write(usb, reg, data, sizeof(data));
}

static int rtl8188eu_write32(struct usb_dev_t *usb, uint16_t reg,
                             uint32_t value)
{
    uint8_t data[4];

    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
    data[2] = (uint8_t)(value >> 16);
    data[3] = (uint8_t)(value >> 24);
    return rtl8188eu_write(usb, reg, data, sizeof(data));
}

static void rtl8188eu_log_preinit_snapshot(struct usb_dev_t *usb)
{
    uint16_t iso;
    uint16_t sys_func;
    uint16_t aps;
    uint16_t sys_clkr;
    uint16_t rsv;
    uint16_t cr;
    uint16_t trxdma;
    uint16_t rx_boundary;
    uint32_t mcu_fwdl;
    uint32_t rqpn;
    uint8_t ldo;
    uint8_t schmitt;
    uint8_t pbp;
    uint8_t tx_boundary;

    if (rtl8188eu_read16(usb, RTL8188EU_REG_SYS_ISO_CTRL, &iso) < 0 ||
        rtl8188eu_read16(usb, RTL8188EU_REG_SYS_FUNC_EN, &sys_func) < 0 ||
        rtl8188eu_read16(usb, RTL8188EU_REG_APS_FSMCO, &aps) < 0 ||
        rtl8188eu_read16(usb, RTL8188EU_REG_SYS_CLKR, &sys_clkr) < 0 ||
        rtl8188eu_read16(usb, RTL8188EU_REG_RSV_CTRL, &rsv) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_MCUFWDL, &mcu_fwdl) < 0 ||
        rtl8188eu_read16(usb, RTL8188EU_REG_CR, &cr) < 0 ||
        rtl8188eu_read8(usb, 0x0023, &ldo) < 0 ||
        rtl8188eu_read8(usb, 0x0026, &schmitt) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_PBP, &pbp) < 0 ||
        rtl8188eu_read16(usb, RTL8188EU_REG_TRXDMA_CTRL, &trxdma) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_TRXFF_BNDY, &tx_boundary) < 0 ||
        rtl8188eu_read16(usb, RTL8188EU_REG_TRXFF_BNDY + 2,
                         &rx_boundary) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_RQPN, &rqpn) < 0) {
        usb_printk("rtl8188eu: PREINIT snapshot read failed\n");
        return;
    }
    usb_printk("rtl8188eu: PREINIT SYS ISO=%04x FUNC=%04x APS=%04x "
               "CLK=%04x RSV=%04x\n", iso, sys_func, aps, sys_clkr, rsv);
    usb_printk("rtl8188eu: PREINIT FW MCUFWDL=%08x CR=%04x "
               "LDO23=%02x SCHMITT26=%02x\n",
               mcu_fwdl, cr, ldo, schmitt);
    usb_printk("rtl8188eu: PREINIT BUF PBP=%02x TRXDMA=%04x "
               "TXBNDY=%02x RXBNDY=%04x RQPN=%08x\n",
               pbp, trxdma, tx_boundary, rx_boundary, rqpn);
}

static int rtl8188eu_recover_stale_power_state(struct usb_dev_t *usb)
{
    uint8_t mcu_fwdl;
    uint8_t sys_func_high;
    uint8_t sys_after;
    uint8_t aps_high;
    uint8_t lpldo;
    uint16_t aps_after;
    uint32_t mcu_after;
    unsigned int attempt;

    if (rtl8188eu_read8(usb, RTL8188EU_REG_MCUFWDL, &mcu_fwdl) < 0)
        return -EIO;
    if (!(mcu_fwdl & (RTL8188EU_MCUFWDL_RDY |
                      RTL8188EU_WINTINI_RDY |
                      RTL8188EU_MCUFWDL_RAM_SEL)))
        return 0;

    usb_printk("rtl8188eu: stale power state MCUFWDL=0x%02x; "
               "stopping MAC/MCU before power-on\n", mcu_fwdl);

    /* Linux RTL8188E_TRANS_ACT_TO_CARDEMU. A failed firmware start can
     * leave the MAC active even after CR/MCUFWDL are cleared, so return the
     * hardware power state machine to card-emulation before powering on. */
    if (rtl8188eu_write8(usb, 0x001f, 0x00) < 0 ||
        rtl8188eu_read8(usb, 0x0023, &lpldo) < 0 ||
        rtl8188eu_write8(usb, 0x0023, lpldo | 0x10) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_APS_FSMCO + 1,
                        &aps_high) < 0 ||
        rtl8188eu_write8(usb, RTL8188EU_REG_APS_FSMCO + 1,
                         aps_high | 0x02) < 0)
        return -EIO;
    for (attempt = 0; attempt < 1000; attempt++) {
        if (rtl8188eu_read8(usb, RTL8188EU_REG_APS_FSMCO + 1,
                            &aps_high) < 0)
            return -EIO;
        if (!(aps_high & 0x02))
            break;
        tick_delay(10);
    }
    if (aps_high & 0x02) {
        usb_printk("rtl8188eu: ACT->CARDEMU timeout APS+1=0x%02x\n",
                   aps_high);
        return -EIO;
    }

    if (rtl8188eu_write16(usb, RTL8188EU_REG_CR, 0x0000) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_SYS_FUNC_EN + 1,
                        &sys_func_high) < 0 ||
        rtl8188eu_write8(usb, RTL8188EU_REG_SYS_FUNC_EN + 1,
                         sys_func_high &
                         (uint8_t)~RTL8188EU_SYS_FUNC_CPU_ENABLE) < 0 ||
        rtl8188eu_write32(usb, RTL8188EU_REG_MCUFWDL, 0x00000000u) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_SYS_FUNC_EN + 1,
                        &sys_after) < 0 ||
        rtl8188eu_read16(usb, RTL8188EU_REG_APS_FSMCO,
                         &aps_after) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_MCUFWDL, &mcu_after) < 0)
        return -EIO;
    usb_printk("rtl8188eu: stale power state cleared SYS_FUNC+1=%02x "
               "APS=%04x MCUFWDL=%08x\n",
               sys_after, aps_after, mcu_after);
    if ((sys_after & RTL8188EU_SYS_FUNC_CPU_ENABLE) ||
        (mcu_after & ~((uint32_t)RTL8188EU_FWDL_CHKSUM_RPT)) != 0)
        return -EIO;
    return 0;
}

static int rtl8188eu_power_on(struct usb_dev_t *usb)
{
    uint8_t rsv_ctrl;
    uint8_t aps_high;
    uint8_t aps_after;
    uint8_t power_ready;
    uint8_t sys_func_low;
    uint8_t loader_func;
    uint8_t loader_clock;
    uint8_t value8;
    uint16_t iso_ctrl;
    uint16_t aps_fsmco;
    uint16_t cr;
    unsigned int attempt;
    int ret;

    ret = rtl8188eu_read8(usb, RTL8188EU_REG_RSV_CTRL, &rsv_ctrl);
    if (ret < 0)
        return ret;
    ret = rtl8188eu_read16(usb, RTL8188EU_REG_APS_FSMCO, &aps_fsmco);
    if (ret < 0)
        return ret;
    usb_printk("rtl8188eu: power RSV_CTRL=0x%02x APS_FSMCO=0x%04x\n",
               rsv_ctrl, aps_fsmco);

    /* Linux reads EFUSE before HAL init and leaves loader power/clock on. */
    ret = rtl8188eu_read16(usb, RTL8188EU_REG_SYS_ISO_CTRL, &iso_ctrl);
    if (ret < 0 || rtl8188eu_write16(usb, RTL8188EU_REG_SYS_ISO_CTRL,
                                     iso_ctrl | 0x8000) < 0)
        return -EIO;
    ret = rtl8188eu_read8(usb, RTL8188EU_REG_SYS_FUNC_EN + 1,
                          &loader_func);
    if (ret < 0 || rtl8188eu_write8(usb, RTL8188EU_REG_SYS_FUNC_EN + 1,
                                    loader_func |
                                    RTL8188EU_SYS_FUNC_ELDR) < 0)
        return -EIO;
    ret = rtl8188eu_read8(usb, RTL8188EU_REG_SYS_CLKR, &loader_clock);
    if (ret < 0 || rtl8188eu_write8(usb, RTL8188EU_REG_SYS_CLKR,
                                    loader_clock |
                                    RTL8188EU_SYS_CLK_LOADER |
                                    RTL8188EU_SYS_CLK_ANA8M) < 0)
        return -EIO;
    if (rtl8188eu_read16(usb, RTL8188EU_REG_SYS_ISO_CTRL, &iso_ctrl) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_SYS_FUNC_EN + 1,
                        &loader_func) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_SYS_CLKR, &loader_clock) < 0)
        return -EIO;
    usb_printk("rtl8188eu: loader ready ISO=%04x SYS_FUNC+1=%02x "
               "SYS_CLKR=%02x\n", iso_ctrl, loader_func, loader_clock);
    if (!(iso_ctrl & 0x8000) ||
        !(loader_func & RTL8188EU_SYS_FUNC_ELDR) ||
        (loader_clock & (RTL8188EU_SYS_CLK_LOADER |
                         RTL8188EU_SYS_CLK_ANA8M)) !=
        (RTL8188EU_SYS_CLK_LOADER | RTL8188EU_SYS_CLK_ANA8M))
        return -EIO;

    /* RTL8188E_TRANS_CARDEMU_TO_ACT from Linux rtl8188eu/pwrseq.h. */
    for (attempt = 0; attempt < 1000; attempt++) {
        ret = rtl8188eu_read8(usb, RTL8188EU_REG_APS_FSMCO + 2,
                              &power_ready);
        if (ret < 0)
            return ret;
        if (power_ready & 0x02)
            break;
        tick_delay(10);
    }
    if (!(power_ready & 0x02)) {
        usb_printk("rtl8188eu: power-ready timeout APS+2=0x%02x\n",
                   power_ready);
        return -EIO;
    }
    ret = rtl8188eu_read8(usb, RTL8188EU_REG_SYS_FUNC_EN, &sys_func_low);
    if (ret < 0 || rtl8188eu_write8(usb, RTL8188EU_REG_SYS_FUNC_EN,
                                    sys_func_low & (uint8_t)~0x03) < 0)
        return -EIO;
    sys_func_low &= (uint8_t)~0x03;
    ret = rtl8188eu_read8(usb, 0x0026, &value8);
    if (ret < 0 || rtl8188eu_write8(usb, 0x0026, value8 | 0x80) < 0)
        return -EIO;
    ret = rtl8188eu_read8(usb, RTL8188EU_REG_APS_FSMCO + 1, &aps_high);
    if (ret < 0)
        return ret;
    aps_high &= (uint8_t)~(0x80 | 0x18);
    ret = rtl8188eu_write8(usb, RTL8188EU_REG_APS_FSMCO + 1, aps_high);
    if (ret < 0)
        return ret;
    ret = rtl8188eu_write8(usb, RTL8188EU_REG_APS_FSMCO + 1,
                           aps_high | 0x01);
    if (ret < 0)
        return ret;
    for (attempt = 0; attempt < 1000; attempt++) {
        ret = rtl8188eu_read8(usb, RTL8188EU_REG_APS_FSMCO + 1,
                              &aps_after);
        if (ret < 0)
            return ret;
        if (!(aps_after & 0x01))
            break;
        tick_delay(10);
    }
    if (aps_after & 0x01) {
        usb_printk("rtl8188eu: APFM_ONMAC timeout APS+1=0x%02x\n",
                   aps_after);
        return -EIO;
    }
    ret = rtl8188eu_read8(usb, 0x0023, &value8);
    if (ret < 0 || rtl8188eu_write8(usb, 0x0023,
                                    value8 & (uint8_t)~0x10) < 0)
        return -EIO;
    ret = rtl8188eu_read16(usb, RTL8188EU_REG_APS_FSMCO, &aps_fsmco);
    if (ret < 0)
        return ret;
    usb_printk("rtl8188eu: CARDEMU->ACT ready APS_FSMCO=0x%04x "
               "SYS_FUNC_LO=0x%02x polls=%u\n",
               aps_fsmco, sys_func_low, attempt + 1);

    if (aps_fsmco & RTL8188EU_APS_HW_SUSPEND) {
        aps_high = (uint8_t)(aps_fsmco >> 8);
        ret = rtl8188eu_write8(usb, RTL8188EU_REG_APS_FSMCO + 1,
                               aps_high & (uint8_t)~0x08);
        if (ret < 0)
            return ret;
        ret = rtl8188eu_read16(usb, RTL8188EU_REG_APS_FSMCO, &aps_fsmco);
        if (ret < 0)
            return ret;
        usb_printk("rtl8188eu: clear HW suspend APS_FSMCO=0x%04x\n",
                   aps_fsmco);
        if (aps_fsmco & RTL8188EU_APS_HW_SUSPEND) {
            usb_printk("rtl8188eu: HW suspend did not clear\n");
            return -EIO;
        }

        ret = rtl8188eu_read8(usb, RTL8188EU_REG_APS_FSMCO + 1,
                              &aps_high);
        if (ret < 0)
            return ret;
        ret = rtl8188eu_write8(usb, RTL8188EU_REG_APS_FSMCO + 1,
                               aps_high | 0x01);
        if (ret < 0)
            return ret;
        for (attempt = 0; attempt < 1000; attempt++) {
            ret = rtl8188eu_read8(usb, RTL8188EU_REG_APS_FSMCO + 1,
                                  &aps_after);
            if (ret < 0)
                return ret;
            if (!(aps_after & 0x01))
                break;
            tick_delay(10);
        }
        if (aps_after & 0x01) {
            usb_printk("rtl8188eu: APFM_ONMAC timeout APS+1=0x%02x\n",
                       aps_after);
            return -EIO;
        }
        ret = rtl8188eu_read16(usb, RTL8188EU_REG_APS_FSMCO,
                               &aps_fsmco);
        if (ret < 0)
            return ret;
        usb_printk("rtl8188eu: APFM_ONMAC complete APS_FSMCO=0x%04x "
                   "polls=%u\n", aps_fsmco, attempt + 1);
    }

    ret = rtl8188eu_write16(usb, RTL8188EU_REG_CR, 0x0000);
    if (ret < 0)
        return ret;
    cr = RTL8188EU_CR_HCI_TXDMA_EN | RTL8188EU_CR_HCI_RXDMA_EN |
         RTL8188EU_CR_TXDMA_EN | RTL8188EU_CR_RXDMA_EN |
         RTL8188EU_CR_PROTOCOL_EN | RTL8188EU_CR_SCHEDULE_EN |
         RTL8188EU_CR_SECURITY_EN | RTL8188EU_CR_CALTMR_EN;
    ret = rtl8188eu_write16(usb, RTL8188EU_REG_CR, cr);
    if (ret < 0 || rtl8188eu_read16(usb, RTL8188EU_REG_CR, &cr) < 0)
        return -EIO;
    if (cr == 0xeaea) {
        usb_printk("rtl8188eu: MAC register window inactive CR=0x%04x; "
                   "requesting APFM_ONMAC\n", cr);
        ret = rtl8188eu_read8(usb, RTL8188EU_REG_APS_FSMCO + 1,
                              &aps_high);
        if (ret < 0 ||
            rtl8188eu_write8(usb, RTL8188EU_REG_APS_FSMCO + 1,
                             aps_high | 0x01) < 0)
            return -EIO;
        for (attempt = 0; attempt < 1000; attempt++) {
            ret = rtl8188eu_read8(usb, RTL8188EU_REG_APS_FSMCO + 1,
                                  &aps_after);
            if (ret < 0)
                return ret;
            if (!(aps_after & 0x01))
                break;
            tick_delay(10);
        }
        if (aps_after & 0x01) {
            usb_printk("rtl8188eu: APFM_ONMAC retry timeout APS+1=0x%02x\n",
                       aps_after);
            return -EIO;
        }
        cr = RTL8188EU_CR_HCI_TXDMA_EN | RTL8188EU_CR_HCI_RXDMA_EN |
             RTL8188EU_CR_TXDMA_EN | RTL8188EU_CR_RXDMA_EN |
             RTL8188EU_CR_PROTOCOL_EN | RTL8188EU_CR_SCHEDULE_EN |
             RTL8188EU_CR_SECURITY_EN | RTL8188EU_CR_CALTMR_EN;
        ret = rtl8188eu_write16(usb, RTL8188EU_REG_CR, cr);
        if (ret < 0 || rtl8188eu_read16(usb, RTL8188EU_REG_CR, &cr) < 0)
            return -EIO;
        usb_printk("rtl8188eu: MAC register retry CR=0x%04x polls=%u\n",
                   cr, attempt + 1);
        if (cr == 0xeaea)
            return -EIO;
    }
    usb_printk("rtl8188eu: MAC power ready APS_FSMCO=0x%04x CR=0x%04x\n",
               aps_fsmco, cr);
    return 0;
}

static int rtl8188eu_write_retry(struct usb_dev_t *usb, uint16_t reg,
                                 const uint8_t *data, uint16_t length,
                                 const char *what, unsigned int offset)
{
    unsigned int attempt;
    int ret = -EIO;

    for (attempt = 1; attempt <= RTL8188EU_FW_WRITE_RETRIES; attempt++) {
        ret = rtl8188eu_write(usb, reg, data, length);
        if (ret == 0) {
            if (attempt > 1)
                usb_printk("rtl8188eu: %s recovered offset=%u attempt=%u\n",
                           what, offset, attempt);
            return 0;
        }
        usb_printk("rtl8188eu: %s retry offset=%u attempt=%u err=%d\n",
                   what, offset, attempt, ret);
        tick_delay(1000);
    }
    return ret;
}

static int rtl8188eu_write8_retry(struct usb_dev_t *usb, uint16_t reg,
                                  uint8_t value, const char *what,
                                  unsigned int offset)
{
    return rtl8188eu_write_retry(usb, reg, &value, 1, what, offset);
}

static uint16_t rtl8188eu_get_le16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static int rtl8188eu_verify_firmware(struct usb_dev_t *usb,
                                     const uint8_t *payload,
                                     size_t payload_length)
{
    uint8_t actual[RTL8188EU_FW_BLOCK_SIZE];
    size_t offset = 0;
    uint8_t current_page = 0xff;
    uint8_t page_select;
    unsigned int blocks = 0;
    int ret;

    while (offset < payload_length) {
        size_t page_offset = offset & (RTL8188EU_FW_PAGE_SIZE - 1);
        size_t page_remaining = RTL8188EU_FW_PAGE_SIZE - page_offset;
        size_t block_length = payload_length - offset;
        uint8_t page = (uint8_t)(offset / RTL8188EU_FW_PAGE_SIZE);
        size_t byte;

        if (block_length > RTL8188EU_FW_BLOCK_SIZE)
            block_length = RTL8188EU_FW_BLOCK_SIZE;
        if (block_length > page_remaining)
            block_length = page_remaining;

        if (page != current_page) {
            if (current_page != 0xff)
                usb_printk("rtl8188eu: firmware verify page %u OK\n",
                           current_page);
            ret = rtl8188eu_read8(usb, RTL8188EU_REG_MCUFWDL + 2,
                                  &page_select);
            if (ret < 0)
                return ret;
            page_select = (uint8_t)((page_select &
                          (uint8_t)~RTL8188EU_MCUFWDL_PAGE_MASK) |
                          (page & RTL8188EU_MCUFWDL_PAGE_MASK));
            ret = rtl8188eu_write8_retry(
                usb, RTL8188EU_REG_MCUFWDL + 2, page_select,
                "firmware verify page select", (unsigned int)offset);
            if (ret < 0)
                return ret;
            current_page = page;
        }

        ret = rtl8188eu_read(
            usb, (uint16_t)(RTL8188EU_FW_START + page_offset),
            actual, (uint16_t)block_length);
        if (ret < 0) {
            usb_printk("rtl8188eu: firmware verify read failed "
                       "offset=%u page=%u err=%d\n",
                       (unsigned int)offset, page, ret);
            return ret;
        }
        for (byte = 0; byte < block_length; byte++) {
            if (actual[byte] != payload[offset + byte]) {
                usb_printk("rtl8188eu: firmware verify mismatch page=%u "
                           "offset=%u first_diff=%u expected=%02x actual=%02x\n",
                           page, (unsigned int)offset, (unsigned int)byte,
                           payload[offset + byte], actual[byte]);
                rtl8188eu_dump_bytes("firmware verify expected",
                                     payload + offset,
                                     (unsigned int)block_length);
                rtl8188eu_dump_bytes("firmware verify actual", actual,
                                     (unsigned int)block_length);
                return -EIO;
            }
        }
        offset += block_length;
        blocks++;
    }

    usb_printk("rtl8188eu: firmware verify page %u OK\n", current_page);
    usb_printk("rtl8188eu: firmware RAM verify OK bytes=%u blocks=%u\n",
               (unsigned int)payload_length, blocks);
    return 0;
}

static int rtl8188eu_wait_mcu_bit(struct usb_dev_t *usb, uint8_t bit,
                                  unsigned int attempts,
                                  unsigned int delay_us, uint8_t *last)
{
    uint8_t value = 0;
    while (attempts--) {
        int ret = rtl8188eu_read8(usb, RTL8188EU_REG_MCUFWDL, &value);
        if (ret < 0)
            return ret;
        if (value & bit) {
            if (last)
                *last = value;
            return 0;
        }
        tick_delay((int)delay_us);
    }
    if (last)
        *last = value;
    return -EIO;
}

static int rtl8188eu_firmware_self_reset(struct usb_dev_t *usb)
{
    uint8_t sys_func;
    uint8_t sys_low;
    uint8_t sys_after;
    int ret;

    ret = rtl8188eu_read8(usb, RTL8188EU_REG_SYS_FUNC_EN + 1, &sys_func);
    if (ret < 0)
        return ret;
    ret = rtl8188eu_write8(usb, RTL8188EU_REG_SYS_FUNC_EN + 1,
                           sys_func & (uint8_t)~RTL8188EU_SYS_FUNC_CPU_ENABLE);
    if (ret < 0)
        return ret;
    /* Windows holds SYS_FUNC_EN+1 BIT2 low for about 423 us before
     * releasing the 8051. Match that observed reset pulse width. */
    tick_delay(500);
    ret = rtl8188eu_read8(usb, RTL8188EU_REG_SYS_FUNC_EN + 1, &sys_low);
    if (ret < 0)
        return ret;
    ret = rtl8188eu_write8(usb, RTL8188EU_REG_SYS_FUNC_EN + 1,
                           sys_func | RTL8188EU_SYS_FUNC_CPU_ENABLE);
    if (ret < 0)
        return ret;
    ret = rtl8188eu_read8(usb, RTL8188EU_REG_SYS_FUNC_EN + 1, &sys_after);
    if (ret < 0)
        return ret;
    usb_printk("rtl8188eu: MCU reset SYS_FUNC_EN+1 before=0x%02x\n",
               sys_func);
    usb_printk("rtl8188eu: MCU reset SYS_FUNC_EN+1 low=0x%02x\n",
               sys_low);
    usb_printk("rtl8188eu: MCU reset SYS_FUNC_EN+1 after=0x%02x\n",
               sys_after);
    if ((sys_low & RTL8188EU_SYS_FUNC_CPU_ENABLE) ||
        !(sys_after & RTL8188EU_SYS_FUNC_CPU_ENABLE))
        return -EIO;
    return 0;
}

static int rtl8188eu_start_mcu(struct usb_dev_t *usb)
{
    uint32_t mcu_fwdl32;
    uint32_t previous = 0xffffffffu;
    unsigned int attempt;
    int ret;

    /* 1. 设置 MCU Ready 位，清除 CPU Reset 位 */
    ret = rtl8188eu_read32(usb, RTL8188EU_REG_MCUFWDL, &mcu_fwdl32);
    if (ret < 0)
        return ret;

    mcu_fwdl32 |= RTL8188EU_MCUFWDL_RDY;
    mcu_fwdl32 &= ~((uint32_t)RTL8188EU_WINTINI_RDY);
    ret = rtl8188eu_write32(usb, RTL8188EU_REG_MCUFWDL, mcu_fwdl32);
    if (ret < 0)
        return ret;

    ret = rtl8188eu_read32(usb, RTL8188EU_REG_MCUFWDL, &mcu_fwdl32);
    if (ret < 0)
        return ret;
    usb_printk("rtl8188eu: MCU handoff readback MCUFWDL=%08x (RDY set)\n", mcu_fwdl32);

    /* 2. 触发 MCU 自复位 */
    ret = rtl8188eu_firmware_self_reset(usb);
    if (ret < 0)
        return ret;

    /* 3. 关键修改：复位后给予 MCU 足够的初始化启动时间 (5~10ms) */
    tick_delay(10000); 

    /* 4. 轮询 MCU 就绪状态：总等待时间提高至 ~100ms (1000 * 100us) */
    for (attempt = 0; attempt < 1000; attempt++) {
        ret = rtl8188eu_read32(usb, RTL8188EU_REG_MCUFWDL, &mcu_fwdl32);
        if (ret < 0)
            return ret;

        if (mcu_fwdl32 != previous && (attempt < 8 || previous != 0xffffffffu)) {
            usb_printk("rtl8188eu: MCU poll[%u] MCUFWDL=%08x\n",
                       attempt + 1, mcu_fwdl32);
            previous = mcu_fwdl32;
        }

        /* 检查 WINTINI_RDY 位 (0x20000000) 是否被 MCU 置 1 */
        /* WINTINI_RDY is BIT6 in the low byte: 0x06 -> 0xC6. */
        if (mcu_fwdl32 & RTL8188EU_WINTINI_RDY) {
            usb_printk("rtl8188eu: MCU start MCUFWDL=%08x ready=yes polls=%u\n",
                       mcu_fwdl32, attempt + 1);
            return 0;
        }

        /* 将单次轮询延时从 5us 增加至 100us */
        tick_delay(100);
    }

    usb_printk("rtl8188eu: MCU start MCUFWDL=%08x ready=timeout polls=%u\n",
               mcu_fwdl32, attempt);
    return -EIO;
}

static int rtl8188eu_start_mcu_bak(struct usb_dev_t *usb)
{
    uint32_t mcu_fwdl32;
    uint32_t previous = 0xffffffffu;
    unsigned int attempt;
    int ret;

    ret = rtl8188eu_read32(usb, RTL8188EU_REG_MCUFWDL, &mcu_fwdl32);
    if (ret < 0)
        return ret;
    mcu_fwdl32 |= RTL8188EU_MCUFWDL_RDY;
    mcu_fwdl32 &= ~((uint32_t)RTL8188EU_WINTINI_RDY);
    ret = rtl8188eu_write32(usb, RTL8188EU_REG_MCUFWDL, mcu_fwdl32);
    if (ret < 0)
        return ret;
    ret = rtl8188eu_read32(usb, RTL8188EU_REG_MCUFWDL, &mcu_fwdl32);
    if (ret < 0)
        return ret;
    usb_printk("rtl8188eu: MCU handoff readback MCUFWDL=%08x "
               "(RDY set)\n", mcu_fwdl32);
    ret = rtl8188eu_firmware_self_reset(usb);
    if (ret < 0)
        return ret;

    for (attempt = 0; attempt < 1000; attempt++) {
        ret = rtl8188eu_read32(usb, RTL8188EU_REG_MCUFWDL, &mcu_fwdl32);
        if (ret < 0)
            return ret;
        if (mcu_fwdl32 != previous && (attempt < 8 || previous != 0xffffffffu)) {
            usb_printk("rtl8188eu: MCU poll[%u] MCUFWDL=%08x\n",
                       attempt + 1, mcu_fwdl32);
            previous = mcu_fwdl32;
        }
        if (mcu_fwdl32 & RTL8188EU_WINTINI_RDY) {
            usb_printk("rtl8188eu: MCU start MCUFWDL=%08x ready=yes polls=%u\n",
                       mcu_fwdl32, attempt + 1);
            return 0;
        }
        /* Match Linux FW_8192C_POLLING_DELAY: 5 microseconds. */
        tick_delay(5);
    }
    usb_printk("rtl8188eu: MCU start MCUFWDL=%08x ready=timeout polls=%u\n",
               mcu_fwdl32, attempt);
    return -EIO;
}

int rtl8188eu_usb_match(const struct usb_dev_t *usb)
{
    if (!usb || usb->vendor != RTL8188EU_USB_VENDOR_ID)
        return 0;

    /* 0x8179 is native; 0x0179 appears on some OEM RTL8188EUS adapters. */
    return usb->product == RTL8188EU_USB_PRODUCT_ID || usb->product == 0x0179;
}

int rtl8188eu_usb_probe(struct usb_interface_t *iface)
{
    struct usb_dev_t *usb;
    uint8_t mcu_fwdl = 0;

    if (!iface || !iface->usb)
        return -EINVAL;
    usb = iface->usb;
    if (!rtl8188eu_usb_match(usb))
        return -ENODEV;
    if (!iface->endpoint_in || !iface->endpoint_out) {
        usb_printk("rtl8188eu: missing bulk endpoint(s)\n");
        return -ENODEV;
    }
    if (iface->endpoint_in->type != USB_ENDPOINT_BULK ||
        iface->endpoint_out->type != USB_ENDPOINT_BULK) {
        usb_printk("rtl8188eu: endpoints are not bulk endpoints\n");
        return -ENODEV;
    }

    rtl8188eu.usb = usb;
    rtl8188eu.iface = iface;
    rtl8188eu.bulk_in = iface->endpoint_in;
    rtl8188eu.bulk_out = iface->endpoint_out;
    rtl8188eu.bulk_out2 = iface->endpoint_out2;
    rtl8188eu.firmware_loaded = 0;
    rtl8188eu.wifi_state = RTL8188EU_WIFI_FIRMWARE_LOADING;
    rtl8188eu.selected_result = -1;
    rtl8188eu.last_error = 0;

    rtl8188eu_log_preinit_snapshot(usb);
    if (rtl8188eu_recover_stale_power_state(usb) < 0) {
        usb_printk("rtl8188eu: stale power-state recovery failed\n");
        rtl8188eu.usb = NULL;
        return -EIO;
    }

#if RTL8188EU_PRE_FW_POWER_ON
    if (rtl8188eu_power_on(usb) < 0) {
        usb_printk("rtl8188eu: MAC power-on sequence failed\n");
        rtl8188eu.usb = NULL;
        return -EIO;
    }
#else
    usb_printk("rtl8188eu: pre-firmware power-on skipped (A/B test)\n");
#endif

    if (rtl8188eu_read8(usb, RTL8188EU_REG_MCUFWDL, &mcu_fwdl) < 0) {
        usb_printk("rtl8188eu: register access test failed\n");
        rtl8188eu.usb = NULL;
        return -EIO;
    }

    rtl8188eu.firmware_loaded =
        (mcu_fwdl & (RTL8188EU_MCUFWDL_RDY | RTL8188EU_WINTINI_RDY)) ==
        (RTL8188EU_MCUFWDL_RDY | RTL8188EU_WINTINI_RDY);
    usb_printk("rtl8188eu: detected %04x:%04x, bulk-in EP%d, bulk-out EP%d",
               usb->vendor, usb->product, rtl8188eu.bulk_in->addr,
               rtl8188eu.bulk_out->addr);
    if (rtl8188eu.bulk_out2)
        usb_printk(", bulk-out2 EP%d\n", rtl8188eu.bulk_out2->addr);
    else
        usb_printk("\n");
    usb_printk("rtl8188eu: MCUFWDL=0x%02x, firmware upload required=%s\n",
               mcu_fwdl, rtl8188eu.firmware_loaded ? "no" : "yes");
    if (rtl8188eu.firmware_loaded) {
        rtl8188eu.wifi_state = RTL8188EU_WIFI_READY;
        rtl8188eu_initialize_mac(usb);
        return 0;
    }
    if (!rtl8188eu.firmware_loaded) {
        int ret;

        ret = rtl8188eu_prepare_firmware_buffers(usb);
        if (ret < 0) {
            usb_printk("rtl8188eu: pre-firmware buffer setup failed (%d)\n",
                       ret);
            return ret;
        }
        ret = rtl8188eu_load_firmware(rtl8188eufw, rtl8188eufw_size);
        if (ret < 0)
            usb_printk("rtl8188eu: firmware download failed (%d)\n", ret);
    }
    return 0;
}

int rtl8188eu_load_firmware(const uint8_t *firmware, size_t length)
{
    struct usb_dev_t *usb = rtl8188eu.usb;
    const uint8_t *payload;
    size_t payload_length;
    size_t download_length;
    size_t offset;
    uint8_t padded_block[RTL8188EU_FW_BLOCK_SIZE];
    unsigned int blocks = 0;
    uint16_t signature;
    uint16_t declared_length;
    uint8_t mcu_fwdl;
    uint8_t page;
    uint8_t current_page = 0xff;
    uint8_t page_select;
    uint32_t mcu_fwdl32;
    int checksum_ret;
    int ret;

    if (!usb)
        return -ENODEV;
    if (!firmware || length < RTL8188EU_FW_HEADER_LEN)
        return -EINVAL;

    signature = rtl8188eu_get_le16(firmware);
    declared_length = rtl8188eu_get_le16(firmware + 12);
    payload = firmware + RTL8188EU_FW_HEADER_LEN;
    payload_length = length - RTL8188EU_FW_HEADER_LEN;
    download_length = (payload_length + 3u) & ~(size_t)3u;
    if (signature != RTL8188EU_FW_SIGNATURE ||
        declared_length != payload_length || payload_length > 0x8000) {
        usb_printk("rtl8188eu: invalid firmware sig=%04x declared=%u actual=%u\n",
                   signature, declared_length, (unsigned int)payload_length);
        return -EINVAL;
    }

    rtl8188eu_fw_usb_trace = 1;
    rtl8188eu_fw_data_writes = 0;
    rtl8188eu_fw_data_total = (unsigned int)(download_length / 4);

    ret = rtl8188eu_read8(usb, RTL8188EU_REG_MCUFWDL, &mcu_fwdl);
    if (ret < 0)
        return ret;
    if (mcu_fwdl & (RTL8188EU_MCUFWDL_RDY |
                    RTL8188EU_WINTINI_RDY |
                    RTL8188EU_MCUFWDL_RAM_SEL)) {
        usb_printk("rtl8188eu: stale MCU state MCUFWDL=0x%02x; "
                   "resetting before download\n", mcu_fwdl);
        ret = rtl8188eu_write8(usb, RTL8188EU_REG_MCUFWDL, 0x00);
        if (ret < 0)
            return ret;
        ret = rtl8188eu_firmware_self_reset(usb);
        if (ret < 0)
            return ret;
        ret = rtl8188eu_read32(usb, RTL8188EU_REG_MCUFWDL, &mcu_fwdl32);
        if (ret < 0)
            return ret;
        usb_printk("rtl8188eu: state after stale reset MCUFWDL=%08x\n",
                   mcu_fwdl32);
        mcu_fwdl = (uint8_t)mcu_fwdl32;
    } else {
        /* The known-good path enters download with MCUFWDL=0x05:
         * keep EN set and clear only stale ready indications. */
        mcu_fwdl &= (uint8_t)~(RTL8188EU_MCUFWDL_RDY |
                              RTL8188EU_WINTINI_RDY);
        ret = rtl8188eu_write8(usb, RTL8188EU_REG_MCUFWDL, mcu_fwdl);
        if (ret < 0)
            return ret;
    }
    ret = rtl8188eu_read8(usb, RTL8188EU_REG_MCUFWDL, &mcu_fwdl);
    if (ret < 0)
        return ret;
    ret = rtl8188eu_read32(usb, RTL8188EU_REG_MCUFWDL, &mcu_fwdl32);
    if (ret < 0)
        return ret;
    usb_printk("rtl8188eu: firmware before enable MCUFWDL=%08x\n",
               mcu_fwdl32);
    usb_printk("rtl8188eu: firmware state before download MCUFWDL=0x%02x\n",
               mcu_fwdl);
    if (mcu_fwdl & (RTL8188EU_MCUFWDL_RDY |
                    RTL8188EU_WINTINI_RDY)) {
        usb_printk("rtl8188eu: firmware ready state did not clear\n");
        return -EIO;
    }
    mcu_fwdl |= RTL8188EU_MCUFWDL_EN;
    ret = rtl8188eu_write8(usb, RTL8188EU_REG_MCUFWDL, mcu_fwdl);
    if (ret < 0)
        return ret;
    ret = rtl8188eu_read8(usb, RTL8188EU_REG_MCUFWDL + 2, &page_select);
    if (ret < 0)
        return ret;
    ret = rtl8188eu_write8(usb, RTL8188EU_REG_MCUFWDL + 2,
                           page_select & (uint8_t)0xf7);
    if (ret < 0)
        return ret;
    /* Realtek uses a write-one sequence to reset the checksum engine. */
    ret = rtl8188eu_read8(usb, RTL8188EU_REG_MCUFWDL, &mcu_fwdl);
    if (ret < 0)
        return ret;
    mcu_fwdl |= RTL8188EU_FWDL_CHKSUM_RPT;
    ret = rtl8188eu_write8(usb, RTL8188EU_REG_MCUFWDL, mcu_fwdl);
    if (ret < 0)
        return ret;
    ret = rtl8188eu_read8(usb, RTL8188EU_REG_MCUFWDL, &mcu_fwdl);
    if (ret < 0)
        return ret;
    usb_printk("rtl8188eu: firmware download mode MCUFWDL=0x%02x\n",
               mcu_fwdl);
    if (!(mcu_fwdl & RTL8188EU_MCUFWDL_EN)) {
        usb_printk("rtl8188eu: firmware download enable did not set\n");
        return -EIO;
    }
    ret = rtl8188eu_read32(usb, RTL8188EU_REG_MCUFWDL, &mcu_fwdl32);
    if (ret < 0)
        return ret;
    usb_printk("rtl8188eu: firmware after enable MCUFWDL=%08x\n",
               mcu_fwdl32);

    usb_printk("rtl8188eu: uploading firmware version=%u payload=%u "
               "download=%u bytes write=block block_max=%u\n",
               rtl8188eu_get_le16(firmware + 4),
               (unsigned int)payload_length,
               (unsigned int)download_length,
               RTL8188EU_FW_BLOCK_SIZE);
    for (offset = 0; offset < download_length;) {
        size_t page_offset = offset & (RTL8188EU_FW_PAGE_SIZE - 1);
        size_t page_remaining = RTL8188EU_FW_PAGE_SIZE - page_offset;
        size_t remaining = download_length - offset;
        size_t block_length = remaining;
        const uint8_t *block_data = payload + offset;

        if (block_length > RTL8188EU_FW_BLOCK_SIZE)
            block_length = RTL8188EU_FW_BLOCK_SIZE;
        if (block_length > page_remaining)
            block_length = page_remaining;

        if (offset + block_length > payload_length) {
            size_t available = offset < payload_length ?
                               payload_length - offset : 0;
            size_t i;

            for (i = 0; i < available; i++)
                padded_block[i] = payload[offset + i];
            for (; i < block_length; i++)
                padded_block[i] = 0;
            block_data = padded_block;
        }

        page = (uint8_t)(offset / RTL8188EU_FW_PAGE_SIZE);
        if (page != current_page) {
            if (current_page != 0xff) {
                ret = rtl8188eu_read32(usb, RTL8188EU_REG_MCUFWDL,
                                       &mcu_fwdl32);
                if (ret < 0)
                    return ret;
                usb_printk("rtl8188eu: firmware page %u complete "
                           "MCUFWDL=%08x\n", current_page, mcu_fwdl32);
            }
            ret = rtl8188eu_read8(usb, RTL8188EU_REG_MCUFWDL + 2,
                                  &page_select);
            if (ret < 0)
                return ret;
            page_select = (uint8_t)((page_select &
                          (uint8_t)~RTL8188EU_MCUFWDL_PAGE_MASK) |
                          (page & RTL8188EU_MCUFWDL_PAGE_MASK));
            ret = rtl8188eu_write8_retry(
                usb, RTL8188EU_REG_MCUFWDL + 2,
                page_select,
                "firmware page select", (unsigned int)offset);
            if (ret < 0)
                return ret;
            current_page = page;
        }
        ret = rtl8188eu_write_retry(
            usb, (uint16_t)(RTL8188EU_FW_START + page_offset),
            block_data, (uint16_t)block_length,
            "firmware block", (unsigned int)offset);
        if (ret < 0) {
            usb_printk("rtl8188eu: firmware write failed at %u page=%u\n",
                       (unsigned int)offset, page);
            return ret;
        }
        offset += block_length;
        blocks++;
    }
    ret = rtl8188eu_read32(usb, RTL8188EU_REG_MCUFWDL, &mcu_fwdl32);
    if (ret < 0)
        return ret;
    usb_printk("rtl8188eu: firmware page %u complete MCUFWDL=%08x\n",
               current_page, mcu_fwdl32);
    usb_printk("rtl8188eu: firmware payload written bytes=%u padded=%u "
               "blocks=%u last_page=%u\n",
               (unsigned int)payload_length, (unsigned int)download_length,
               blocks, current_page);

#if RTL8188EU_FW_RAM_VERIFY
    ret = rtl8188eu_verify_firmware(usb, payload, payload_length);
    if (ret < 0)
        return ret;
#else
    usb_printk("rtl8188eu: firmware RAM verify skipped (known-good payload)\n");
#endif

    /* Linux _rtl88e_enable_fw_download(false), before checksum polling. */
    ret = rtl8188eu_read8(usb, RTL8188EU_REG_MCUFWDL, &mcu_fwdl);
    if (ret < 0)
        return ret;
    ret = rtl8188eu_write8(usb, RTL8188EU_REG_MCUFWDL,
                           mcu_fwdl &
                           (uint8_t)~RTL8188EU_MCUFWDL_EN);
    if (ret < 0)
        return ret;
    ret = rtl8188eu_write8(usb, RTL8188EU_REG_MCUFWDL + 1, 0x00);
    if (ret < 0)
        return ret;
    ret = rtl8188eu_read32(usb, RTL8188EU_REG_MCUFWDL, &mcu_fwdl32);
    if (ret < 0)
        return ret;
    usb_printk("rtl8188eu: firmware download disabled MCUFWDL=%08x\n",
               mcu_fwdl32);
    if (mcu_fwdl32 & RTL8188EU_MCUFWDL_EN)
        return -EIO;

    checksum_ret = rtl8188eu_wait_mcu_bit(usb, RTL8188EU_FWDL_CHKSUM_RPT,
                                           1000, 0, &mcu_fwdl);
    ret = rtl8188eu_read32(usb, RTL8188EU_REG_MCUFWDL, &mcu_fwdl32);
    if (ret < 0)
        return ret;
    usb_printk("rtl8188eu: firmware write complete MCUFWDL=%08x checksum=%s\n",
               mcu_fwdl32, checksum_ret == 0 ? "ready" : "timeout");
    rtl8188eu_fw_usb_trace = 0;
    if (checksum_ret < 0)
        return checksum_ret;

    ret = rtl8188eu_start_mcu(usb);
    if (ret < 0)
        return ret;

    rtl8188eu.firmware_loaded = 1;
    rtl8188eu.wifi_state = RTL8188EU_WIFI_READY;
    usb_printk("rtl8188eu: firmware initialized successfully\n");
    rtl8188eu_initialize_mac(usb);
    return 0;
}

void rtl8188eu_usb_remove(struct usb_dev_t *usb)
{
    if (rtl8188eu.usb != usb)
        return;
    rtl8188eu.usb = NULL;
    rtl8188eu.iface = NULL;
    rtl8188eu.bulk_in = NULL;
    rtl8188eu.bulk_out = NULL;
    rtl8188eu.bulk_out2 = NULL;
    rtl8188eu.mac_addr[0] = 0;
    rtl8188eu.mac_addr[1] = 0;
    rtl8188eu.mac_addr[2] = 0;
    rtl8188eu.mac_addr[3] = 0;
    rtl8188eu.mac_addr[4] = 0;
    rtl8188eu.mac_addr[5] = 0;
    rtl8188eu.firmware_loaded = 0;
    rtl8188eu.wifi_state = RTL8188EU_WIFI_ABSENT;
    rtl8188eu.selected_result = -1;
    rtl8188eu.last_error = -ENODEV;
}
