/**
 * @file mouse.h
 * @brief Mouse subsystem compatibility stubs
 */
#ifndef KERNEL_MOUSE_H
#define KERNEL_MOUSE_H

#include <stdint.h>

/* Mouse button flags */
typedef uint8_t mouse_buttons_t;
#define MOUSE_LBUTTON_DOWN  0x01
#define MOUSE_RBUTTON_DOWN  0x02
#define MOUSE_MBUTTON_DOWN  0x04

extern mouse_buttons_t cur_button_state;
extern struct task_t *mouse_task;

/* Stub: these functions don't exist in Hillson OS */
static inline void add_mouse_packet(int dx, int dy, uint8_t buttons) {
    (void)dx; (void)dy; (void)buttons;
}

#endif /* KERNEL_MOUSE_H */
