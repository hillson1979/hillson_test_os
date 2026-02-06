/**
 * @file msi_test.h
 * @brief MSI 测试工具声明
 */

#ifndef MSI_TEST_H
#define MSI_TEST_H

#include <types.h>

// 手动触发 MSI 写操作（模拟 E1000 行为）
void msi_test_trigger(void);

// 测试软件中断（验证 IDT 路径）
void msi_test_softint(void);

// 检查 LAPIC 状态
void msi_test_check_lapic(void);

// 检查 CPU IF 标志
void msi_test_check_if(void);

// 完整的 MSI 路径测试
void msi_test_full_path(void);

#endif // MSI_TEST_H
