/**
 * java_lang.c — java.lang.* 本地方法实现
 *
 * 实现核心 Java 类的 native 方法:
 *   - java.lang.Object
 *   - java.lang.System
 *   - java.lang.String
 *   - java.lang.Thread
 */
#include "jvm.h"
#include "os_port.h"

/* ================================================================
 * java.lang.Object
 * ================================================================ */

/**
 * public native int hashCode();
 */
void java_lang_Object_hashCode(jvm_frame_t *frame) {
    /* 简化实现: 返回对象地址作为哈希码 */
    j_ref obj = (j_ref)(uint32_t)frame->locals[0]; /* this */
    int32_t hash = (int32_t)(uint32_t)obj;
    /* 把返回值压到调用帧 */
    /* push_int(caller_frame, hash); */
    (void)frame;
    (void)obj;
    (void)hash;
}

/* ================================================================
 * java.lang.System
 * ================================================================ */

/**
 * public static native void arraycopy(
 *     Object src, int srcPos, Object dst, int dstPos, int length);
 */
void java_lang_System_arraycopy(jvm_frame_t *frame) {
    /* 参数在局部变量表中:
     *   locals[0]: src (j_ref)
     *   locals[1]: srcPos (int)
     *   locals[2]: dst (j_ref)
     *   locals[3]: dstPos (int)
     *   locals[4]: length (int)
     */
    /* TODO: 实现数组拷贝 */
    (void)frame;
}

/**
 * public static native long currentTimeMillis();
 */
void java_lang_System_currentTimeMillis(jvm_frame_t *frame) {
    /* 返回 os_time_ms() */
    /* push_long(caller_frame, (int64_t)os_time_ms()); */
    (void)frame;
}

/**
 * 注册 java.lang 包的所有本地方法
 */
void java_lang_register_natives(void) {
    os_print("[Classpath] Registering java.lang natives\n");

    /* native_register("java/lang/Object", "hashCode", "()I",
     *                 java_lang_Object_hashCode); */
    /* native_register("java/lang/System", "arraycopy",
     *     "(Ljava/lang/Object;ILjava/lang/Object;II)V",
     *     java_lang_System_arraycopy); */
    /* native_register("java/lang/System", "currentTimeMillis", "()J",
     *     java_lang_System_currentTimeMillis); */
}
