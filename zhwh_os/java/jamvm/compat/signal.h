/**
 * compat/signal.h — signal.h 替代
 *
 * HillsonOS 无信号机制。提供空/桩实现。
 */
#ifndef COMPAT_SIGNAL_H
#define COMPAT_SIGNAL_H

#include "compat.h"

typedef int sigset_t;

/* 信号编号（用于编译，实际不使用） */
#define SIGUSR1  10
#define SIGUSR2  12
#define SIGPIPE  13
#define SIG_DFL  ((void(*)(int))0)
#define SIG_IGN  ((void(*)(int))1)

/* sigaction 结构 */
struct sigaction {
    void (*sa_handler)(int);
    sigset_t sa_mask;
    int sa_flags;
};

/* 信号函数桩 */
static inline int sigaction(int sig, struct sigaction *act, struct sigaction *old) {
    (void)sig; (void)act; (void)old; return 0;
}
static inline int sigemptyset(sigset_t *set) {
    *set = 0; return 0;
}
static inline int sigfillset(sigset_t *set) {
    *set = ~0; return 0;
}
static inline int sigdelset(sigset_t *set, int sig) {
    (void)set; (void)sig; return 0;
}
static inline int sigaddset(sigset_t *set, int sig) {
    (void)set; (void)sig; return 0;
}
static inline int sigprocmask(int how, sigset_t *set, sigset_t *old) {
    (void)how; (void)set; (void)old; return 0;
}
static inline int pthread_sigmask(int how, sigset_t *set, sigset_t *old) {
    (void)how; (void)set; (void)old; return 0;
}

/* sigsetjmp/siglongjmp 使用标准 setjmp/longjmp */
typedef unsigned int sigjmp_buf[16];
static inline int sigsetjmp(sigjmp_buf env, int savesigs) {
    (void)savesigs; return 0; /* 简化: 始终返回 0 */
}
static inline void siglongjmp(sigjmp_buf env, int val) {
    (void)env; (void)val; /* 简化: 无操作 */
}

#endif
