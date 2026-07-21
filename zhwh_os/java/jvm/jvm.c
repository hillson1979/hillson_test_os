/**
 * jvm.c — JVM 初始化和主循环
 *
 * 负责:
 *   - 堆初始化
 *   - 类加载器初始化
 *   - 启动主线程并执行 main() 方法
 */
#include "jvm.h"
#include "os_port.h"
#include "stdint.h"

/* ================================================================
 * 全局 JVM 状态
 * ================================================================ */

typedef struct {
    uint8_t       initialized;

    /* 类加载 */
    char         *classpath;
    jvm_class_t  *class_list;       /* 已加载的类链表 */
    uint32_t      class_count;

    /* 堆 */
    uint8_t      *heap_base;
    uint32_t      heap_size;
    uint32_t      heap_used;

    /* 主线程 */
    jvm_thread_t *main_thread;

    /* 静态字段存储 */
    uint64_t     *static_fields;
    uint32_t      static_field_count;
    uint32_t      static_field_capacity;
} jvm_state_t;

static jvm_state_t jvm;

/* ================================================================
 * 初始化
 * ================================================================ */

int jvm_init(const char *classpath, uint32_t heap_size) {
    if (jvm.initialized) return -1;

    /* 默认堆大小: 4MB */
    if (heap_size == 0) {
        heap_size = 4 * 1024 * 1024;
    }

    /* 分配堆 */
    jvm.heap_base = (uint8_t *)os_malloc(heap_size);
    if (!jvm.heap_base) {
        os_print("[JVM] ERROR: Failed to allocate heap\n");
        return -1;
    }
    jvm.heap_size = heap_size;
    jvm.heap_used = 0;

    /* 保存 classpath */
    jvm.classpath = NULL;
    if (classpath) {
        /* 复制 classpath 字符串 */
        uint32_t len = 0;
        while (classpath[len]) len++;
        jvm.classpath = (char *)os_malloc(len + 1);
        if (jvm.classpath) {
            for (uint32_t i = 0; i <= len; i++) {
                jvm.classpath[i] = classpath[i];
            }
        }
    }

    /* 初始化静态字段表（预分配 256 个槽） */
    jvm.static_field_capacity = 256;
    jvm.static_field_count = 0;
    jvm.static_fields = (uint64_t *)os_malloc(
        jvm.static_field_capacity * sizeof(uint64_t));
    if (!jvm.static_fields) {
        os_print("[JVM] ERROR: Failed to allocate static field table\n");
        os_free(jvm.heap_base);
        return -1;
    }

    /* 创建主线程 */
    jvm.main_thread = (jvm_thread_t *)os_malloc(sizeof(jvm_thread_t));
    if (!jvm.main_thread) {
        os_print("[JVM] ERROR: Failed to create main thread\n");
        os_free(jvm.static_fields);
        os_free(jvm.heap_base);
        return -1;
    }
    jvm.main_thread->current_frame = NULL;
    jvm.main_thread->frame_pool    = NULL;
    jvm.main_thread->running       = 0;

    jvm.initialized = 1;
    os_print("[JVM] Initialized successfully\n");
    os_print("[JVM] Heap: ");
    /* TODO: 打印堆大小的数字 */
    os_print(" bytes\n");

    return 0;
}

void jvm_shutdown(void) {
    if (!jvm.initialized) return;

    /* TODO: 释放所有类、帧、对象 */

    os_free(jvm.main_thread);
    os_free(jvm.static_fields);
    if (jvm.classpath) os_free(jvm.classpath);
    os_free(jvm.heap_base);

    jvm.initialized = 0;
    os_print("[JVM] Shutdown complete\n");
}

/* ================================================================
 * 类加载
 * ================================================================ */

/* 类加载器的简单哈希表 */
#define CLASS_HASH_SIZE  64
static jvm_class_t *class_hash[CLASS_HASH_SIZE];

static uint32_t hash_class_name(const char *name) {
    uint32_t h = 0;
    while (*name) {
        h = h * 31 + (uint8_t)(*name);
        name++;
    }
    return h % CLASS_HASH_SIZE;
}

jvm_class_t *jvm_load_class(const char *class_name) {
    /* 先查哈希表 */
    uint32_t idx = hash_class_name(class_name);
    jvm_class_t *c = class_hash[idx];
    while (c) {
        /* 比较类名 */
        const char *a = class_name;
        const char *b = c->name;
        while (*a && *b && (*a == *b)) { a++; b++; }
        if (*a == 0 && *b == 0) return c; /* 已加载 */
        c = c->next;
    }

    /* 需要加载类 */
    /* TODO: 调用 classfile.c 的解析器从文件系统加载 .class 文件 */
    os_print("[JVM] Loading class: ");
    os_print(class_name);
    os_print("\n");

    return NULL; /* 暂未实现 */
}

/* ================================================================
 * 方法查找
 * ================================================================ */

jvm_method_t *jvm_find_method(jvm_class_t *class,
                              const char *name, const char *descriptor) {
    if (!class || !name) return NULL;

    for (uint16_t i = 0; i < class->methods_count; i++) {
        jvm_method_t *m = &class->methods[i];
        if (!m->name) continue;

        /* 比较方法名 */
        const char *a = name;
        const char *b = m->name;
        while (*a && *b && (*a == *b)) { a++; b++; }
        if (*a != 0 || *b != 0) continue;

        /* 比较描述符 */
        if (descriptor) {
            const char *c = descriptor;
            const char *d = m->descriptor;
            while (*c && *d && (*c == *d)) { c++; d++; }
            if (*c != 0 || *d != 0) continue;
        }

        return m;
    }

    /* 在父类中查找 */
    if (class->super) {
        return jvm_find_method(class->super, name, descriptor);
    }

    return NULL;
}

/* ================================================================
 * 静态方法调用
 * ================================================================ */

int jvm_invoke_static(jvm_method_t *method,
                      const char **args, int32_t arg_count) {
    if (!method) return -1;

    os_print("[JVM] Invoking: ");
    os_print(method->name);
    os_print("\n");

    /* TODO: 创建栈帧，开始解释执行 */
    (void)args;
    (void)arg_count;

    return 0;
}

/* ================================================================
 * 运行 Java 程序
 * ================================================================ */

int jvm_run(const char *main_class, const char **argv, int32_t argc) {
    if (!jvm.initialized) return -1;

    /* 加载主类 */
    jvm_class_t *class = jvm_load_class(main_class);
    if (!class) {
        os_print("[JVM] ERROR: Class not found: ");
        os_print(main_class);
        os_print("\n");
        return -1;
    }

    /* 查找 main(String[] args) 方法 */
    jvm_method_t *main_method = jvm_find_method(class, "main",
                                                 "([Ljava/lang/String;)V");
    if (!main_method) {
        os_print("[JVM] ERROR: main() method not found in ");
        os_print(main_class);
        os_print("\n");
        return -1;
    }

    /* 调用 main */
    return jvm_invoke_static(main_method, argv, argc);
}
