/**
 * compat/sched.h — sched.h 替代
 */
#ifndef COMPAT_SCHED_H
#define COMPAT_SCHED_H

static inline int sched_yield(void) { return 0; }

#endif
