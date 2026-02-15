/**
 * @file usb_hcd.h
 * @brief USB Host Controller Driver definitions
 */

#ifndef USB_HCD_H
#define USB_HCD_H

#include <stdint.h>
#include "usb.h"

#define USB_MAX_CONTROLLERS  8

/**
 * @brief Initialize USB Host Controller Driver
 * @return 0 on success, negative on error
 */
int usb_hcd_init(void);

/**
 * @brief Perform USB control transfer
 * @param controller_id Controller index
 * @param dev_addr USB device address
 * @param ep Endpoint number
 * @param req Setup packet
 * @param data Data buffer (can be NULL)
 * @return Number of bytes transferred or negative error
 */
int usb_control_transfer(int controller_id, uint8_t dev_addr, uint8_t ep,
                         usb_device_request_t *req, void *data);

/**
 * @brief Perform USB interrupt IN transfer (for reading data like mouse/keyboard)
 * @param controller_id Controller index
 * @param dev_addr USB device address
 * @param ep Endpoint number
 * @param data Data buffer
 * @param len Max length to read
 * @return Number of bytes read or negative error
 */
int usb_interrupt_transfer(int controller_id, uint8_t dev_addr, uint8_t ep,
                          void *data, int len);

/**
 * @brief Check if interrupt transfer has completed
 * @param controller_id Controller index
 * @return 1 if completed, 0 if still pending, negative on error
 */
int usb_interrupt_poll(int controller_id);

/**
 * @brief Scan root hub ports for connected devices
 * @param controller_id Controller index
 * @return Number of devices found or negative error
 */
int usb_hcd_scan_ports(int controller_id);

/**
 * @brief Initialize USB mouse periodic polling
 * @param controller_id Controller index
 * @param dev_addr USB device address
 * @param ep Interrupt endpoint number
 * @param low_speed 1 for low-speed, 0 for full-speed
 * @return 0 on success, negative on error
 */
int usb_mouse_periodic_init(int controller_id, uint8_t dev_addr, uint8_t ep, int low_speed);

/**
 * @brief Poll for mouse data (non-blocking)
 * @param report Output buffer (must be at least 8 bytes)
 * @return Number of bytes read (0 if no data, negative on error)
 */
int usb_mouse_periodic_poll(uint8_t *report);

/**
 * @brief Get device low-speed flag from UHCI controller
 * @param controller_id Controller index
 * @return 1 if low-speed, 0 if full-speed, negative on error
 */
int usb_hcd_get_device_speed(int controller_id);

/**
 * @brief Check for USB device connect/disconnect events
 * @param controller_id Controller index
 * @return Positive if device state changed, 0 if no change, negative on error
 *
 * This function should be called periodically to detect hot-plug events.
 * It will automatically enumerate new devices or clean up disconnected ones.
 */
int usb_hcd_poll_hotplug(int controller_id);

#endif  // USB_HCD_H
