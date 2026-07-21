/**
 * os.c — JamVM HillsonOS 操作系统移植层
 *
 * 提供 JamVM 核心所需的平台特定函数。
 * 参考: jamvm_src/src/os/linux/os.c
 */
#include "../../compat/compat.h"
#include "../../../port/os_port.h"
#include "../../config.h"

/**
 * 获取物理内存大小（字节）
 * HillsonOS: 从内核获取或返回默认值
 */
long long nativePhysicalMemory() {
    /* HillsonOS: 返回 128MB（根据实际内存调整） */
    return 128LL * 1024 * 1024;
}

/**
 * 获取当前线程栈基地址
 * HillsonOS: 返回用户栈顶（链接脚本定义的 _stack_top 符号）
 */
extern char _stack_top;
void *nativeStackBase() {
    return (void*)&_stack_top;
}

/**
 * 获取可用处理器数量
 * HillsonOS: 单核
 */
int nativeAvailableProcessors() {
    return 1;
}

/**
 * 动态库加载
 * HillsonOS: 不支持动态库，使用静态链接 classpath
 */
char *nativeLibPath() {
    return NULL;
}

char *nativeLibError() {
    return "Dynamic loading not supported on HillsonOS";
}

void *nativeLibOpen(char *path) {
    (void)path;
    return NULL;
}

void nativeLibClose(void *handle) {
    (void)handle;
}

void *nativeLibSym(void *handle, char *symbol) {
    (void)handle;
    (void)symbol;
    return NULL;
}

char *nativeLibMapName(char *name) {
    /* 返回静态链接名 */
    unsigned int len = strlen(name);
    char *buff = (char *)malloc(len + 1);
    if (buff) strcpy(buff, name);
    return buff;
}

/**
 * 获取 JVM 自身路径
 * HillsonOS: 返回固定路径
 */
char *nativeJVMPath() {
    char *path = (char *)malloc(32);
    if (path) strcpy(path, "/boot/jvm.elf");
    return path;
}

/**
 * 获取当前工作目录
 */
char *getCwd(void) {
    char *cwd = (char *)malloc(256);
    if (cwd) strcpy(cwd, "/");
    return cwd;
}

/**
 * 获取 Java Home
 */
char *getJavaHome(void) {
    char *home = (char *)malloc(64);
    if (home) strcpy(home, "/java");
    return home;
}
