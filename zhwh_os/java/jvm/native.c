/**
 * native.c — 本地方法注册和调用
 *
 * 管理 Java native 方法到 C 函数的映射。
 * 每个被 @native 标记的 Java 方法需要一个对应的 C 实现。
 *
 * 注册表按类名+方法名+描述符索引。
 */
#include "jvm.h"
#include "os_port.h"

/* ================================================================
 * 本地方法注册表
 * ================================================================ */

#define MAX_NATIVE_METHODS  256

typedef struct {
    const char *class_name;      /* "java/lang/Object" */
    const char *method_name;     /* "hashCode" */
    const char *descriptor;      /* "()I" */
    void (*func)(jvm_frame_t *); /* C 实现函数 */
} native_entry_t;

static native_entry_t native_table[MAX_NATIVE_METHODS];
static uint32_t native_count = 0;

/* ================================================================
 * 注册 API
 * ================================================================ */

/**
 * 注册一个本地方法
 */
int native_register(const char *class_name,
                    const char *method_name,
                    const char *descriptor,
                    void (*func)(jvm_frame_t *)) {
    if (native_count >= MAX_NATIVE_METHODS) {
        os_print("[Native] ERROR: Native method table full\n");
        return -1;
    }

    native_entry_t *entry = &native_table[native_count++];
    entry->class_name  = class_name;
    entry->method_name = method_name;
    entry->descriptor  = descriptor;
    entry->func        = func;

    return 0;
}

/**
 * 查找本地方法实现
 */
void *native_lookup(const char *class_name,
                    const char *method_name,
                    const char *descriptor) {
    for (uint32_t i = 0; i < native_count; i++) {
        native_entry_t *entry = &native_table[i];

        /* 比较类名 */
        const char *a = class_name;
        const char *b = entry->class_name;
        while (*a && *b && (*a == *b)) { a++; b++; }
        if (*a != 0 || *b != 0) continue;

        /* 比较方法名 */
        a = method_name;
        b = entry->method_name;
        while (*a && *b && (*a == *b)) { a++; b++; }
        if (*a != 0 || *b != 0) continue;

        /* 比较描述符 */
        a = descriptor;
        b = entry->descriptor;
        while (*a && *b && (*a == *b)) { a++; b++; }
        if (*a != 0 || *b != 0) continue;

        return (void *)entry->func;
    }

    return NULL;
}

/* ================================================================
 * 调用本地方法
 * ================================================================ */

/**
 * 调用已注册的本地方法
 *
 * @param frame   当前栈帧
 * @param method  要调用的方法
 * @return 0 = 成功, -1 = 未找到本地实现
 */
int native_invoke(jvm_frame_t *frame, jvm_method_t *method) {
    if (!method || !method->class || !method->class->name) {
        return -1;
    }

    /* 如果方法有已绑定的 native_func，直接调用 */
    if (method->native_func) {
        method->native_func(frame);
        return 0;
    }

    /* 否则查注册表 */
    void *func = native_lookup(method->class->name,
                               method->name, method->descriptor);
    if (func) {
        ((void (*)(jvm_frame_t *))func)(frame);
        return 0;
    }

    os_print("[Native] WARNING: Unsatisfied native method: ");
    os_print(method->class->name);
    os_print(".");
    os_print(method->name);
    os_print("\n");
    return -1;
}

/* ================================================================
 * 初始化
 * ================================================================ */

/**
 * 初始化本地方法表（注册 java.lang.* 等系统类的本地方法）
 */
void native_init(void) {
    native_count = 0;

    /* java.lang.Object */
    /* native_register("java/lang/Object", "hashCode", "()I", ...); */
    /* native_register("java/lang/Object", "getClass", "()Ljava/lang/Class;", ...); */

    /* java.lang.System */
    /* native_register("java/lang/System", "arraycopy", "...", ...); */
    /* native_register("java/lang/System", "currentTimeMillis", "()J", ...); */

    /* java.lang.Thread */
    /* native_register("java/lang/Thread", "start", "()V", ...); */

    os_print("[Native] Initialized (0 native methods registered)\n");
}
