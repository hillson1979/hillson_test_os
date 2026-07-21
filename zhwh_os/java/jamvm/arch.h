/**
 * arch.h — 架构定义
 *
 * JamVM 期望 config.h 或构建系统生成此文件，
 * 将架构代码映射到对应的 arch/<arch>.h 头文件。
 */
#ifndef JAMVM_ARCH_H
#define JAMVM_ARCH_H

#include "arch/i386.h"

/* 与 arch/i386.h 兼容的附加定义 */
/* configure 会生成 arch.h，我们手动指定 */
/* arch 子目录在 jamvm_src/src/arch/ 下 */
/* Makefile 的 -I 路径中包含了 jamvm_src/src/ */

#endif
