/**
 * @file cherryusb_hid_mouse.h
 * @brief CherryUSB-style HID Mouse Driver for HillsonOS
 *
 * Ported from CherryUSB class/hid/hid_mouse.c architecture.
 * Follows the layered design: Core USB → HID Class → Mouse Driver
 */

#ifndef CHERRYUSB_HID_MOUSE_H
#define CHERRYUSB_HID_MOUSE_H

#include <stdint.h>

#define CHERRYUSB_MAX_MICE  4

/* HID Mouse Report (Boot Protocol, 3 bytes) */
typedef struct {
    uint8_t buttons;       /* Bit0=Left, Bit1=Right, Bit2=Middle */
    int8_t  x;             /* X displacement */
    int8_t  y;             /* Y displacement */
    int8_t  wheel;         /* Wheel (optional, 4th byte) */
} __attribute__((packed)) cherryusb_mouse_report_t;

/* Mouse event callback */
typedef void (*cherryusb_mouse_callback_t)(int dx, int dy, uint8_t buttons, int8_t wheel);

/* HID Mouse device descriptor */
typedef struct {
    uint8_t  dev_addr;           /* USB device address */
    uint8_t  interface;          /* HID interface number */
    uint8_t  ep_in;              /* Interrupt IN endpoint address */
    uint8_t  ep_in_max;          /* Max packet size for IN endpoint */
    uint8_t  ep_in_interval;     /* Polling interval (ms) */
    int      controller_id;      /* USB host controller index */
    uint8_t  protocol;           /* 0=Boot, 1=Report */
    uint8_t  idle_rate;          /* Idle rate (4ms units) */

    cherryusb_mouse_report_t last;   /* Last report */
    cherryusb_mouse_callback_t cb;   /* User callback */
    uint8_t  ready;                  /* Device initialized? */
} cherryusb_mouse_dev_t;

/**
 * @brief Initialize HID mouse device
 *
 * CherryUSB-style: descriptor discovery → set protocol → set idle → start polling
 *
 * @param controller_id USB host controller index
 * @param dev_addr      USB device address
 * @param interface     HID interface number
 * @param ep_in         Interrupt IN endpoint address
 * @param ep_max        Max packet size
 * @param interval      Polling interval (ms)
 * @return mouse index (>=0) or -1 on error
 */
int cherryusb_hid_mouse_init(int controller_id, uint8_t dev_addr, uint8_t interface,
                              uint8_t ep_in, uint8_t ep_max, uint8_t interval);

/**
 * @brief Read mouse report (interrupt transfer, blocking)
 * @param idx  Mouse index
 * @param rpt  Output report buffer
 * @return bytes read or -1 on error
 */
int cherryusb_hid_mouse_read(int idx, cherryusb_mouse_report_t *rpt);

/**
 * @brief Poll mouse (non-blocking check)
 * @param idx Mouse index
 * @return 1 if data available, 0 if not, -1 on error
 */
int cherryusb_hid_mouse_poll(int idx);

/**
 * @brief Register mouse movement callback
 * @param idx Mouse index
 * @param cb  Callback(dx, dy, buttons, wheel)
 */
void cherryusb_hid_mouse_set_callback(int idx, cherryusb_mouse_callback_t cb);

/**
 * @brief Get number of initialized mice
 */
int cherryusb_hid_mouse_count(void);

/**
 * @brief Reset/deinit a mouse device
 * @param idx Mouse index
 */
void cherryusb_hid_mouse_deinit(int idx);

#endif /* CHERRYUSB_HID_MOUSE_H */
