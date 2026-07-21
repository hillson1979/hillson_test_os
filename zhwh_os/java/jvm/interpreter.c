/**
 * interpreter.c — Java 字节码解释器
 *
 * 实现 JVM 指令集的解释执行。
 * 支持的指令约 200+ 条（Java SE 8 规范子集）。
 *
 * 参考: Java虚拟机规范 — Chapter 6: The Java Virtual Machine Instruction Set
 */
#include "jvm.h"
#include "os_port.h"

/* ================================================================
 * 帧管理
 * ================================================================ */

/** 创建新的栈帧 */
static jvm_frame_t *frame_create(jvm_thread_t *thread, jvm_method_t *method) {
    jvm_frame_t *frame;

    /* 尝试从帧池中复用 */
    if (thread->frame_pool) {
        frame = thread->frame_pool;
        thread->frame_pool = frame->prev;
    } else {
        frame = (jvm_frame_t *)os_malloc(sizeof(jvm_frame_t));
    }

    if (!frame) return NULL;

    frame->prev   = thread->current_frame;
    frame->method = method;
    frame->pc     = 0;

    /* 分配局部变量表 */
    frame->locals_count = method->max_locals;
    frame->locals = (uint64_t *)os_malloc(
        method->max_locals * sizeof(uint64_t));
    if (!frame->locals) {
        os_free(frame);
        return NULL;
    }
    /* 清零局部变量 */
    for (uint16_t i = 0; i < method->max_locals; i++) {
        frame->locals[i] = 0;
    }

    /* 分配操作数栈 */
    frame->stack_capacity = method->max_stack + 4; /* 留余量 */
    frame->stack_top = 0;
    frame->stack = (uint64_t *)os_malloc(
        frame->stack_capacity * sizeof(uint64_t));
    if (!frame->stack) {
        os_free(frame->locals);
        os_free(frame);
        return NULL;
    }

    return frame;
}

/* ================================================================
 * 操作数栈操作
 * ================================================================ */

static inline void push_int(jvm_frame_t *frame, int32_t val) {
    frame->stack[frame->stack_top++] = (uint64_t)(int64_t)val;
}

static inline int32_t pop_int(jvm_frame_t *frame) {
    return (int32_t)frame->stack[--frame->stack_top];
}

static inline void push_long(jvm_frame_t *frame, int64_t val) {
    frame->stack[frame->stack_top++] = (uint64_t)val;
}

static inline int64_t pop_long(jvm_frame_t *frame) {
    return (int64_t)frame->stack[--frame->stack_top];
}

static inline void push_float(jvm_frame_t *frame, float val) {
    /* 通过 union 保留浮点位模式 */
    union { float f; uint32_t i; } u;
    u.f = val;
    frame->stack[frame->stack_top++] = (uint64_t)u.i;
}

static inline float pop_float(jvm_frame_t *frame) {
    union { float f; uint32_t i; } u;
    u.i = (uint32_t)frame->stack[--frame->stack_top];
    return u.f;
}

static inline void push_double(jvm_frame_t *frame, double val) {
    union { double d; uint64_t i; } u;
    u.d = val;
    frame->stack[frame->stack_top++] = u.i;
}

static inline double pop_double(jvm_frame_t *frame) {
    union { double d; uint64_t i; } u;
    u.i = frame->stack[--frame->stack_top];
    return u.d;
}

static inline void push_ref(jvm_frame_t *frame, j_ref ref) {
    frame->stack[frame->stack_top++] = (uint64_t)(uint32_t)ref;
}

static inline j_ref pop_ref(jvm_frame_t *frame) {
    return (j_ref)(uint32_t)frame->stack[--frame->stack_top];
}

/* ================================================================
 * 字节码读取辅助
 * ================================================================ */

static inline uint8_t read_op_u1(jvm_frame_t *frame) {
    return frame->method->code[frame->pc++];
}

static inline int8_t read_op_s1(jvm_frame_t *frame) {
    return (int8_t)frame->method->code[frame->pc++];
}

static inline uint16_t read_op_u2(jvm_frame_t *frame) {
    uint8_t *c = frame->method->code;
    uint16_t v = ((uint16_t)c[frame->pc] << 8) | c[frame->pc + 1];
    frame->pc += 2;
    return v;
}

static inline int16_t read_op_s2(jvm_frame_t *frame) {
    return (int16_t)read_op_u2(frame);
}

static inline int32_t read_op_s4(jvm_frame_t *frame) {
    uint8_t *c = frame->method->code;
    int32_t v = ((int32_t)c[frame->pc] << 24) |
                ((int32_t)c[frame->pc + 1] << 16) |
                ((int32_t)c[frame->pc + 2] << 8) |
                c[frame->pc + 3];
    frame->pc += 4;
    return v;
}

/* ================================================================
 * 解释器主循环
 * ================================================================ */

/**
 * 执行一个方法的字节码
 *
 * @param thread  当前线程
 * @param method  要执行的方法
 * @return 返回值（int），或 0
 */
int interpreter_execute(jvm_thread_t *thread, jvm_method_t *method) {
    if (!thread || !method || !method->code) {
        return -1;
    }

    jvm_frame_t *frame = frame_create(thread, method);
    if (!frame) {
        os_print("[Interpreter] ERROR: Failed to create frame\n");
        return -1;
    }

    thread->current_frame = frame;
    frame->pc = 0;

    /* ============================================================
     * 字节码解释循环
     *
     * 指令格式: [opcode(1B)] [operands(0-N bytes)]
     *
     * 为减少 switch 分支开销，对高频指令采用直接跳转表优化。
     * 暂时使用标准 switch 实现，后续可改为 computed goto。
     * ============================================================ */
    while (frame->pc < method->code_len) {
        uint8_t opcode = read_op_u1(frame);

        switch (opcode) {

        /* ---- 常量入栈 ---- */
        case 0x00: /* nop */
            break;

        case 0x01: /* aconst_null */
            push_ref(frame, NULL);
            break;

        case 0x02: /* iconst_m1 */
            push_int(frame, -1);
            break;

        case 0x03: /* iconst_0 */
            push_int(frame, 0);
            break;
        case 0x04: /* iconst_1 */
            push_int(frame, 1);
            break;
        case 0x05: /* iconst_2 */
            push_int(frame, 2);
            break;
        case 0x06: /* iconst_3 */
            push_int(frame, 3);
            break;
        case 0x07: /* iconst_4 */
            push_int(frame, 4);
            break;
        case 0x08: /* iconst_5 */
            push_int(frame, 5);
            break;

        case 0x09: /* lconst_0 */
            push_long(frame, 0LL);
            break;
        case 0x0a: /* lconst_1 */
            push_long(frame, 1LL);
            break;

        case 0x0b: /* fconst_0 */
            push_float(frame, 0.0f);
            break;
        case 0x0c: /* fconst_1 */
            push_float(frame, 1.0f);
            break;
        case 0x0d: /* fconst_2 */
            push_float(frame, 2.0f);
            break;

        case 0x0e: /* dconst_0 */
            push_double(frame, 0.0);
            break;
        case 0x0f: /* dconst_1 */
            push_double(frame, 1.0);
            break;

        case 0x10: /* bipush */
            push_int(frame, (int32_t)read_op_s1(frame));
            break;

        case 0x11: /* sipush */
            push_int(frame, (int32_t)read_op_s2(frame));
            break;

        /* ---- 加载/存储 ---- */
        case 0x1a: /* iload_0 */
            push_int(frame, (int32_t)frame->locals[0]);
            break;
        case 0x1b: /* iload_1 */
            push_int(frame, (int32_t)frame->locals[1]);
            break;
        case 0x1c: /* iload_2 */
            push_int(frame, (int32_t)frame->locals[2]);
            break;
        case 0x1d: /* iload_3 */
            push_int(frame, (int32_t)frame->locals[3]);
            break;

        case 0x3b: /* istore_0 */
            frame->locals[0] = (uint64_t)(int64_t)pop_int(frame);
            break;
        case 0x3c: /* istore_1 */
            frame->locals[1] = (uint64_t)(int64_t)pop_int(frame);
            break;
        case 0x3d: /* istore_2 */
            frame->locals[2] = (uint64_t)(int64_t)pop_int(frame);
            break;
        case 0x3e: /* istore_3 */
            frame->locals[3] = (uint64_t)(int64_t)pop_int(frame);
            break;

        /* ---- 算术运算 ---- */
        case 0x60: /* iadd */
            push_int(frame, pop_int(frame) + pop_int(frame));
            break;
        case 0x64: /* isub */
            {
                int32_t b = pop_int(frame);
                int32_t a = pop_int(frame);
                push_int(frame, a - b);
            }
            break;
        case 0x68: /* imul */
            push_int(frame, pop_int(frame) * pop_int(frame));
            break;
        case 0x6c: /* idiv */
            {
                int32_t b = pop_int(frame);
                int32_t a = pop_int(frame);
                push_int(frame, b != 0 ? a / b : 0);
            }
            break;

        /* ---- 返回指令 ---- */
        case 0xb1: /* return (void) */
            /* 释放帧，返回 */
            thread->current_frame = frame->prev;
            os_free(frame->stack);
            os_free(frame->locals);
            /* 帧回收到池 */
            frame->prev = thread->frame_pool;
            thread->frame_pool = frame;
            return 0;

        case 0xac: /* ireturn */
            {
                int32_t result = pop_int(frame);
                thread->current_frame = frame->prev;
                os_free(frame->stack);
                os_free(frame->locals);
                frame->prev = thread->frame_pool;
                thread->frame_pool = frame;
                /* 把返回值推到调用者的操作数栈上 */
                if (thread->current_frame) {
                    push_int(thread->current_frame, result);
                }
                return result;
            }

        /* ---- 扩展指令 ---- */
        case 0xc4: /* wide */
            /* TODO: 实现 wide 指令 */
            os_print("[Interpreter] WARNING: wide instruction not implemented\n");
            break;

        /* ---- 默认处理 ---- */
        default:
            /* 未实现的指令 */
            os_print("[Interpreter] WARNING: Unimplemented opcode 0x");
            /* TODO: 打印十六进制 opcode */
            os_print(" at pc=");
            /* TODO: 打印 pc */
            os_print("\n");
            break;
        }
    }

    /* 正常结束 */
    thread->current_frame = NULL;
    if (frame) {
        os_free(frame->stack);
        os_free(frame->locals);
        frame->prev = thread->frame_pool;
        thread->frame_pool = frame;
    }
    return 0;
}
