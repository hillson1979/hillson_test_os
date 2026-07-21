/**
 * init.c — JamVM HillsonOS i386 平台初始化
 *
 * 参考: jamvm_src/src/os/linux/i386/init.c
 */
#include "../../../config.h"

/**
 * 设置 x87 FPU 为双精度模式（64-bit）
 *
 * Java 语言规范要求 float/double 使用 IEEE 754 32/64 位精度。
 * x87 FPU 默认使用 80 位扩展精度，需要切换。
 */
void setDoublePrecision() {
    /* 修改 x87 FPU 控制字：
     *   - 清除位 8-9 (PC: Precision Control) = 清为 0b00 (单精度)
     *   - 设置位 8-9 = 0b10 (双精度)
     *   实际上我们想要双精度: PC = 10
     *
     * FPU 控制字各位:
     *   Bit 8-9 (PC):  00=单精度, 10=双精度, 11=扩展精度(默认)
     *   Bit 10-11 (RC): 00=最近偶数舍入(默认)
     */
    unsigned short cw;
    __asm__ volatile (
        "fstcw %0\n"       /* 保存当前控制字 */
        "movw %0, %%ax\n"  /* 加载到 ax */
        "andw $0xfcff, %%ax\n"  /* 清除 PC 位 (bit 8-9) */
        "orw  $0x0200, %%ax\n"  /* 设置 PC = 10 (双精度) */
        "movw %%ax, %0\n"
        "fldcw %0\n"       /* 加载新控制字 */
        : "=m"(cw)
        :
        : "ax"
    );
}

/**
 * 平台初始化
 */
void initialisePlatform() {
    setDoublePrecision();
}
