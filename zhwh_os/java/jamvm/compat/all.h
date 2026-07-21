/**
 * compat/all.h — 统一标准库映射头文件
 *
 * JamVM 源码中使用 #include <stdio.h>, <stdlib.h> 等标准头文件。
 * 通过 -I compat 让编译器优先找到此目录下的替代头文件。
 *
 * 每个标准头文件在此目录下都有对应的替代版本。
 * 此文件列出所有被拦截的标准头文件。
 */

/* 所有标准头文件的兼容替代已在此目录下的对应文件中实现:
 *
 * stdio.h      → compat/stdio.h       (printf, fprintf, sprintf, ...)
 * stdlib.h     → compat/stdlib.h      (malloc, free, atoi, exit, ...)
 * string.h     → compat/string.h      (memcpy, strlen, strcmp, ...)
 * pthread.h    → compat/pthread.h     (单线程桩)
 * signal.h     → compat/signal.h      (空实现)
 * unistd.h     → compat/unistd.h      (sysconf, getpid, sched_yield, ...)
 * sys/mman.h   → compat/sys_mman.h    (mmap→malloc, munmap→free)
 * time.h       → compat/time_compat.h (clock_gettime, nanosleep)
 * errno.h      → compat/errno.h       (errno 变量)
 * dlfcn.h      → compat/dlfnc.h       (空实现)
 * sched.h      → compat/sched.h       (sched_yield)
 * sys/time.h   → compat/time_compat.h (gettimeofday)
 *
 * 使用方法:
 *   在 Makefile 中添加 -I$(JAMVM_DIR)/compat 作为第一个 include 路径。
 */
