/**
 * @file pci_msi.h
 * @brief PCI MSI (Message Signaled Interrupt) 支持
 */

#ifndef PCI_MSI_H
#define PCI_MSI_H

#include <types.h>

/**
 * @brief 启用 PCI 设备的 MSI 中断
 *
 * @param bus PCI 总线号
 * @param dev 设备号
 * @param func 功能号
 * @param vector 中断向量 (0x20-0xFF)
 * @return 成功返回 0，失败返回 -1
 */
int pci_enable_msi(unsigned bus, unsigned dev, unsigned func, uint8_t vector);

/**
 * @brief 禁用 PCI 设备的 MSI 中断
 */
void pci_disable_msi(unsigned bus, unsigned dev, unsigned func);

#endif // PCI_MSI_H
