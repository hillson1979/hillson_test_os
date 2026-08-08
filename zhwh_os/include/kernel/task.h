/**
 * @file task.h
 *
 */
#ifndef KERNEL_TASK_H
#define KERNEL_TASK_H

#include <spinlock.h>
#include <stdint.h>

/* Forward declaration for task_t (not used directly by USB driver) */
struct task_t;

/* PIT frequency (ticks per second) */
#define PIT_FREQUENCY  100

/* Kernel task priority */
#define KERNEL_TASK_ELEVATED_PRIORITY  1

/* Global tick counter */
extern uint32_t ticks;

/*
 * Mutex compatibility:
 * Hillson: spinlock_t + acquire/release
 *
 * We map kernel_mutex_t directly to spinlock_t.
 * This is safe because USB driver mutex usages are short (protecting lists).
 *
 * primitive, but maps similarly to a spinlock for our purposes.
 */
typedef spinlock_t kernel_mutex_t;

#define KERNEL_MUTEX_INITIALIZER  SPINLOCK_INITIALIZER

static inline void kernel_mutex_lock(volatile kernel_mutex_t *lk) {
    acquire((spinlock_t *)(uintptr_t)lk);
}
static inline void kernel_mutex_unlock(volatile kernel_mutex_t *lk) {
    release((spinlock_t *)(uintptr_t)lk);
}
static inline void kernel_mutex_lock_infinite_wait(volatile kernel_mutex_t *lk) {
    acquire((spinlock_t *)(uintptr_t)lk);
}
static inline int kernel_mutex_trylock(volatile kernel_mutex_t *lk) {
    /* spinlocks don't have trylock; always lock */
    acquire((spinlock_t *)(uintptr_t)lk);
    return 1;
}

static inline void elevated_priority_lock(volatile kernel_mutex_t *lk) {
    acquire((spinlock_t *)(uintptr_t)lk);
}
static inline void elevated_priority_unlock(volatile kernel_mutex_t *lk) {
    release((spinlock_t *)(uintptr_t)lk);
}

/*
 * Task management stubs.
 *
 * that runs every 10ms to check interrupt transfers and every second
 * to check for hotplug events.
 *
 * In Hillson OS, we don't have kthreads. Instead, usb_poll() is called
 * from the main kernel loop or from a timer callback.
 */

/* start_kernel_task stub — always succeeds but never actually runs the function */
static inline int start_kernel_task(const char *name,
                                     void (*func)(void *),
                                     void *arg,
                                     volatile void **task_out,
                                     int priority) {
    (void)name; (void)func; (void)arg; (void)priority;
    if (task_out) *task_out = NULL;
    return 1; /* "success" — task is not actually started */
}

static inline void block_task_timeout(struct task_t *task, int ms) {
    (void)task; (void)ms;
}

static inline void set_task_waitchan(struct task_t *task, volatile void *chan) {
    (void)task; (void)chan;
}

static inline void unblock_kernel_task(struct task_t *task) {
    (void)task;
}

static inline void set_task_waking_signal(struct task_t *task, int sig) {
    (void)task; (void)sig;
}

struct cpu_core {
    struct task_t *cur_task;
};
extern struct cpu_core cpu0;
#define this_core  (&cpu0)

#endif /* KERNEL_TASK_H */
