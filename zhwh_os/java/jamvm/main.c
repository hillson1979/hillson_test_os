/**
 * main.c — JamVM on HillsonOS 入口点
 *
 * 初始化 JVM 并运行指定的主类。
 *
 * 用法:
 *   jamvm.elf <main_class> [args...]
 *   jamvm.elf -cp <classpath> <main_class> [args...]
 */
#include "config.h"
#include "compat/compat.h"
#include "../port/os_port.h"

/* JamVM 内部函数声明 */
extern int jamvm_main(int argc, char *argv[]);

/* 错误变量 */
int jam_errno = 0;

/**
 * jam_exit — 退出 VM
 */
void jam_exit(int status) {
    (void)status;
    os_print("\n[JamVM] Exiting...\n");
    /* TODO: 调用 shutdownVM() */
    sys_exit(status);
}

/**
 * 主入口
 */
int main(int argc, char *argv[]) {
    os_print("\n");
    os_print("========================================\n");
    os_print("  HillsonOS JamVM 2.0 Port             \n");
    os_print("  Lightweight Java Virtual Machine      \n");
    os_print("========================================\n\n");

    if (argc < 2) {
        os_print("Usage: jamvm.elf [options] <main_class> [args...]\n");
        os_print("\n");
        os_print("Options:\n");
        os_print("  -cp <path>    Set classpath\n");
        os_print("  -Xms<size>    Set initial heap size\n");
        os_print("  -Xmx<size>    Set maximum heap size\n");
        os_print("\n");
        os_print("Example:\n");
        os_print("  jamvm.elf HelloWorld\n");
        os_print("  jamvm.elf -cp /java/classes HelloWorld\n");
        return 1;
    }

    os_print("[JamVM] Initializing...\n");

    /* 调用 JamVM 的初始化 */
    int result = jamvm_main(argc, argv);

    os_print("\n[JamVM] Exit code: ");
    /* TODO: 打印数字 */
    os_print("\n");

    return result;
}
