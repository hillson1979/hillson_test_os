/**
 * @file pic.h
 */
#ifndef KERNEL_PIC_H
#define KERNEL_PIC_H

/* PIC EOI - stub since we use polling, not interrupts */
static inline void pic_send_eoi(uint8_t irq) {
    (void)irq;
}

#endif /* KERNEL_PIC_H */
