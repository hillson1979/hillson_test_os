/**
 * @file usb_mouse.h
 * @brief USB Mouse Driver definitions
 */

#ifndef USB_MOUSE_H
#define USB_MOUSE_H

#include <stdint.h>

#define USB_MAX_MICE  4

// Mouse report structure (boot protocol)
typedef struct {
    uint8_t buttons;      // Bit 0: Left, Bit 1: Right, Bit 2: Middle
    int8_t  x;            // X displacement
    int8_t  y;            // Y displacement
} __attribute__((packed)) usb_mouse_report_t;

/**
 * @brief Initialize USB mouse
 * @param controller_id Controller index
 * @param dev_addr USB device address
 * @param interface Interface number
 * @param endpoint_in Interrupt IN endpoint
 * @param max_packet Max packet size
 * @return Mouse index or negative error
 */
int usb_mouse_init(int controller_id, uint8_t dev_addr, uint8_t interface,
                   uint8_t endpoint_in, uint8_t max_packet);

/**
 * @brief Read mouse report (interrupt transfer)
 * @param mouse_index Mouse index
 * @param report Report buffer
 * @return Number of bytes read or negative error
 */
int usb_mouse_read(int mouse_index, void *report);

/**
 * @brief Check if mouse has data available (non-blocking)
 * @param mouse_index Mouse index
 * @return 1 if data available, 0 otherwise
 */
int usb_mouse_data_available(int mouse_index);

/**
 * @brief Register callback for mouse events
 * @param mouse_index Mouse index
 * @param callback Callback function(dx, dy, buttons)
 */
void usb_mouse_register_callback(int mouse_index,
                                 void (*callback)(int dx, int dy, uint8_t buttons));

/**
 * @brief Get mouse count
 * @return Number of mice
 */
int usb_mouse_get_count(void);

/**
 * @brief Test mouse driver
 */
void usb_mouse_test(void);

/**
 * @brief Test mouse driver with periodic polling
 */
void usb_mouse_test_periodic(void);

#endif  // USB_MOUSE_H
