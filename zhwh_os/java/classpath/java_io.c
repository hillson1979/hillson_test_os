/**
 * java_io.c — java.io.* 本地方法实现
 *
 * 实现 I/O 相关本地方法:
 *   - java.io.FileInputStream / FileOutputStream
 *   - java.io.PrintStream (System.out / System.err)
 *
 * 映射到 HillsonOS 文件系统调用。
 */
#include "jvm.h"
#include "os_port.h"

/**
 * 注册 java.io 包的所有本地方法
 */
void java_io_register_natives(void) {
    os_print("[Classpath] Registering java.io natives\n");
    /* TODO: 实现 */
}
