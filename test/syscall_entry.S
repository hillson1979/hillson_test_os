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
    push ebp
    mov ebp, esp
    mov eax, \num          # 系统调用号
    mov ebx, [ebp + 8]     # 第一个参数
    mov ecx, [ebp + 12]    # 第二个参数
    mov edx, [ebp + 16]    # 第三个参数
    int 0x80               # 触发系统调用
    pop ebp
    ret
.endm

# ---- 定义系统调用 ----
# 号要与你内核 syscall_dispatch 对应
SYSCALL sys_printf, 1
SYSCALL sys_exit, 2
SYSCALL sys_yield, 3
SYSCALL sys_get_mem_stats, 4
SYSCALL sys_read_mem, 5
SYSCALL sys_getchar, 6          # 新增：读取单个字符
SYSCALL sys_kbhit, 7            # 新增：检查是否有按键
SYSCALL sys_putchar, 8          # 新增：输出单个字符

    .att_syntax prefix

