# -----------------------------------------------------------------------------
# interrupt_exit.S
# 完整 interrupt_exit 用户态返回模板
# -----------------------------------------------------------------------------

.text
.global interrupt_exit
.extern need_resched
.extern schedule
.extern th_u
.extern current
# ⚠️ 这些偏移必须与 struct trapframe 的布局完全匹配！
# ⚠️ 关键修复：struct trapframe 的实际布局（按C结构体定义）
# struct从offset 0开始：edi(0), esi(4), ebp(8), oesp(12), ebx(16), edx(20), ecx(24), eax(28)
#                      ds(32), es(36), fs(40), gs(44), trapno(48), err(52)
#                      eip(56), cs(60), eflags(64), esp(68), ss(72)

# task_t 偏移量（匹配 include/task.h 的结构体定义）
# ⚠️ tf 在 task_t 结构体中的偏移量是 148
.set TASK_TF, 148

.equ TRAPFRAME_EDI, 0
.equ TRAPFRAME_ESI, 4
.equ TRAPFRAME_EBP, 8
.equ TRAPFRAME_OESP, 12
.equ TRAPFRAME_EBX, 16
.equ TRAPFRAME_EDX, 20
.equ TRAPFRAME_ECX, 24
.equ TRAPFRAME_EAX, 28
.equ TRAPFRAME_DS, 32      # ⚠️ 修正: ds = offset 32 (C结构体定义)
.equ TRAPFRAME_ES, 36      # ⚠️ 修正: es = offset 36
.equ TRAPFRAME_FS, 40      # ⚠️ 修正: fs = offset 40
.equ TRAPFRAME_GS, 44      # ⚠️ 修正: gs = offset 44
.equ TRAPFRAME_TRAPNO, 48
.equ TRAPFRAME_ERR, 52
.equ TRAPFRAME_EIP, 56
.equ TRAPFRAME_CS, 60
.equ TRAPFRAME_EFLAGS, 64
.equ TRAPFRAME_ESP, 68
.equ TRAPFRAME_SS, 72
.equ TRAPFRAME_PDE, 544

interrupt_exit:
    # ⚠️⚠️⚠️ 关键修复：立即关中断，防止在恢复过程中被中断打断！
    cli

    # ⚠️⚠️⚠️ 关键修复：跳过 call 指令压入的返回地址！
    # 根据 Intel SDM：call 指令会将返回地址压入栈
    # ESP 当前指向返回地址，需要调整 ESP 使其指向 trapframe 起始位置
    addl $4, %esp          # 跳过返回地址，ESP 现在指向 trapframe 的 edi (offset 0)

    # ⚠️⚠️⚠️ 关键修复：不再检查 current->tf！
    # 原因：ESP 现在已经指向正确的 trapframe（由 alltraps 压栈创建）
    #      current->tf 只在 fork/schedule 时使用，不用于普通中断返回
    #      直接使用 ESP 作为 trapframe 指针即可！

    # ⚠️ 调试：暂时禁用这些调试打印，因为它们会破坏栈
    # # ⚠️ 调试：打印 interrupt_exit 入口信息（只使用寄存器，不压栈）
    # pushl %eax
    # pushl %ecx
    # pushl %edx
    # movl %esp, %eax
    # pushl %eax
    # call debug_print_interrupt_exit_entry
    # addl $4, %esp
    # popl %edx
    # popl %ecx
    # popl %eax

    # ⚠️⚠️⚠️ 关键修复：interrupt_exit 只用于"不切换任务"的情况!
    # 如果需要调度,schedule 会切换到新任务,并且新任务会自己处理返回
    # -------------------------------
    # 1. 调度检查（Linux 风格）
    # -------------------------------
    cmpl $0, need_resched
    je 1f

    # ⚠️ 调试：暂时禁用
    # # ⚠️ 调试：打印 schedule 调用前信息
    # pushl %eax
    # pushl %ecx
    # pushl %edx
    # movl %esp, %eax
    # pushl %eax
    # call debug_print_before_schedule
    # addl $4, %esp
    # popl %edx
    # popl %ecx
    # popl %eax

    call schedule

    # ⚠️⚠️⚠️ 注意：schedule 可能会切换任务！
    # 如果切换到用户任务，会返回到这里
    # 如果切换到内核任务，switch_to 会直接跳转到 efficient_scheduler_loop，不会返回

    # ⚠️ 调试：暂时禁用
    # # ⚠️ 调试：打印 schedule 返回后信息
    # pushl %eax
    # pushl %ecx
    # pushl %edx
    # movl %esp, %eax
    # pushl %eax
    # call debug_print_after_schedule
    # addl $4, %esp
    # popl %edx
    # popl %ecx
    # popl %eax

    movl $0, need_resched

1:
    # ⚠️⚠️⚠️ 继续恢复 trapframe
    # 只有用户任务会返回到这里，内核任务已在 schedule 中进入调度循环

    # ⚠️ 调试：暂时禁用
    # # ⚠️ 调试：打印开始恢复寄存器前的信息
    # pushl %eax
    # pushl %esp
    # call debug_print_before_restore_regs
    # addl $4, %esp
    # popl %eax

    # -------------------------------
    # 2. 恢复通用寄存器 (包括 EBX)
    #    参考 trapret 的实现:先恢复所有通用寄存器
    # -------------------------------
    popl %edi             # ESP = 4
    popl %esi             # ESP = 8
    popl %ebp             # ESP = 12
    addl $4, %esp         # ESP = 16 (跳过 old_esp)

    popl %ebx             # ESP = 20, 恢复 EBX ✅
    popl %edx             # ESP = 24, 恢复 EDX
    popl %ecx             # ESP = 28, 恢复 ECX
    popl %eax             # ESP = 32, 恢复 EAX
    # 现在 ESP 指向 offset 32 (GS)

    # ⚠️⚠️⚠️ 关键修复：暂时不要恢复段寄存器！
    # 先检查是否来自用户态，如果是用户态才恢复段寄存器
    # 如果是内核态，保持当前段寄存器（内核段）

    # -------------------------------
    # 3. 检查特权级（在恢复段寄存器之前！）
    # -------------------------------
    # ⚠️ 重要：ESP 现在指向 GS（offset 32）
    # CS 在 offset 56，即 ESP+24
    movl 24(%esp), %eax     # eax = cs (ESP=32, CS在32+24=56)
    testl $3, %eax          # 检查 RPL 位 (cs & 3)
    jz from_kernel_restore  # 如果 RPL=0，跳转到内核态恢复路径

    # ================================
    # 来自用户态 (CPL3) 的恢复路径
    # ================================
from_user_restore:
    # 恢复段寄存器
    popl %ebx             # ESP = 36, 读取 GS
    movw %bx, %gs         # 恢复 GS
    popl %ebx             # ESP = 40, 读取 FS
    movw %bx, %fs         # 恢复 FS
    popl %ebx             # ESP = 44, 读取 ES
    movw %bx, %es         # 恢复 ES
    popl %ebx             # ESP = 48, 读取 DS
    movw %bx, %ds         # 恢复 DS

    # ⚠️ 调试：暂时禁用
    # # ⚠️ 调试：打印恢复寄存器后的信息
    # pushl %eax
    # pushl %esp
    # call debug_print_after_restore_regs
    # addl $4, %esp
    # popl %eax

    # -------------------------------
    # 4. 跳过 trapno + errcode (8字节)
    # -------------------------------
    addl $8, %esp           # ESP = 56, 现在指向 EIP ✅

    # 栈内容: [eip, cs, eflags, esp, ss]
    # iret 会自动弹出所有 5 个值
    iret

    # ================================
    # 来自内核态 (CPL0) 的恢复路径
    # ================================
from_kernel_restore:
    # ⚠️ 关键：内核态中断时，不要恢复段寄存器！
    # 保持当前段寄存器（内核段），直接跳过它们
    addl $16, %esp          # 跳过 GS/FS/ES/DS (4个寄存器 × 4字节)

    # ⚠️ 调试：暂时禁用
    # # ⚠️ 调试：打印内核态跳过段寄存器恢复
    # pushl %eax
    # pushl %esp
    # call debug_print_after_restore_regs
    # addl $4, %esp
    # popl %eax

    # -------------------------------
    # 4. 跳过 trapno + errcode (8字节)
    # -------------------------------
    addl $8, %esp           # ESP = 56, 现在指向 EIP ✅

    # 栈内容: [eip, cs, eflags] (没有 esp/ss)
    # iret 只弹出 3 个值
    iret

# ================================
# interrupt_exit 致命错误处理
# ================================
interrupt_exit_panic:
    # ⚠️⚠️⚠️ interrupt_exit 被非法调用！
    # 保存所有寄存器（不破坏栈）
    pushl %eax
    pushl %ecx
    pushl %edx

    # 打印错误信息
    pushl $384  # 384 = strlen(error_msg)
    pushl $interrupt_exit_panic_msg
    call printf
    addl $8, %esp

    # 打印 current 值
    pushl current
    pushl $31
    pushl $interrupt_exit_panic_current
    call printf
    addl $12, %esp

    # 打印 current->tf 值
    movl current, %eax
    cmpl $0, %eax
    je 1f
    pushl TASK_TF(%eax)
    jmp 2f
1:
    pushl $0
2:
    pushl $27
    pushl $interrupt_exit_panic_tf
    call printf
    addl $12, %esp

    # 打印调用者信息（栈上的返回地址）
    movl 12(%esp), %eax   # 跳过 eax/ecx/edx，读取返回地址
    pushl %eax
    pushl $23
    pushl $interrupt_exit_panic_caller
    call printf
    addl $12, %esp

    popl %edx
    popl %ecx
    popl %eax

    # 停止系统
    cli
    hlt
    jmp interrupt_exit_panic

# 错误消息字符串
interrupt_exit_panic_msg:
    .asciz "\n\n==================== PANIC ====================\ninterrupt_exit called illegally!\n"
interrupt_exit_panic_current:
    .asciz "  current = 0x%p (should NOT be NULL!)\n"
interrupt_exit_panic_tf:
    .asciz "  current->tf = 0x%p (should NOT be NULL!)\n"
interrupt_exit_panic_caller:
    .asciz "  called from: 0x%p\n=============================================\n"
