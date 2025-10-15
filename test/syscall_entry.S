# ============================================
# syscall_entry.S
# 用户态系统调用封装
# ============================================

    .section .text
    .intel_syntax noprefix

# ---- 通用 syscall 宏 ----
.macro SYSCALL name, num
    .globl \name
\name:
    mov eax, \num          # 系统调用号
    mov ebx, [esp + 4]     # 第一个参数
    mov ecx, [esp + 8]     # 第二个参数
    mov edx, [esp + 12]    # 第三个参数
    int 0x80               # 触发系统调用
    ret
.endm

# ---- 定义系统调用 ----
# 号要与你内核 syscall_dispatch 对应
SYSCALL sys_printf, 1
SYSCALL sys_exit,   2

    .att_syntax prefix

