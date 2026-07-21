/**
 * jvm.h — JVM 核心接口
 *
 * 定义了 Java 虚拟机的主要数据结构和 API。
 * 实现 Java 虚拟机规范 (SE 8) 的子集。
 *
 * 支持的 Java 版本: 1.1 / 1.4 (class file 45.0 - 48.0)
 * 目标: 能运行基本的命令行 Java 程序
 */
#ifndef JVM_H
#define JVM_H

#include "stdint.h"

/* ================================================================
 * 基本类型
 * ================================================================ */

/* Java 类型映射 */
typedef uint8_t  j_boolean;   /* 0 = false, 非0 = true */
typedef int8_t   j_byte;
typedef uint16_t j_char;      /* UTF-16 */
typedef int16_t  j_short;
typedef int32_t  j_int;
typedef int64_t  j_long;
typedef float    j_float;
typedef double   j_double;

/* 对象引用（当前为直接指针，后续可改为句柄） */
typedef void *j_ref;

/* 数组引用 */
typedef void *j_array;

/* ================================================================
 * Class 文件结构
 * ================================================================ */

/* Class 文件版本 */
#define JAVA_MAGIC           0xCAFEBABE
#define CLASS_VERSION_1_1    45
#define CLASS_VERSION_1_4    48
#define CLASS_VERSION_1_8    52

/* 常量池标签 */
enum {
    CONSTANT_Utf8               = 1,
    CONSTANT_Integer            = 3,
    CONSTANT_Float              = 4,
    CONSTANT_Long               = 5,
    CONSTANT_Double             = 6,
    CONSTANT_Class              = 7,
    CONSTANT_String             = 8,
    CONSTANT_Fieldref           = 9,
    CONSTANT_Methodref          = 10,
    CONSTANT_InterfaceMethodref = 11,
    CONSTANT_NameAndType        = 12,
    CONSTANT_MethodHandle       = 15,
    CONSTANT_MethodType         = 16,
    CONSTANT_InvokeDynamic      = 18,
};

/* 访问标志 */
#define ACC_PUBLIC       0x0001
#define ACC_PRIVATE      0x0002
#define ACC_PROTECTED    0x0004
#define ACC_STATIC       0x0008
#define ACC_FINAL        0x0010
#define ACC_SYNCHRONIZED 0x0020
#define ACC_NATIVE       0x0100
#define ACC_INTERFACE    0x0200
#define ACC_ABSTRACT     0x0400

/* ================================================================
 * JVM 类型描述符
 * ================================================================ */

/* 基本类型编码 */
#define JVM_T_BOOLEAN  4
#define JVM_T_CHAR     5
#define JVM_T_FLOAT    6
#define JVM_T_DOUBLE   7
#define JVM_T_BYTE     8
#define JVM_T_SHORT    9
#define JVM_T_INT      10
#define JVM_T_LONG     11

/* 方法返回值类型 */
typedef enum {
    JVM_RET_VOID = 0,
    JVM_RET_INT,
    JVM_RET_LONG,
    JVM_RET_FLOAT,
    JVM_RET_DOUBLE,
    JVM_RET_REF,
} jvm_ret_type_t;

/* ================================================================
 * JVM 内部结构
 * ================================================================ */

/* 前向声明 */
typedef struct jvm_class_s    jvm_class_t;
typedef struct jvm_method_s   jvm_method_t;
typedef struct jvm_field_s    jvm_field_t;
typedef struct jvm_frame_s    jvm_frame_t;
typedef struct jvm_thread_s   jvm_thread_t;
typedef struct jvm_instance_s jvm_instance_t;

/* 常量池条目 */
typedef struct {
    uint8_t  tag;
    union {
        struct { uint16_t len; char *bytes; } utf8;
        struct { uint32_t value; } integer;
        struct { float value; } float_val;
        struct { uint64_t value; } long_val;
        struct { double value; } double_val;
        struct { uint16_t name_idx; } class;
        struct { uint16_t str_idx; } string;
        struct { uint16_t class_idx; uint16_t nt_idx; } ref;
        struct { uint16_t name_idx; uint16_t desc_idx; } name_and_type;
    } data;
} cp_entry_t;

/* 类结构 */
struct jvm_class_s {
    char             *name;          /* 全限定名 "java/lang/Object" */
    uint8_t          *data;          /* class 文件原始数据 */
    uint32_t          data_len;

    uint16_t          minor_version;
    uint16_t          major_version;

    uint16_t          cp_count;
    cp_entry_t       *constant_pool;

    uint16_t          access_flags;
    uint16_t          this_class;
    uint16_t          super_class;
    uint16_t          interfaces_count;
    uint16_t         *interfaces;

    uint16_t          fields_count;
    jvm_field_t      *fields;

    uint16_t          methods_count;
    jvm_method_t     *methods;

    /* 继承关系 */
    jvm_class_t      *super;         /* 父类 */
    jvm_class_t      *next;          /* 类哈希表链表 */

    /* 状态 */
    uint8_t           initialized;   /* <clinit> 是否已执行 */
    int32_t           instance_size; /* 实例数据大小 */
};

/* 方法结构 */
struct jvm_method_s {
    jvm_class_t  *class;
    uint16_t      access_flags;
    uint16_t      name_idx;
    uint16_t      desc_idx;
    char         *name;
    char         *descriptor;

    uint16_t      max_stack;
    uint16_t      max_locals;
    uint32_t      code_len;
    uint8_t      *code;

    /* 异常处理表 */
    uint16_t      exc_table_len;
    struct {
        uint16_t start_pc;
        uint16_t end_pc;
        uint16_t handler_pc;
        uint16_t catch_type;
    } *exc_table;

    /* 本地方法指针 */
    void (*native_func)(jvm_frame_t *frame);
};

/* 字段结构 */
struct jvm_field_s {
    jvm_class_t  *class;
    uint16_t      access_flags;
    uint16_t      name_idx;
    uint16_t      desc_idx;
    char         *name;
    char         *descriptor;

    /* 静态字段的偏移或值 */
    int32_t       offset;  /* 非静态: 实例中的偏移; 静态: 全局表中的偏移 */
};

/* 栈帧 */
struct jvm_frame_s {
    jvm_frame_t  *prev;           /* 调用者帧 */
    jvm_method_t *method;         /* 当前方法 */
    uint32_t      pc;             /* 程序计数器 */

    /* 局部变量表 */
    uint32_t      locals_count;
    uint64_t     *locals;         /* 双字槽，兼容 long/double */

    /* 操作数栈 */
    uint32_t      stack_capacity;
    uint32_t      stack_top;
    uint64_t     *stack;
};

/* 线程 */
struct jvm_thread_s {
    jvm_frame_t  *current_frame;  /* 栈顶帧 */
    jvm_frame_t  *frame_pool;     /* 帧缓存池 */
    uint8_t       running;
};

/* ================================================================
 * JVM API
 * ================================================================ */

/**
 * 初始化 JVM
 *
 * @param classpath  搜索 class 文件的路径列表
 * @param heap_size  堆大小（字节），0 表示使用默认值
 * @return 0 = 成功, -1 = 失败
 */
int jvm_init(const char *classpath, uint32_t heap_size);

/**
 * 加载并初始化一个类
 *
 * @param class_name  全限定类名，如 "java/lang/Object"
 * @return 类结构指针，失败返回 NULL
 */
jvm_class_t *jvm_load_class(const char *class_name);

/**
 * 查找一个类的静态方法
 *
 * @param class      类指针
 * @param name       方法名
 * @param descriptor 方法描述符
 * @return 方法指针，未找到返回 NULL
 */
jvm_method_t *jvm_find_method(jvm_class_t *class,
                              const char *name, const char *descriptor);

/**
 * 调用静态方法（程序入口）
 *
 * @param method     要调用的方法
 * @param args       参数字符串数组（用于 main(String[] args)）
 * @param arg_count  参数个数
 * @return 返回值（int），或退出码
 */
int jvm_invoke_static(jvm_method_t *method,
                      const char **args, int32_t arg_count);

/**
 * 运行 Java 程序
 *
 * 等价于:
 *   class = jvm_load_class(main_class);
 *   method = jvm_find_method(class, "main", "([Ljava/lang/String;)V");
 *   return jvm_invoke_static(method, argv, argc);
 */
int jvm_run(const char *main_class, const char **argv, int32_t argc);

/**
 * 关闭 JVM，释放所有资源
 */
void jvm_shutdown(void);

#endif /* JVM_H */
