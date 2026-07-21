/**
 * classfile.c — Java Class 文件解析器
 *
 * 负责解析 .class 文件格式:
 *   - 读取魔数 (0xCAFEBABE) 和版本号
 *   - 解析常量池
 *   - 解析类/字段/方法信息
 *   - 解析 Code 属性（字节码）
 *
 * 参考: Java虚拟机规范 — Chapter 4: The class File Format
 */
#include "jvm.h"
#include "os_port.h"

/* ================================================================
 * 读取辅助函数
 * ================================================================ */

static inline uint8_t read_u1(const uint8_t **p) {
    uint8_t v = (*p)[0];
    *p += 1;
    return v;
}

static inline uint16_t read_u2(const uint8_t **p) {
    uint16_t v = ((uint16_t)(*p)[0] << 8) | (*p)[1];
    *p += 2;
    return v;
}

static inline uint32_t read_u4(const uint8_t **p) {
    uint32_t v = ((uint32_t)(*p)[0] << 24) | ((uint32_t)(*p)[1] << 16) |
                 ((uint32_t)(*p)[2] << 8)  | (*p)[3];
    *p += 4;
    return v;
}

/* ================================================================
 * 常量池解析
 * ================================================================ */

static int parse_constant_pool(const uint8_t **data, jvm_class_t *class) {
    uint16_t count = read_u2(data);
    class->cp_count = count;

    if (count == 0) return 0;

    class->constant_pool = (cp_entry_t *)os_malloc(count * sizeof(cp_entry_t));
    if (!class->constant_pool) return -1;

    /* 常量池索引从 1 开始 */
    for (uint16_t i = 1; i < count; i++) {
        cp_entry_t *entry = &class->constant_pool[i];
        entry->tag = read_u1(data);

        switch (entry->tag) {
        case CONSTANT_Utf8: {
            uint16_t len = read_u2(data);
            entry->data.utf8.len = len;
            entry->data.utf8.bytes = (char *)os_malloc(len + 1);
            if (entry->data.utf8.bytes) {
                for (uint16_t j = 0; j < len; j++) {
                    entry->data.utf8.bytes[j] = (char)read_u1(data);
                }
                entry->data.utf8.bytes[len] = '\0';
            }
            break;
        }
        case CONSTANT_Integer:
            entry->data.integer.value = read_u4(data);
            break;
        case CONSTANT_Float:
            entry->data.float_val.value = *(float*)&(uint32_t){read_u4(data)};
            break;
        case CONSTANT_Long:
            entry->data.long_val.value =
                ((uint64_t)read_u4(data) << 32) | read_u4(data);
            i++; /* long/double 占两个常量池槽 */
            break;
        case CONSTANT_Double: {
            uint64_t hi = read_u4(data);
            uint64_t lo = read_u4(data);
            uint64_t bits = (hi << 32) | lo;
            entry->data.double_val.value = *(double*)&bits;
            i++;
            break;
        }
        case CONSTANT_Class:
            entry->data.class.name_idx = read_u2(data);
            break;
        case CONSTANT_String:
            entry->data.string.str_idx = read_u2(data);
            break;
        case CONSTANT_Fieldref:
        case CONSTANT_Methodref:
        case CONSTANT_InterfaceMethodref:
            entry->data.ref.class_idx = read_u2(data);
            entry->data.ref.nt_idx = read_u2(data);
            break;
        case CONSTANT_NameAndType:
            entry->data.name_and_type.name_idx = read_u2(data);
            entry->data.name_and_type.desc_idx = read_u2(data);
            break;
        default:
            os_print("[ClassFile] WARNING: Unknown constant pool tag: ");
            /* TODO: print number */
            os_print("\n");
            break;
        }
    }

    return 0;
}

/* ================================================================
 * 属性解析
 * ================================================================ */

static int parse_attributes(const uint8_t **data, jvm_method_t *method) {
    uint16_t attr_count = read_u2(data);

    for (uint16_t i = 0; i < attr_count; i++) {
        uint16_t name_idx = read_u2(data);
        uint32_t attr_len = read_u4(data);

        /* 获取属性名 */
        cp_entry_t *name_entry = &method->class->constant_pool[name_idx];
        const char *attr_name = name_entry->data.utf8.bytes;

        /* 检查属性名长度 */
        int is_code = 1;
        const char *code_str = "Code";
        for (int j = 0; j < 4; j++) {
            if (!attr_name || attr_name[j] != code_str[j]) {
                is_code = 0;
                break;
            }
        }

        if (is_code && attr_name && attr_name[4] == '\0') {
            /* Code 属性 */
            method->max_stack = read_u2(data);
            method->max_locals = read_u2(data);

            uint32_t code_len = read_u4(data);
            method->code_len = code_len;
            method->code = (uint8_t *)os_malloc(code_len);
            if (method->code) {
                for (uint32_t j = 0; j < code_len; j++) {
                    method->code[j] = read_u1(data);
                }
            }

            /* 异常处理表 */
            uint16_t exc_count = read_u2(data);
            method->exc_table_len = exc_count;
            if (exc_count > 0) {
                method->exc_table = (void *)os_malloc(
                    exc_count * sizeof(*method->exc_table));
                for (uint16_t j = 0; j < exc_count; j++) {
                    method->exc_table[j].start_pc   = read_u2(data);
                    method->exc_table[j].end_pc     = read_u2(data);
                    method->exc_table[j].handler_pc = read_u2(data);
                    method->exc_table[j].catch_type = read_u2(data);
                }
            }

            /* Code 属性内部可能还有子属性 */
            uint16_t sub_attr_count = read_u2(data);
            for (uint16_t j = 0; j < sub_attr_count; j++) {
                read_u2(data); /* 跳过 name_idx */
                uint32_t sub_len = read_u4(data);
                *data += sub_len; /* 跳过数据 */
            }
        } else {
            /* 跳过其他属性 */
            *data += attr_len;
        }
    }

    return 0;
}

/* ================================================================
 * 主解析函数
 * ================================================================ */

/**
 * 从内存中的 class 文件数据解析类结构
 *
 * @param class  预分配的类结构
 * @param data   class 文件原始数据
 * @param len    数据长度
 * @return 0 = 成功
 */
int classfile_parse(jvm_class_t *class, const uint8_t *data, uint32_t len) {
    const uint8_t *p = data;
    const uint8_t *end = data + len;

    class->data = (uint8_t *)data;
    class->data_len = len;

    /* --- 魔数 --- */
    uint32_t magic = read_u4(&p);
    if (magic != JAVA_MAGIC) {
        os_print("[ClassFile] ERROR: Invalid magic number\n");
        return -1;
    }

    /* --- 版本号 --- */
    class->minor_version = read_u2(&p);
    class->major_version = read_u2(&p);

    os_print("[ClassFile] Class version: ");
    /* TODO: print version */
    os_print("\n");

    /* 检查版本兼容性（我们支持最高 Java 8） */
    if (class->major_version > CLASS_VERSION_1_8) {
        os_print("[ClassFile] WARNING: Class version newer than Java 8\n");
    }

    /* --- 常量池 --- */
    if (parse_constant_pool(&p, class) != 0) {
        os_print("[ClassFile] ERROR: Failed to parse constant pool\n");
        return -1;
    }

    /* --- 访问标志 --- */
    class->access_flags = read_u2(&p);

    /* --- 本类 --- */
    class->this_class = read_u2(&p);

    /* --- 父类 --- */
    class->super_class = read_u2(&p);

    /* --- 接口 --- */
    class->interfaces_count = read_u2(&p);
    if (class->interfaces_count > 0) {
        class->interfaces = (uint16_t *)os_malloc(
            class->interfaces_count * sizeof(uint16_t));
        for (uint16_t i = 0; i < class->interfaces_count; i++) {
            class->interfaces[i] = read_u2(&p);
        }
    }

    /* --- 字段 --- */
    class->fields_count = read_u2(&p);
    if (class->fields_count > 0) {
        class->fields = (jvm_field_t *)os_malloc(
            class->fields_count * sizeof(jvm_field_t));
        for (uint16_t i = 0; i < class->fields_count; i++) {
            jvm_field_t *f = &class->fields[i];
            f->class        = class;
            f->access_flags = read_u2(&p);
            f->name_idx     = read_u2(&p);
            f->desc_idx     = read_u2(&p);
            f->offset       = -1;

            /* 解析字段名和描述符 */
            if (f->name_idx > 0 && f->name_idx < class->cp_count) {
                f->name = class->constant_pool[f->name_idx].data.utf8.bytes;
            }
            if (f->desc_idx > 0 && f->desc_idx < class->cp_count) {
                f->descriptor = class->constant_pool[f->desc_idx]
                                    .data.utf8.bytes;
            }

            /* 跳过字段属性 */
            uint16_t attr_count = read_u2(&p);
            for (uint16_t j = 0; j < attr_count; j++) {
                read_u2(&p); /* name_idx */
                uint32_t attr_len = read_u4(&p);
                p += attr_len; /* 跳过 */
            }
        }
    }

    /* --- 方法 --- */
    class->methods_count = read_u2(&p);
    if (class->methods_count > 0) {
        class->methods = (jvm_method_t *)os_malloc(
            class->methods_count * sizeof(jvm_method_t));
        for (uint16_t i = 0; i < class->methods_count; i++) {
            jvm_method_t *m = &class->methods[i];
            m->class        = class;
            m->access_flags = read_u2(&p);
            m->name_idx     = read_u2(&p);
            m->desc_idx     = read_u2(&p);
            m->code         = NULL;
            m->code_len     = 0;
            m->max_stack    = 0;
            m->max_locals   = 0;
            m->exc_table    = NULL;
            m->exc_table_len = 0;
            m->native_func  = NULL;

            /* 解析方法名和描述符 */
            if (m->name_idx > 0 && m->name_idx < class->cp_count) {
                m->name = class->constant_pool[m->name_idx]
                              .data.utf8.bytes;
            }
            if (m->desc_idx > 0 && m->desc_idx < class->cp_count) {
                m->descriptor = class->constant_pool[m->desc_idx]
                                    .data.utf8.bytes;
            }

            /* 解析方法属性 */
            parse_attributes(&p, m);

            (void)end; /* 后续使用 */
        }
    }

    /* --- 类属性 --- */
    uint16_t class_attr_count = read_u2(&p);
    for (uint16_t i = 0; i < class_attr_count; i++) {
        read_u2(&p); /* name_idx */
        uint32_t attr_len = read_u4(&p);
        p += attr_len;
    }

    os_print("[ClassFile] Parsed class: ");
    /* 获取类名 */
    if (class->this_class > 0 && class->this_class < class->cp_count) {
        uint16_t name_idx = class->constant_pool[class->this_class]
                                .data.class.name_idx;
        class->name = class->constant_pool[name_idx].data.utf8.bytes;
    }
    os_print(class->name ? class->name : "(unknown)");
    os_print("\n");

    return 0;
}
