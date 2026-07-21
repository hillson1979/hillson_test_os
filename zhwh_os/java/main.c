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
 * _start — ELF 入口点 (在 .text.start 段, 确保被链接器放在最前面)
 */
__asm__(".section .text.start\n"
        ".global _start\n"
        "_start:\n"
        "    movl 4(%esp), %eax\n"   /* argc */
        "    leal 8(%esp), %edx\n"   /* argv */
        "    pushl %edx\n"
        "    pushl %eax\n"
        "    call  main\n"
        "    pushl %eax\n"
        "    call  exit\n"
        "    hlt\n"
        ".text\n");

extern void exit(int code);

/**
 * main — 程序入口，由 _start 调用
 */
int main(int argc, const char **argv) {
    (void)argc; (void)argv;

    os_print("\n");
    os_print("========================================\n");
    os_print("  HillsonOS Java Virtual Machine v0.1  \n");
    os_print("========================================\n\n");

    const char *main_class = "HelloWorld";

    os_print("[JVM] Initializing...\n");
    if (jvm_init("/java/classes", 3*1024*1024) != 0) {
        os_print("[JVM] Fatal: Failed to initialize JVM\n");
        return 1;
    }

    native_init();
    java_lang_register_natives();
    java_io_register_natives();
    java_net_register_natives();

    os_print("[JVM] Running: ");
    os_print(main_class);
    os_print("\n");
    int result = jvm_run(main_class, NULL, 0);

    jvm_shutdown();
    os_print("\n[JVM] Exit.\n");
    return result;
}
