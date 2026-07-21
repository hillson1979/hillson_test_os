/**
 * classfile.c — Java Class 文件解析器（精简版）
 *
 * 只解析运行 HelloWorld 所需的最小结构。
 */
#include "jvm.h"
#include "os_port.h"

static inline uint8_t  r8(const uint8_t **p) { uint8_t  v = (*p)[0]; *p += 1; return v; }
static inline uint16_t r16(const uint8_t **p) { uint16_t v = ((uint16_t)(*p)[0]<<8)|(*p)[1]; *p+=2; return v; }
static inline uint32_t r32(const uint8_t **p) { uint32_t v = ((uint32_t)(*p)[0]<<24)|((uint32_t)(*p)[1]<<16)|((uint32_t)(*p)[2]<<8)|(*p)[3]; *p+=4; return v; }

/* 从常量池获取 UTF8 字符串 */
static const char *cp_utf8(jvm_class_t *cls, uint16_t idx) {
    if (idx == 0 || idx >= cls->cp_count) return "(invalid)";
    return (const char *)cls->constant_pool[idx].data.utf8.bytes;
}

int classfile_parse(jvm_class_t *cls, const uint8_t *data, uint32_t len) {
    const uint8_t *p = data;

    /* 魔数 */
    if (r32(&p) != 0xCAFEBABE) { os_print("[CF] Bad magic\n"); return -1; }

    cls->minor_version = r16(&p);
    cls->major_version = r16(&p);

    /* ---- 常量池 ---- */
    cls->cp_count = r16(&p);
    os_print("[CF] CP count: "); /* TODO: print */ os_print("\n");

    cls->constant_pool = (cp_entry_t *)os_malloc(cls->cp_count * sizeof(cp_entry_t));
    if (!cls->constant_pool) return -1;
    /* 清零 */
    for (uint16_t i = 0; i < cls->cp_count; i++)
        for (uint32_t j = 0; j < sizeof(cp_entry_t); j++)
            ((uint8_t*)&cls->constant_pool[i])[j] = 0;

    for (uint16_t i = 1; i < cls->cp_count; i++) {
        cp_entry_t *e = &cls->constant_pool[i];
        e->tag = r8(&p);

        switch (e->tag) {
        case 1: { /* Utf8 */
            uint16_t l = r16(&p);
            e->data.utf8.len = l;
            e->data.utf8.bytes = (char *)os_malloc(l + 1);
            if (e->data.utf8.bytes) {
                for (uint16_t j = 0; j < l; j++) e->data.utf8.bytes[j] = (char)r8(&p);
                e->data.utf8.bytes[l] = 0;
            }
            break;
        }
        case 3: e->data.integer.value = (int32_t)r32(&p); break;
        case 4: { uint32_t v = r32(&p); e->data.float_val.value = *(float*)&v; break; }
        case 5: { uint64_t hi = r32(&p); e->data.long_val.value = (hi << 32) | r32(&p); i++; break; }
        case 6: { uint64_t hi = r32(&p); uint64_t lo = r32(&p); e->data.double_val.value = *(double*)&(uint64_t){(hi<<32)|lo}; i++; break; }
        case 7:  e->data.class.name_idx = r16(&p); break;
        case 8:  e->data.string.str_idx = r16(&p); break;
        case 9: case 10: case 11:
            e->data.ref.class_idx = r16(&p);
            e->data.ref.nt_idx = r16(&p);
            break;
        case 12: e->data.name_and_type.name_idx = r16(&p);
                 e->data.name_and_type.desc_idx = r16(&p); break;
        case 15: { r8(&p); r16(&p); break; } /* MethodHandle */
        case 16: { r16(&p); break; } /* MethodType */
        case 18: { r16(&p); r16(&p); break; } /* InvokeDynamic */
        default: os_print("[CF] Unknown tag "); break;
        }
    }

    /* ---- 访问标志/类/父类/接口 ---- */
    cls->access_flags = r16(&p);
    cls->this_class = r16(&p);
    cls->super_class = r16(&p);
    uint16_t ifc = r16(&p);
    for (uint16_t i = 0; i < ifc; i++) r16(&p);

    /* 类名 */
    if (cls->this_class > 0 && cls->this_class < cls->cp_count) {
        uint16_t ni = cls->constant_pool[cls->this_class].data.class.name_idx;
        cls->name = (char *)cp_utf8(cls, ni);
    }
    os_print("[CF] Class: "); os_print(cls->name ? cls->name : "?"); os_print("\n");

    /* ---- 字段 (跳过) ---- */
    uint16_t fc = r16(&p);
    for (uint16_t i = 0; i < fc; i++) {
        r16(&p); r16(&p); r16(&p); /* access, name, desc */
        uint16_t ac = r16(&p); /* attr count */
        for (uint16_t j = 0; j < ac; j++) { r16(&p); uint32_t al = r32(&p); p += al; }
    }

    /* ---- 方法 ---- */
    cls->methods_count = r16(&p);
    os_print("[CF] Methods: "); /* TODO print */ os_print("\n");
    cls->methods = (jvm_method_t *)os_malloc(cls->methods_count * sizeof(jvm_method_t));
    if (!cls->methods) return -1;
    for (uint16_t i = 0; i < cls->methods_count; i++)
        for (uint32_t j = 0; j < sizeof(jvm_method_t); j++)
            ((uint8_t*)&cls->methods[i])[j] = 0;

    for (uint16_t mi = 0; mi < cls->methods_count; mi++) {
        jvm_method_t *m = &cls->methods[mi];
        m->class = cls;
        m->access_flags = r16(&p);
        m->name_idx = r16(&p);
        m->desc_idx = r16(&p);
        m->name = (char *)cp_utf8(cls, m->name_idx);
        m->descriptor = (char *)cp_utf8(cls, m->desc_idx);
        os_print("[CF] Method: "); os_print(m->name); os_print(" "); os_print(m->descriptor); os_print("\n");

        /* 属性 */
        uint16_t ac = r16(&p);
        for (uint16_t ai = 0; ai < ac; ai++) {
            uint16_t an = r16(&p);
            uint32_t al = r32(&p);
            const char *aname = cp_utf8(cls, an);

            /* 检查是否为 "Code" 属性 */
            const char *code_str = "Code";
            int is_code = (aname[0]=='C' && aname[1]=='o' && aname[2]=='d' && aname[3]=='e' && aname[4]==0);

            if (is_code) {
                m->max_stack = r16(&p);
                m->max_locals = r16(&p);
                m->code_len = r32(&p);
                m->code = (uint8_t *)os_malloc(m->code_len);
                if (m->code)
                    for (uint32_t j = 0; j < m->code_len; j++)
                        m->code[j] = r8(&p);
                /* 异常表 */
                uint16_t et = r16(&p);
                for (uint16_t j = 0; j < et; j++) { r16(&p); r16(&p); r16(&p); r16(&p); }
                /* Code 内部属性 */
                uint16_t ca = r16(&p);
                for (uint16_t j = 0; j < ca; j++) {
                    r16(&p); uint32_t cal = r32(&p);
                    for (uint32_t k = 0; k < cal; k++) r8(&p);
                }
                os_print("[CF]   Code: stack="); /* TODO print max_stack */ os_print(" locals="); /* TODO */ os_print(" len="); /* TODO */ os_print("\n");
            } else {
                /* 跳过其他属性 */
                for (uint32_t j = 0; j < al; j++) r8(&p);
            }
        }
    }

    /* 类属性 (跳过) */
    uint16_t ca = r16(&p);
    for (uint16_t i = 0; i < ca; i++) { r16(&p); uint32_t al = r32(&p); p += al; }

    os_print("[CF] Parse done.\n");
    return 0;
}
