/**
 * @file qplatformdefs.h
 * @brief HillsonOS platform definitions for Qt/Embedded 3.3.8
 *
 * HillsonOS is a freestanding, no-POSIX bare-metal OS:
 * - No unistd.h, fcntl.h, sys/mman.h, etc.
 * - Framebuffer at fixed virtual address (0xF0000000)
 * - Input via int $0x80 syscalls (SYS_GUI_INPUT_READ = 72)
 * - No threads, no shared memory, no sockets, no dynamic libs
 */

#ifndef QPLATFORMDEFS_H
#define QPLATFORMDEFS_H

// Prevent Qt from including POSIX headers
#define QT_NO_COMPAT
// Qt 3 uses this to skip most POSIX code paths
#define Q_OS_HILLSON

// --- Freestanding environment: no standard headers ---
// Qt expects these but we stub them out.

// Types that would come from sys/types.h
typedef int             pid_t;
typedef unsigned int    uid_t;
typedef unsigned int    gid_t;
typedef unsigned int    mode_t;
typedef long            off_t;
typedef int             ssize_t;
typedef unsigned long   size_t;

// Socket types (stubs — no networking via Qt)
typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;

// --- Filesystem stubs (we have ramfs, but Qt uses this minimally) ---
// Qt/Embedded single-process mode skips most file I/O
#ifndef O_RDONLY
#define O_RDONLY    0
#define O_WRONLY    1
#define O_RDWR      2
#define O_CREAT     0100
#define O_TRUNC     01000
#define O_APPEND    02000
#endif

// --- Time stubs ---
struct timeval {
    long tv_sec;
    long tv_usec;
};

struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

// Minimal gettimeofday — always returns 0 (we don't have RTC in userspace)
inline int gettimeofday(struct timeval *tv, struct timezone *) {
    if (tv) { tv->tv_sec = 0; tv->tv_usec = 0; }
    return 0;
}

// --- Signal stubs ---
#define SIG_IGN    ((void(*)(int))1)
#define QT_SIGNAL_RETTYPE   void
#define QT_SIGNAL_ARGS      int
#define QT_SIGNAL_IGNORE    SIG_IGN

// --- Memory allocation (use our bump allocator from cxx_runtime.cpp) ---
#include <stddef.h> // for size_t in our compat headers

// --- Errno stub ---
#define EINVAL  22
#define ENOMEM  12

// --- File descriptor stubs (we don't use fd-based I/O in QWS) ---
// Qt/Embedded in single-process mode doesn't use file descriptors for GUI

// --- Directory stubs (not needed) ---
struct dirent {
    char d_name[256];
};

// --- Assert ---
extern "C" void __assert_fail(const char *, const char *, int, const char *);
#define Q_ASSERT(cond)  ((cond) ? (void)0 : __assert_fail(#cond, __FILE__, __LINE__, __PRETTY_FUNCTION__))

#endif // QPLATFORMDEFS_H
