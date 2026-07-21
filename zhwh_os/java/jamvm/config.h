/**
 * config.h — JamVM 编译配置（手动生成，替代 autotools configure）
 *
 * HillsonOS 移植配置: i386, 单线程模式, 无动态库, 无信号
 */
#ifndef JAMVM_CONFIG_H
#define JAMVM_CONFIG_H

/* ================================================================
 * 操作系统和架构
 * ================================================================ */
#define OS_HILLSON  1
#define ARCH_i386   1
#define OS_ARCH     "i386"

/* ================================================================
 * 功能开关 — 适配 HillsonOS 环境
 * ================================================================ */

/* 无 POSIX 线程 → 单线程模式 */
/* #undef HAVE_PTHREAD */

/* 无 mmap → 使用 malloc 分配堆 */
/* #undef HAVE_MMAP */

/* 无信号 → 禁止线程挂起/恢复 */
/* #undef HAVE_SIGNAL */

/* 无动态链接 → 静态链接 classpath */
/* #undef HAVE_DLOPEN */

/* 无 libffi → 手写 native 调用桩 */
/* #undef HAVE_FFI */

/* 编译器功能 */
#define HAVE_GCC_INLINE      1
#define HAVE_GCC_INLINE_ASM  1
#define HAVE_TLS             0   /* 无线程局部存储 */

/* C 标准库功能（简化） */
#define HAVE_FENV_H          0
#define HAVE_FPU_CONTROL_H   0
#define HAVE_INTTYPES_H      1
#define HAVE_STDINT_H        1
#define HAVE_STDLIB_H        1
#define HAVE_STRING_H        1
#define HAVE_STDIO_H         1
#define HAVE_STDARG_H        1
#define HAVE_LIMITS_H        1
#define HAVE_TIME_H          1

/* 内存屏障 */
#define HAVE_MBARRIER        1

/* CAS 指令 */
#define USE_CMPXCHG8B        0  /* i386 不一定有 cmpxchg8b */

/* ================================================================
 * 解释器配置
 * ================================================================ */

/* 使用间接线程解释器（switch-based, 更可移植） */
#define DIRECT               0
#define INLINING             0

/* 不启用调试跟踪 */
#define TRACEGC              0
#define TRACEALLOC           0
#define TRACEFNLZ            0
#define TRACEDLL             0
#define TRACELOCK            0
#define TRACETHREAD          0
#define TRACECOMPACT         0
#define TRACEDIRECT          0

/* 不启用性能分析桩 */
#define HAVE_PROFILE_STUBS   0

/* ================================================================
 * Java 标准支持
 * ================================================================ */
#define JSR292               1  /* invokedynamic */
#define JSR308               0  /* Type Annotations */
#define JSR335               0  /* Lambda */
#define JSR901               0  /* Method Parameters */

/* ================================================================
 * 堆配置
 * ================================================================ */
#define DEFAULT_MIN_HEAP     (4 * 1024 * 1024)   /* 4MB */
#define DEFAULT_MAX_HEAP     (64 * 1024 * 1024)  /* 64MB */
#define MIN_MIN_HEAP         (1 * 1024 * 1024)   /* 1MB */
#define MAX_MAX_HEAP         (128 * 1024 * 1024) /* 128MB */
#define DEFAULT_STACK        (128 * 1024)        /* 128KB */
#define STACK_RED_ZONE_SIZE  (1 * 1024)          /* 1KB */

/* ================================================================
 * Classpath 配置
 * ================================================================ */
#define CLASSLIB_CLASS_PAD   /* 空 — 不需要额外填充 */
#define CLASSLIB_CLASS_SPECIAL    0x8000
#define CLASSLIB_ARRAY_CLASS_EXTRA_FIELDS
#define CLASSLIB_CLASS_EXTRA_FIELDS

/* ================================================================
 * 特定于 HillsonOS 的宏
 * ================================================================ */

/* 标准库函数替代（在 compat/ 中实现） */
extern int jam_printf(const char *fmt, ...);
extern int jam_fprintf(void *stream, const char *fmt, ...);
extern void jam_exit(int status);
extern void *jam_malloc(unsigned int n);
extern void jam_free(void *ptr);
extern void *jam_realloc(void *ptr, unsigned int n);

#define printf    jam_printf
#define fprintf   jam_fprintf
#define sprintf   jam_sprintf  /* TODO */
#define snprintf  jam_snprintf /* TODO */
#define puts(s)   jam_print(s)
#define fputs(s,f) jam_print(s)

/* 内存分配映射 */
#define malloc(n)      jam_malloc(n)
#define free(p)        jam_free(p)
#define realloc(p,n)   jam_realloc(p,n)
#define calloc(n,s)    jam_calloc(n,s)

/* 文件操作映射 */
#define FILE void

/* 时间类型 */
struct timespec {
    long tv_sec;
    long tv_nsec;
};

/* va_list 类型（GCC 内置） */

/* NULL 定义 */
#ifndef NULL
#define NULL ((void*)0)
#endif

/* stdout/stderr 占位 */
extern void *stdout;
extern void *stderr;

#endif /* JAMVM_CONFIG_H */
