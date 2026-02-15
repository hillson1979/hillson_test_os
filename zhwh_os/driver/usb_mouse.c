/**
 * @file usb_mouse.c
 * @brief USB HID Mouse Driver
 *
 * Implements driver for USB HID mouse devices
 */

#include <stdint.h>
#include "string.h"
#include "printf.h"
#include "x86/io.h"
#include "usb_mouse.h"
#include "usb.h"
#include "usb_hcd.h"

// HID 请求码
#ifndef USB_REQ_SET_PROTOCOL
#define USB_REQ_SET_PROTOCOL  0x0B
#endif

#ifndef USB_REQ_SET_IDLE
#define USB_REQ_SET_IDLE      0x0A
#endif

#ifndef USB_REQ_GET_REPORT
#define USB_REQ_GET_REPORT    0x01
#endif

// Forward declarations for periodic polling state
typedef struct usb_mouse_periodic_state usb_mouse_periodic_state_t;

// Mouse state
typedef struct usb_mouse_device {
    uint8_t address;
    uint8_t interface;
    uint8_t endpoint_in;
    uint8_t endpoint_in_max;
    int controller_id;    // USB controller ID
    int initialized;
    int transfer_active;  // Is an interrupt transfer active?
    usb_mouse_report_t last_report;
    void (*callback)(int dx, int dy, uint8_t buttons);
    int periodic_initialized;  // 🔥 Is periodic polling initialized?
} usb_mouse_device_t;

static usb_mouse_device_t usb_mice[USB_MAX_MICE];
static int num_usb_mice = 0;

/**
 * @brief Set mouse boot protocol
 */
static int usb_mouse_set_boot_protocol(int controller_id, uint8_t dev_addr,
                                       uint8_t interface) {
    usb_device_request_t req;

    req.bmRequestType = USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE;
    req.bRequest = USB_REQ_SET_PROTOCOL;
    req.wValue = 0;  // Boot protocol
    req.wIndex = interface;
    req.wLength = 0;

    int ret = usb_control_transfer(controller_id, dev_addr, 0, &req, NULL);
    if (ret < 0) {
        printf("[USB Mouse] ERROR: Failed to set boot protocol\n");
        return -1;
    }

    printf("[USB Mouse] Set boot protocol\n");
    return 0;
}

/**
 * @brief Set idle rate (0 = continuous reports)
 */
static int usb_mouse_set_idle(int controller_id, uint8_t dev_addr,
                              uint8_t interface) {
    usb_device_request_t req;

    req.bmRequestType = USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE;
    req.bRequest = USB_REQ_SET_IDLE;  // 0x0A
    req.wValue = 0;  // Idle rate = 0 (continuous)
    req.wIndex = interface;
    req.wLength = 0;

    int ret = usb_control_transfer(controller_id, dev_addr, 0, &req, NULL);
    if (ret < 0) {
        printf("[USB Mouse] WARNING: Failed to set idle rate\n");
        return -1;
    }

    return 0;
}

/**
 * @brief Initialize USB mouse
 */
int usb_mouse_init(int controller_id, uint8_t dev_addr, uint8_t interface,
                   uint8_t endpoint_in, uint8_t max_packet) {
    if (num_usb_mice >= USB_MAX_MICE) {
        printf("[USB Mouse] ERROR: Too many mice\n");
        return -1;
    }

    printf("[USB Mouse] Initializing mouse at address %d\n", dev_addr);

    usb_mouse_device_t *mouse = &usb_mice[num_usb_mice];
    memset(mouse, 0, sizeof(usb_mouse_device_t));

    mouse->address = dev_addr;
    mouse->interface = interface;
    mouse->endpoint_in = endpoint_in;
    mouse->endpoint_in_max = max_packet;
    mouse->controller_id = controller_id;  // 🔥 保存控制器ID
    mouse->transfer_active = 0;              // 🔥 初始化为未激活

    // Set boot protocol
    if (usb_mouse_set_boot_protocol(controller_id, dev_addr, interface) != 0) {
        return -1;
    }

    // Set idle rate
    usb_mouse_set_idle(controller_id, dev_addr, interface);

    mouse->initialized = 1;
    num_usb_mice++;

    // 🔥 Initialize periodic polling automatically (only for first mouse)
    extern int usb_hcd_get_device_speed(int controller_id);
    extern int usb_mouse_periodic_init(int controller_id, uint8_t dev_addr, uint8_t ep, int low_speed);

    if (num_usb_mice == 1) {  // Only initialize once (first mouse)
        int is_low_speed = usb_hcd_get_device_speed(controller_id);
        if (is_low_speed < 0) {
            printf("[USB Mouse] WARNING: Could not determine device speed\n");
            is_low_speed = 0;  // Assume full-speed
        }

        int ret = usb_mouse_periodic_init(controller_id, dev_addr, endpoint_in, is_low_speed);
        if (ret < 0) {
            printf("[USB Mouse] WARNING: Failed to initialize periodic polling\n");
            mouse->periodic_initialized = 0;
        } else {
            mouse->periodic_initialized = 1;
            printf("[USB Mouse] Periodic polling initialized\n");
        }
    } else {
        printf("[USB Mouse] NOTE: Periodic polling already active (multiple mice not yet supported)\n");
        mouse->periodic_initialized = 1;
    }

    printf("[USB Mouse] Mouse initialized successfully (total: %d)\n",
           num_usb_mice);

    return num_usb_mice - 1;  // Return mouse index
}

/**
 * @brief Read mouse report using periodic polling (non-blocking)
 * @return Number of bytes read (0 if no data yet), negative on error
 */
int usb_mouse_read_bak(int mouse_index, void *report) {
    if (mouse_index < 0 || mouse_index >= num_usb_mice) {
        return -1;
    }

    usb_mouse_device_t *mouse = &usb_mice[mouse_index];
    if (!mouse->initialized) {
        return -1;
    }

    // 🔥 NEW: Use periodic polling API (non-blocking)
    extern int usb_mouse_periodic_poll(uint8_t *report);

    int ret = usb_mouse_periodic_poll((uint8_t *)report);
    if (ret < 0) {
        // Error or no data yet
        return ret;
    }

    return ret;
}
extern int usb_mouse_periodic_poll(uint8_t *report);

int usb_mouse_read(int mouse_index, void *report)
{
    if (mouse_index < 0 || mouse_index >= num_usb_mice)
        return -1;

    usb_mouse_device_t *mouse = &usb_mice[mouse_index];
    if (!mouse->initialized)
        return -1;

    return usb_mouse_periodic_poll((uint8_t *)report);
}

/**
 * @brief Check if mouse has data available (non-blocking)
 */
int usb_mouse_data_available(int mouse_index) {
    if (mouse_index < 0 || mouse_index >= num_usb_mice) {
        return 0;
    }

    usb_mouse_device_t *mouse = &usb_mice[mouse_index];
    if (!mouse->initialized) {
        return 0;
    }

    // 🔥 Synchronous mode: we can always start a transfer
    return 1;
}

/**
 * @brief Register callback for mouse events
 */
void usb_mouse_register_callback(int mouse_index,
                                 void (*callback)(int dx, int dy, uint8_t buttons)) {
    if (mouse_index < 0 || mouse_index >= num_usb_mice) {
        return;
    }

    usb_mice[mouse_index].callback = callback;
}

/**
 * @brief Get mouse count
 */
int usb_mouse_get_count(void) {
    return num_usb_mice;
}

/**
 * @brief Test for USB mouse presence
 */
void usb_mouse_test(void) {
    printf("[USB Mouse] Testing mouse driver...\n");
    printf("[USB Mouse] Found %d mouse(es)\n", num_usb_mice);

    for (int i = 0; i < num_usb_mice; i++) {
        usb_mouse_device_t *mouse = &usb_mice[i];
        printf("[USB Mouse]   Mouse %d: addr=%d, ep=0x%x\n",
               i, mouse->address, mouse->endpoint_in);
    }
}

/**
 * @brief Test USB mouse with periodic polling
 */
void usb_mouse_test_periodic(void) {
    printf("[USB Mouse] ========================================\n");
    printf("[USB Mouse] Testing periodic polling...\n");
    printf("[USB Mouse] ========================================\n");

    if (num_usb_mice == 0) {
        printf("[USB Mouse] No mice found!\n");
        return;
    }

    usb_mouse_device_t *mouse = &usb_mice[0];

    if (!mouse->periodic_initialized) {
        printf("[USB Mouse] ERROR: Periodic polling not initialized!\n");
        return;
    }

    printf("[USB Mouse] Periodic polling active, waiting for data...\n");

    // Poll for data
    uint8_t report[8];
    int count = 0;
    int max_polls = 100;  // Poll up to 100 times

    while (count < max_polls) {
        int n = usb_mouse_periodic_poll(report);
        if (n > 0) {
            // 🔥 注释掉调试输出，避免刷屏导致LVGL无法渲染
            // printf("[USB Mouse] Got data! Button=%d X=%d Y=%d\n",
            //        report[0] & 0x07, (int8_t)report[1], (int8_t)report[2]);
            count = 0;  // Reset counter on successful read
        } else if (n < 0) {
            printf("[USB Mouse] ERROR: Poll failed\n");
            break;
        }

        count++;
        // Small delay between polls
        for (volatile int i = 0; i < 10000; i++);
    }

    printf("[USB Mouse] Test complete!\n");
    printf("[USB Mouse] ========================================\n");
}
