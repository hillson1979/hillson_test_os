/**
 * @file kbd.h
 * @brief Keyboard subsystem compatibility stubs
 */
#ifndef KERNEL_KBD_H
#define KERNEL_KBD_H

#include <stdint.h>

struct task_t;
extern struct task_t *kbd_task;

#define kbd_queue  kbd_task

static inline int kbdbuf_enqueue(void *queue, int keycode) {
    (void)queue; (void)keycode;
    return 0;
}

#endif /* KERNEL_KBD_H */
