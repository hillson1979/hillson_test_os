/**
 * @file msi_test.c
 * @brief MSI 测试工具 - 手动触发 MSI 来验证路径
 *
 * 用于调试 MSI 不工作的问题
 */

#include "types.h"
#include "printf.h"
#include "io.h"

// MSI 地址定义
#define MSI_ADDRESS_BASE  0xFEE00000
#define MSI_TEST_VECTOR  0x40

// 手动触发 MSI 写操作
void msi_test_trigger(void) {
    printf("\n");
    printf("MSI Test - Manual Trigger\n");
    printf("========================================\n");

    printf("[MSI Test] NOTE: Manual MSI write test SKIPPED\n");
    printf("[MSI Test]   The MSI address (0xFEE00000) is not mapped in kernel page table\n");
    printf("[MSI Test]   Hardware MSI writes work differently (via PCI bus)\n");
    printf("[MSI Test] \n");
    printf("[MSI Test]   To test MSI interrupt delivery:\n");
    printf("[MSI Test]   1. Software interrupt test (int $0x40) verifies IDT path\n");
    printf("[MSI Test]   2. Real E1000 hardware will trigger MSI via PCI writes\n");
    printf("[MSI Test] ========================================\n");

    // 不执行手动 MSI 写入，因为 0xFEE00000 未映射
    // 真正的 MSI 是 E1000 硬件通过 PCI 总线写入的，不需要通过 CPU 内存访问
}

// 测试软件中断（验证 IDT 和 trap() 是否工作）
void msi_test_softint(void) {
    printf("\n");
    printf("[MSI Test] ===== Testing Software Interrupt =====\n");
    printf("[MSI Test] Triggering int $0x40...\n");

    __asm__ volatile ("int $0x40");

    printf("[MSI Test] Returned from int $0x40\n");
    printf("[MSI Test] ========================================\n");
}

// 检查 LAPIC 状态
void msi_test_check_lapic(void) {
    printf("\n");
    printf("LAPIC Status Check\n");
    printf("============================================\n");

    // 读取 APIC BASE MSR
    uint64_t apic_base;
    __asm__ volatile ("rdmsr" : "=a"(apic_base) : "c"(0x1B));

    printf("[LAPIC] APIC BASE MSR = 0x%llx\n", apic_base);
    printf("[LAPIC]   Base Address = 0x%llx\n", apic_base & 0xFFFFF000ULL);
    printf("[LAPIC]   Bit 11 (Enable) = %d %s\n",
           (int)((apic_base >> 11) & 1),
           ((apic_base >> 11) & 1) ? "ENABLED" : "DISABLED!");

    if (!((apic_base >> 11) & 1)) {
        printf("[LAPIC] WARNING: LAPIC is DISABLED!\n");
        printf("[LAPIC]   MSI will NOT work!\n");
    }

    // 使用全局 lapic 指针（已在 mp.c/lapic.c 中初始化）
    extern volatile uint32_t *lapic;

    if (lapic) {
        // 读取 LAPIC SVR (Spurious Interrupt Vector Register)
        // SVR 偏移 = 0xF0
        uint32_t svr = lapic[0xF0 / 4];
        printf("[LAPIC] SVR = 0x%08x\n", svr);
        printf("[LAPIC]   Bit 8 (Enable) = %d %s\n",
               (int)(svr & 0x10000 ? 1 : 0),
               (svr & 0x10000) ? "ENABLED" : "DISABLED!");

        // 读取 LAPIC TPR (Task Priority Register)
        // TPR 偏移 = 0x80
        uint32_t tpr = lapic[0x80 / 4];
        printf("[LAPIC] TPR = 0x%08x (Task Priority)\n", tpr);
        if (tpr != 0) {
            printf("[LAPIC] WARNING: TPR != 0, may block interrupts!\n");
        }
    } else {
        printf("[LAPIC] WARNING: lapic pointer is NULL!\n");
        printf("[LAPIC]   Cannot read LAPIC registers!\n");
    }

    printf("[LAPIC] ============================================\n");
}

// 检查 CPU IF 标志
void msi_test_check_if(void) {
    uint32_t eflags;

    printf("\n");
    printf("[MSI Test] ===== Checking CPU Interrupt Flag =====\n");

    __asm__ volatile ("pushfl\n"
                      "popl %0\n"
                      : "=r"(eflags));

    printf("[CPU] EFLAGS = 0x%08x\n", eflags);
    printf("[CPU] Bit 9 (IF) = %d %s\n",
           (int)((eflags >> 9) & 1),
           ((eflags >> 9) & 1) ? "Enabled (interrupts on)" : "Disabled (interrupts off)");

    if (!((eflags >> 9) & 1)) {
        printf("[CPU] WARNING: Interrupts are DISABLED!\n");
        printf("[CPU]   Need to call sti() to enable!\n");
    }

    printf("[CPU] ===========================================\n");
}

// 完整的 MSI 路径测试
void msi_test_full_path(void) {
    printf("\n");
    printf("MSI Path Test\n");
    printf("========================================\n");
    printf("\n");

    // 1. 检查 LAPIC
    msi_test_check_lapic();

    // 2. 检查 IF 标志
    msi_test_check_if();

    // 3. 测试软件中断（验证 IDT 和 trap()）
    printf("\n");
    printf("[MSI Test] ===== Step 1: Testing Soft Interrupt =====\n");
    printf("[MSI Test] This verifies IDT[0x40] and trap() work...\n");
    msi_test_softint();

    // 4. 手动触发 MSI
    printf("\n");
    printf("[MSI Test] ===== Step 2: Testing Manual MSI Write =====\n");
    printf("[MSI Test] This simulates what E1000 hardware does...\n");
    msi_test_trigger();

    printf("\n");
}
