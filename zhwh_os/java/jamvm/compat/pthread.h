/**
 * compat/pthread.h — pthread.h 替代（单线程模式）
 *
 * JamVM 单线程移植：所有 pthread API 映射为单线程实现。
 * 互斥锁、条件变量在单线程模式下为空操作。
 */
#ifndef COMPAT_PTHREAD_H
#define COMPAT_PTHREAD_H

#include "compat.h"

/* ================================================================
 * 类型定义
 * ================================================================ */

typedef unsigned int pthread_t;
typedef struct { volatile int locked; } pthread_mutex_t;
typedef struct { volatile int signaled; } pthread_cond_t;
typedef struct { int dummy; } pthread_attr_t;
typedef struct { int dummy; } pthread_key_t;

/* ================================================================
 * 线程
 * ================================================================ */

#define pthread_self()           ((pthread_t)1)
#define pthread_create(t,a,f,arg) (-1) /* 不支持创建线程 */
#define pthread_join(t,rv)       (0)
#define pthread_kill(t,sig)      (0)
#define pthread_sigmask(how,set,old) (0)
#define pthread_getspecific(k)   (NULL)
#define pthread_setspecific(k,v) (0)
#define pthread_key_create(k,d)  (0)

/* ================================================================
 * 互斥锁（单线程空操作）
 * ================================================================ */

#define PTHREAD_MUTEX_INITIALIZER {0}

static inline int pthread_mutex_init(pthread_mutex_t *m, void *attr) {
    m->locked = 0; (void)attr; return 0;
}
static inline int pthread_mutex_lock(pthread_mutex_t *m) {
    m->locked = 1; return 0;
}
static inline int pthread_mutex_unlock(pthread_mutex_t *m) {
    m->locked = 0; return 0;
}
static inline int pthread_mutex_destroy(pthread_mutex_t *m) {
    (void)m; return 0;
}

/* ================================================================
 * 条件变量（单线程空操作）
 * ================================================================ */

#define PTHREAD_COND_INITIALIZER {0}

static inline int pthread_cond_init(pthread_cond_t *c, void *attr) {
    c->signaled = 0; (void)attr; return 0;
}
static inline int pthread_cond_wait(pthread_cond_t *c, pthread_mutex_t *m) {
    c->signaled = 0; m->locked = 1; return 0;
}
static inline int pthread_cond_timedwait(pthread_cond_t *c,
                                          pthread_mutex_t *m, void *ts) {
    (void)c; (void)m; (void)ts; return 0;
}
static inline int pthread_cond_signal(pthread_cond_t *c) {
    c->signaled = 1; return 0;
}
static inline int pthread_cond_broadcast(pthread_cond_t *c) {
    c->signaled = 1; return 0;
}
static inline int pthread_cond_destroy(pthread_cond_t *c) {
    (void)c; return 0;
}

/* ================================================================
 * 线程属性
 * ================================================================ */

static inline int pthread_attr_init(pthread_attr_t *attr) {
    (void)attr; return 0;
}
static inline int pthread_attr_setstacksize(pthread_attr_t *attr, size_t sz) {
    (void)attr; (void)sz; return 0;
}
static inline int pthread_attr_setguardsize(pthread_attr_t *attr, size_t sz) {
    (void)attr; (void)sz; return 0;
}
static inline int pthread_attr_getstack(pthread_attr_t *attr,
                                         void **addr, size_t *size) {
    (void)attr; *addr = NULL; *size = 0; return 0;
}
#define pthread_getattr_np(thread, attr) (0)

#endif /* COMPAT_PTHREAD_H */
