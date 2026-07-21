/**
 * compat/dlfcn.h — dlfcn.h 替代（无动态链接支持）
 */
#ifndef COMPAT_DLFCN_H
#define COMPAT_DLFCN_H

#include "compat.h"

#define RTLD_LAZY  1
#define RTLD_NOW   2

/* Dl_info 结构 */
typedef struct {
    const char *dli_fname;
    void       *dli_fbase;
    const char *dli_sname;
    void       *dli_saddr;
} Dl_info;

static inline void *dlopen(const char *file, int mode) {
    (void)file; (void)mode; return NULL;
}
static inline void *dlsym(void *handle, const char *name) {
    (void)handle; (void)name; return NULL;
}
static inline int dlclose(void *handle) {
    (void)handle; return -1;
}
static inline char *dlerror(void) {
    return "Dynamic loading not supported on HillsonOS";
}
static inline int dladdr(const void *addr, Dl_info *info) {
    (void)addr;
    if (info) {
        info->dli_fname = "/boot/jvm.elf";
        info->dli_fbase = NULL;
        info->dli_sname = NULL;
        info->dli_saddr = NULL;
    }
    return 0;
}

#endif
