/**
 * classlib-defs.h — HillsonOS classlib 定义
 *
 * 简化版: 移除 OpenJDK 特有的 padding 和复杂字段。
 */
#ifndef JAMVM_CLASSLIB_DEFS_H
#define JAMVM_CLASSLIB_DEFS_H

/* ClassBlock 填充（无额外 padding） */
#define CLASSLIB_CLASS_PAD

/* Class 特殊标志 */
#define CLASSLIB_CLASS_SPECIAL    0x8000

/* Class 扩展字段 */
#define CLASSLIB_CLASS_EXTRA_FIELDS  \
    Object *protection_domain;       \
    int is_array;

/* ClassBlock 引用标记宏 */
#define CLASSLIB_CLASSBLOCK_REFS_DO(action, cb, ...) \
    action(cb, protection_domain, ## __VA_ARGS__);

/* 数组类扩展字段 */
#define CLASSLIB_ARRAY_CLASS_EXTRA_FIELDS  \
    Class *component_class;
#define CLASSLIB_CLASSBLOCK_ARRAY_REFS_DO(action, cb, ...) \
    action(cb, component_class, ## __VA_ARGS__);

/* 线程扩展字段 */
#define CLASSLIB_THREAD_EXTRA_FIELDS

/* JSR 292 相关 */
#ifdef JSR292
#define ID_invokeGeneric   (MB_PREPARED + 1)
#define ID_invokeBasic     (MB_PREPARED + 2)
#define ID_linkToStatic    (MB_PREPARED + 3)
#define ID_linkToSpecial   (MB_PREPARED + 4)
#define ID_linkToVirtual   (MB_PREPARED + 5)
#define ID_linkToInterface (MB_PREPARED + 6)

#define mbPolymorphicNameID(mb) mb->state

#define CLASSLIB_METHOD_ANNOTATIONS(mb, type_name) {                \
    /* Simplified */                                                 \
}

typedef struct cached_poly_offsets {
    int mem_name_vmtarget;
    int lmda_form_vmentry;
    int mthd_hndl_form;
} CachedPolyOffsets;
#endif /* JSR292 */

#endif /* JAMVM_CLASSLIB_DEFS_H */
