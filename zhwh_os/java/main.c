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
        "    movl (%esp), %eax\n"    /* argc at ESP+0 */
        "    leal 4(%esp), %edx\n"   /* argv at ESP+4 */
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
    const char *main_class = (argc > 1 && argv[1]) ? argv[1] : "HelloWorld";
    (void)argc;
    if (jvm_init("/java/classes", 3*1024*1024) != 0) return 1;
    native_init();
    java_lang_register_natives();
    java_io_register_natives();
    java_net_register_natives();
    int result = jvm_run(main_class, NULL, 0);
    jvm_shutdown();
    return result;
}
