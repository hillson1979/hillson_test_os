/**
 * interpreter.c — Java 字节码解释器（最小版）
 */
#include "jvm.h"
#include "os_port.h"

static const char *cp_str(jvm_class_t *cls, uint16_t idx) {
    if (idx == 0 || idx >= cls->cp_count) return "";
    cp_entry_t *e = &cls->constant_pool[idx];
    if (e->tag == 1) return e->data.utf8.bytes;
    if (e->tag == 8) {
        uint16_t s = e->data.string.str_idx;
        if (s > 0 && s < cls->cp_count)
            return cls->constant_pool[s].data.utf8.bytes;
    }
    return "";
}

int interpreter_execute(jvm_thread_t *thread, jvm_method_t *method) {
    (void)thread;
    if (!method || !method->code) return -1;

    uint8_t *code = method->code;
    uint32_t codelen = method->code_len;
    jvm_class_t *cls = method->class;
    uint32_t pc = 0;

    while (pc < codelen) {
        uint8_t op = code[pc++];

        switch (op) {
        case 0x00: /* nop */ break;

        case 0xB1: /* return */
            return 0;

        case 0xB2: /* getstatic */
            pc += 2; break;

        case 0x12: /* ldc */
            { uint8_t idx = code[pc++]; const char *s = cp_str(cls, idx);
              os_print("[Interp] ldc \""); os_print(s); os_print("\"\n"); break; }

        case 0xB6: /* invokevirtual */
            pc += 2;
            os_print("\n"); /* println */
            break;

        case 0x2A: /* aload_0 */ break;
        case 0xB7: /* invokespecial */ pc += 2; break;
        case 0x4C: /* astore_1 */ break;
        case 0x2B: /* aload_1 */ break;
        case 0xBE: /* arraylength */ break;
        case 0x3C: /* istore_1 */ break;
        case 0x1B: /* iload_1 */ break;
        case 0xA2: /* if_icmpge */ pc += 2; break;
        case 0x84: /* iinc */ pc += 2; break;
        case 0xA7: /* goto */ pc += 2; break;
        case 0x03: /* iconst_0 */ break;

        default: break;
        }
    }
    return 0;
}
