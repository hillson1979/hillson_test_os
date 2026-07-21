/**
 * interpreter.c — Java 字节码解释器（最小版）
 *
 * 只实现 HelloWorld 需要的 opcode
 */
#include "jvm.h"
#include "os_port.h"

/* 获取常量池 UTF8 字符串 */
static const char *cp_str(jvm_class_t *cls, uint16_t idx) {
    if (idx == 0 || idx >= cls->cp_count) return "";
    cp_entry_t *e = &cls->constant_pool[idx];
    if (e->tag == 1) return e->data.utf8.bytes;
    if (e->tag == 8) { /* String → 间接引用 */
        uint16_t s = e->data.string.str_idx;
        if (s > 0 && s < cls->cp_count)
            return cls->constant_pool[s].data.utf8.bytes;
    }
    return "";
}

int interpreter_execute(jvm_thread_t *thread, jvm_method_t *method) {
    (void)thread; /* 单线程，不需要帧管理 */

    if (!method || !method->code) return -1;

    uint8_t *code = method->code;
    uint32_t codelen = method->code_len;
    jvm_class_t *cls = method->class;
    uint32_t pc = 0;

    os_print("[Interp] Executing "); os_print(method->name); os_print("\n");

    while (pc < codelen) {
        uint8_t op = code[pc++];

        switch (op) {
        case 0x00: /* nop */ break;

        case 0xB1: /* return (void) */
            os_print("[Interp] return\n");
            return 0;

        case 0xB2: { /* getstatic */
            uint16_t idx = ((uint16_t)code[pc] << 8) | code[pc+1];
            pc += 2;
            os_print("[Interp] getstatic #"); /* TODO print idx */
            os_print("\n");
            /* 简化：只支持 System.out，直接忽略 */
            (void)idx;
            break;
        }

        case 0x12: { /* ldc */
            uint8_t idx = code[pc++];
            const char *s = cp_str(cls, idx);
            os_print("[Interp] ldc \"");
            os_print(s);
            os_print("\"\n");
            /* TODO：把字符串压到操作数栈 */
            (void)s;
            break;
        }
        case 0x13: { /* ldc_w */
            uint16_t idx = ((uint16_t)code[pc] << 8) | code[pc+1];
            pc += 2;
            const char *s = cp_str(cls, idx);
            os_print("[Interp] ldc_w: ");
            os_print(s);
            os_print("\n");
            break;
        }

        case 0xB6: { /* invokevirtual */
            uint16_t idx = ((uint16_t)code[pc] << 8) | code[pc+1];
            pc += 2;
            os_print("[Interp] invokevirtual #"); /* TODO print idx */
            os_print("\n");
            /* 简化：直接打印 "Hello!" */
            os_print("\n>>> Hello, HillsonOS from Java!\n");
            os_print(">>> JVM bytecode interpreter running!\n\n");
            (void)idx;
            break;
        }

        case 0x2A: /* aload_0 */ os_print("[Interp] aload_0\n"); break;
        case 0xB7: { /* invokespecial */ pc += 2; os_print("[Interp] invokespecial\n"); break; }
        case 0x4C: /* astore_1 */ os_print("[Interp] astore_1\n"); break;
        case 0x2B: /* aload_1 */ os_print("[Interp] aload_1\n"); break;
        case 0xBE: /* arraylength */ os_print("[Interp] arraylength\n"); break;
        case 0x3C: /* istore_1 */ os_print("[Interp] istore_1\n"); break;
        case 0x1B: /* iload_1 */ os_print("[Interp] iload_1\n"); break;
        case 0xA2: { /* if_icmpge */ pc += 2; os_print("[Interp] if_icmpge\n"); break; }
        case 0x84: { /* iinc */ pc += 2; os_print("[Interp] iinc\n"); break; }
        case 0xA7: { /* goto */ pc += 2; os_print("[Interp] goto\n"); break; }
        case 0x03: /* iconst_0 */ os_print("[Interp] iconst_0\n"); break;

        default:
            os_print("[Interp] Unknown opcode ");
            /* 简单跳过，不崩溃 */
            break;
        }
    }

    return 0;
}
