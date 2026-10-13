#include <stdint.h>
#include <stddef.h>
#include <kernel/errno.h>
#include <kernel/hillsonos.h>
#include <kernel/usb.h>
#include <kernel/usb_xhci.h>
#include <kernel/rtl8188eu_wifi.h>
#include <kernel/wpa2_crypto.h>
#include <net.h>
#include <usb_log.h>
#include "rtl8188eu_tables.h"

extern void vga_setcolor(uint8_t fg, uint8_t bg);

#define RTL8188EU_LOG_COLOR_DEFAULT       2
#define RTL8188EU_LOG_COLOR_FAILURE       4
#define RTL8188EU_LOG_COLOR_TARGET_BEACON 9
#define RTL8188EU_LOG_COLOR_AUTH          10
#define RTL8188EU_LOG_COLOR_TARGET_PROBE  11
#define RTL8188EU_LOG_COLOR_OTHER_MGMT    13
#define RTL8188EU_LOG_COLOR_TX            14
#define RTL8188EU_LOG_COLOR_STATS         15

static void rtl8188eu_log_set_color(uint8_t color)
{
    vga_setcolor(color, 0);
}

static void rtl8188eu_log_reset_color(void)
{
    vga_setcolor(RTL8188EU_LOG_COLOR_DEFAULT, 0);
}

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
#define RTL8188EU_REG_MSR       0x0102
#define RTL8188EU_REG_BWOPMODE  0x0103
#define RTL8188EU_REG_PBP       0x0104
#define RTL8188EU_REG_TRXFF_BNDY 0x0114
#define RTL8188EU_REG_TRXDMA_CTRL 0x010c
#define RTL8188EU_REG_RQPN      0x0200
#define RTL8188EU_REG_TDECTRL   0x0208
#define RTL8188EU_REG_TXDMA_STATUS 0x0210
#define RTL8188EU_REG_TXPKT_EMPTY 0x041a
#define RTL8188EU_REG_TX_RPT_CTRL 0x04ec
#define RTL8188EU_REG_TXPAUSE 0x0522
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
#define RTL8188EU_REG_BSSID     0x0618
#define RTL8188EU_REG_CAMCMD    0x0670
#define RTL8188EU_REG_CAMWRITE  0x0674
#define RTL8188EU_REG_SECCFG    0x0680
#define RTL8188EU_REG_DATA_SC   0x0483
#define RTL8188EU_REG_RX_DRVINFO_SZ 0x060f
#define RTL8188EU_REG_RXFLTMAP0 0x06a0
#define RTL8188EU_REG_RXFLTMAP1 0x06a2
#define RTL8188EU_REG_RXFLTMAP2 0x06a4
#define RTL8188EU_REG_USB_SPECIAL_OPTION 0xfe55
#define RTL8188EU_REG_USB_AGG_TO 0xfe5c
#define RTL8188EU_REG_USB_AGG_TH 0xfe5d
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
#define RTL8188EU_CR_MACTXEN       0x0040
#define RTL8188EU_CR_MACRXEN       0x0080
#define RTL8188EU_CR_SECURITY_EN  0x0200
#define RTL8188EU_CR_CALTMR_EN    0x0400
#define RTL8188EU_CR_NETTYPE_MASK  0x00030000u
#define RTL8188EU_CR_NETTYPE_STA    0x00020000u
#define RTL8188EU_TX_DESC_SIZE     32
#define RTL8188EU_TX_TEST_ENABLE   1
#define RTL8188EU_RX_TEST_ENABLE   1
#define RTL8188EU_PRE_FW_POWER_ON  1
#define RTL8188EU_FW_RAM_VERIFY    0
#define RTL8188EU_RX_TEST_SIZE     15360
#define RTL8188EU_RX_RAW_DEBUG     0
#define RTL8188EU_RX_DESCRIPTOR_DEBUG 0
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
/* Do not restrict RX to the programmed BSSID before authentication. */
#define RTL8188EU_RCR_CBSSID_BCN     (1u << 12)
#define RTL8188EU_RCR_CBSSID_DATA    (1u << 23)
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
#define RTL8188EU_SCAN_DWELL_TICKS    10
#define RTL8188EU_SCAN_MAX_POLLS      20000
#define RTL8188EU_SCAN_POLL_DELAY_US  1
#define RTL8188EU_MAX_SCAN_RESULTS    32
#define RTL8188EU_MAX_SSID_LEN        32
#define RTL8188EU_SCAN_HASH_BUCKETS   64
#define RTL8188EU_BB_CCK_ENABLE       (1u << 24)
#define RTL8188EU_BB_OFDM_ENABLE      (1u << 25)
#define RTL8188EU_RXDMA_AGG_ENABLE    (1u << 2)
#define RTL8188EU_USB_AGG_ENABLE      (1u << 3)
#define RTL8188EU_TXDW4_HWSEQ_EN       (1u << 7)
#define RTL8188EU_TXDW4_USE_RATE       (1u << 8)
#define RTL8188EU_TXDW4_DISDATAFB      (1u << 10)
#define RTL8188EU_TXDW3_EN_HWSEQ       (1u << 31)
#define RTL8188EU_TXDW5_RTY_LMT_EN     (1u << 17)
#define RTL8188EU_TXDW5_RTY_LMT_6      (6u << 18)
#define RTL8188EU_TXDW5_RATE_1M        0u
#define RTL8188EU_TXDW1_QSEL_MGNT      (0x12u << 8)
#define RTL8188EU_TXDW1_RATE_ID_11G    (5u << 16)
#define RTL8188EU_DESC_RATE_1M         0
#define RTL8188EU_TXDW2_SEC_TYPE_AES   (3u << 22)
#define RTL8188EU_CAM_POLLING          (1u << 31)
#define RTL8188EU_CAM_WRITE             (1u << 16)
#define RTL8188EU_CAM_VALID             (1u << 15)
#define RTL8188EU_CAM_ALGO_AES          4u
#define RTL8188EU_SECCFG_TX             (1u << 0)
#define RTL8188EU_SECCFG_RX             (1u << 1)
#define RTL8188EU_SECCFG_TX_USE_SW_SEC (1u << 5)
#define RTL8188EU_EAPOL_KEY_INFO_ENCR   0x1000
#define RTL8188EU_EAPOL_KEY_INFO_KEYID  0x0003
#define RTL8188EU_DHCP_CLIENT_PORT      68
#define RTL8188EU_DHCP_SERVER_PORT      67
#define RTL8188EU_DHCP_MAGIC            0x63825363u
#define RTL8188EU_DHCP_DISCOVER         1
#define RTL8188EU_DHCP_OFFER            2
#define RTL8188EU_DHCP_REQUEST          3
#define RTL8188EU_DHCP_ACK              5
#define RTL8188EU_DHCP_OPTION_PAD       0
#define RTL8188EU_DHCP_OPTION_SUBNET    1
#define RTL8188EU_DHCP_OPTION_ROUTER    3
#define RTL8188EU_DHCP_OPTION_DNS       6
#define RTL8188EU_DHCP_OPTION_MESSAGE   53
#define RTL8188EU_DHCP_OPTION_SERVER    54
#define RTL8188EU_DHCP_OPTION_REQUESTED 50
#define RTL8188EU_DHCP_OPTION_END       255

extern const uint8_t rtl8188eufw[];
extern const uint32_t rtl8188eufw_size;
extern uint32_t ticks;

struct rtl8188eu_device {
    struct usb_dev_t *usb;
    struct usb_interface_t *iface;
    uint8_t bound_slot;
    uint8_t bound_port;
    uint8_t bound_interface;
    struct usb_endpoint_t *bulk_in;
    struct usb_endpoint_t *bulk_out;
    struct usb_endpoint_t *bulk_out2;
    uint8_t bound_bulk_in_addr;
    uint8_t bound_bulk_out_addr;
    uint8_t mac_addr[6];
    uint8_t connected_bssid[6];
    uint8_t connected_ssid[RTL8188EU_MAX_SSID_LEN];
    uint8_t connected_ssid_length;
    uint8_t current_channel;
    uint8_t associated;
    uint8_t firmware_loaded;
    uint8_t wifi_state;
    int last_error;
    int selected_result;
    unsigned int scan_result_count;
    uint8_t net_registered;
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
static int rtl8188eu_scan_hash_heads[RTL8188EU_SCAN_HASH_BUCKETS];
static int rtl8188eu_scan_hash_next[RTL8188EU_MAX_SCAN_RESULTS];
static int rtl8188eu_scan_hash_valid;
static struct usb_transfer_t rtl8188eu_rx_transfer;
static uint8_t rtl8188eu_rx_buffer[RTL8188EU_RX_TEST_SIZE];
static int rtl8188eu_rx_armed;
static unsigned int rtl8188eu_rx_packets;
static uint8_t rtl8188eu_scan_channel;
static uint8_t rtl8188eu_connect_bssid[6];
static volatile int rtl8188eu_auth_status = -1;
static volatile int rtl8188eu_assoc_status = -1;
static volatile uint16_t rtl8188eu_assoc_aid;
static unsigned int rtl8188eu_auth_poll_pending;
static unsigned int rtl8188eu_auth_poll_completions;
static unsigned int rtl8188eu_auth_rx_mgmt;
static unsigned int rtl8188eu_auth_rx_control;
static unsigned int rtl8188eu_auth_rx_data;
static unsigned int rtl8188eu_auth_rx_other;
static unsigned int rtl8188eu_auth_rx_target;
static unsigned int rtl8188eu_auth_rx_target_beacon;
static unsigned int rtl8188eu_auth_rx_target_probe;
static unsigned int rtl8188eu_auth_rx_auth;
static unsigned int rtl8188eu_auth_rx_target_logs;
static int rtl8188eu_rx_poll_busy;
static int rtl8188eu_auth_tx_started;
static volatile unsigned int rtl8188eu_eapol_messages;
static volatile unsigned int rtl8188eu_eapol_msg1_seen;
static volatile unsigned int rtl8188eu_eapol_msg2_seen;
static volatile unsigned int rtl8188eu_eapol_msg3_seen;
static volatile unsigned int rtl8188eu_eapol_msg4_seen;
static uint8_t rtl8188eu_eapol_replay_counter[8];
static uint8_t rtl8188eu_eapol_anonce[32];
static uint8_t rtl8188eu_eapol_snonce[32];
static uint8_t rtl8188eu_eapol_key_data[256];
static unsigned int rtl8188eu_eapol_key_data_length;
static uint16_t rtl8188eu_eapol_key_info;
static uint16_t rtl8188eu_eapol_key_length;
static uint8_t rtl8188eu_wifi_password[64];
static size_t rtl8188eu_wifi_password_length;
static uint8_t rtl8188eu_pmk[32];
static uint8_t rtl8188eu_ptk[64];
static int rtl8188eu_ptk_ready;
static volatile unsigned int rtl8188eu_eapol_msg3_ready;
static volatile unsigned int rtl8188eu_eapol_mic_valid;
static uint32_t rtl8188eu_dhcp_xid;
static uint16_t rtl8188eu_tx_sequence;
static net_device_t rtl8188eu_netdev;

static void rtl8188eu_log_auth_rx_stats(const char *tag)
{
    rtl8188eu_log_set_color(RTL8188EU_LOG_COLOR_STATS);
    usb_printk("rtl8188eu: auth RX stats %s completions=%u pending=%u "
               "mgmt=%u control=%u data=%u other=%u target=%u "
               "target_beacon=%u target_probe=%u auth=%u\n",
               tag ? tag : "", rtl8188eu_auth_poll_completions,
               rtl8188eu_auth_poll_pending, rtl8188eu_auth_rx_mgmt,
               rtl8188eu_auth_rx_control, rtl8188eu_auth_rx_data,
               rtl8188eu_auth_rx_other, rtl8188eu_auth_rx_target,
               rtl8188eu_auth_rx_target_beacon,
               rtl8188eu_auth_rx_target_probe, rtl8188eu_auth_rx_auth);
    rtl8188eu_log_reset_color();
}

/* Poll only the transfer owned by this synchronous caller.  On xHCI,
 * usb_poll_transfer() drains the shared event ring and latches completions
 * for the other slot/DCI owners, so calling usb_poll_interrupts() here is
 * unnecessary and creates a recursive cross-poll path back through
 * rtl8188eu_wifi_poll(). */
static int rtl8188eu_wait_transfer(struct usb_transfer_t *transfer,
                                   unsigned int poll_limit)
{
    unsigned int poll;

    if (!transfer)
        return -EINVAL;
    rtl8188eu_rx_poll_busy = 1;
    for (poll = 0; poll < poll_limit; poll++) {
        if (usb_poll_transfer(transfer)) {
            rtl8188eu_rx_poll_busy = 0;
            return transfer->success ? 0 : -EIO;
        }
        tick_delay(100);
    }

    /* A stack-backed transfer cannot be returned while xHCI may still own
     * its TRB.  Let the HCI wait path perform its timeout recovery (Stop
     * Endpoint + Set TR Dequeue on xHCI) before the caller deletes it. */
    usb_wait_transfer(transfer);
    rtl8188eu_rx_poll_busy = 0;
    return transfer->success ? 0 : -ETIMEDOUT;
}

static int rtl8188eu_binding_valid(void)
{
    return rtl8188eu.usb && rtl8188eu.iface &&
           rtl8188eu.iface->usb == rtl8188eu.usb &&
           rtl8188eu.usb->num == rtl8188eu.bound_slot &&
           rtl8188eu.usb->port == rtl8188eu.bound_port &&
           rtl8188eu.iface->desc.interfacenum == rtl8188eu.bound_interface;
}

static int rtl8188eu_bssid_equal(const uint8_t *a, const uint8_t *b);
static void rtl8188eu_handle_eapol(const uint8_t *frame,
                                   unsigned int length);

static unsigned int rtl8188eu_ssid_hash(const uint8_t *ssid,
                                         unsigned int length)
{
    unsigned int hash = 2166136261u;
    unsigned int i;

    for (i = 0; i < length; i++) {
        hash ^= ssid[i];
        hash *= 16777619u;
    }
    return hash & (RTL8188EU_SCAN_HASH_BUCKETS - 1u);
}

static void rtl8188eu_build_scan_hash(void)
{
    unsigned int bucket;
    unsigned int index;

    for (bucket = 0; bucket < RTL8188EU_SCAN_HASH_BUCKETS; bucket++)
        rtl8188eu_scan_hash_heads[bucket] = -1;
    for (index = 0; index < RTL8188EU_MAX_SCAN_RESULTS; index++)
        rtl8188eu_scan_hash_next[index] = -1;
    for (index = rtl8188eu.scan_result_count; index > 0; index--) {
        unsigned int result_index = index - 1;
        bucket = rtl8188eu_ssid_hash(
            rtl8188eu_scan_results[result_index].ssid,
            rtl8188eu_scan_results[result_index].ssid_length);
        rtl8188eu_scan_hash_next[result_index] =
            rtl8188eu_scan_hash_heads[bucket];
        rtl8188eu_scan_hash_heads[bucket] = (int)result_index;
    }
    rtl8188eu_scan_hash_valid = 1;
}

static int rtl8188eu_find_scanned_ssid(const uint8_t *ssid,
                                        unsigned int length)
{
    unsigned int bucket;
    int index;

    if (!rtl8188eu_scan_hash_valid || !ssid || !length)
        return -1;
    bucket = rtl8188eu_ssid_hash(ssid, length);
    for (index = rtl8188eu_scan_hash_heads[bucket]; index >= 0;
         index = rtl8188eu_scan_hash_next[index]) {
        unsigned int byte;

        if (rtl8188eu_scan_results[index].ssid_length != length)
            continue;
        for (byte = 0; byte < length; byte++)
            if (rtl8188eu_scan_results[index].ssid[byte] != ssid[byte])
                break;
        if (byte == length)
            return index;
    }
    return -1;
}

static int rtl8188eu_is_own_probe_request(const uint8_t *frame,
                                          unsigned int length)
{
    static const uint8_t broadcast[ETH_ALEN] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    };

    if (!frame || length < 24 || (frame[0] & 0xfc) != 0x40)
        return 0;
    return rtl8188eu_bssid_equal(frame + 4, broadcast) &&
           rtl8188eu_bssid_equal(frame + 10, rtl8188eu.mac_addr) &&
           rtl8188eu_bssid_equal(frame + 16, broadcast);
}

static void rtl8188eu_clear_connection_context(void)
{
    A_memset(rtl8188eu.connected_bssid, 0,
             sizeof(rtl8188eu.connected_bssid));
    A_memset(rtl8188eu_connect_bssid, 0,
             sizeof(rtl8188eu_connect_bssid));
    A_memset(rtl8188eu.connected_ssid, 0,
             sizeof(rtl8188eu.connected_ssid));
    rtl8188eu.connected_ssid_length = 0;
    rtl8188eu.current_channel = 0;
    rtl8188eu.associated = 0;
}

/* A connect attempt is pinned to one scan result and one physical USB
 * function.  This prevents stray Beacons from another channel/BSS from
 * changing the selected target while authentication is in progress. */
static int rtl8188eu_connection_binding_valid(void)
{
    if (!rtl8188eu_binding_valid() || !rtl8188eu.bulk_in ||
        !rtl8188eu.bulk_out || !rtl8188eu.connected_ssid_length ||
        !rtl8188eu.current_channel)
        return 0;
    if (rtl8188eu.bulk_in->addr != rtl8188eu.bound_bulk_in_addr ||
        rtl8188eu.bulk_out->addr != rtl8188eu.bound_bulk_out_addr ||
        rtl8188eu.bulk_in->type != USB_ENDPOINT_BULK ||
        rtl8188eu.bulk_in->direction != USB_ENDPOINT_IN ||
        rtl8188eu.bulk_out->type != USB_ENDPOINT_BULK ||
        rtl8188eu.bulk_out->direction != USB_ENDPOINT_OUT)
        return 0;
    return rtl8188eu_bssid_equal(rtl8188eu.connected_bssid,
                                 rtl8188eu_connect_bssid);
}

static void rtl8188eu_bind_connection_result(unsigned int index)
{
    const struct rtl8188eu_scan_result *result =
        &rtl8188eu_scan_results[index];
    unsigned int i;

    rtl8188eu_clear_connection_context();
    rtl8188eu.selected_result = (int)index;
    rtl8188eu.connected_ssid_length = result->ssid_length;
    rtl8188eu.current_channel = result->channel;
    for (i = 0; i < result->ssid_length; i++)
        rtl8188eu.connected_ssid[i] = result->ssid[i];
    for (i = 0; i < 6; i++) {
        rtl8188eu_connect_bssid[i] = result->bssid[i];
        rtl8188eu.connected_bssid[i] = result->bssid[i];
    }
    usb_printk("rtl8188eu: connect binding slot=%u port=%u iface=%u "
               "EP-IN=%u EP-OUT=%u channel=%u BSSID="
               "%02x:%02x:%02x:%02x:%02x:%02x SSID=",
               rtl8188eu.bound_slot, rtl8188eu.bound_port,
               rtl8188eu.bound_interface, rtl8188eu.bound_bulk_in_addr,
               rtl8188eu.bound_bulk_out_addr, rtl8188eu.current_channel,
               rtl8188eu_connect_bssid[0], rtl8188eu_connect_bssid[1],
               rtl8188eu_connect_bssid[2], rtl8188eu_connect_bssid[3],
               rtl8188eu_connect_bssid[4], rtl8188eu_connect_bssid[5]);
    for (i = 0; i < rtl8188eu.connected_ssid_length; i++)
        usb_printk("%c", rtl8188eu.connected_ssid[i]);
    usb_printk("\n");
}
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
static int rtl8188eu_write(struct usb_dev_t *usb, uint16_t reg,
                           const uint8_t *data, uint16_t length);
static int rtl8188eu_read(struct usb_dev_t *usb, uint16_t reg,
                          uint8_t *data, uint16_t length);
static int rtl8188eu_read32(struct usb_dev_t *usb, uint16_t reg,
                            uint32_t *value);
static int rtl8188eu_write32(struct usb_dev_t *usb, uint16_t reg,
                             uint32_t value);
static int rtl8188eu_bulk_out(struct rtl8188eu_device *dev,
                              struct usb_endpoint_t *endpoint,
                              uint8_t *buffer, size_t length);

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

static uint16_t rtl8188eu_get_le16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

/* 802.11 Frame Control type bits.  These are read only after the USB/RX
 * descriptor has been removed; never search the complete USB aggregate for
 * a byte pattern such as b0 00. */
#define RTL8188EU_IEEE80211_FCTL_TYPE_MASK 0x000c
#define RTL8188EU_IEEE80211_FTYPE_MGMT     0x0000
#define RTL8188EU_IEEE80211_FTYPE_DATA     0x0008

static int rtl8188eu_bssid_equal(const uint8_t *a, const uint8_t *b)
{
    unsigned int i;

    for (i = 0; i < 6; i++)
        if (a[i] != b[i])
            return 0;
    return 1;
}

static void rtl8188eu_handle_connection_management(const uint8_t *frame,
                                                     unsigned int length)
{
    uint8_t subtype;

    if (length < 30)
        return;
    subtype = frame[0] & 0xf0;
    if (subtype == 0xb0 || subtype == 0x10 || subtype == 0x30)
        usb_printk("rtl8188eu: RX management response subtype=%02x "
                   "addr2=%02x:%02x:%02x:%02x:%02x:%02x "
                   "addr3=%02x:%02x:%02x:%02x:%02x:%02x\n", subtype,
                   frame[10], frame[11], frame[12], frame[13], frame[14],
                   frame[15],
                   frame[16], frame[17], frame[18], frame[19], frame[20],
                   frame[21]);
    if (subtype == 0xb0 && length >= 30 &&
        rtl8188eu.wifi_state != RTL8188EU_WIFI_AUTHENTICATING) {
        usb_printk("rtl8188eu: observed auth frame state=%u "
                   "DA=%02x:%02x:%02x:%02x:%02x:%02x "
                   "SA=%02x:%02x:%02x:%02x:%02x:%02x "
                   "BSSID=%02x:%02x:%02x:%02x:%02x:%02x "
                   "seq=%u status=%u\n",
                   rtl8188eu.wifi_state,
                   frame[4], frame[5], frame[6], frame[7], frame[8], frame[9],
                   frame[10], frame[11], frame[12], frame[13], frame[14],
                   frame[15], frame[16], frame[17], frame[18], frame[19],
                   frame[20], frame[21],
                   rtl8188eu_get_le16(frame + 26),
                   rtl8188eu_get_le16(frame + 28));
    }
    /* Authentication replies seen from some RTL8188 firmware revisions have
     * a non-canonical addr3 (or a locally-generated destination address).
     * The AP transmitter (addr2) is the authoritative identity here; do not
     * discard an otherwise valid response merely because addr1/addr3 is not
     * an exact copy of the interface/BSSID tuple. */
    if (!rtl8188eu_bssid_equal(frame + 10, rtl8188eu_connect_bssid) &&
        !rtl8188eu_bssid_equal(frame + 16, rtl8188eu_connect_bssid))
        return;
    if (subtype == 0xb0 && rtl8188eu.wifi_state ==
        RTL8188EU_WIFI_AUTHENTICATING && rtl8188eu_auth_tx_started) {
        uint16_t algorithm = rtl8188eu_get_le16(frame + 24);
        uint16_t sequence = rtl8188eu_get_le16(frame + 26);
        uint16_t status = rtl8188eu_get_le16(frame + 28);

        usb_printk("rtl8188eu: auth response candidate alg=%u seq=%u "
                   "status=%u DA=%02x:%02x:%02x:%02x:%02x:%02x "
                   "SA=%02x:%02x:%02x:%02x:%02x:%02x "
                   "BSSID=%02x:%02x:%02x:%02x:%02x:%02x\n",
                   algorithm, sequence, status,
                   frame[4], frame[5], frame[6], frame[7], frame[8], frame[9],
                   frame[10], frame[11], frame[12], frame[13], frame[14], frame[15],
                   frame[16], frame[17], frame[18], frame[19], frame[20], frame[21]);

        if (!rtl8188eu_bssid_equal(frame + 10, rtl8188eu_connect_bssid) &&
            !rtl8188eu_bssid_equal(frame + 16, rtl8188eu_connect_bssid)) {
            usb_printk("rtl8188eu: ignore auth response from unexpected "
                       "SA/BSSID\n");
            return;
        }

        usb_printk("rtl8188eu: authentication response alg=%u seq=%u "
                   "status=%u\n", algorithm, sequence, status);
        if (algorithm != 0 || sequence != 2) {
            usb_printk("rtl8188eu: authentication response invalid "
                       "alg=%u seq=%u\n", algorithm, sequence);
            /* The shared ieee80211 parser is authoritative for state. */
        } else if (rtl8188eu_auth_status < 0) {
            rtl8188eu_auth_status = status;
            usb_printk("rtl8188eu: auth status accepted=%d\n",
                       rtl8188eu_auth_status);
        }
    } else if ((subtype == 0x10 || subtype == 0x30) &&
               rtl8188eu.wifi_state == RTL8188EU_WIFI_ASSOCIATING) {
        if (rtl8188eu_assoc_status < 0) {
            rtl8188eu_assoc_status = rtl8188eu_get_le16(frame + 26);
            rtl8188eu_assoc_aid = rtl8188eu_get_le16(frame + 28) & 0x3fff;
        }
        usb_printk("rtl8188eu: association response status=%d AID=%u\n",
                   rtl8188eu_assoc_status, rtl8188eu_assoc_aid);
    }
}

static void rtl8188eu_put_be16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value >> 8);
    data[1] = (uint8_t)value;
}

static int rtl8188eu_mac_less(const uint8_t *a, const uint8_t *b)
{
    unsigned int i;
    for (i = 0; i < 6; i++) {
        if (a[i] < b[i])
            return 1;
        if (a[i] > b[i])
            return 0;
    }
    return 0;
}

static void rtl8188eu_make_snonce(void)
{
    struct wpa2_sha1_ctx sha;
    uint8_t digest[20];
    uint32_t seed;
    unsigned int i;

    extern uint32_t ticks;
    seed = ticks ^ ((uint32_t)rtl8188eu_tx_sequence << 16);
    wpa2_sha1_init(&sha);
    wpa2_sha1_update(&sha, rtl8188eu.mac_addr, 6);
    wpa2_sha1_update(&sha, rtl8188eu_connect_bssid, 6);
    wpa2_sha1_update(&sha, rtl8188eu_eapol_anonce,
                     sizeof(rtl8188eu_eapol_anonce));
    wpa2_sha1_update(&sha, (const uint8_t *)&seed, sizeof(seed));
    wpa2_sha1_final(&sha, digest);
    for (i = 0; i < sizeof(digest); i++)
        rtl8188eu_eapol_snonce[i] = digest[i];
    seed++;
    wpa2_sha1_init(&sha);
    wpa2_sha1_update(&sha, digest, sizeof(digest));
    wpa2_sha1_update(&sha, (const uint8_t *)&seed, sizeof(seed));
    wpa2_sha1_final(&sha, digest);
    for (i = 0; i < sizeof(rtl8188eu_eapol_snonce) - sizeof(digest); i++)
        rtl8188eu_eapol_snonce[sizeof(digest) + i] = digest[i];
}

static int rtl8188eu_derive_ptk(void)
{
    uint8_t data[76];
    uint8_t digest[20];
    unsigned int i;
    const uint8_t *mac_first;
    const uint8_t *mac_second;
    const uint8_t *nonce_first;
    const uint8_t *nonce_second;

    if (!rtl8188eu_wifi_password_length)
        return -EINVAL;
    if (wpa2_pbkdf2_sha1(rtl8188eu_wifi_password,
                         rtl8188eu_wifi_password_length,
                         rtl8188eu.connected_ssid,
                         rtl8188eu.connected_ssid_length, 4096,
                         rtl8188eu_pmk, sizeof(rtl8188eu_pmk)) < 0)
        return -EIO;
    if (rtl8188eu_mac_less(rtl8188eu.mac_addr, rtl8188eu_connect_bssid)) {
        mac_first = rtl8188eu.mac_addr;
        mac_second = rtl8188eu_connect_bssid;
    } else {
        mac_first = rtl8188eu_connect_bssid;
        mac_second = rtl8188eu.mac_addr;
    }
    if (rtl8188eu_mac_less(rtl8188eu_eapol_anonce,
                           rtl8188eu_eapol_snonce)) {
        nonce_first = rtl8188eu_eapol_anonce;
        nonce_second = rtl8188eu_eapol_snonce;
    } else {
        nonce_first = rtl8188eu_eapol_snonce;
        nonce_second = rtl8188eu_eapol_anonce;
    }
    for (i = 0; i < 6; i++)
        data[i] = mac_first[i];
    for (i = 0; i < 6; i++)
        data[6 + i] = mac_second[i];
    for (i = 0; i < 32; i++)
        data[12 + i] = nonce_first[i];
    for (i = 0; i < 32; i++)
        data[44 + i] = nonce_second[i];
    if (wpa2_prf512(rtl8188eu_pmk, sizeof(rtl8188eu_pmk),
                    "Pairwise key expansion", data, sizeof(data),
                    rtl8188eu_ptk) < 0)
        return -EIO;
    rtl8188eu_ptk_ready = 1;
    usb_printk("rtl8188eu: PMK/PTK derived for WPA2-PSK\n");
    return 0;
}

static int rtl8188eu_write_cam_key(unsigned int entry, const uint8_t mac[6],
                                   unsigned int key_id, const uint8_t key[16])
{
    uint32_t content;
    unsigned int i;

    if (!rtl8188eu.usb || !mac || !key || entry >= 4)
        return -EINVAL;
    content = (uint32_t)mac[0] | ((uint32_t)mac[1] << 8) |
              ((uint32_t)mac[2] << 16) | ((uint32_t)mac[3] << 24);
    if (rtl8188eu_write32(rtl8188eu.usb, RTL8188EU_REG_CAMWRITE,
                          content) < 0 ||
        rtl8188eu_write32(rtl8188eu.usb, RTL8188EU_REG_CAMCMD,
                          RTL8188EU_CAM_POLLING | RTL8188EU_CAM_WRITE |
                          entry * 6) < 0)
        return -EIO;
    content = (uint32_t)mac[4] | ((uint32_t)mac[5] << 8) |
              ((key_id & 3u) << 16) |
              (RTL8188EU_CAM_ALGO_AES << 2) | RTL8188EU_CAM_VALID;
    if (rtl8188eu_write32(rtl8188eu.usb, RTL8188EU_REG_CAMWRITE,
                          content) < 0 ||
        rtl8188eu_write32(rtl8188eu.usb, RTL8188EU_REG_CAMCMD,
                          RTL8188EU_CAM_POLLING | RTL8188EU_CAM_WRITE |
                          entry * 6 + 1) < 0)
        return -EIO;
    for (i = 0; i < 4; i++) {
        content = (uint32_t)key[i * 4] |
                  ((uint32_t)key[i * 4 + 1] << 8) |
                  ((uint32_t)key[i * 4 + 2] << 16) |
                  ((uint32_t)key[i * 4 + 3] << 24);
        if (rtl8188eu_write32(rtl8188eu.usb, RTL8188EU_REG_CAMWRITE,
                              content) < 0 ||
            rtl8188eu_write32(rtl8188eu.usb, RTL8188EU_REG_CAMCMD,
                              RTL8188EU_CAM_POLLING | RTL8188EU_CAM_WRITE |
                              entry * 6 + 2 + i) < 0)
            return -EIO;
    }
    if (rtl8188eu_write8(rtl8188eu.usb, RTL8188EU_REG_SECCFG,
                         RTL8188EU_SECCFG_TX | RTL8188EU_SECCFG_RX) < 0)
        return -EIO;
    usb_printk("rtl8188eu: installed AES CAM entry=%u keyid=%u\n",
               entry, key_id);
    return 0;
}

static int rtl8188eu_eapol_mic_matches(const uint8_t *eapol,
                                        unsigned int total_length,
                                        const uint8_t expected[16])
{
    uint8_t copy[4 + 95 + 256];
    uint8_t digest[20];
    unsigned int i;

    if (!eapol || !expected || !rtl8188eu_ptk_ready ||
        total_length < 4 + 95 || total_length > sizeof(copy))
        return 0;
    for (i = 0; i < total_length; i++)
        copy[i] = eapol[i];
    for (i = 0; i < 16; i++)
        copy[4 + 77 + i] = 0;
    wpa2_hmac_sha1(rtl8188eu_ptk, 16, copy, total_length, digest);
    for (i = 0; i < 16; i++)
        if (digest[i] != expected[i])
            return 0;
    return 1;
}

static int rtl8188eu_install_group_key(const uint8_t *key_data,
                                       unsigned int key_data_length)
{
    uint8_t plain[256];
    uint8_t broadcast[6];
    size_t plain_length;
    unsigned int pos;
    unsigned int i;

    if (!key_data || !key_data_length || key_data_length > sizeof(plain) ||
        !rtl8188eu_ptk_ready)
        return -EINVAL;
    if (rtl8188eu_eapol_key_info & RTL8188EU_EAPOL_KEY_INFO_ENCR) {
        if (wpa2_aes128_unwrap(rtl8188eu_ptk + 16, key_data,
                               key_data_length, plain, sizeof(plain),
                               &plain_length) < 0)
            return -EIO;
    } else {
        plain_length = key_data_length;
        for (i = 0; i < key_data_length; i++)
            plain[i] = key_data[i];
    }

    for (i = 0; i < sizeof(broadcast); i++)
        broadcast[i] = 0xff;
    pos = 0;
    while (pos + 2 <= plain_length) {
        uint8_t element_id = plain[pos];
        unsigned int element_length = plain[pos + 1];
        const uint8_t *element;
        unsigned int gtk_length;
        unsigned int key_id;

        pos += 2;
        if (element_id == RTL8188EU_DHCP_OPTION_PAD)
            continue;
        if (element_id == RTL8188EU_DHCP_OPTION_END)
            break;
        if (pos + element_length > plain_length)
            return -EIO;
        element = plain + pos;
        if (element_id == 0xdd && element_length >= 22 &&
            element[0] == 0x00 && element[1] == 0x0f &&
            element[2] == 0xac && element[3] == 0x01) {
            key_id = (element[4] >> 2) & RTL8188EU_EAPOL_KEY_INFO_KEYID;
            gtk_length = element_length - 6;
            if (gtk_length < 16)
                return -EIO;
            if (rtl8188eu_write_cam_key(1 + key_id, broadcast, key_id,
                                         element + 6) < 0)
                return -EIO;
            usb_printk("rtl8188eu: installed GTK keyid=%u length=%u\n",
                       key_id, gtk_length);
            return 0;
        }
        pos += element_length;
    }
    usb_printk("rtl8188eu: WPA2 M3 contained no GTK KDE\n");
    return -ENOENT;
}
static size_t rtl8188eu_wrap_data_frame(uint8_t *packet, size_t capacity,
                                        const uint8_t *destination,
                                        uint16_t ethertype,
                                        const uint8_t *payload,
                                        size_t payload_length,
                                        int encrypted)
{
    uint8_t *frame;
    uint32_t txdw0;
    uint32_t txdw1;
    uint32_t txdw4;
    size_t frame_length = 24 + 8 + payload_length;
    size_t i;

    if (!packet || !destination || (!payload && payload_length) ||
        capacity < RTL8188EU_TX_DESC_SIZE + frame_length)
        return 0;
    A_memset(packet, 0, RTL8188EU_TX_DESC_SIZE + frame_length);
    frame = packet + RTL8188EU_TX_DESC_SIZE;
    frame[0] = encrypted ? 0x08 : 0x08;
    frame[1] = encrypted ? 0x41 : 0x01;
    for (i = 0; i < 6; i++) {
        frame[4 + i] = rtl8188eu_connect_bssid[i];
        frame[10 + i] = rtl8188eu.mac_addr[i];
        frame[16 + i] = destination[i];
    }
    frame[24] = 0xaa; frame[25] = 0xaa; frame[26] = 0x03;
    frame[27] = 0x00; frame[28] = 0x00; frame[29] = 0x00;
    frame[30] = (uint8_t)(ethertype >> 8);
    frame[31] = (uint8_t)ethertype;
    for (i = 0; i < payload_length; i++)
        frame[32 + i] = payload[i];
    txdw0 = (uint32_t)frame_length | (RTL8188EU_TX_DESC_SIZE << 16) |
            (1u << 26) | (1u << 27) | (1u << 31);
    txdw1 = encrypted ? RTL8188EU_TXDW2_SEC_TYPE_AES : 0;
    txdw4 = RTL8188EU_DESC_RATE_1M | RTL8188EU_TXDW4_HWSEQ_EN |
            RTL8188EU_TXDW4_USE_RATE | RTL8188EU_TXDW4_DISDATAFB;
    rtl8188eu_put_le32(packet, txdw0);
    rtl8188eu_put_le32(packet + 4, txdw1);
    rtl8188eu_put_le32(packet + 16, txdw4);
    rtl8188eu_txdesc_checksum(packet);
    return RTL8188EU_TX_DESC_SIZE + frame_length;
}

static int rtl8188eu_net_send(net_device_t *dev, uint8_t *data,
                              uint32_t length)
{
    uint8_t packet[RTL8188EU_TX_DESC_SIZE + 24 + 8 + ETH_MTU];
    uint16_t ethertype;
    size_t packet_length;

    if (!dev || dev != &rtl8188eu_netdev || !data ||
        length < ETH_HDR_LEN || length > ETH_MAX_FRAME ||
        !rtl8188eu_connection_binding_valid() ||
        rtl8188eu.wifi_state != RTL8188EU_WIFI_CONNECTED)
        return -ENODEV;
    ethertype = ((uint16_t)data[12] << 8) | data[13];
    packet_length = rtl8188eu_wrap_data_frame(
        packet, sizeof(packet), data, ethertype,
        data + ETH_HDR_LEN, length - ETH_HDR_LEN, 1);
    if (!packet_length)
        return -EINVAL;
    return rtl8188eu_bulk_out(&rtl8188eu, rtl8188eu.bulk_out,
                              packet, packet_length);
}

static void rtl8188eu_deliver_data_frame(const uint8_t *frame,
                                         unsigned int length)
{
    uint16_t frame_control;
    unsigned int header_length;
    unsigned int payload_length;
    uint16_t ethertype;
    uint8_t ethernet[ETH_MAX_FRAME];
    unsigned int i;

    if (!frame || length < 24)
        return;
    frame_control = rtl8188eu_get_le16(frame);
    if ((frame_control & RTL8188EU_IEEE80211_FCTL_TYPE_MASK) !=
        RTL8188EU_IEEE80211_FTYPE_DATA)
        return;
    if ((frame_control & 0x0100) || !(frame_control & 0x0200))
        return;
    if (!rtl8188eu_bssid_equal(frame + 10, rtl8188eu_connect_bssid))
        return;
    header_length = 24;
    if (((frame_control >> 4) & 0x0f) >= 8)
        header_length += 2;
    if (frame_control & 0x8000)
        header_length += 4;
    if (length < header_length + 8 ||
        frame[header_length] != 0xaa ||
        frame[header_length + 1] != 0xaa ||
        frame[header_length + 2] != 0x03 ||
        frame[header_length + 3] != 0x00 ||
        frame[header_length + 4] != 0x00 ||
        frame[header_length + 5] != 0x00)
        return;
    ethertype = ((uint16_t)frame[header_length + 6] << 8) |
                frame[header_length + 7];
    if (ethertype == 0x888e) {
        rtl8188eu_handle_eapol(frame, length);
        return;
    }
    payload_length = length - header_length - 8;
    if (ETH_HDR_LEN + payload_length > sizeof(ethernet))
        return;
    for (i = 0; i < ETH_ALEN; i++) {
        ethernet[i] = frame[4 + i];
        ethernet[ETH_ALEN + i] = frame[16 + i];
    }
    ethernet[12] = (uint8_t)(ethertype >> 8);
    ethernet[13] = (uint8_t)ethertype;
    for (i = 0; i < payload_length; i++)
        ethernet[ETH_HDR_LEN + i] = frame[header_length + 8 + i];
    net_rx_packet(&rtl8188eu_netdev, ethernet,
                  ETH_HDR_LEN + payload_length);
}

static int rtl8188eu_register_netdev(void)
{
    unsigned int i;

    if (rtl8188eu.net_registered)
        return 0;
    A_memset(&rtl8188eu_netdev, 0, sizeof(rtl8188eu_netdev));
    rtl8188eu_netdev.name[0] = 'w';
    rtl8188eu_netdev.name[1] = 'l';
    rtl8188eu_netdev.name[2] = 'a';
    rtl8188eu_netdev.name[3] = 'n';
    rtl8188eu_netdev.name[4] = '0';
    for (i = 0; i < ETH_ALEN; i++) {
        rtl8188eu_netdev.mac_addr[i] = rtl8188eu.mac_addr[i];
        local_mac[i] = rtl8188eu.mac_addr[i];
    }
    rtl8188eu_netdev.priv = &rtl8188eu;
    rtl8188eu_netdev.send = rtl8188eu_net_send;
    if (net_device_register(&rtl8188eu_netdev) < 0) {
        usb_printk("rtl8188eu: network device registration failed\n");
        return -EIO;
    }
    rtl8188eu.net_registered = 1;
    usb_printk("rtl8188eu: registered network device wlan0\n");
    return 0;
}

static void rtl8188eu_dhcp_put_be32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)(value >> 24);
    data[1] = (uint8_t)(value >> 16);
    data[2] = (uint8_t)(value >> 8);
    data[3] = (uint8_t)value;
}

static size_t rtl8188eu_build_dhcp_frame(uint8_t *frame, size_t capacity,
                                         uint8_t message_type,
                                         uint32_t requested_ip,
                                         uint32_t server_ip)
{
    uint8_t *ip;
    uint8_t *udp;
    uint8_t *dhcp;
    size_t option_length;
    size_t dhcp_length;
    size_t ip_length;
    unsigned int i;

    if (!frame || capacity < ETH_HDR_LEN + IP_HDR_LEN + UDP_HDR_LEN + 300)
        return 0;
    A_memset(frame, 0, capacity);
    for (i = 0; i < ETH_ALEN; i++) {
        frame[i] = 0xff;
        frame[ETH_ALEN + i] = rtl8188eu.mac_addr[i];
    }
    frame[12] = (uint8_t)(ETH_P_IP >> 8);
    frame[13] = (uint8_t)ETH_P_IP;
    ip = frame + ETH_HDR_LEN;
    udp = ip + IP_HDR_LEN;
    dhcp = udp + UDP_HDR_LEN;
    A_memset(dhcp, 0, 300);
    dhcp[0] = 1;
    dhcp[1] = 1;
    dhcp[2] = 6;
    rtl8188eu_dhcp_put_be32(dhcp + 4, rtl8188eu_dhcp_xid);
    dhcp[10] = 0x80;
    for (i = 0; i < ETH_ALEN; i++)
        dhcp[28 + i] = rtl8188eu.mac_addr[i];
    rtl8188eu_dhcp_put_be32(dhcp + 236, RTL8188EU_DHCP_MAGIC);
    option_length = 0;
    dhcp[240 + option_length++] = RTL8188EU_DHCP_OPTION_MESSAGE;
    dhcp[240 + option_length++] = 1;
    dhcp[240 + option_length++] = message_type;
    if (message_type == RTL8188EU_DHCP_REQUEST) {
        dhcp[240 + option_length++] = RTL8188EU_DHCP_OPTION_REQUESTED;
        dhcp[240 + option_length++] = 4;
        rtl8188eu_dhcp_put_be32(dhcp + 240 + option_length,
                                requested_ip);
        option_length += 4;
        dhcp[240 + option_length++] = RTL8188EU_DHCP_OPTION_SERVER;
        dhcp[240 + option_length++] = 4;
        rtl8188eu_dhcp_put_be32(dhcp + 240 + option_length, server_ip);
        option_length += 4;
    }
    dhcp[240 + option_length++] = 55;
    dhcp[240 + option_length++] = 3;
    dhcp[240 + option_length++] = RTL8188EU_DHCP_OPTION_SUBNET;
    dhcp[240 + option_length++] = RTL8188EU_DHCP_OPTION_ROUTER;
    dhcp[240 + option_length++] = RTL8188EU_DHCP_OPTION_DNS;
    dhcp[240 + option_length++] = RTL8188EU_DHCP_OPTION_END;
    dhcp_length = 240 + option_length;
    ip_length = IP_HDR_LEN + UDP_HDR_LEN + dhcp_length;
    ip[0] = 0x45;
    ip[2] = (uint8_t)(ip_length >> 8);
    ip[3] = (uint8_t)ip_length;
    ip[8] = IP_TTL;
    ip[9] = IPPROTO_UDP;
    rtl8188eu_dhcp_put_be32(ip + 12, 0);
    rtl8188eu_dhcp_put_be32(ip + 16, 0xffffffffu);
    *(uint16_t *)(ip + 10) = 0;
    *(uint16_t *)(ip + 10) = internet_checksum((uint16_t *)ip,
                                                 IP_HDR_LEN);
    udp[0] = 0;
    udp[1] = RTL8188EU_DHCP_CLIENT_PORT;
    udp[2] = 0;
    udp[3] = RTL8188EU_DHCP_SERVER_PORT;
    udp[4] = (uint8_t)((UDP_HDR_LEN + dhcp_length) >> 8);
    udp[5] = (uint8_t)(UDP_HDR_LEN + dhcp_length);
    udp[6] = 0;
    udp[7] = 0;
    return ETH_HDR_LEN + ip_length;
}

static int rtl8188eu_dhcp_option(const uint8_t *packet, unsigned int length,
                                 uint8_t wanted, uint8_t *value,
                                 unsigned int capacity, unsigned int *value_length)
{
    unsigned int pos = 240;

    if (!packet || length < 240 ||
        packet[236] != 0x63 || packet[237] != 0x82 ||
        packet[238] != 0x53 || packet[239] != 0x63)
        return -EINVAL;
    while (pos < length) {
        uint8_t id = packet[pos++];
        uint8_t option_length;
        if (id == RTL8188EU_DHCP_OPTION_PAD)
            continue;
        if (id == RTL8188EU_DHCP_OPTION_END)
            break;
        if (pos >= length)
            return -EIO;
        option_length = packet[pos++];
        if (pos + option_length > length)
            return -EIO;
        if (id == wanted) {
            if (option_length > capacity || !value)
                return -EINVAL;
            for (unsigned int i = 0; i < option_length; i++)
                value[i] = packet[pos + i];
            if (value_length)
                *value_length = option_length;
            return 0;
        }
        pos += option_length;
    }
    return -ENOENT;
}

static int rtl8188eu_dhcp_read_packet(uint8_t *packet, unsigned int capacity,
                                      uint32_t xid, uint8_t wanted_type,
                                      uint32_t *yiaddr, uint32_t *server_ip,
                                      uint32_t *netmask, uint32_t *gateway)
{
    int source_port;
    int length;
    uint8_t option[4];
    unsigned int option_length;
    uint8_t message_type;
    uint32_t packet_xid;

    length = net_recv_udp_internal((char *)packet, capacity, &source_port);
    if (length < 240 || source_port != RTL8188EU_DHCP_SERVER_PORT)
        return -EAGAIN;
    packet_xid = ((uint32_t)packet[4] << 24) |
                 ((uint32_t)packet[5] << 16) |
                 ((uint32_t)packet[6] << 8) | packet[7];
    if (packet_xid != xid || packet[0] != 2 || packet[1] != 1 ||
        packet[2] != 6)
        return -EAGAIN;
    if (rtl8188eu_dhcp_option(packet, (unsigned int)length,
                              RTL8188EU_DHCP_OPTION_MESSAGE, &message_type,
                              1, &option_length) < 0 ||
        message_type != wanted_type)
        return -EAGAIN;
    if (yiaddr)
        *yiaddr = ((uint32_t)packet[16] << 24) |
                  ((uint32_t)packet[17] << 16) |
                  ((uint32_t)packet[18] << 8) | packet[19];
    if (server_ip && rtl8188eu_dhcp_option(packet, (unsigned int)length,
                                            RTL8188EU_DHCP_OPTION_SERVER,
                                            option, sizeof(option),
                                            &option_length) == 0 &&
        option_length == 4)
        *server_ip = ((uint32_t)option[0] << 24) |
                     ((uint32_t)option[1] << 16) |
                     ((uint32_t)option[2] << 8) | option[3];
    if (netmask && rtl8188eu_dhcp_option(packet, (unsigned int)length,
                                         RTL8188EU_DHCP_OPTION_SUBNET,
                                         option, sizeof(option),
                                         &option_length) == 0 &&
        option_length == 4)
        *netmask = ((uint32_t)option[0] << 24) |
                  ((uint32_t)option[1] << 16) |
                  ((uint32_t)option[2] << 8) | option[3];
    if (gateway && rtl8188eu_dhcp_option(packet, (unsigned int)length,
                                         RTL8188EU_DHCP_OPTION_ROUTER,
                                         option, sizeof(option),
                                         &option_length) == 0 &&
        option_length >= 4)
        *gateway = ((uint32_t)option[0] << 24) |
                  ((uint32_t)option[1] << 16) |
                  ((uint32_t)option[2] << 8) | option[3];
    return 0;
}

static int rtl8188eu_dhcp_configure(void)
{
    uint8_t frame[ETH_MAX_FRAME];
    uint8_t packet[600];
    size_t frame_length;
    uint32_t offered_ip = 0;
    uint32_t server_ip = 0;
    uint32_t mask = 0;
    uint32_t gateway = 0;
    unsigned int poll;
    int got_offer = 0;

    if (!rtl8188eu.net_registered)
        return -ENODEV;
    extern uint32_t ticks;
    rtl8188eu_dhcp_xid = ticks ^ ((uint32_t)rtl8188eu_tx_sequence << 16) ^
                          0x8188e000u;
    net_set_ipaddr(0);
    rtl8188eu_netdev.ip_addr = 0;
    net_bind_udp_port(RTL8188EU_DHCP_CLIENT_PORT);
    frame_length = rtl8188eu_build_dhcp_frame(
        frame, sizeof(frame), RTL8188EU_DHCP_DISCOVER, 0, 0);
    if (!frame_length || rtl8188eu_net_send(&rtl8188eu_netdev, frame,
                                             (uint32_t)frame_length) < 0)
        return -EIO;
    usb_printk("rtl8188eu: DHCP discover sent xid=%08x\n",
               rtl8188eu_dhcp_xid);
    for (poll = 0; poll < 5000; poll++) {
        usb_poll_interrupts();
        if ((poll % 10u) == 0 &&
            rtl8188eu_dhcp_read_packet(packet, sizeof(packet),
                                       rtl8188eu_dhcp_xid,
                                       RTL8188EU_DHCP_OFFER, &offered_ip,
                                       &server_ip, &mask, &gateway) == 0) {
            got_offer = 1;
            break;
        }
        tick_delay(2000);
    }
    if (!got_offer || !offered_ip || !server_ip)
        return -ETIMEDOUT;
    frame_length = rtl8188eu_build_dhcp_frame(
        frame, sizeof(frame), RTL8188EU_DHCP_REQUEST, offered_ip,
        server_ip);
    if (!frame_length || rtl8188eu_net_send(&rtl8188eu_netdev, frame,
                                             (uint32_t)frame_length) < 0)
        return -EIO;
    usb_printk("rtl8188eu: DHCP request sent address=%u.%u.%u.%u\n",
               (offered_ip >> 24) & 0xff, (offered_ip >> 16) & 0xff,
               (offered_ip >> 8) & 0xff, offered_ip & 0xff);
    for (poll = 0; poll < 5000; poll++) {
        usb_poll_interrupts();
        if ((poll % 10u) == 0 &&
            rtl8188eu_dhcp_read_packet(packet, sizeof(packet),
                                       rtl8188eu_dhcp_xid,
                                       RTL8188EU_DHCP_ACK, &offered_ip,
                                       &server_ip, &mask, &gateway) == 0)
            break;
        tick_delay(2000);
    }
    if (poll == 5000 || !offered_ip || !mask || !gateway)
        return -ETIMEDOUT;
    net_set_ipaddr(offered_ip);
    net_set_netmask(mask);
    net_set_gateway(gateway);
    rtl8188eu_netdev.ip_addr = offered_ip;
    usb_printk("rtl8188eu: DHCP configured IP=%u.%u.%u.%u mask=%u.%u.%u.%u gateway=%u.%u.%u.%u\n",
               (offered_ip >> 24) & 0xff, (offered_ip >> 16) & 0xff,
               (offered_ip >> 8) & 0xff, offered_ip & 0xff,
               (mask >> 24) & 0xff, (mask >> 16) & 0xff,
               (mask >> 8) & 0xff, mask & 0xff,
               (gateway >> 24) & 0xff, (gateway >> 16) & 0xff,
               (gateway >> 8) & 0xff, gateway & 0xff);
    return 0;
}
static int rtl8188eu_send_eapol_response(uint16_t key_info,
                                         const uint8_t nonce[32],
                                         const uint8_t replay[8],
                                         const uint8_t *key_data,
                                         unsigned int key_data_length)
{
    uint8_t eapol[4 + 95 + 256];
    uint8_t packet[RTL8188EU_TX_DESC_SIZE + 24 + 8 + sizeof(eapol)];
    uint8_t mic[20];
    size_t packet_length;
    unsigned int i;
    unsigned int eapol_length = 95 + key_data_length;

    if (!rtl8188eu_ptk_ready || !nonce || !replay ||
        key_data_length > 256)
        return -EINVAL;
    A_memset(eapol, 0, sizeof(eapol));
    eapol[0] = 2;
    eapol[1] = 3;
    rtl8188eu_put_be16(eapol + 2, (uint16_t)eapol_length);
    eapol[4] = 2;
    rtl8188eu_put_be16(eapol + 5, key_info);
    rtl8188eu_put_be16(eapol + 7, 16);
    for (i = 0; i < 8; i++)
        eapol[9 + i] = replay[i];
    for (i = 0; i < 32; i++)
        eapol[17 + i] = nonce[i];
    rtl8188eu_put_be16(eapol + 97, (uint16_t)key_data_length);
    for (i = 0; i < key_data_length; i++)
        eapol[99 + i] = key_data[i];
    wpa2_hmac_sha1(rtl8188eu_ptk, 16, eapol, 4 + eapol_length, mic);
    for (i = 0; i < 16; i++)
        eapol[81 + i] = mic[i];
    packet_length = rtl8188eu_wrap_data_frame(
        packet, sizeof(packet), rtl8188eu_connect_bssid, 0x888e,
        eapol, 4 + eapol_length, 0);
    if (!packet_length || rtl8188eu_bulk_out(&rtl8188eu, rtl8188eu.bulk_out,
                                              packet, packet_length) < 0)
        return -EIO;
    usb_printk("rtl8188eu: sent EAPOL-Key response key_info=%04x data=%u\n",
               key_info, key_data_length);
    return 0;
}
static void rtl8188eu_handle_eapol(const uint8_t *frame, unsigned int length)
{
    static const uint8_t rsn_ie[] = {
        0x30, 0x14, 0x01, 0x00,
        0x00, 0x0f, 0xac, 0x04,
        0x01, 0x00, 0x00, 0x0f, 0xac, 0x04,
        0x01, 0x00, 0x00, 0x0f, 0xac, 0x02,
        0x00, 0x00
    };
    unsigned int hdr_len;
    unsigned int llc;
    unsigned int eapol;
    unsigned int body;
    unsigned int key_data_length;
    unsigned int key_frame_length;
    unsigned int i;
    uint16_t key_info;
    uint8_t descriptor_type;
    int from_ap;
    int pairwise;
    int install;
    int ack;
    int mic;
    int secure;
    int message;
    uint8_t eapol_type;
    uint16_t eapol_length;
    uint8_t expected_mic[16];

    if (!frame || length < 24 || (frame[0] & 0x0c) != 0x08)
        return;
    hdr_len = (frame[0] & 0x80) ? 26u : 24u;
    if (length < hdr_len + 12)
        return;
    from_ap = rtl8188eu_bssid_equal(frame + 10, rtl8188eu_connect_bssid) &&
              rtl8188eu_bssid_equal(frame + 4, rtl8188eu.mac_addr);
    if (!from_ap && !(rtl8188eu_bssid_equal(frame + 10, rtl8188eu.mac_addr) &&
                      rtl8188eu_bssid_equal(frame + 4,
                                            rtl8188eu_connect_bssid)))
        return;
    llc = hdr_len;
    if (frame[llc] != 0xaa || frame[llc + 1] != 0xaa ||
        frame[llc + 2] != 0x03 || frame[llc + 3] != 0x00 ||
        frame[llc + 4] != 0x00 || frame[llc + 5] != 0x00 ||
        frame[llc + 6] != 0x88 || frame[llc + 7] != 0x8e)
        return;
    eapol = llc + 8;
    if (length < eapol + 4)
        return;
    eapol_type = frame[eapol + 1];
    eapol_length = ((uint16_t)frame[eapol + 2] << 8) |
                   frame[eapol + 3];
    if (eapol_type != 3 || eapol_length < 95 ||
        eapol_length > 95 + sizeof(rtl8188eu_eapol_key_data) ||
        length < eapol + 4 + eapol_length)
        return;
    key_frame_length = eapol_length;
    body = eapol + 4;
    descriptor_type = frame[body];
    if (descriptor_type != 2)
        return;
    key_info = ((uint16_t)frame[body + 1] << 8) | frame[body + 2];
    rtl8188eu_eapol_key_info = key_info;
    rtl8188eu_eapol_key_length = ((uint16_t)frame[body + 3] << 8) |
                                 frame[body + 4];
    key_data_length = ((uint16_t)frame[body + 93] << 8) |
                      frame[body + 94];
    if (95u + key_data_length > key_frame_length ||
        key_data_length > sizeof(rtl8188eu_eapol_key_data))
        return;

    pairwise = !!(key_info & 0x0008);
    install = !!(key_info & 0x0040);
    ack = !!(key_info & 0x0080);
    mic = !!(key_info & 0x0100);
    secure = !!(key_info & 0x0200);
    message = 0;
    if (pairwise && ack && !mic && from_ap)
        message = 1;
    else if (pairwise && mic && !ack && !secure && !from_ap)
        message = 2;
    else if (pairwise && install && ack && mic && from_ap)
        message = 3;
    else if (pairwise && mic && !ack && secure && !from_ap)
        message = 4;
    if (!message)
        return;

    rtl8188eu_eapol_messages++;
    usb_printk("rtl8188eu: EAPOL-Key message=%u key_info=%04x replay=",
               message, key_info);
    for (i = 0; i < 8; i++) {
        rtl8188eu_eapol_replay_counter[i] = frame[body + 5 + i];
        usb_printk("%02x", rtl8188eu_eapol_replay_counter[i]);
    }
    usb_printk(" key_data=%u direction=%s\n", key_data_length,
               from_ap ? "AP->STA" : "STA->AP");
    for (i = 0; i < key_data_length; i++)
        rtl8188eu_eapol_key_data[i] = frame[body + 95 + i];
    rtl8188eu_eapol_key_data_length = key_data_length;
    if (message == 1) {
        for (i = 0; i < sizeof(rtl8188eu_eapol_anonce); i++)
            rtl8188eu_eapol_anonce[i] = frame[body + 13 + i];
        rtl8188eu_make_snonce();
        if (rtl8188eu_derive_ptk() < 0 ||
            rtl8188eu_send_eapol_response(0x010a,
                                          rtl8188eu_eapol_snonce,
                                          rtl8188eu_eapol_replay_counter,
                                          rsn_ie, sizeof(rsn_ie)) < 0) {
            rtl8188eu.last_error = -EIO;
            return;
        }
        rtl8188eu_eapol_msg1_seen = 1;
        usb_printk("rtl8188eu: EAPOL M1 received; M2 sent\n");
    } else if (message == 2) {
        rtl8188eu_eapol_msg2_seen = 1;
    } else if (message == 3) {
        for (i = 0; i < 16; i++)
            expected_mic[i] = frame[body + 77 + i];
        rtl8188eu_eapol_mic_valid = rtl8188eu_eapol_mic_matches(
            frame + eapol, 4 + key_frame_length, expected_mic);
        if (!rtl8188eu_eapol_mic_valid) {
            usb_printk("rtl8188eu: WPA2 M3 MIC verification failed\n");
            rtl8188eu.last_error = -EIO;
            return;
        }
        if (rtl8188eu_write_cam_key(0, rtl8188eu_connect_bssid, 0,
                                     rtl8188eu_ptk + 32) < 0 ||
            rtl8188eu_install_group_key(rtl8188eu_eapol_key_data,
                                        rtl8188eu_eapol_key_data_length) < 0) {
            rtl8188eu.last_error = -EIO;
            return;
        }
        rtl8188eu_eapol_msg3_ready = 1;
        rtl8188eu_eapol_msg3_seen = 1;
        {
            uint8_t zero_nonce[32];
            A_memset(zero_nonce, 0, sizeof(zero_nonce));
            if (rtl8188eu_send_eapol_response(
                    0x030a, zero_nonce,
                    rtl8188eu_eapol_replay_counter, NULL, 0) < 0) {
                rtl8188eu.last_error = -EIO;
                return;
            }
        }
        rtl8188eu_eapol_msg4_seen = 1;
        rtl8188eu.wifi_state = RTL8188EU_WIFI_CONNECTED;
        rtl8188eu.associated = 1;
        usb_printk("rtl8188eu: WPA2 4-way handshake complete\n");
    } else {
        rtl8188eu_eapol_msg4_seen = 1;
    }
}
static void rtl8188eu_dump_bytes(const char *label, const uint8_t *data,
                                 unsigned int length)
{
    unsigned int i;

    usb_printk("rtl8188eu: %s len=%u bytes:\n", label, length);
    for (i = 0; i < length; i++) {
        if ((i & 15u) == 0)
            usb_printk("  %04x:", i);
        usb_printk(" %02x", data[i]);
        if ((i & 15u) == 15 || i + 1 == length)
            usb_printk("\n");
    }
}

static void rtl8188eu_log_tx_descriptor(const uint8_t *packet)
{
    unsigned int i;
    uint32_t dw0;
    uint32_t dw1;
    uint32_t dw4;

    usb_printk("rtl8188eu: TXDESC");
    for (i = 0; i < RTL8188EU_TX_DESC_SIZE; i += 4)
        usb_printk(" DW%u=%08x", i / 4, rtl8188eu_get_le32_early(packet + i));
    usb_printk(" checksum=%s frame_offset=%u\n",
               rtl8188eu_txdesc_checksum_valid(packet) ? "valid" : "bad",
               RTL8188EU_TX_DESC_SIZE);
    dw0 = rtl8188eu_get_le32_early(packet);
    dw1 = rtl8188eu_get_le32_early(packet + 4);
    dw4 = rtl8188eu_get_le32_early(packet + 16);
    if (dw0 & 0x3fff) {
        usb_printk("rtl8188eu: TX decoded pkt_len=%u offset=%u "
                   "qsel=%u raid=%u own=%u hwseq=%u use_rate=%u "
                   "frame_fc=%02x%02x DA=%02x:%02x:%02x:%02x:%02x:%02x\n",
                   dw0 & 0x3fff, (dw0 >> 16) & 0xff,
                   (dw1 >> 8) & 0x1f, (dw1 >> 16) & 0x1f,
                   !!(dw0 & (1u << 31)), !!(dw4 & (1u << 7)),
                   !!(dw4 & (1u << 8)), packet[32], packet[33],
                   packet[36], packet[37], packet[38],
                   packet[39], packet[40], packet[41]);
    }
}

static void rtl8188eu_parse_management_ies(const uint8_t *frame,
                                            unsigned int frame_length)
{
    struct ieee80211_mgmt_info common_info;
    int common_valid;
    unsigned int offset = 24 + 12;
    struct rtl8188eu_scan_result result;
    unsigned int result_index;
    uint8_t subtype;

    if (frame_length < 24)
        return;

    /* Feed connection-management frames through the dedicated AUTH/ASSOC
     * state handler as well as the generic IE parser.  This function used to
     * be defined but never reached from the RX path, so AUTH responses could
     * only be observed by diagnostics and could not advance auth_status. */
    rtl8188eu_handle_connection_management(frame, frame_length);

    common_valid = ieee80211_parse_mgmt(frame, frame_length, &common_info) == 0;
    if (common_valid) {
        if (common_info.subtype == IEEE80211_STYPE_AUTH &&
            rtl8188eu.wifi_state == RTL8188EU_WIFI_AUTHENTICATING &&
            rtl8188eu_auth_tx_started &&
            rtl8188eu_bssid_equal(common_info.addr1, rtl8188eu.mac_addr) &&
            (rtl8188eu_bssid_equal(common_info.addr2,
                                   rtl8188eu_connect_bssid) ||
             rtl8188eu_bssid_equal(common_info.addr3,
                                   rtl8188eu_connect_bssid)) &&
            common_info.auth_algorithm == 0 &&
            common_info.auth_transaction == 2) {
            usb_printk("rtl8188eu: common 80211 auth response status=%u\n",
                       common_info.status_code);
            rtl8188eu_auth_status = common_info.status_code;
        } else if (common_info.subtype == IEEE80211_STYPE_ASSOC_RESP &&
                   rtl8188eu.wifi_state == RTL8188EU_WIFI_ASSOCIATING &&
                   rtl8188eu_bssid_equal(common_info.addr2,
                                         rtl8188eu_connect_bssid)) {
            rtl8188eu_assoc_status = common_info.status_code;
            rtl8188eu_assoc_aid = common_info.assoc_aid;
        }
    }
    subtype = frame[0] & 0xf0;
    if (rtl8188eu.wifi_state == RTL8188EU_WIFI_AUTHENTICATING &&
        rtl8188eu_auth_tx_started && subtype != 0x80 && subtype != 0x50 &&
        frame_length >= 24 &&
        !rtl8188eu_is_own_probe_request(frame, frame_length)) {
        usb_printk("rtl8188eu: auth RX mgmt fc=%02x%02x "
                   "DA=%02x:%02x:%02x:%02x:%02x:%02x "
                   "SA=%02x:%02x:%02x:%02x:%02x:%02x "
                   "BSSID=%02x:%02x:%02x:%02x:%02x:%02x\n",
                   frame[0], frame[1],
                   frame[4], frame[5], frame[6], frame[7], frame[8], frame[9],
                   frame[10], frame[11], frame[12], frame[13], frame[14], frame[15],
                   frame[16], frame[17], frame[18], frame[19], frame[20], frame[21]);
    }
    if (subtype != 0x50 && subtype != 0x80) {
        return;
    }
    /* Beacon/Probe Response IEs belong to scan results only.  During
     * connect/authentication other APs may still be visible in the RX ring;
     * never let their advertised channel or SSID overwrite the selected
     * result. */
    if (rtl8188eu.wifi_state != RTL8188EU_WIFI_SCANNING)
        return;
    if (frame_length < offset) {
        static unsigned int truncated_mgmt_logs;
        if (truncated_mgmt_logs++ < 4)
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
            static unsigned int truncated_ie_logs;
            if (truncated_ie_logs++ < 8)
                usb_printk("rtl8188eu: RX IE id=%u truncated length=%u remaining=%u\n",
                           id, length, frame_length - offset);
            return;
        }
        if (id == 0) {
            result.ssid_length = length > RTL8188EU_MAX_SSID_LEN ?
                                 RTL8188EU_MAX_SSID_LEN : length;
            for (i = 0; i < result.ssid_length; i++)
                result.ssid[i] = frame[offset + i];
        } else if (id == 3 && length == 1) {
            result.channel = frame[offset];
        } else if (id == 48 && length >= 2) {
            result.rsn = 1;
        }
        offset += length;
    }
    /* Prefer the shared 802.11 parser's validated fields.  The legacy IE
     * walk above remains only as a fallback for older callers. */
    if (common_valid &&
        (common_info.subtype == IEEE80211_STYPE_BEACON ||
         common_info.subtype == IEEE80211_STYPE_PROBE_RESP)) {
        unsigned int copy_len = common_info.ssid_length;
        if (copy_len > RTL8188EU_MAX_SSID_LEN)
            copy_len = RTL8188EU_MAX_SSID_LEN;
        A_memset(result.ssid, 0, sizeof(result.ssid));
        for (result_index = 0; result_index < 6; result_index++)
            result.bssid[result_index] = common_info.addr3[result_index];
        for (result_index = 0; result_index < copy_len; result_index++)
            result.ssid[result_index] = common_info.ssid[result_index];
        result.ssid_length = (uint8_t)copy_len;
        result.channel = common_info.channel;
        result.privacy = common_info.privacy;
        result.rsn = common_info.rsn;
    }
    if (!result.channel)
        result.channel = rtl8188eu_scan_channel;
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
            if (!result.ssid_length &&
                rtl8188eu_scan_results[result_index].ssid_length)
                return;
            rtl8188eu_scan_results[result_index] = result;
            return;
        }
    }
    /* A beacon with an empty SSID is a hidden network, not a usable
     * name-based scan result. Do not expose it as a connectable SSID. */
    if (!result.ssid_length) {
        usb_printk("rtl8188eu: scan reject hidden/empty SSID "
                   "BSSID=%02x:%02x:%02x:%02x:%02x:%02x channel=%u\n",
                   result.bssid[0], result.bssid[1], result.bssid[2],
                   result.bssid[3], result.bssid[4], result.bssid[5],
                   result.channel);
        return;
    }
    if (rtl8188eu.scan_result_count < RTL8188EU_MAX_SCAN_RESULTS) {
        rtl8188eu_scan_results[rtl8188eu.scan_result_count++] = result;
        usb_printk("rtl8188eu: AP found channel=%u BSSID="
                   "%02x:%02x:%02x:%02x:%02x:%02x SSID=",
                   result.channel,
                   result.bssid[0], result.bssid[1], result.bssid[2],
                   result.bssid[3], result.bssid[4], result.bssid[5]);
        for (result_index = 0; result_index < result.ssid_length;
             result_index++) {
            uint8_t c = result.ssid[result_index];
            usb_printk("%c", c >= 0x20 && c <= 0x7e ? c : '.');
        }
        usb_printk(" type=%s\n",
                   subtype == 0x50 ? "probe-response" : "beacon");
    }
}

static void rtl8188eu_log_rx_descriptor(const uint8_t *data,
                                        unsigned int packet_length,
                                        unsigned int driver_info_length,
                                        unsigned int shift,
                                        unsigned int frame_offset)
{
    uint32_t dw[6];
    unsigned int i;

    if (!data)
        return;
    for (i = 0; i < 6; i++)
        dw[i] = rtl8188eu_get_le32_early(data + i * 4);
    usb_printk("rtl8188eu: RX descriptor DW0=%08x DW1=%08x DW2=%08x "
               "DW3=%08x DW4=%08x DW5=%08x\n",
               dw[0], dw[1], dw[2], dw[3], dw[4], dw[5]);
    usb_printk("rtl8188eu: RX descriptor decoded pkt_len=%u drvinfo=%u "
               "shift=%u frame_offset=%u crc=%u icv=%u\n",
               packet_length, driver_info_length, shift, frame_offset,
               (dw[0] >> 14) & 1, (dw[0] >> 15) & 1);
#if RTL8188EU_RX_RAW_DEBUG
    usb_printk("rtl8188eu: RX descriptor raw");
    for (i = 0; i < 24; i++)
        usb_printk(" %02x", data[i]);
    usb_printk("\n");
#endif
}

static unsigned int rtl8188eu_parse_rx_item(const uint8_t *data,
                                             unsigned int available)
{
    uint32_t dw0;
    unsigned int packet_length;
    unsigned int driver_info_length;
    unsigned int shift;
    unsigned int frame_offset;

    if (available < 24)
        return 0;
    dw0 = rtl8188eu_get_le32_early(data);
    packet_length = dw0 & 0x3fff;
    driver_info_length = ((dw0 >> 16) & 0x0f) * 8;
    shift = (dw0 >> 24) & 0x03;
    frame_offset = 24 + driver_info_length + shift;
#if RTL8188EU_RX_DESCRIPTOR_DEBUG
    usb_printk("rtl8188eu: RX item available=%u pkt_len=%u "
               "drvinfo=%u shift=%u frame_offset=%u\n",
               available, packet_length, driver_info_length, shift,
               frame_offset);
#endif
    /* USB aggregation transfers can end in zero-filled buffer space. This
     * marks the end of valid RX items, rather than a malformed packet. */
    if (!dw0)
        return 0;
    if ((dw0 & ((1u << 14) | (1u << 15))) ||
        frame_offset > available || !packet_length ||
        packet_length > available - frame_offset) {
#if RTL8188EU_RX_DESCRIPTOR_DEBUG
        usb_printk("rtl8188eu: RX item invalid\n");
#endif
        return 0;
    }
    if (rtl8188eu.wifi_state == RTL8188EU_WIFI_AUTHENTICATING &&
        rtl8188eu_auth_tx_started && packet_length >= 24 &&
        (data[frame_offset] & 0x0c) == 0 &&
        ((data[frame_offset] & 0xfc) == 0xb0 ||
         (data[frame_offset] & 0xfc) == 0x10 ||
         (data[frame_offset] & 0xfc) == 0x30 ||
         (data[frame_offset] & 0xfc) == 0xc0 ||
         (data[frame_offset] & 0xfc) == 0xa0)) {
        const uint8_t *frame = data + frame_offset;
        unsigned int head = packet_length < 12 ? packet_length : 12;
        usb_printk("rtl8188eu: auth RX frame len=%u offset=%u head:",
                   packet_length, frame_offset);
        for (unsigned int i = 0; i < head; i++)
            usb_printk(" %02x", frame[i]);
        usb_printk("\n");
    }
#if RTL8188EU_RX_DESCRIPTOR_DEBUG
    rtl8188eu_log_rx_descriptor(data, packet_length, driver_info_length,
                                shift, frame_offset);
#endif
#if RTL8188EU_RX_RAW_DEBUG
    usb_printk("rtl8188eu: RX frame head len=%u", packet_length);
    {
        unsigned int i;
        unsigned int head_length = packet_length < 32 ? packet_length : 32;
        for (i = 0; i < head_length; i++)
            usb_printk(" %02x", data[frame_offset + i]);
    }
    usb_printk("\n");
#endif
    {
        const uint8_t *frame = data + frame_offset;
        uint16_t frame_control = rtl8188eu_get_le16(frame);
        uint16_t frame_type = frame_control &
                              RTL8188EU_IEEE80211_FCTL_TYPE_MASK;

        if (rtl8188eu.wifi_state == RTL8188EU_WIFI_AUTHENTICATING &&
            rtl8188eu_auth_tx_started) {
            if (frame_type == RTL8188EU_IEEE80211_FTYPE_MGMT) {
                uint8_t subtype = frame[0] & 0xf0;
                int from_target = packet_length >= 24 &&
                    (rtl8188eu_bssid_equal(frame + 10,
                                           rtl8188eu_connect_bssid) ||
                     rtl8188eu_bssid_equal(frame + 16,
                                           rtl8188eu_connect_bssid));

                rtl8188eu_auth_rx_mgmt++;
                if (subtype == 0xb0)
                    rtl8188eu_auth_rx_auth++;
                if (from_target) {
                    uint8_t log_color = RTL8188EU_LOG_COLOR_OTHER_MGMT;

                    rtl8188eu_auth_rx_target++;
                    if (subtype == 0x80) {
                        rtl8188eu_auth_rx_target_beacon++;
                        log_color = RTL8188EU_LOG_COLOR_TARGET_BEACON;
                    } else if (subtype == 0x50) {
                        rtl8188eu_auth_rx_target_probe++;
                        log_color = RTL8188EU_LOG_COLOR_TARGET_PROBE;
                    } else if (subtype == 0xb0) {
                        log_color = RTL8188EU_LOG_COLOR_AUTH;
                    }
                    if (subtype == 0xb0 ||
                        rtl8188eu_auth_rx_target_logs++ < 8 ||
                        (rtl8188eu_auth_rx_target % 50u) == 0) {
                        rtl8188eu_log_set_color(log_color);
                        usb_printk("rtl8188eu: auth RX target fc=%02x%02x "
                                   "len=%u DA=%02x:%02x:%02x:%02x:%02x:%02x "
                                   "SA=%02x:%02x:%02x:%02x:%02x:%02x\n",
                                   frame[0], frame[1], packet_length,
                                   frame[4], frame[5], frame[6], frame[7],
                                   frame[8], frame[9], frame[10], frame[11],
                                   frame[12], frame[13], frame[14], frame[15]);
                        rtl8188eu_log_reset_color();
                    }
                }
            } else if (frame_type == 0x0004) {
                rtl8188eu_auth_rx_control++;
            } else if (frame_type == RTL8188EU_IEEE80211_FTYPE_DATA) {
                rtl8188eu_auth_rx_data++;
            } else {
                rtl8188eu_auth_rx_other++;
            }
        }

        /* Management parsing is selected by the frame header, not by a
         * b0 00 byte appearing anywhere in an IP/EAPOL payload. */
        if (frame_type == RTL8188EU_IEEE80211_FTYPE_MGMT)
            rtl8188eu_parse_management_ies(frame, packet_length);
        else if (frame_type == RTL8188EU_IEEE80211_FTYPE_DATA)
            rtl8188eu_deliver_data_frame(frame, packet_length);
    }
    return frame_offset + packet_length;
}

static void rtl8188eu_parse_rx_packet(const uint8_t *data,
                                      unsigned int actual_length)
{
    static unsigned int aggregate_dump_count;
    static unsigned int aggregate_summary_count;
    unsigned int offset = 0;
    unsigned int packets = 0;
    unsigned int invalid_items = 0;

    if (RTL8188EU_RX_RAW_DEBUG && aggregate_dump_count++ == 0)
        rtl8188eu_dump_bytes("RX raw", data, actual_length);
    while (offset + 24 <= actual_length) {
        unsigned int consumed = rtl8188eu_parse_rx_item(
            data + offset, actual_length - offset);

        if (consumed) {
            packets++;
            /* Linux rounds USB-aggregation items to four bytes.  The
             * 128-byte rule belongs to RXDMA aggregation, which is disabled
             * by rtl8188eu_configure_rx_aggregation(). */
            offset += (consumed + 3u) & ~3u;
            continue;
        }
        /* Descriptor boundaries are deterministic in USB aggregation mode.
         * Do not scan payload every 8 bytes looking for plausible DW0 data. */
        if (rtl8188eu_get_le32_early(data + offset) == 0)
            break;
        if (rtl8188eu.wifi_state == RTL8188EU_WIFI_AUTHENTICATING &&
            rtl8188eu_auth_tx_started && invalid_items < 8)
            usb_printk("rtl8188eu: auth RX invalid descriptor "
                       "usb_offset=%u dw0=%08x\n", offset,
                       rtl8188eu_get_le32_early(data + offset));
        invalid_items++;
        break;
    }
    if (RTL8188EU_RX_DESCRIPTOR_DEBUG && packets > 1 &&
        aggregate_summary_count++ < 8)
        usb_printk("rtl8188eu: RX aggregate len=%u packets=%u\n",
                   actual_length, packets);
}

static void rtl8188eu_log_xhci_transfer(struct usb_transfer_t *transfer,
                                        const char *stage)
{
    struct xhci_transfer_debug_t debug;

    if (!transfer || !transfer->dev ||
        transfer->dev->type != USB_TYPE_XHCI)
        return;
    if (xhci_get_transfer_debug(transfer, &debug) < 0) {
        usb_printk("rtl8188eu: xHCI TX %s snapshot unavailable\n",
                   stage ? stage : "");
        return;
    }
    usb_printk("rtl8188eu: xHCI TX %s slot=%u dci=%u ep_state=%u "
               "owner=%u enq=%u deq=%u pcs=%u ring=%08x "
               "epctx0=%08x hw_deq=%08x "
               "expect=%08x event=%08x cc=%u pending=%u ready=%u "
               "released=%u success=%u actual=%u usbsts=%08x\n",
               stage ? stage : "", debug.slot, debug.dci,
               debug.ep_state, debug.owner_matches, debug.enqueue,
               debug.dequeue, debug.cycle, debug.ring_phys,
               debug.ep_ctx0, debug.ep_dequeue,
               debug.expected_trb, debug.event_trb, debug.event_cc,
               debug.event_pending, debug.event_ready,
               transfer->trb_released, transfer->success,
               transfer->actual_length, debug.usb_sts);
}

static int rtl8188eu_bulk_out(struct rtl8188eu_device *dev,
                              struct usb_endpoint_t *endpoint,
                              uint8_t *buffer, size_t length)
{
    struct usb_transfer_t transfer;
    static unsigned int management_tx_log_count;
    int ret;

    if (!dev || !dev->usb || !endpoint || !buffer || !length)
        return -EINVAL;
    usb_setup_transfer(dev->usb, endpoint, &transfer, USB_TRANSFER_BULK);
    usb_out_transaction(&transfer, 0, buffer, length);
    rtl8188eu_log_xhci_transfer(&transfer, "setup");
    usb_schedule_transfer(&transfer);
    rtl8188eu_log_xhci_transfer(&transfer, "scheduled");
    ret = rtl8188eu_wait_transfer(&transfer, 50000);
    rtl8188eu_log_xhci_transfer(&transfer,
                                ret == 0 ? "completed" : "timeout");
    if (length <= RTL8188EU_TX_DESC_SIZE + 96 &&
        ((rtl8188eu.wifi_state == RTL8188EU_WIFI_AUTHENTICATING) ||
         management_tx_log_count++ < 4))
        usb_printk("rtl8188eu: bulk-out EP=%u requested=%u actual=%u "
                   "success=%s wait_ret=%d\n", endpoint->addr,
                   (unsigned int)length,
                   (unsigned int)transfer.actual_length,
                   transfer.success ? "yes" : "no", ret);
    usb_delete_transfer(&transfer);
    return ret;
}

/* Snapshot MAC-side TX state after a management packet leaves USB.  USB
 * completion only proves that the host controller accepted the transfer;
 * these registers show whether the RTL8188E TXDMA/MAC consumed it. */
static void rtl8188eu_log_tx_state(struct usb_dev_t *usb, const char *tag)
{
    uint32_t txdma = 0, hisr = 0;
    uint16_t empty = 0;
    uint8_t pause = 0;
    uint8_t rpt = 0;

    if (!usb)
        return;
    if (rtl8188eu_read32(usb, RTL8188EU_REG_TXDMA_STATUS, &txdma) < 0 ||
        rtl8188eu_read16(usb, RTL8188EU_REG_TXPKT_EMPTY, &empty) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_TXPAUSE, &pause) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_TX_RPT_CTRL, &rpt) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_HISR, &hisr) < 0)
        return;
    usb_printk("rtl8188eu: TX state %s TXDMA_STATUS=%08x "
               "TXPKT_EMPTY=%04x TXPAUSE=%02x TX_RPT_CTRL=%02x HISR=%08x\n",
               tag ? tag : "", txdma, empty, pause, rpt, hisr);
}

static int rtl8188eu_clear_txdma_status(struct usb_dev_t *usb)
{
    uint32_t pending;
    uint32_t readback;

    if (!usb ||
        rtl8188eu_read32(usb, RTL8188EU_REG_TXDMA_STATUS, &pending) < 0)
        return -EIO;
    if (pending &&
        rtl8188eu_write32(usb, RTL8188EU_REG_TXDMA_STATUS, pending) < 0)
        return -EIO;
    if (rtl8188eu_read32(usb, RTL8188EU_REG_TXDMA_STATUS, &readback) < 0)
        return -EIO;
    usb_printk("rtl8188eu: TXDMA status clear old=%08x readback=%08x\n",
               pending, readback);
    return readback == 0 ? 0 : -EIO;
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

static int rtl8188eu_arm_rx(void)
{
    int ret;

    if (rtl8188eu_rx_armed)
        return 0;
    ret = rtl8188eu_bulk_in_start(&rtl8188eu, &rtl8188eu_rx_transfer,
                                  rtl8188eu_rx_buffer,
                                  sizeof(rtl8188eu_rx_buffer));
    if (ret == 0) {
        static unsigned int rx_arm_log;
        rtl8188eu_rx_armed = 1;
        if (rx_arm_log++ == 0) {
            struct usb_transaction_t *trans =
                rtl8188eu_rx_transfer.trans_head;
            usb_printk("rtl8188eu: RX Bulk IN armed EP%d request=%u "
                       "transaction=%u trb=%x\n",
                       rtl8188eu.bulk_in->addr,
                       (unsigned int)sizeof(rtl8188eu_rx_buffer),
                       trans ? (unsigned int)trans->len : 0,
                       trans ? (uint32_t)trans->hc_trb_phys : 0);
        }
    }
    return ret;
}

/* Stop the long-lived scan/runtime Bulk-IN request before changing owners of
 * the RX path.  A scan timeout intentionally keeps the request armed while
 * changing channels, but it must not escape the scan command. */
static int rtl8188eu_stop_rx(void)
{
    unsigned int wait;

    if (!rtl8188eu_rx_armed)
        return 0;

    rtl8188eu_rx_poll_busy = 1;
    usb_printk("rtl8188eu: RX stop begin released=%u\n",
               rtl8188eu_rx_transfer.trb_released);
    if (!rtl8188eu_rx_transfer.trb_released) {
        if (rtl8188eu_rx_transfer.dev->type != USB_TYPE_XHCI ||
            xhci_cancel_transfer(&rtl8188eu_rx_transfer) < 0)
            usb_wait_transfer(&rtl8188eu_rx_transfer);
    }
    /* A cancelled xHCI TD may complete through a late event.  Pump the
     * dispatcher for a bounded interval before declaring ownership stuck;
     * otherwise MSC immediately reuses the controller while WiFi still owns
     * the RX TD. */
    for (wait = 0;
         wait < 200 && !rtl8188eu_rx_transfer.trb_released; wait++) {
        usb_poll_interrupts();
        usb_poll_hid_interrupts();
        tick_delay(1000);
    }
    if (!rtl8188eu_rx_transfer.trb_released) {
        usb_printk("rtl8188eu: RX stop failed; controller still owns TRB\n");
        rtl8188eu_rx_poll_busy = 0;
        return -EIO;
    }
    usb_delete_transfer(&rtl8188eu_rx_transfer);
    rtl8188eu_rx_armed = 0;
    rtl8188eu_rx_poll_busy = 0;
    usb_printk("rtl8188eu: RX stop done\n");
    return 0;
}

/* Quiesce a failed connection so queued frames cannot remain on the air.
 * TXPAUSE alone only blocks queue scheduling.  Disable the MAC TX/RX engines
 * too; scan explicitly enables them again before it submits a Probe Request. */
static void rtl8188eu_quiesce_radio(void)
{
    uint16_t cr_before;
    uint16_t cr_after;

    if (!rtl8188eu.usb)
        return;
    if (rtl8188eu_stop_rx() < 0) {
        usb_printk("rtl8188eu: radio quiesce could not stop RX\n");
        return;
    }
    if (rtl8188eu_write8(rtl8188eu.usb, RTL8188EU_REG_TXPAUSE, 0xff) < 0 ||
        rtl8188eu_read16(rtl8188eu.usb, RTL8188EU_REG_CR, &cr_before) < 0) {
        usb_printk("rtl8188eu: radio quiesce register access failed\n");
        return;
    }
    if (rtl8188eu_write16(rtl8188eu.usb, RTL8188EU_REG_CR,
                          cr_before & ~(RTL8188EU_CR_MACTXEN |
                                        RTL8188EU_CR_MACRXEN)) < 0 ||
        rtl8188eu_read16(rtl8188eu.usb, RTL8188EU_REG_CR, &cr_after) < 0) {
        usb_printk("rtl8188eu: radio quiesce MAC disable failed\n");
        return;
    }
    usb_printk("rtl8188eu: radio quiesced TXPAUSE=ff CR=%04x->%04x\n",
               cr_before, cr_after);
    if (cr_after & (RTL8188EU_CR_MACTXEN | RTL8188EU_CR_MACRXEN))
        usb_printk("rtl8188eu: warning: radio quiesce MAC bits still set\n");
}


void rtl8188eu_wifi_poll(void)
{
    uint16_t rx_length;
    int success;

    /* Scan owns its RX transfer synchronously through scan_poll_rx(). */
    if (rtl8188eu.wifi_state == RTL8188EU_WIFI_SCANNING)
        return;
    /* usb_poll_interrupts() and synchronous connect() can both reach this
     * function.  A transfer must have exactly one owner while it is being
     * completed, deleted, and re-armed. */
    if (rtl8188eu_rx_poll_busy)
        return;
    if (!rtl8188eu_rx_armed || !rtl8188eu_binding_valid() ||
        !rtl8188eu.firmware_loaded)
        return;
    rtl8188eu_rx_poll_busy = 1;
    if (!usb_poll_transfer(&rtl8188eu_rx_transfer)) {
        if (rtl8188eu.wifi_state == RTL8188EU_WIFI_AUTHENTICATING &&
            (++rtl8188eu_auth_poll_pending % 10000u) == 0)
            usb_printk("rtl8188eu: auth RX poll pending count=%u\n",
                       rtl8188eu_auth_poll_pending);
        rtl8188eu_rx_poll_busy = 0;
        return;
    }

    success = rtl8188eu_rx_transfer.success;
    rx_length = rtl8188eu_rx_transfer.actual_length;
    if (rtl8188eu.wifi_state == RTL8188EU_WIFI_AUTHENTICATING &&
        rtl8188eu_auth_tx_started) {
        rtl8188eu_auth_poll_completions++;
        if (rtl8188eu_auth_poll_completions <= 8 ||
            (rtl8188eu_auth_poll_completions % 100u) == 0)
            usb_printk("rtl8188eu: auth RX complete #%u success=%d actual=%u\n",
                       rtl8188eu_auth_poll_completions, success, rx_length);
        /* Full aggregate dumps are intentionally disabled here: Beacon
         * traffic is expected during authentication.  Meaningful response
         * subtypes are logged by rtl8188eu_parse_rx_item(). */
    }
    usb_delete_transfer(&rtl8188eu_rx_transfer);
    rtl8188eu_rx_armed = 0;
    if (success && rx_length) {
        rtl8188eu_rx_packets++;
        rtl8188eu_parse_rx_packet(rtl8188eu_rx_buffer, rx_length);
    }
    if (rtl8188eu_arm_rx() < 0)
        usb_printk("rtl8188eu: runtime RX re-arm failed\n");
    rtl8188eu_rx_poll_busy = 0;
}

/* Scan has its own RX owner.  Authentication, association, EAPOL, and data
 * traffic continue to use rtl8188eu_wifi_poll(); scan only feeds received
 * Beacon/Probe Response aggregates into the shared frame parser. */
static int rtl8188eu_scan_poll_rx(unsigned int *completions)
{
    uint16_t rx_length;
    int success;

    if (rtl8188eu.wifi_state != RTL8188EU_WIFI_SCANNING ||
        !rtl8188eu_rx_armed)
        return -EINVAL;
    if (rtl8188eu_rx_poll_busy)
        return 0;

    rtl8188eu_rx_poll_busy = 1;
    if (!usb_poll_transfer(&rtl8188eu_rx_transfer)) {
        rtl8188eu_rx_poll_busy = 0;
        return 0;
    }

    success = rtl8188eu_rx_transfer.success;
    rx_length = rtl8188eu_rx_transfer.actual_length;
    if (completions)
        (*completions)++;
    usb_delete_transfer(&rtl8188eu_rx_transfer);
    rtl8188eu_rx_armed = 0;

    if (success && rx_length) {
        rtl8188eu_rx_packets++;
        rtl8188eu_parse_rx_packet(rtl8188eu_rx_buffer, rx_length);
    }
    if (rtl8188eu_arm_rx() < 0) {
        rtl8188eu_rx_poll_busy = 0;
        usb_printk("rtl8188eu: scan RX re-arm failed\n");
        return -EIO;
    }
    rtl8188eu_rx_poll_busy = 0;
    return 1;
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

static size_t rtl8188eu_wrap_management_frame(uint8_t *packet,
                                               size_t capacity,
                                               const uint8_t *frame,
                                               size_t frame_len)
{
    uint32_t txdw0;
    uint32_t txdw1;
    uint32_t txdw4;
    unsigned int i;

    if (!packet || !frame || capacity < RTL8188EU_TX_DESC_SIZE + frame_len)
        return 0;
    A_memset(packet, 0, RTL8188EU_TX_DESC_SIZE + frame_len);
    for (i = 0; i < frame_len; i++)
        packet[RTL8188EU_TX_DESC_SIZE + i] = frame[i];
    txdw0 = (uint32_t)frame_len | (RTL8188EU_TX_DESC_SIZE << 16) |
            (1u << 26) | (1u << 27) | (1u << 31);
    /* Use the same descriptor fields as the probe request path, which is
     * already proven to transmit through this adapter and EP2.  Authentication
     * is unicast, so the only DW0 difference is that BMC remains clear. */
    txdw1 = RTL8188EU_TXDW1_QSEL_MGNT;
    txdw4 = RTL8188EU_DESC_RATE_1M | RTL8188EU_TXDW4_HWSEQ_EN |
            RTL8188EU_TXDW4_USE_RATE | RTL8188EU_TXDW4_DISDATAFB;
    rtl8188eu_put_le32(packet, txdw0);
    rtl8188eu_put_le32(packet + 4, txdw1);
    rtl8188eu_put_le32(packet + 16, txdw4);
    rtl8188eu_txdesc_checksum(packet);
    return RTL8188EU_TX_DESC_SIZE + frame_len;
}

static size_t rtl8188eu_build_auth_request(uint8_t *packet, size_t capacity,
                                           const uint8_t bssid[6])
{
    uint8_t frame[30];
    int frame_length = ieee80211_build_auth(frame, sizeof(frame),
                                             rtl8188eu.mac_addr, bssid);

    if (frame_length != (int)sizeof(frame))
        return 0;
    return rtl8188eu_wrap_management_frame(packet, capacity,
                                            frame, (size_t)frame_length);
}

static size_t rtl8188eu_build_assoc_request(uint8_t *packet, size_t capacity,
                                            const uint8_t bssid[6],
                                            const uint8_t *ssid,
                                            unsigned int ssid_length)
{
    static const uint8_t rsn_ie[] = {
        0x30, 0x14, 0x01, 0x00,
        0x00, 0x0f, 0xac, 0x04,
        0x01, 0x00, 0x00, 0x0f, 0xac, 0x04,
        0x01, 0x00, 0x00, 0x0f, 0xac, 0x02,
        0x00, 0x00
    };
    uint8_t frame[96];
    unsigned int i;
    unsigned int pos = 24;

    if (ssid_length > RTL8188EU_MAX_SSID_LEN)
        return 0;
    A_memset(frame, 0, sizeof(frame));
    frame[0] = 0x00;
    for (i = 0; i < 6; i++) {
        frame[4 + i] = bssid[i];
        frame[10 + i] = rtl8188eu.mac_addr[i];
        frame[16 + i] = bssid[i];
    }
    frame[pos++] = 0x31; frame[pos++] = 0x04; /* ESS/privacy/short preamble/slot. */
    frame[pos++] = 0x0a; frame[pos++] = 0x00; /* Listen interval. */
    frame[pos++] = 0x00; frame[pos++] = (uint8_t)ssid_length;
    for (i = 0; i < ssid_length; i++)
        frame[pos++] = ssid[i];
    frame[pos++] = 0x01; frame[pos++] = 0x04;
    frame[pos++] = 0x82; frame[pos++] = 0x84;
    frame[pos++] = 0x8b; frame[pos++] = 0x96;
    frame[pos++] = 0x32; frame[pos++] = 0x08;
    frame[pos++] = 0x0c; frame[pos++] = 0x12;
    frame[pos++] = 0x18; frame[pos++] = 0x24;
    frame[pos++] = 0x30; frame[pos++] = 0x48;
    frame[pos++] = 0x60; frame[pos++] = 0x6c;
    for (i = 0; i < sizeof(rsn_ie); i++)
        frame[pos++] = rsn_ie[i];
    return rtl8188eu_wrap_management_frame(packet, capacity, frame, pos);
}

static int rtl8188eu_set_station_mode(struct usb_dev_t *usb)
{
    uint8_t old_mode;
    uint8_t readback;

    /* RTL8188E MSR_INFRA is 0x02; 0x03 is AP mode. */
    if (rtl8188eu_read8(usb, RTL8188EU_REG_MSR, &old_mode) < 0 ||
        rtl8188eu_write8(usb, RTL8188EU_REG_MSR, 0x02) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_MSR, &readback) < 0)
        return -EIO;
    usb_printk("rtl8188eu: station mode MSR=%02x->%02x\n",
               old_mode, readback);
    return readback == 0x02 ? 0 : -EIO;
}

static int rtl8188eu_program_bssid(struct usb_dev_t *usb,
                                   const uint8_t bssid[6])
{
    uint8_t readback[6];

    if (rtl8188eu_write(usb, RTL8188EU_REG_BSSID, bssid, 6) < 0 ||
        rtl8188eu_read(usb, RTL8188EU_REG_BSSID, readback, 6) < 0)
        return -EIO;
    if (!rtl8188eu_bssid_equal(readback, bssid))
        return -EIO;
    usb_printk("rtl8188eu: BSSID=%02x:%02x:%02x:%02x:%02x:%02x\n",
               readback[0], readback[1], readback[2], readback[3],
               readback[4], readback[5]);
    return 0;
}

static void rtl8188eu_log_channel_state(struct usb_dev_t *usb,
                                        const char *stage)
{
    uint8_t bw = 0;
    uint8_t data_sc = 0;
    uint32_t rcr = 0;
    uint32_t hisr = 0;

    if (!usb || rtl8188eu_read8(usb, RTL8188EU_REG_BWOPMODE, &bw) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_DATA_SC, &data_sc) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_RCR, &rcr) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_HISR, &hisr) < 0)
        return;
    usb_printk("rtl8188eu: channel state %s BWOPMODE=%02x DATA_SC=%02x "
               "RCR=%08x HISR=%08x\n",
               stage ? stage : "", bw, data_sc, rcr, hisr);
}

static void rtl8188eu_print_scan_results(void)
{
    unsigned int result_index;

    rtl8188eu_log_set_color(RTL8188EU_LOG_COLOR_STATS);
    usb_printk("rtl8188eu: ===== WiFi scan SSID results (%u) =====\n",
               rtl8188eu.scan_result_count);
    if (!rtl8188eu.scan_result_count)
        usb_printk("rtl8188eu: SSID results: <none>\n");
    for (result_index = 0; result_index < rtl8188eu.scan_result_count;
         result_index++) {
        const struct rtl8188eu_scan_result *result =
            &rtl8188eu_scan_results[result_index];
        unsigned int byte;

        usb_printk("rtl8188eu: SSID[%u]=\"", result_index);
        for (byte = 0; byte < result->ssid_length; byte++) {
            uint8_t c = result->ssid[byte];
            usb_printk("%c", c >= 0x20 && c <= 0x7e ? c : '.');
        }
        usb_printk("\" channel=%u security=%s BSSID="
                   "%02x:%02x:%02x:%02x:%02x:%02x\n",
                   result->channel,
                   result->rsn ? "WPA2/RSN" :
                   (result->privacy ? "protected" : "open"),
                   result->bssid[0], result->bssid[1], result->bssid[2],
                   result->bssid[3], result->bssid[4], result->bssid[5]);
    }
    usb_printk("rtl8188eu: ===== end WiFi scan SSID results =====\n");
    rtl8188eu_log_reset_color();
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
    rcr = (rcr & ~(RTL8188EU_RCR_CBSSID_BCN |
                   RTL8188EU_RCR_CBSSID_DATA)) |
          RTL8188EU_RCR_AAP | RTL8188EU_RCR_APM | RTL8188EU_RCR_AM |
          RTL8188EU_RCR_AB | RTL8188EU_RCR_APP_PHYSTS;
    if (rtl8188eu_write32(usb, RTL8188EU_REG_RCR, rcr) < 0 ||
        rtl8188eu_write8(usb, RTL8188EU_REG_RX_DRVINFO_SZ, 4) < 0 ||
        rtl8188eu_write16(usb, RTL8188EU_REG_RXFLTMAP0, 0xffff) < 0 ||
        /* Keep control and management frames enabled; Authentication and
         * Association responses are management subtypes. */
        rtl8188eu_write16(usb, RTL8188EU_REG_RXFLTMAP1, 0xffff) < 0 ||
        rtl8188eu_write16(usb, RTL8188EU_REG_RXFLTMAP2, 0xffff) < 0)
        return -EIO;
    if (rtl8188eu_read32(usb, RTL8188EU_REG_RCR, &rcr_after) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_RX_DRVINFO_SZ, &drvinfo) < 0 ||
        rtl8188eu_read16(usb, RTL8188EU_REG_RXFLTMAP0, &map0) < 0 ||
        rtl8188eu_read16(usb, RTL8188EU_REG_RXFLTMAP1, &map1) < 0 ||
        rtl8188eu_read16(usb, RTL8188EU_REG_RXFLTMAP2, &map2) < 0)
        return -EIO;
    usb_printk("rtl8188eu: RX filter RCR=%08x DRVINFO=%02x MAP0=%04x MAP1=%04x MAP2=%04x\n",
               rcr_after, drvinfo, map0, map1, map2);
    if ((rcr_after & (RTL8188EU_RCR_CBSSID_BCN |
                      RTL8188EU_RCR_CBSSID_DATA)) ||
        (rcr_after & (RTL8188EU_RCR_AAP | RTL8188EU_RCR_APM |
                      RTL8188EU_RCR_AM | RTL8188EU_RCR_AB |
                      RTL8188EU_RCR_APP_PHYSTS)) !=
        (RTL8188EU_RCR_AAP | RTL8188EU_RCR_APM | RTL8188EU_RCR_AM |
         RTL8188EU_RCR_AB | RTL8188EU_RCR_APP_PHYSTS) ||
        drvinfo != 4 || map0 != 0xffff || map1 != 0xffff ||
        map2 != 0xffff)
        return -EIO;
    return 0;
}

static int rtl8188eu_configure_rx_aggregation(struct usb_dev_t *usb)
{
    uint8_t dma_old;
    uint8_t dma_new;
    uint8_t dma_readback = 0xff;
    uint8_t usb_old;
    uint8_t usb_new;
    uint8_t usb_readback = 0xff;

    if (rtl8188eu_read8(usb, RTL8188EU_REG_TRXDMA_CTRL, &dma_old) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_USB_SPECIAL_OPTION,
                        &usb_old) < 0)
        return -EIO;
    dma_new = dma_old & (uint8_t)~RTL8188EU_RXDMA_AGG_ENABLE;
    usb_new = usb_old | RTL8188EU_USB_AGG_ENABLE;
    if (rtl8188eu_write8(usb, RTL8188EU_REG_TRXDMA_CTRL, dma_new) < 0 ||
        rtl8188eu_write8(usb, RTL8188EU_REG_USB_SPECIAL_OPTION,
                         usb_new) < 0 ||
        rtl8188eu_write8(usb, RTL8188EU_REG_USB_AGG_TH, 8) < 0 ||
        rtl8188eu_write8(usb, RTL8188EU_REG_USB_AGG_TO, 6) < 0)
        return -EIO;
    if (rtl8188eu_read8(usb, RTL8188EU_REG_TRXDMA_CTRL,
                        &dma_readback) < 0 ||
        rtl8188eu_read8(usb, RTL8188EU_REG_USB_SPECIAL_OPTION,
                        &usb_readback) < 0 ||
        (dma_readback & RTL8188EU_RXDMA_AGG_ENABLE) ||
        !(usb_readback & RTL8188EU_USB_AGG_ENABLE)) {
        rtl8188eu_write8(usb, RTL8188EU_REG_TRXDMA_CTRL, dma_old);
        rtl8188eu_write8(usb, RTL8188EU_REG_USB_SPECIAL_OPTION, usb_old);
        usb_printk("rtl8188eu: RX aggregation setup failed; "
                   "TRXDMA=%02x USB_SPECIAL=%02x restored\n",
                   dma_readback, usb_readback);
        return -EIO;
    }
    usb_printk("rtl8188eu: RX aggregation USB mode TRXDMA=%02x->%02x "
               "USB_SPECIAL=%02x->%02x threshold=8 timeout=6\n",
               dma_old, dma_readback, usb_old, usb_readback);
    return 0;
}

static int rtl8188eu_enable_mac_tx_rx(struct usb_dev_t *usb)
{
    uint16_t cr_before;
    uint16_t cr_after;
    uint8_t cr_low;

    /* Linux enables these two blocks only after the RxFF boundary is set. */
    if (rtl8188eu_read16(usb, RTL8188EU_REG_CR, &cr_before) < 0)
        return -EIO;
    cr_low = (uint8_t)cr_before |
             RTL8188EU_CR_MACTXEN | RTL8188EU_CR_MACRXEN;
    if (rtl8188eu_write8(usb, RTL8188EU_REG_CR, cr_low) < 0 ||
        rtl8188eu_read16(usb, RTL8188EU_REG_CR, &cr_after) < 0)
        return -EIO;
    usb_printk("rtl8188eu: MAC TX/RX enabled CR=%04x->%04x\n",
               cr_before, cr_after);
    if ((cr_after & (RTL8188EU_CR_MACTXEN | RTL8188EU_CR_MACRXEN)) !=
        (RTL8188EU_CR_MACTXEN | RTL8188EU_CR_MACRXEN))
        return -EIO;
    return 0;
}

static int rtl8188eu_initialize_network_type(struct usb_dev_t *usb)
{
    uint32_t cr_before;
    uint32_t cr_after;
    uint32_t cr_new;

    if (rtl8188eu_read32(usb, RTL8188EU_REG_CR, &cr_before) < 0)
        return -EIO;
    cr_new = (cr_before & ~RTL8188EU_CR_NETTYPE_MASK) |
             RTL8188EU_CR_NETTYPE_STA;
    if (rtl8188eu_write32(usb, RTL8188EU_REG_CR, cr_new) < 0 ||
        rtl8188eu_read32(usb, RTL8188EU_REG_CR, &cr_after) < 0)
        return -EIO;
    usb_printk("rtl8188eu: network type STA/INFRA CR=%08x->%08x\n",
               cr_before, cr_after);
    if ((cr_after & RTL8188EU_CR_NETTYPE_MASK) !=
        RTL8188EU_CR_NETTYPE_STA)
        return -EIO;
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
    if ((old_value & RTL8188EU_RF_CHANNEL_MASK) == channel) {
        usb_printk("rtl8188eu: channel already set RF_CHNLBW=%05x "
                   "channel=%u\n", old_value, channel);
        return 0;
    }
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
    uint32_t cck_value;
    uint32_t new_value;
    uint32_t readback;
    uint32_t required = RTL8188EU_BB_CCK_ENABLE |
                        RTL8188EU_BB_OFDM_ENABLE;

    if (rtl8188eu_read32(usb, RTL8188EU_REG_FPGA0_RFMOD,
                         &old_value) < 0)
        return -EIO;
    cck_value = old_value | RTL8188EU_BB_CCK_ENABLE;
    if (rtl8188eu_write32(usb, RTL8188EU_REG_FPGA0_RFMOD,
                          cck_value) < 0)
        return -EIO;
    new_value = cck_value | RTL8188EU_BB_OFDM_ENABLE;
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
    usb_printk("rtl8188eu: BB modulation FPGA0=%08x->%08x->%08x "
               "readback=%08x CCK=on OFDM=on\n",
               old_value, cck_value, new_value, readback);
    return 0;
}

static int rtl8188eu_read8(struct usb_dev_t *usb, uint16_t reg, uint8_t *value)
{
    if (!usb || !value)
        return -EINVAL;
    return rtl8188eu_read(usb, reg, value, 1);
}

static int rtl8188eu_read16(struct usb_dev_t *usb, uint16_t reg,
                            uint16_t *value)
{
    uint8_t data[2];
    int ret;

    if (!value)
        return -EINVAL;
    ret = rtl8188eu_read(usb, reg, data, sizeof(data));
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
    unsigned int attempt;

    if (!usb || !data || !length)
        return -EINVAL;
    ok = 0;
    for (attempt = 0; attempt < 3 && !ok; attempt++) {
        ok = usb_ctrl_in(usb, data, RTL8188EU_USB_REQ_READ,
                         RTL8188EU_USB_REQ_REGS, (uint8_t)(reg >> 8),
                         (uint8_t)reg, 0, length);
        if (!ok && attempt + 1 < 3)
            tick_delay(1000);
    }
    if (ok && attempt > 1)
        usb_printk("rtl8188eu: EP0 read recovered reg=%04x attempts=%u\n",
                   reg, attempt);
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

static int rtl8188eu_initialize_mac(struct usb_dev_t *usb)
{
    uint32_t rf_chnlbw;

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
    if (rtl8188eu_configure_rx_aggregation(usb) < 0)
        usb_printk("rtl8188eu: RX aggregation state not ready\n");
    rtl8188eu_log_bb_rf_state(usb);
    if (rtl8188eu_enable_bb_rf(usb) < 0)
        usb_printk("rtl8188eu: BB/RF not ready\n");
    else if (rtl8188eu_load_phy_tables(usb) < 0)
        usb_printk("rtl8188eu: PHY/AGC/RF table load failed\n");
    if (rtl8188eu_initialize_network_type(usb) < 0)
        usb_printk("rtl8188eu: network type setup failed\n");
    if (rtl8188eu_enable_mac_tx_rx(usb) < 0)
        usb_printk("rtl8188eu: MAC TX/RX enable failed\n");
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
    usb_printk("rtl8188eu: MAC/BB/RF ready TX=EP%d secondary=EP%d%s; "
               "scan deferred\n",
               rtl8188eu.bulk_out->addr,
               rtl8188eu.bulk_out2 ? rtl8188eu.bulk_out2->addr :
                                     rtl8188eu.bulk_out->addr,
               rtl8188eu.bulk_out2 ? "" : " (shared)");

    /* The network stack must see a fully initialized MAC before DHCP or
     * runtime RX delivery can use the device.  Keep this at the common
     * firmware-ready exit so both preloaded and newly downloaded firmware
     * follow the same registration path. */
    return rtl8188eu_register_netdev();
}


// int rtl8188eu_wifi_scan_bak(void)
// {
//     uint8_t tx[RTL8188EU_TX_DESC_SIZE + 34];
//     size_t tx_length;
//     unsigned int probes = 0;
//     unsigned int rx_packets_before;
//     uint8_t channel;

//     if (!rtl8188eu.usb || !rtl8188eu.firmware_loaded)
//         return -ENODEV;
//     if (rtl8188eu.wifi_state == RTL8188EU_WIFI_SCANNING)
//         return -EBUSY;
//     /* Reclaim a request left by a previous connect/runtime RX poll. */
//     rtl8188eu_stop_rx();
//     /* Restore radio queues after a previous failed connection. */
//     rtl8188eu_write8(rtl8188eu.usb, RTL8188EU_REG_TXPAUSE, 0x00);
//     if (rtl8188eu_enable_mac_tx_rx(rtl8188eu.usb) < 0)
//         return -EIO;
//     /* A timed-out Bulk-IN can leave stale completion/status bits behind.
//      * Start every scan with a fresh buffer and a clean interrupt state. */
//     A_memset(rtl8188eu_rx_buffer, 0, sizeof(rtl8188eu_rx_buffer));
//     if (rtl8188eu_clear_interrupt_status(rtl8188eu.usb) < 0)
//         usb_printk("rtl8188eu: scan start IRQ clear failed\n");
//     rtl8188eu_clear_connection_context();
//     tx_length = rtl8188eu_build_probe_request(tx, sizeof(tx),
//                                               rtl8188eu.mac_addr);
//     if (!tx_length)
//         return -EIO;

//     rtl8188eu.wifi_state = RTL8188EU_WIFI_SCANNING;
//     rx_packets_before = rtl8188eu_rx_packets;
//     rtl8188eu.last_error = 0;
//     rtl8188eu.scan_result_count = 0;
//     A_memset(rtl8188eu_scan_results, 0, sizeof(rtl8188eu_scan_results));
//     usb_printk("rtl8188eu: scan start channels=%u-%u dwell=%u us\n",
//                RTL8188EU_SAMPLE_SCAN_ONLY ? RTL8188EU_SAMPLE_CHANNEL :
//                                              RTL8188EU_SCAN_FIRST_CHANNEL,
//                RTL8188EU_SAMPLE_SCAN_ONLY ? RTL8188EU_SAMPLE_CHANNEL :
//                                              RTL8188EU_SCAN_LAST_CHANNEL,
//                RTL8188EU_SCAN_DWELL_POLLS * RTL8188EU_SCAN_POLL_DELAY_US);
//     if (rtl8188eu_arm_rx() < 0) {
//         rtl8188eu.wifi_state = RTL8188EU_WIFI_FAILED;
//         rtl8188eu.last_error = -EIO;
//         return -EIO;
//     }

//     for (channel = RTL8188EU_SAMPLE_SCAN_ONLY ?
//              RTL8188EU_SAMPLE_CHANNEL : RTL8188EU_SCAN_FIRST_CHANNEL;
//          channel <= (RTL8188EU_SAMPLE_SCAN_ONLY ?
//                      RTL8188EU_SAMPLE_CHANNEL : RTL8188EU_SCAN_LAST_CHANNEL);
//          channel++) {
//         uint16_t rx_length = 0;
//         int ret;

//         ret = 0;
// #if !RTL8188EU_SAMPLE_SCAN_ONLY
//         ret = 0;
// #if !RTL8188EU_SAMPLE_SCAN_ONLY
//         ret = rtl8188eu_set_channel(rtl8188eu.usb, channel);
// #endif
// #endif
//         if (ret < 0) {
//             rtl8188eu_stop_rx();
//             A_memset(rtl8188eu_rx_buffer, 0, sizeof(rtl8188eu_rx_buffer));
//             usb_printk("rtl8188eu: scan channel=%u tune failed=%d\n",
//                        channel, ret);
//             /* A result collected while channel programming failed is not
//              * trustworthy.  Do not complete the scan successfully or let
//              * connect() consume a stale/old AP entry. */
//             rtl8188eu.scan_result_count = 0;
//             rtl8188eu.wifi_state = RTL8188EU_WIFI_FAILED;
//             rtl8188eu.last_error = ret;
//             usb_printk("rtl8188eu: scan aborted error=%d\n", ret);
//             return ret;
//         }
//         tick_delay(2000);
//         rtl8188eu_clear_interrupt_status(rtl8188eu.usb);
//         ret = rtl8188eu_bulk_out(&rtl8188eu, rtl8188eu.bulk_out,
//                                  tx, tx_length);
//         if (ret == 0)
//             probes++;
//         usb_printk("rtl8188eu: scan channel=%u probe=%s rx_arm=%s\n",
//                    channel, ret == 0 ? "sent" : "failed",
//                    rtl8188eu_rx_armed ? "yes" : "no");
//         ret = rtl8188eu_bulk_in_finish(&rtl8188eu_rx_transfer, &rx_length);
//         if (ret != -ETIMEDOUT)
//             rtl8188eu_rx_armed = 0;
//         if (ret == 0) {
//             if (rx_length) {
//                 rtl8188eu_rx_packets++;
//                 rtl8188eu_parse_rx_packet(rtl8188eu_rx_buffer, rx_length);
//             }
//             if (rtl8188eu_arm_rx() < 0) {
//                 rtl8188eu_stop_rx();
//                 rtl8188eu.wifi_state = RTL8188EU_WIFI_FAILED;
//                 rtl8188eu.last_error = -EIO;
//                 return -EIO;
//             }
//         } else if (ret != -ETIMEDOUT) {
//             rtl8188eu_rx_armed = 0;
//             if (rtl8188eu_arm_rx() < 0)
//                 usb_printk("rtl8188eu: RX re-arm failed after channel=%u\n",
//                            channel);
//         }
// #if RTL8188EU_SAMPLE_SCAN_ONLY
//         if (rtl8188eu.scan_result_count > 0)
//             break;
// #endif
//     }
//     /* Do not leave a scan-owned transfer armed after this synchronous command.
//      * The next scan or connect operation will create a fresh request. */
//     rtl8188eu_stop_rx();
//     rtl8188eu.wifi_state = RTL8188EU_WIFI_READY;
//     usb_printk("rtl8188eu: scan complete probes=%u rx_packets=%u networks=%u\n",
//                probes, rtl8188eu_rx_packets - rx_packets_before,
//                rtl8188eu.scan_result_count);
//     rtl8188eu_print_scan_results();
//     rtl8188eu_log_rx_runtime_state(rtl8188eu.usb, "scan-complete");
//     return (int)rtl8188eu.scan_result_count;
// } 

// int rtl8188eu_wifi_connect_bak(const char *ssid, const char *password)
// {
//     unsigned int ssid_length = 0, password_length = 0, i, j;
//     int rescanned = 0;

//     usb_printk("rtl8188eu: connect precheck usb=%p firmware=%u state=%u networks=%u\n",
//                rtl8188eu.usb, rtl8188eu.firmware_loaded,
//                rtl8188eu.wifi_state, rtl8188eu.scan_result_count);
//     if (!rtl8188eu.usb || !rtl8188eu.firmware_loaded) {
//         usb_printk("rtl8188eu: connect rejected adapter/firmware not ready\n");
//         return -ENODEV;
//     }
//     if (!ssid || !password)
//         return -EINVAL;
//     while (ssid[ssid_length] && ssid_length <= RTL8188EU_MAX_SSID_LEN)
//         ssid_length++;
//     while (password[password_length] && password_length < 64)
//         password_length++;
//     if (!ssid_length || ssid_length > RTL8188EU_MAX_SSID_LEN ||
//         password_length < 8 || password_length > 63)
//         return -EINVAL;

// retry_results:
//     for (i = 0; i < rtl8188eu.scan_result_count; i++) {
//         if (rtl8188eu_scan_results[i].ssid_length != ssid_length)
//             continue;
//         for (j = 0; j < ssid_length; j++)
//             if (rtl8188eu_scan_results[i].ssid[j] != (uint8_t)ssid[j])
//                 break;
//         if (j != ssid_length)
//             continue;
//         {
//             uint8_t tx[RTL8188EU_TX_DESC_SIZE + 96];
//             size_t tx_length;
//             unsigned int poll;
//             unsigned int attempt;

//             rtl8188eu.selected_result = (int)i;
//             for (j = 0; j < 6; j++)
//                 rtl8188eu_connect_bssid[j] =
//                     rtl8188eu_scan_results[i].bssid[j];
//             rtl8188eu_clear_connection_context();
//             for (j = 0; j < 6; j++)
//                 rtl8188eu.connected_bssid[j] =
//                     rtl8188eu_connect_bssid[j];
//             rtl8188eu.current_channel = rtl8188eu_scan_results[i].channel;
//             usb_printk("rtl8188eu: connect select SSID=");
//             for (j = 0; j < ssid_length; j++)
//                 usb_printk("%c", ssid[j] >= 0x20 && ssid[j] <= 0x7e ?
//                            ssid[j] : '.');
//             usb_printk(" BSSID=%02x:%02x:%02x:%02x:%02x:%02x channel=%u\n",
//                        rtl8188eu_connect_bssid[0],
//                        rtl8188eu_connect_bssid[1],
//                        rtl8188eu_connect_bssid[2],
//                        rtl8188eu_connect_bssid[3],
//                        rtl8188eu_connect_bssid[4],
//                        rtl8188eu_connect_bssid[5],
//                        rtl8188eu_scan_results[i].channel);
//             if (!RTL8188EU_SAMPLE_SCAN_ONLY ||
//                 rtl8188eu_scan_results[i].channel != RTL8188EU_SAMPLE_CHANNEL) {
//                 if (rtl8188eu_set_channel(
//                         rtl8188eu.usb,
//                         rtl8188eu_scan_results[i].channel) < 0)
//                     return -EIO;
//             } else {
//                 usb_printk("rtl8188eu: sample mode keeps channel=%u; "
//                            "skip RF retune\n",
//                            RTL8188EU_SAMPLE_CHANNEL);
//             }
//             /* Allow the RF synthesizer/BB to settle before management TX. */
//             tick_delay(10000);
//             if (rtl8188eu_set_station_mode(rtl8188eu.usb) < 0)
//                 return -EIO;
//             if (rtl8188eu_program_bssid(rtl8188eu.usb,
//                                         rtl8188eu_connect_bssid) < 0) {
//                 usb_printk("rtl8188eu: BSSID programming failed\n");
//                 return -EIO;
//             }

//             rtl8188eu_auth_status = -1;

//             rtl8188eu_auth_poll_pending = 0;
//             rtl8188eu_auth_poll_completions = 0;
//             rtl8188eu_auth_tx_started = 0;
//             rtl8188eu.wifi_state = RTL8188EU_WIFI_AUTHENTICATING;
//             /* The scan path can leave a completed/partially completed Bulk-IN
//              * request and stale RX status behind.  Recreate the request at
//              * the authentication boundary so old scan data cannot be treated
//              * as an authentication response. */
//             rtl8188eu_stop_rx();
//             A_memset(rtl8188eu_rx_buffer, 0, sizeof(rtl8188eu_rx_buffer));
//             if (rtl8188eu_clear_interrupt_status(rtl8188eu.usb) < 0) {
//                 rtl8188eu.wifi_state = RTL8188EU_WIFI_FAILED;
//                 rtl8188eu.last_error = -EIO;
//                 return -EIO;
//             }
//             if (rtl8188eu_arm_rx() < 0) {
//                 rtl8188eu.wifi_state = RTL8188EU_WIFI_FAILED;
//                 rtl8188eu.last_error = -EIO;
//                 return -EIO;
//             }
//             tx_length = rtl8188eu_build_auth_request(
//                 tx, sizeof(tx), rtl8188eu_connect_bssid);
//             if (!tx_length)
//                 return -EIO;
//             rtl8188eu_log_tx_state(rtl8188eu.usb, "before-auth-usb");
//             for (attempt = 0; attempt < 1 && rtl8188eu_auth_status < 0;
//                  attempt++) {
//                 if (rtl8188eu_bulk_out(&rtl8188eu, rtl8188eu.bulk_out,
//                                        tx, tx_length) < 0)
//                     return -EIO;
//                 rtl8188eu_auth_tx_started = 1;
//                 if (attempt == 0)
//                     rtl8188eu_log_tx_state(rtl8188eu.usb, "after-auth-usb");
//                 if (attempt == 0)
//                     rtl8188eu_log_tx_descriptor(tx);
//                 if (attempt == 0)
//                     rtl8188eu_dump_bytes("AUTH TX descriptor/frame",
//                                          tx, tx_length);
//                 if (attempt == 0)
//                     usb_printk("rtl8188eu: Open Authentication request sent\n");
//                 else
//                     usb_printk("rtl8188eu: authentication retry=%u\n",
//                                attempt + 1);
//                 /* Let the MAC/TX queue retire the previous management frame
//                  * before a retry; immediate reuse can leave EP2 busy. */
//                 tick_delay(50000);
//                 for (poll = 0; poll < 100000 && rtl8188eu_auth_status < 0;
//                      poll++) {
//                     usb_poll_interrupts();
//                     tick_delay(100);
//                 }
//             }
//             if (rtl8188eu_auth_status != 0) {
//                 rtl8188eu_quiesce_radio();
//                 rtl8188eu.wifi_state = RTL8188EU_WIFI_FAILED;
//                 rtl8188eu.last_error = rtl8188eu_auth_status < 0 ?
//                                        -ETIMEDOUT : -EIO;
//                 usb_printk("rtl8188eu: authentication failed status=%d\n",
//                            rtl8188eu_auth_status);
//                 return rtl8188eu.last_error;
//             }

//             rtl8188eu_assoc_status = -1;
//             rtl8188eu_assoc_aid = 0;
//             rtl8188eu.wifi_state = RTL8188EU_WIFI_ASSOCIATING;
//             tx_length = rtl8188eu_build_assoc_request(
//                 tx, sizeof(tx), rtl8188eu_connect_bssid,
//                 (const uint8_t *)ssid, ssid_length);
//             if (!tx_length || rtl8188eu_bulk_out(
//                     &rtl8188eu, rtl8188eu.bulk_out,
//                     tx, tx_length) < 0)
//                 return -EIO;
//             usb_printk("rtl8188eu: WPA2 Association request sent\n");
//             for (poll = 0; poll < 5000 && rtl8188eu_assoc_status < 0;
//                  poll++) {
//                 usb_poll_interrupts();
//                 tick_delay(1000);
//             }
//             if (rtl8188eu_assoc_status != 0) {
//                 rtl8188eu.wifi_state = RTL8188EU_WIFI_FAILED;
//                 rtl8188eu.last_error = rtl8188eu_assoc_status < 0 ?
//                                        -ETIMEDOUT : -EIO;
//                 usb_printk("rtl8188eu: association failed status=%d\n",
//                            rtl8188eu_assoc_status);
//                 return rtl8188eu.last_error;
//             }

//             rtl8188eu.wifi_state = RTL8188EU_WIFI_WPA2_HANDSHAKE;
//             rtl8188eu.associated = 1;
//             rtl8188eu.last_error = -ENOTSUP;
//             usb_printk("rtl8188eu: associated AID=%u; waiting for WPA2 "
//                        "EAPOL message 1\n", rtl8188eu_assoc_aid);
//             return -ENOTSUP;
//         }
//     }
//     if (!rescanned) {
//         int scan_result;

//         rescanned = 1;
//         scan_result = rtl8188eu_wifi_scan();
//         if (scan_result < 0)
//             return scan_result;
//         goto retry_results;
//     }
//     rtl8188eu.wifi_state = RTL8188EU_WIFI_FAILED;
//     rtl8188eu.last_error = -ENOENT;
//     return -ENOENT;
// }


//#include <errno.h>

int rtl8188eu_wifi_scan(void)
{
    extern void usb_poll_hid_interrupts(void);
    uint8_t tx[RTL8188EU_TX_DESC_SIZE + 34];
    size_t tx_length;
    unsigned int probes = 0;
    unsigned int rx_packets_before;
    uint8_t channel;
    uint8_t start_chan, end_chan;

    if (!rtl8188eu.usb || !rtl8188eu.firmware_loaded)
        return -ENODEV;
    if (rtl8188eu.wifi_state == RTL8188EU_WIFI_SCANNING ||
        rtl8188eu.wifi_state == RTL8188EU_WIFI_AUTHENTICATING ||
        rtl8188eu.wifi_state == RTL8188EU_WIFI_ASSOCIATING ||
        rtl8188eu.wifi_state == RTL8188EU_WIFI_WPA2_HANDSHAKE ||
        rtl8188eu.wifi_state == RTL8188EU_WIFI_CONNECTED)
        return -EBUSY;

    /* A new scan attempt invalidates the previous snapshot immediately.  If
     * hardware setup fails, connect must not fall back to stale SSIDs. */
    rtl8188eu_scan_hash_valid = 0;
    rtl8188eu.scan_result_count = 0;
    A_memset(rtl8188eu_scan_results, 0, sizeof(rtl8188eu_scan_results));
    if (rtl8188eu_stop_rx() < 0)
        return -EIO;
    rtl8188eu_write8(rtl8188eu.usb, RTL8188EU_REG_TXPAUSE, 0x00);
    if (rtl8188eu_enable_mac_tx_rx(rtl8188eu.usb) < 0)
        return -EIO;

    A_memset(rtl8188eu_rx_buffer, 0, sizeof(rtl8188eu_rx_buffer));
    if (rtl8188eu_clear_interrupt_status(rtl8188eu.usb) < 0)
        usb_printk("rtl8188eu: scan start IRQ clear failed\n");

    rtl8188eu_clear_connection_context();
    tx_length = rtl8188eu_build_probe_request(tx, sizeof(tx), rtl8188eu.mac_addr);
    if (!tx_length)
        return -EIO;

    rtl8188eu.wifi_state = RTL8188EU_WIFI_SCANNING;
    rx_packets_before = rtl8188eu_rx_packets;
    rtl8188eu.last_error = 0;
    rtl8188eu_scan_channel = 0;
    A_memset(rtl8188eu_scan_results, 0, sizeof(rtl8188eu_scan_results));

    start_chan = RTL8188EU_SCAN_FIRST_CHANNEL;
    end_chan = RTL8188EU_SCAN_LAST_CHANNEL;

    usb_printk("rtl8188eu: scan start channels=%u-%u dwell=%u ticks "
               "poll_limit=%u\n", start_chan, end_chan,
               RTL8188EU_SCAN_DWELL_TICKS, RTL8188EU_SCAN_MAX_POLLS);

    if (rtl8188eu_arm_rx() < 0) {
        rtl8188eu_quiesce_radio();
        rtl8188eu.wifi_state = RTL8188EU_WIFI_FAILED;
        rtl8188eu.last_error = -EIO;
        return -EIO;
    }

    for (channel = start_chan; channel <= end_chan; channel++) {
        unsigned int poll;
        uint32_t channel_start_tick;
        unsigned int completions = 0;
        unsigned int channel_rx_before = rtl8188eu_rx_packets;
        unsigned int networks_before = rtl8188eu.scan_result_count;
        int ret;

        usb_printk("rtl8188eu: scan channel=%u begin\n", channel);
        ret = 0;
        ret = rtl8188eu_set_channel(rtl8188eu.usb, channel);
        if (ret < 0) {
            /* A single EP0 channel-tune timeout must not erase results from
             * earlier channels or abort the whole scan.  EP0 recovery and the
             * next channel provide a bounded in-command retry. */
            usb_printk("rtl8188eu: scan channel=%u tune failed=%d; continuing\n",
                       channel, ret);
            continue;
        }
        rtl8188eu_scan_channel = channel;

        tick_delay(2000);
        usb_poll_hid_interrupts();
        rtl8188eu_clear_interrupt_status(rtl8188eu.usb);

        ret = rtl8188eu_bulk_out(&rtl8188eu, rtl8188eu.bulk_out, tx, tx_length);
        if (ret == 0)
            probes++;

        usb_printk("rtl8188eu: scan channel=%u probe=%s rx_arm=%s\n",
                   channel, ret == 0 ? "sent" : "failed",
                   rtl8188eu_rx_armed ? "yes" : "no");
        if (ret < 0) {
            /* A failed Probe TX leaves the shared WiFi Bulk-OUT endpoint in
             * timeout recovery.  Do not continue scanning or publish this
             * partial result set for a later connect attempt. */
            usb_printk("rtl8188eu: scan abort channel=%u probe TX error=%d\n",
                       channel, ret);
            rtl8188eu_scan_hash_valid = 0;
            rtl8188eu.scan_result_count = 0;
            if (rtl8188eu_stop_rx() < 0)
                usb_printk("rtl8188eu: scan abort RX stop failed\n");
            rtl8188eu_scan_channel = 0;
            rtl8188eu.wifi_state = RTL8188EU_WIFI_FAILED;
            rtl8188eu.last_error = ret;
            return ret;
        }

        channel_start_tick = ticks;
        for (poll = 0;
             poll < RTL8188EU_SCAN_MAX_POLLS &&
             (uint32_t)(ticks - channel_start_tick) <
                 RTL8188EU_SCAN_DWELL_TICKS;
             poll++) {
            ret = rtl8188eu_scan_poll_rx(&completions);
            if (ret < 0) {
                rtl8188eu_quiesce_radio();
                rtl8188eu.wifi_state = RTL8188EU_WIFI_FAILED;
                rtl8188eu.last_error = ret;
                return ret;
            }
            usb_poll_hid_interrupts();
            tick_delay(RTL8188EU_SCAN_POLL_DELAY_US);
        }
        if ((uint32_t)(ticks - channel_start_tick) <
            RTL8188EU_SCAN_DWELL_TICKS)
            usb_printk("rtl8188eu: scan channel=%u poll watchdog "
                       "elapsed_ticks=%u\n", channel,
                       (uint32_t)(ticks - channel_start_tick));
        usb_printk("rtl8188eu: scan channel=%u done completions=%u "
                   "polls=%u ticks=%u rx_packets=%u new_networks=%u "
                   "total_networks=%u\n",
                   channel, completions, poll,
                   (uint32_t)(ticks - channel_start_tick),
                   rtl8188eu_rx_packets - channel_rx_before,
                   rtl8188eu.scan_result_count - networks_before,
                   rtl8188eu.scan_result_count);

        if (channel == 255) break; /* é—ƒå‰î„?uint8_t å¯°î†å¹†å©§ãˆ åš­ */
    }

    if (rtl8188eu_stop_rx() < 0) {
        rtl8188eu.wifi_state = RTL8188EU_WIFI_FAILED;
        rtl8188eu.last_error = -EIO;
        return -EIO;
    }
    rtl8188eu_scan_channel = 0;
    rtl8188eu_build_scan_hash();
    rtl8188eu.wifi_state = RTL8188EU_WIFI_READY;
    usb_printk("rtl8188eu: scan complete probes=%u rx_packets=%u networks=%u\n",
               probes, rtl8188eu_rx_packets - rx_packets_before,
               rtl8188eu.scan_result_count);
    rtl8188eu_print_scan_results();
    rtl8188eu_log_rx_runtime_state(rtl8188eu.usb, "scan-complete");

    return (int)rtl8188eu.scan_result_count;
}

int rtl8188eu_wifi_connect(const char *ssid, const char *password)
{
    unsigned int ssid_length = 0, password_length = 0, j;
    int scan_index;

    if (!rtl8188eu.usb || !rtl8188eu.firmware_loaded) {
        usb_printk("rtl8188eu: connect rejected adapter/firmware not ready\n");
        return -ENODEV;
    }
    if (!ssid || !password)
        return -EINVAL;
    if (rtl8188eu.wifi_state == RTL8188EU_WIFI_SCANNING ||
        rtl8188eu.wifi_state == RTL8188EU_WIFI_AUTHENTICATING ||
        rtl8188eu.wifi_state == RTL8188EU_WIFI_ASSOCIATING ||
        rtl8188eu.wifi_state == RTL8188EU_WIFI_WPA2_HANDSHAKE)
        return -EBUSY;

    /* ç€¹å¤Šåç’ï¼„ç•»é—€åž®å®³é”›å²„æ§»å§?NULL ç¼æ’³ç†¬ç“’å©„æ™« */
    while (ssid_length < RTL8188EU_MAX_SSID_LEN && ssid[ssid_length] != '\0')
        ssid_length++;
    if (ssid[ssid_length] != '\0' && ssid_length == RTL8188EU_MAX_SSID_LEN) {
        /* SSID ç“’å‘°ç¹ƒéˆâ‚¬æ¾¶Ñ‡æš±æ´ï¹‚æªºé?*/
        return -EINVAL; 
    }

    while (password_length < 64 && password[password_length] != '\0')
        password_length++;

    if (ssid_length == 0 || password_length < 8 || password_length > 63)
        return -EINVAL;

    if (rtl8188eu.wifi_state == RTL8188EU_WIFI_CONNECTED) {
        if (rtl8188eu.connected_ssid_length == ssid_length) {
            for (j = 0; j < ssid_length; j++)
                if (rtl8188eu.connected_ssid[j] != (uint8_t)ssid[j])
                    break;
            if (j == ssid_length) {
                usb_printk("rtl8188eu: connect already satisfied for SSID\n");
                return 0;
            }
        }
        usb_printk("rtl8188eu: connect rejected; disconnect current SSID first\n");
        return -EBUSY;
    }

    scan_index = rtl8188eu_find_scanned_ssid((const uint8_t *)ssid,
                                             ssid_length);
    if (scan_index >= 0) {
        {
            uint8_t tx[RTL8188EU_TX_DESC_SIZE + 96];
            uint8_t txpause;
            size_t tx_length;
            unsigned int poll;

            rtl8188eu_bind_connection_result((unsigned int)scan_index);
            if (!rtl8188eu_connection_binding_valid()) {
                usb_printk("rtl8188eu: connect binding invalid before channel set\n");
                goto connect_failed;
            }
            usb_printk("rtl8188eu: auth target SSID=");
            for (j = 0; j < rtl8188eu.connected_ssid_length; j++)
                usb_printk("%c", rtl8188eu.connected_ssid[j]);
            usb_printk(" BSSID=%02x:%02x:%02x:%02x:%02x:%02x channel=%u\n",
                       rtl8188eu_connect_bssid[0],
                       rtl8188eu_connect_bssid[1],
                       rtl8188eu_connect_bssid[2],
                       rtl8188eu_connect_bssid[3],
                       rtl8188eu_connect_bssid[4],
                       rtl8188eu_connect_bssid[5],
                       rtl8188eu.current_channel);

            /* é–¿æ¬’î‡¤æ¾¶å‹­æ‚Šé¨å‹­ç²ºæ¶“â‚¬å¨“å‘¯æ‚Šéãƒ¥å½?*/
            if (rtl8188eu_write8(rtl8188eu.usb,
                                  RTL8188EU_REG_TXPAUSE, 0x00) < 0 ||
                rtl8188eu_read8(rtl8188eu.usb,
                                 RTL8188EU_REG_TXPAUSE, &txpause) < 0 ||
                txpause != 0x00 ||
                rtl8188eu_set_channel(
                    rtl8188eu.usb,
                    rtl8188eu.current_channel) < 0 ||
                rtl8188eu_set_station_mode(rtl8188eu.usb) < 0 ||
                rtl8188eu_program_bssid(rtl8188eu.usb, rtl8188eu_connect_bssid) < 0) {
                goto connect_failed;
            }

            rtl8188eu_log_channel_state(rtl8188eu.usb, "before-auth");

            tick_delay(10000);

            rtl8188eu_auth_status = -1;
            rtl8188eu_auth_poll_pending = 0;
            rtl8188eu_auth_poll_completions = 0;
            rtl8188eu_auth_rx_mgmt = 0;
            rtl8188eu_auth_rx_control = 0;
            rtl8188eu_auth_rx_data = 0;
            rtl8188eu_auth_rx_other = 0;
            rtl8188eu_auth_rx_target = 0;
            rtl8188eu_auth_rx_target_beacon = 0;
            rtl8188eu_auth_rx_target_probe = 0;
            rtl8188eu_auth_rx_auth = 0;
            rtl8188eu_auth_rx_target_logs = 0;
            rtl8188eu_auth_tx_started = 0;
            rtl8188eu.wifi_state = RTL8188EU_WIFI_AUTHENTICATING;

            if (rtl8188eu_stop_rx() < 0)
                goto connect_failed;
            A_memset(rtl8188eu_rx_buffer, 0, sizeof(rtl8188eu_rx_buffer));

            if (rtl8188eu_clear_interrupt_status(rtl8188eu.usb) < 0 ||
                rtl8188eu_clear_txdma_status(rtl8188eu.usb) < 0 ||
                !rtl8188eu_connection_binding_valid()) {
                goto connect_failed;
            }
            tx_length = rtl8188eu_build_auth_request(tx, sizeof(tx), rtl8188eu_connect_bssid);
            if (!tx_length)
                goto connect_failed;
            rtl8188eu_auth_tx_started = 1;
            rtl8188eu_log_set_color(RTL8188EU_LOG_COLOR_TX);
            rtl8188eu_log_tx_descriptor(tx);
            rtl8188eu_dump_bytes("AUTH TX descriptor and frame",
                                 tx, (unsigned int)tx_length);
            rtl8188eu_log_reset_color();
            rtl8188eu_log_tx_state(rtl8188eu.usb, "before-auth-usb");
            if (rtl8188eu_bulk_out(&rtl8188eu, rtl8188eu.bulk_out,
                                   tx, tx_length) < 0) {
                goto connect_failed;
            }
            /* Submit the authentication frame before arming the persistent
             * Bulk-IN TD.  Keeping both WiFi bulk endpoints outstanding
             * during the first management TX can lose the EP-OUT completion
             * in the current shared xHCI dispatcher.  The AP cannot answer
             * before the OUT transaction has completed, so arming RX here
             * does not discard an authentication response. */
            if (rtl8188eu_arm_rx() < 0)
                goto connect_failed;
            tick_delay(1000);
            rtl8188eu_log_tx_state(rtl8188eu.usb, "after-auth-usb");

            /* æµ¼æ¨ºå¯²æžî†¿î‡—éˆå“„åŸ—é”›å²€ç¼‰é­î…¢æ£¿é—…æ—€å½é—å›§æ·æ´æ–¿è‹Ÿé—„å¶„ç¶† CPU å¯®â‚¬é–¿â‚¬ */
            for (poll = 0; poll < 5000 && rtl8188eu_auth_status < 0; poll++) {
                usb_poll_interrupts();
                if (poll && (poll % 1000u) == 0 &&
                    rtl8188eu_auth_status < 0) {
                    if (!rtl8188eu_connection_binding_valid()) {
                        usb_printk("rtl8188eu: connect binding changed during auth\n");
                        goto connect_failed;
                    }
                    usb_printk("rtl8188eu: auth retry #%u\n",
                               poll / 1000u);
                    rtl8188eu_log_auth_rx_stats("before-retry");
                    if (rtl8188eu_bulk_out(&rtl8188eu,
                                           rtl8188eu.bulk_out,
                                           tx, tx_length) < 0) {
                        rtl8188eu_log_set_color(
                            RTL8188EU_LOG_COLOR_FAILURE);
                        usb_printk("rtl8188eu: auth retry TX failed\n");
                        rtl8188eu_log_reset_color();
                        /* A timed-out xHCI TD has already required Stop
                         * Endpoint + Set TR Dequeue recovery.  Repeating the
                         * same synchronous TX several more times accumulates
                         * late events and can starve unrelated MSC traffic on
                         * the shared controller.  Abort and quiesce RX at the
                         * first failed retry. */
                        goto connect_failed;
                    } else {
                        tick_delay(1000);
                        rtl8188eu_log_tx_state(rtl8188eu.usb,
                                               "after-auth-retry");
                    }
                }
                tick_delay(2000); /* 2ms polling interval */
            }

            if (rtl8188eu_auth_status != 0) {
                rtl8188eu_log_auth_rx_stats("final");
                rtl8188eu_log_tx_state(rtl8188eu.usb,
                                       "auth-failed-final");
                rtl8188eu_log_set_color(RTL8188EU_LOG_COLOR_FAILURE);
                usb_printk("rtl8188eu: auth failed status=%d\n", rtl8188eu_auth_status);
                rtl8188eu_log_reset_color();
                goto connect_failed;
            }

            /* Association Phase */
            rtl8188eu_assoc_status = -1;
            rtl8188eu_eapol_messages = 0;
            rtl8188eu_eapol_msg1_seen = 0;
            rtl8188eu_eapol_msg2_seen = 0;
            rtl8188eu_eapol_msg3_seen = 0;
            rtl8188eu_eapol_msg4_seen = 0;
            A_memset(rtl8188eu_eapol_replay_counter, 0,
                     sizeof(rtl8188eu_eapol_replay_counter));
            A_memset(rtl8188eu_eapol_anonce, 0,
                     sizeof(rtl8188eu_eapol_anonce));
            rtl8188eu.wifi_state = RTL8188EU_WIFI_ASSOCIATING;

            if (!rtl8188eu_connection_binding_valid()) {
                usb_printk("rtl8188eu: connect binding invalid before assoc\n");
                goto connect_failed;
            }
            tx_length = rtl8188eu_build_assoc_request(
                tx, sizeof(tx), rtl8188eu_connect_bssid,
                rtl8188eu.connected_ssid,
                rtl8188eu.connected_ssid_length);
            if (!tx_length || rtl8188eu_bulk_out(&rtl8188eu, rtl8188eu.bulk_out, tx, tx_length) < 0) {
                goto connect_failed;
            }

            for (poll = 0; poll < 1000 && rtl8188eu_assoc_status < 0; poll++) {
                usb_poll_interrupts();
                tick_delay(2000);
            }

            if (rtl8188eu_assoc_status != 0) {
                usb_printk("rtl8188eu: assoc failed status=%d\n", rtl8188eu_assoc_status);
                goto connect_failed;
            }

            rtl8188eu.wifi_state = RTL8188EU_WIFI_WPA2_HANDSHAKE;
            rtl8188eu.associated = 1;
            usb_printk("rtl8188eu: associated AID=%u; WPA2 EAPOL Handshake required\n", rtl8188eu_assoc_aid);

            /* FIXME: é—‡â‚¬ç›ãƒ¥åŽ?WPA2 4-Way Handshake é–«æ˜ç·«æµ ãƒ¥ç–„éœæ¿å§žç€µå—šç¹›éŽº?*/
            /* Keep RX armed long enough to observe the AP's first EAPOL-Key.
             * Cryptographic response generation is the next phase; this
             * window verifies that the associated data path is alive. */
            for (poll = 0; poll < 15000 &&
                 rtl8188eu.wifi_state == RTL8188EU_WIFI_WPA2_HANDSHAKE;
                 poll++) {
                usb_poll_interrupts();
                tick_delay(2000);
            }
            if (rtl8188eu_eapol_msg1_seen)
                usb_printk("rtl8188eu: WPA2 EAPOL message 1 captured; "
                           "PTK/GTK derivation and message 2 TX pending\n");
            else
                usb_printk("rtl8188eu: WPA2 EAPOL message 1 timeout "
                           "messages=%u\n", rtl8188eu_eapol_messages);
            if (rtl8188eu.wifi_state == RTL8188EU_WIFI_CONNECTED) {
                usb_printk("rtl8188eu: connect success SSID=%s\n",
                           rtl8188eu.connected_ssid);
                rtl8188eu.last_error = 0;
                return 0;
            }
            usb_printk("rtl8188eu: WPA2 handshake timeout/state=%u messages=%u\n",
                       rtl8188eu.wifi_state, rtl8188eu_eapol_messages);
            /* Do not leave the long-lived WiFi RX Bulk-IN request armed
             * while control returns to the shell.  MSC/FAT32 uses the same
             * xHCI controller and its BOT CBW/DATA/CSW sequence must not
             * compete with a stale WPA2 receive TD.  The connection is not
             * complete yet, so quiesce the radio and let a later connection
             * attempt arm RX again explicitly. */
            /* RX must be released before returning to the shell.  Avoid
             * additional MAC register traffic here: a failed control TD can
             * poison the shared xHCI schedule used by MSC. */
            if (rtl8188eu_stop_rx() < 0)
                usb_printk("rtl8188eu: WPA2 exit RX stop failed\n");
            rtl8188eu.wifi_state = RTL8188EU_WIFI_FAILED;
            rtl8188eu.last_error = -ETIMEDOUT;
            return -ETIMEDOUT;
        }
    }

    usb_printk("rtl8188eu: connect rejected; SSID not present in latest "
               "successful scan\n");
    rtl8188eu.last_error = -ENOENT;
    return -ENOENT;

connect_failed:
    /* Do not issue more WiFi control transfers on the shared controller
     * while handing ownership back to MSC. */
    if (rtl8188eu_stop_rx() < 0)
        usb_printk("rtl8188eu: connect cleanup RX stop failed\n");
    if (rtl8188eu_rx_armed) {
        usb_printk("rtl8188eu: connect cleanup RX still armed; retry stop\n");
        if (rtl8188eu_stop_rx() < 0)
            usb_printk("rtl8188eu: connect cleanup RX stop failed\n");
    }
    rtl8188eu.wifi_state = RTL8188EU_WIFI_FAILED;
    rtl8188eu.last_error = -EIO;
    return rtl8188eu.last_error;
}

int rtl8188eu_wifi_disconnect(void)
{
    if (!rtl8188eu.usb)
        return -ENODEV;
    rtl8188eu_quiesce_radio();
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
    if (rtl8188eu.wifi_state == RTL8188EU_WIFI_WPA2_HANDSHAKE ||
        rtl8188eu.wifi_state == RTL8188EU_WIFI_CONNECTED)
        usb_printk("rtl8188eu: WPA2 EAPOL messages=%u m1=%u m2=%u "
                   "m3=%u m4=%u\n", rtl8188eu_eapol_messages,
                   rtl8188eu_eapol_msg1_seen, rtl8188eu_eapol_msg2_seen,
                   rtl8188eu_eapol_msg3_seen, rtl8188eu_eapol_msg4_seen);
    return rtl8188eu.wifi_state;
}

static int rtl8188eu_write(struct usb_dev_t *usb, uint16_t reg,
                           const uint8_t *data, uint16_t length)
{
    int ok;
    int trace_write = rtl8188eu_fw_usb_trace;
    unsigned int attempt;

    if (!usb || (!data && length))
        return -EINVAL;
    ok = 0;
    for (attempt = 0; attempt < 3 && !ok; attempt++) {
        ok = usb_ctrl_out(usb, (void *)data, RTL8188EU_USB_REQ_WRITE,
                          RTL8188EU_USB_REQ_REGS,
                          (uint8_t)(reg >> 8), (uint8_t)reg, 0, length);
        if (!ok && attempt + 1 < 3)
            tick_delay(1000);
    }
    if (ok && attempt > 1)
        usb_printk("rtl8188eu: EP0 write recovered reg=%04x attempts=%u\n",
                   reg, attempt);
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
         RTL8188EU_CR_MACTXEN | RTL8188EU_CR_MACRXEN |
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
             RTL8188EU_CR_MACTXEN | RTL8188EU_CR_MACRXEN |
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

    /* 1. ç’å‰§ç–?MCU Ready æµ£å¶ç´å¨“å‘´æ«Ž CPU Reset ??*/
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

    /* 2. ç‘™ï¹€å½?MCU é‘·î„î˜??*/
    ret = rtl8188eu_firmware_self_reset(usb);
    if (ret < 0)
        return ret;

    /* 3. éæŠ½æ•­æ·‡î†½æ•¼é”›æ°¬î˜²æµ£å¶…æ‚—ç¼æ¬Žç°?MCU ç“’å†²î™„é¨å‹«åžµæ¿®å¬ªå¯²éšîˆšå§©éƒå •æ£?(5~10ms) */
    tick_delay(10000); 

    /* 4. æžî†¿î‡?MCU çè¾©åŽé˜èˆµâ‚¬ä¾Šç´°éŽ¬è¤ç“‘å¯°å‘®æ¤‚é—‚å­˜å½æ¥‚æ¨¿åš¦ ~100ms (1000 * 100us) */
    for (attempt = 0; attempt < 1000; attempt++) {
        ret = rtl8188eu_read32(usb, RTL8188EU_REG_MCUFWDL, &mcu_fwdl32);
        if (ret < 0)
            return ret;

        if (mcu_fwdl32 != previous && (attempt < 8 || previous != 0xffffffffu)) {
            usb_printk("rtl8188eu: MCU poll[%u] MCUFWDL=%08x\n",
                       attempt + 1, mcu_fwdl32);
            previous = mcu_fwdl32;
        }

        /* å¦«â‚¬??WINTINI_RDY ??(0x20000000) é„îˆšæƒ??MCU ??1 */
        /* WINTINI_RDY is BIT6 in the low byte: 0x06 -> 0xC6. */
        if (mcu_fwdl32 & RTL8188EU_WINTINI_RDY) {
            usb_printk("rtl8188eu: MCU start MCUFWDL=%08x ready=yes polls=%u\n",
                       mcu_fwdl32, attempt + 1);
            return 0;
        }

        /* çå——å´Ÿå¨†Â¤ç–†ç’‡ãˆ æ¬¢éƒæœµç²?5us æ¾§ç‚²å§??100us */
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
        iface->endpoint_in->direction != USB_ENDPOINT_IN ||
        iface->endpoint_out->type != USB_ENDPOINT_BULK ||
        iface->endpoint_out->direction != USB_ENDPOINT_OUT)
        {
        usb_printk("rtl8188eu: endpoints are not usable bulk IN/OUT\n");
        return -ENODEV;
    }
    if (iface->endpoint_in->type != USB_ENDPOINT_BULK ||
        iface->endpoint_out->type != USB_ENDPOINT_BULK) {
        usb_printk("rtl8188eu: endpoints are not bulk endpoints\n");
        return -ENODEV;
    }

    rtl8188eu.usb = usb;
    rtl8188eu.iface = iface;
    rtl8188eu.bound_slot = usb->num;
    rtl8188eu.bound_port = usb->port;
    rtl8188eu.bound_interface = iface->desc.interfacenum;
    rtl8188eu.bulk_in = iface->endpoint_in;
    rtl8188eu.bulk_out = iface->endpoint_out;
    rtl8188eu.bulk_out2 = iface->endpoint_out2;
    rtl8188eu.bound_bulk_in_addr = iface->endpoint_in->addr;
    rtl8188eu.bound_bulk_out_addr = iface->endpoint_out->addr;
    rtl8188eu.firmware_loaded = 0;
    rtl8188eu_rx_armed = 0;
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
        if (rtl8188eu_initialize_mac(usb) < 0) {
            rtl8188eu.wifi_state = RTL8188EU_WIFI_FAILED;
            rtl8188eu.last_error = -EIO;
            return -EIO;
        }
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
        if (ret < 0) {
            usb_printk("rtl8188eu: firmware download failed (%d)\n", ret);
        }
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
    if (rtl8188eu_initialize_mac(usb) < 0) {
        rtl8188eu.wifi_state = RTL8188EU_WIFI_FAILED;
        rtl8188eu.last_error = -EIO;
        return -EIO;
    }
    return 0;
}

void rtl8188eu_usb_remove(struct usb_dev_t *usb)
{
    if (rtl8188eu.usb != usb)
        return;
    usb_printk("rtl8188eu: USB adapter removed usb=%p\n", usb);
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
