// Mutual exclusion lock.
#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

struct spinlock {
  uint32_t locked;       // Is the lock held?

  // For debugging:
  char *name;        // Name of lock.
  struct cpu *cpu;   // The cpu holding the lock.
  uint32_t pcs[10];      // The call stack (an array of program counters)
                     // that locked the lock.
};

typedef struct spinlock spinlock_t;

// Spinlock initializer
#define SPINLOCK_INITIALIZER { 0, "", 0, {0} }

// Spinlock functions
void initlock(struct spinlock *lk, char *name);
void acquire(struct spinlock *lk);
void release(struct spinlock *lk);
int holding(struct spinlock *lock);
void pushcli(void);
void popcli(void);

#endif /* __SPINLOCK_H__ */

