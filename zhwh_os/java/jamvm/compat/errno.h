/**
 * compat/errno.h — errno.h 替代
 */
#ifndef COMPAT_ERRNO_H
#define COMPAT_ERRNO_H

extern int jam_errno;
#define errno jam_errno

#define EINVAL  22
#define ENOMEM  12
#define EAGAIN  11
#define EACCES  13
#define ENOENT  2
#define EIO     5

#endif
