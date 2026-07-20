/**
 * @file usb.c
 * @brief USB Core Protocol Stack
 *
 * Implements USB device enumeration, configuration, and communication
 */

#include <stdint.h>
#include "string.h"
#include "printf.h"

uint8_t g_usb_mouse_ep = 0x81;
uint8_t g_usb_mouse_maxpkt = 8;
uint8_t g_usb_mouse_interval = 10;
uint8_t g_usb_if_proto = 0;  // HID interface protocol
uint8_t g_usb_eps[4] = {0};   // endpoint addresses found
int g_usb_ep_count = 0;
int g_usb_setproto_result = -1;  // SET_PROTOCOL return value
#include "usb.h"
#include "usb_hcd.h"

// USB device states
typedef enum {
    USB_STATE_ATTACHED,
    USB_STATE_POWERED,
    USB_STATE_DEFAULT,
    USB_STATE_ADDRESS,
    USB_STATE_CONFIGURED,
} usb_device_state_t;

// USB device structure
typedef struct usb_device {
    uint8_t address;
    uint8_t num_configurations;
    usb_device_state_t state;
    usb_device_descriptor_t device_desc;
    usb_config_descriptor_t config_desc;
    int controller_id;
    int initialized;
} usb_device_t;

static usb_device_t usb_devices[USB_MAX_DEVICES];
static int num_usb_devices = 0;
static uint8_t next_device_addr = 1;



/**
 * @brief Get USB descriptor string
 */
const char *usb_descriptor_type_string(uint8_t type) {
    switch (type) {
        case USB_DT_DEVICE: return "Device";
        case USB_DT_CONFIG: return "Configuration";
        case USB_DT_STRING: return "String";
        case USB_DT_INTERFACE: return "Interface";
        case USB_DT_ENDPOINT: return "Endpoint";
        case USB_DT_HID: return "HID";
        case USB_DT_REPORT: return "Report";
        case USB_DT_PHYSICAL: return "Physical";
        default: return "Unknown";
    }
}

/**
 * @brief Print device descriptor
 */
void usb_print_device_descriptor(usb_device_descriptor_t *desc) {
    printf("[USB] Device Descriptor:\n");
    printf("      Length: %d\n", desc->bLength);
    printf("      Type: %s\n", usb_descriptor_type_string(desc->bDescriptorType));
    printf("      USB Version: %x.%x\n",
           (desc->bcdUSB >> 8) & 0xF, (desc->bcdUSB >> 4) & 0xF);
    printf("      Class: 0x%x, SubClass: 0x%x, Protocol: 0x%x\n",
           desc->bDeviceClass, desc->bDeviceSubClass, desc->bDeviceProtocol);
    printf("      Max Packet Size: %d\n", desc->bMaxPacketSize0);
    printf("      Vendor:Product = 0x%x:0x%x\n",
           desc->idVendor, desc->idProduct);
    printf("      Manufacturer: %d, Product: %d, Serial: %d\n",
           desc->iManufacturer, desc->iProduct, desc->iSerialNumber);
    printf("      Num Configurations: %d\n", desc->bNumConfigurations);
}

/**
 * @brief Read device descriptor
 */
static int usb_get_device_descriptor(int controller_id, uint8_t addr,
                                     usb_device_descriptor_t *desc) {
    usb_device_request_t req;

    // Read first 8 bytes only (to get max packet size)
    req.bmRequestType = USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE;
    req.bRequest = USB_REQ_GET_DESCRIPTOR;
    req.wValue = (USB_DT_DEVICE << 8) | 0;
    req.wIndex = 0;
    req.wLength = 8;

    int ret = usb_control_transfer(controller_id, addr, 0, &req, desc);
    if (ret < 0) {
        printf("[USB] ERROR: Failed to read device descriptor (partial)\n");
        return -1;
    }

    // Now read full descriptor
    req.wLength = desc->bLength;
    ret = usb_control_transfer(controller_id, addr, 0, &req, desc);
    if (ret < 0) {
        printf("[USB] ERROR: Failed to read device descriptor (full)\n");
        return -1;
    }

    return 0;
}

/**
 * @brief Set USB device address
 */
static int usb_set_address(int controller_id, uint8_t old_addr, uint8_t new_addr) {
    usb_device_request_t req;

    req.bmRequestType = USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE;
    req.bRequest = USB_REQ_SET_ADDRESS;
    req.wValue = new_addr;
    req.wIndex = 0;
    req.wLength = 0;

    int ret = usb_control_transfer(controller_id, old_addr, 0, &req, NULL);
    if (ret < 0) {
        printf("[USB] ERROR: Failed to set address\n");
        return -1;
    }

    // Small delay to let device settle
    for (volatile int i = 0; i < 10000; i++);

    return 0;
}

/**
 * @brief Set USB configuration
 */
static int usb_set_configuration(int controller_id, uint8_t addr, uint8_t config) {
    usb_device_request_t req;

    req.bmRequestType = USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE;
    req.bRequest = USB_REQ_SET_CONFIGURATION;
    req.wValue = config;
    req.wIndex = 0;
    req.wLength = 0;

    int ret = usb_control_transfer(controller_id, addr, 0, &req, NULL);
    if (ret < 0) {
        printf("[USB] ERROR: Failed to set configuration\n");
        return -1;
    }

    return 0;
}

/**
 * @brief Enumerate a new USB device
 */
int usb_enumerate_device(int controller_id, uint8_t port) {
    if (num_usb_devices >= USB_MAX_DEVICES) {
        printf("[USB] ERROR: Too many USB devices\n");
        return -1;
    }

    printf("[USB] Enumerating device on controller %d, port %d\n",
           controller_id, port);

    usb_device_t *dev = &usb_devices[num_usb_devices];
    memset(dev, 0, sizeof(usb_device_t));
    dev->controller_id = controller_id;
    dev->state = USB_STATE_ATTACHED;

    // Device starts at address 0
    dev->address = 0;
    extern void ehci_port_reset(int port);
    ehci_port_reset(port);

    // Init F1 display buffer
    extern void ehci_display_clear(void);
    extern void ehci_display_append(const char *s);
    extern void ehci_display_hex(const uint8_t *data, int len);
    extern void ehci_display_sprintf(const char *fmt, ...);
    ehci_display_clear();
    ehci_display_append("=== UHCI ENUM ===\n");
    ehci_display_sprintf("ctrl=%d port=%d\n", controller_id, port);

    // Read device descriptor (still at address 0)
    if (usb_get_device_descriptor(controller_id, 0, &dev->device_desc) != 0) {
        printf("[USB] ERROR: Failed to read device descriptor\n");
        ehci_display_append("GET_DEV_DESC FAILED\n");
        return -1;
    }

    usb_print_device_descriptor(&dev->device_desc);

    // Write device descriptor to F1 display buffer
    ehci_display_append("[DEV DESC]\n  DATA: ");
    ehci_display_hex((uint8_t*)&dev->device_desc, 18);
    ehci_display_sprintf("\n  idVendor=%04x idProduct=%04x Class=%02x\n",
        dev->device_desc.idVendor, dev->device_desc.idProduct,
        dev->device_desc.bDeviceClass);

    // Assign unique address
    if (usb_set_address(controller_id, 0, next_device_addr) != 0) {
        printf("[USB] ERROR: Failed to set address\n");
        return -1;
    }

    dev->address = next_device_addr;
    dev->state = USB_STATE_ADDRESS;
    printf("[USB] Device assigned address %d\n", dev->address);

    next_device_addr++;

    // Set configuration (use first configuration)
    if (usb_set_configuration(controller_id, dev->address, 1) != 0) {
        printf("[USB] WARNING: Failed to set configuration\n");
        // Continue anyway, some devices work without this
    }

    dev->state = USB_STATE_CONFIGURED;
    dev->initialized = 1;
    num_usb_devices++;

    printf("[USB] Device enumerated successfully (total: %d)\n", num_usb_devices);

    // Check if this is a HID device (mouse/keyboard)
    if (dev->device_desc.bDeviceClass == 0x00 ||  // Interface-specific
        dev->device_desc.bDeviceClass == 0x0C) {  // HID class

        printf("[USB] Detected HID device\n");

        // Read config descriptor to find real endpoint info
        uint8_t cfg_buf[64];
        memset(cfg_buf, 0, 64);
        {
            usb_device_request_t req2;
            req2.bmRequestType = USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE;
            req2.bRequest = 0x06;  // GET_DESCRIPTOR
            req2.wValue = (0x02 << 8) | 0;  // CONFIGURATION descriptor, index 0
            req2.wIndex = 0;
            req2.wLength = 64;
            int r2 = usb_control_transfer(controller_id, dev->address, 1, &req2, cfg_buf);
            ehci_display_sprintf("[CFG DESC] r=%d\n", r2);
            if (r2 > 0) {
                ehci_display_append("  DATA: ");
                ehci_display_hex(cfg_buf, r2 < 64 ? r2 : 64);
                ehci_display_sprintf("\n  len=%d bNumIf=%d\n", r2, cfg_buf[4]);
                // Parse endpoint from config descriptor
                // wTotalLength at offset 2, bNumInterfaces at offset 4
                // Interface descriptor starts at offset 9 (after config desc = 9 bytes)
                int pos = 9;  // skip config descriptor header
                int cfg_len = cfg_buf[2] | (cfg_buf[3] << 8);
                printf("[USB] Config descriptor: total_len=%d\n", cfg_len);
                while (pos < cfg_len && pos < 64) {
                    uint8_t len = cfg_buf[pos];
                    uint8_t type = cfg_buf[pos + 1];
                    if (type == 0x04) {  // Interface
                        uint8_t if_num = cfg_buf[pos + 2];
                        uint8_t if_class = cfg_buf[pos + 5];
                        uint8_t if_subclass = cfg_buf[pos + 6];
                        uint8_t if_protocol = cfg_buf[pos + 7];
                        g_usb_if_proto = if_protocol;
                        printf("[USB]   Interface %d: class=0x%x sub=0x%x proto=0x%x\n",
                               if_num, if_class, if_subclass, if_protocol);
                        ehci_display_sprintf("  IF%d: class=%x sub=%x proto=%x\n",
                            if_num, if_class, if_subclass, if_protocol);
                    } else if (type == 0x05) {  // Endpoint
                        uint8_t ep_addr = cfg_buf[pos + 2];
                        uint16_t maxpkt = cfg_buf[pos + 4] | (cfg_buf[pos + 5] << 8);
                        uint8_t interval = cfg_buf[pos + 6];
                                        // Store for user-space query
                        extern uint8_t g_usb_mouse_ep, g_usb_mouse_maxpkt, g_usb_mouse_interval;
                        g_usb_mouse_ep = ep_addr;
                        g_usb_mouse_maxpkt = maxpkt & 0xFF;
                        g_usb_mouse_interval = interval;
                        printf("[USB] *** MOUSE ENDPOINT: addr=0x%x maxpkt=%d interval=%d ***\n",
                               ep_addr, maxpkt, interval);
                        ehci_display_sprintf("  EP: addr=%02x attr=%02x maxpkt=%d interval=%d\n",
                            ep_addr, cfg_buf[pos+3], maxpkt, interval);
                    }
                    pos += len;
                    if (len == 0) break;
                }
            } else {
                ehci_display_append("  FAILED\n");
            }
        }

        // Try to initialize as a USB mouse
        // Standard HID mouse uses: interface=0, endpoint=1 (IN=0x81), max_packet=8
        // Read HID descriptor to verify endpoint and report length
        {
            uint8_t hid_desc[16];
            memset(hid_desc, 0, 16);
            usb_device_request_t rhid;
            rhid.bmRequestType = 0x81;  // IN, Standard, Interface
            rhid.bRequest = 0x06;       // GET_DESCRIPTOR
            rhid.wValue = 0x2100;       // HID descriptor
            rhid.wIndex = 0;            // interface 0
            rhid.wLength = 9;
            int rh = usb_control_transfer(controller_id, dev->address, 1, &rhid, hid_desc);
            ehci_display_sprintf("[HID DESC] r=%d\n", rh);
            if (rh > 0) {
                int rpt_len = hid_desc[7] | (hid_desc[8] << 8);
                printf("[USB] HID Report Descriptor length: %d bytes\n", rpt_len);
                ehci_display_append("  DATA: ");
                ehci_display_hex(hid_desc, rh < 9 ? rh : 9);
                ehci_display_sprintf("\n  rpt_len=%d\n", rpt_len);
                g_usb_mouse_interval = (uint8_t)(rpt_len & 0xFF);
            } else {
                printf("[USB] WARNING: Failed to read HID descriptor (ret=%d)\n", rh);
                ehci_display_append("  FAILED\n");
            }
        }

        extern int usb_mouse_init(int controller_id, uint8_t dev_addr,
                                  uint8_t interface, uint8_t endpoint_in,
                                  uint8_t max_packet);
        int mouse_idx = usb_mouse_init(controller_id, dev->address, 0, g_usb_mouse_ep,g_usb_mouse_maxpkt);
        if (mouse_idx >= 0) {
            printf("[USB] USB mouse initialized (index=%d)\n", mouse_idx);
            ehci_display_sprintf("MOUSE INIT OK idx=%d\n", mouse_idx);
        } else {
            printf("[USB] Not a USB mouse or initialization failed\n");
            ehci_display_append("MOUSE INIT FAILED\n");
        }
    }

    ehci_display_append("=== UHCI ENUM DONE ===\n");
    return dev->address;
}

/**
 * @brief Initialize USB stack
 */
int usb_init(void) {
    printf("[USB] Initializing USB stack...\n");

    memset(usb_devices, 0, sizeof(usb_devices));
    num_usb_devices = 0;
    next_device_addr = 1;

    // Initialize host controller driver
    if (usb_hcd_init() != 0) {
        printf("[USB] ERROR: Failed to initialize HCD\n");
        return -1;
    }

    // EHCI: mouse init → does its own enumeration via periodic schedule
    extern int g_using_ehci;
    extern int usb_mouse_init(int, uint8_t, uint8_t, uint8_t, uint8_t);
    extern uint8_t g_usb_mouse_ep, g_usb_mouse_maxpkt;
    if (g_using_ehci) {
        uint8_t ep = g_usb_mouse_ep ? g_usb_mouse_ep : 0x81;
        uint8_t mx = g_usb_mouse_maxpkt ? g_usb_mouse_maxpkt : 8;
        usb_mouse_init(0, 1, 0, ep, mx);
    }

    // Scan for devices on each controller
    extern int usb_hcd_scan_ports(int controller_id);
    extern int num_uhci_controllers;

    for (int ctrl_id = 0; ctrl_id < num_uhci_controllers; ctrl_id++) {
        printf("[USB] Scanning controller %d for devices...\n", ctrl_id);

        int found = usb_hcd_scan_ports(ctrl_id);
        if (found > 0) {
            printf("[USB] Found %d device(s) on controller %d\n", found, ctrl_id);

            // Try to enumerate each port
            for (int port = 0; port < found; port++) {
                printf("[USB] Enumerating device on port %d...\n", port);
                if (usb_enumerate_device(ctrl_id, port) < 0) {
                    printf("[USB] WARNING: Failed to enumerate device on port %d\n", port);
                }
            }
        }
    }

    printf("[USB] USB stack initialized\n");
    return 0;
}

/**
 * @brief Get USB device by address
 */
usb_device_t *usb_get_device(uint8_t addr) {
    for (int i = 0; i < num_usb_devices; i++) {
        if (usb_devices[i].address == addr) {
            return &usb_devices[i];
        }
    }
    return NULL;
}
