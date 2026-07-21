/**
 * compat/sys_sysctl.h — sys/sysctl.h 和 sys/sysinfo.h 替代
 */
#ifndef COMPAT_SYS_SYSCTL_H
#define COMPAT_SYS_SYSCTL_H

/* get_nprocs — 返回处理器数量 */
static inline int get_nprocs(void) { return 1; }

#endif
