/**
 * java_net.c — java.net.* 本地方法实现
 *
 * 实现网络相关本地方法:
 *   - java.net.Socket
 *   - java.net.ServerSocket
 *
 * 映射到 HillsonOS 网络协议栈。
 */
#include "jvm.h"
#include "os_port.h"

/**
 * 注册 java.net 包的所有本地方法
 */
void java_net_register_natives(void) {
    os_print("[Classpath] Registering java.net natives\n");
    /* TODO: 实现 */
}
