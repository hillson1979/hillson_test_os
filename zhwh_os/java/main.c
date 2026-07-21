/**
 * main.c — JVM 入口点
 *
 * 用法（在 HillsonOS shell 中）:
 *   jvm.elf <classpath> <main_class> [args...]
 *
 * 示例:
 *   jvm.elf /java/classes HelloWorld
 *   jvm.elf /java/classes com.example.MyApp arg1 arg2
 */
#include "jvm.h"
#include "os_port.h"
#include "stdint.h"

/* 前向声明 */
extern void native_init(void);
extern void java_lang_register_natives(void);
extern void java_io_register_natives(void);
extern void java_net_register_natives(void);

/* 默认堆大小: 4MB */
#define DEFAULT_HEAP_SIZE  (4 * 1024 * 1024)

/**
 * 程序入口 — 由 _start 调用
 */
int main(int argc, const char **argv) {
    os_print("\n");
    os_print("========================================\n");
    os_print("  HillsonOS Java Virtual Machine v0.1  \n");
    os_print("========================================\n\n");

    /* 参数检查 */
    if (argc < 2) {
        os_print("Usage: jvm.elf <main_class> [args...]\n");
        os_print("  default classpath: /java/classes\n\n");
        os_print("Example:\n");
        os_print("  jvm.elf HelloWorld\n");
        os_print("  jvm.elf com.example.MyApp arg1 arg2\n");
        return 1;
    }

    const char *main_class = argv[1];
    const char *classpath = "/java/classes"; /* 默认 classpath */

    /* 初始化 JVM */
    os_print("[JVM] Initializing...\n");
    if (jvm_init(classpath, DEFAULT_HEAP_SIZE) != 0) {
        os_print("[JVM] Fatal: Failed to initialize JVM\n");
        return 1;
    }

    /* 注册本地方法 */
    native_init();
    java_lang_register_natives();
    java_io_register_natives();
    java_net_register_natives();

    /* 运行主类 */
    os_print("[JVM] Running: ");
    os_print(main_class);
    os_print("\n\n");

    int result = jvm_run(main_class, &argv[2], argc - 2);

    /* 关闭 */
    jvm_shutdown();

    os_print("\n[JVM] Exit code: ");
    /* TODO: 打印十进制数字 */
    os_print("\n");

    return result;
}
