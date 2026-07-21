/**
 * dll_md.c — JamVM HillsonOS i386 动态库桩代码
 *
 * HillsonOS 上使用静态链接，此文件提供空实现。
 * 参考: jamvm_src/src/os/linux/i386/dll_md.c
 */
#include "../../../config.h"

/* 静态链接模式下不需要动态库加载功能 */
/* 所有 nativeLib* 函数已在 os.c 中实现 */
