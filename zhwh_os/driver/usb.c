/**
 * @file usb.c
 * @brief USB Core Protocol Stack
 *
 * Implements USB device enumeration, configuration, and communication
 */

#include <stdint.h>
#include "string.h"
#include "printf.h"
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

    // Read device descriptor (still at address 0)
    if (usb_get_device_descriptor(controller_id, 0, &dev->device_desc) != 0) {
        printf("[USB] ERROR: Failed to read device descriptor\n");
        return -1;
    }

    usb_print_device_descriptor(&dev->device_desc);

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

        // Try to initialize as a USB mouse
        // Standard HID mouse uses: interface=0, endpoint=1, max_packet=8
        extern int usb_mouse_init(int controller_id, uint8_t dev_addr,
                                  uint8_t interface, uint8_t endpoint_in,
                                  uint8_t max_packet);
        int mouse_idx = usb_mouse_init(controller_id, dev->address, 0, 1, 8);
        if (mouse_idx >= 0) {
            printf("[USB] USB mouse initialized (index=%d)\n", mouse_idx);
        } else {
            printf("[USB] Not a USB mouse or initialization failed\n");
        }
    }

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
