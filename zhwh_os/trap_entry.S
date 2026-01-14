# trap_entry.S
# 汇编实现所有中断入口 alltraps
# 用于 i386，兼容 GCC 内核编译 -m32 模式
# 提供符号 alltraps 供 C 函数 trap_init / set_gate 使用

    .section .text
    .align 4
    .globl alltraps
alltraps:
    # --- 0. 检测特权级，跳转到不同的处理路径 ---
    # 问题：中断从用户态进入时，CPU自动压入 SS:ESP:EFLAGS:CS:EIP
    #      中断从内核态进入时，CPU只压入 EFLAGS:CS:EIP（没有SS:ESP）
    # 解决：内核态中断时，手动压入 dummy SS/ESP 来统一栈布局
    #
    # 进入 alltraps 时的栈布局：
    #   用户态：ESP -> [err][trapno][EIP][CS][EFLAGS][ESP][SS]
    #   内核态：ESP -> [err][trapno][EIP][CS][EFLAGS]
    #
    # 检查 CS 的 RPL 位（在 ESP+12）
    #   如果 RPL=3，说明是用户态中断，跳转到 alltraps_user
    #   如果 RPL=0，说明是内核态中断，跳转到 alltraps_kernel
    # ⚠️ 重要：使用ECX而不是EAX，避免破坏系统调用号!

    movl 12(%esp), %ecx  # 读取CS到ECX（不破坏EAX）
    testl $3, %ecx        # 检查RPL位（最低2位）
    jz alltraps_kernel    # RPL=0，内核态中断
    jmp alltraps_user     # RPL=3，用户态中断

# ================================
# 用户态中断处理路径
# ================================
alltraps_user:
    # ⚠️ 调试：暂时禁用调试打印（会导致除零错误）
    # pushl %eax
    # pushl %ecx
    # pushl %edx
    # movl $0xDEAD0001, %eax  # 标记：用户态中断
    # call debug_print_trap_entry
    # popl %edx
    # popl %ecx
    # popl %eax

    # --- 1. 先保存通用寄存器(包括EAX,它包含系统调用号!) ---
    # pusha 顺序: EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    # 栈布局(低地址): EDI, ESI, EBP, old_esp, EBX, EDX, ECX, EAX
    pusha

    # --- 2. 保存段寄存器(⚠️ 不能使用EAX,会破坏系统调用号!) ---
    # 使用 ECX 作为临时寄存器保存段寄存器值
    movw %ds, %cx
    movzwl %cx, %ecx
    pushl %ecx           # ds

    movw %es, %cx
    movzwl %cx, %ecx
    pushl %ecx           # es

    movw %fs, %cx
    movzwl %cx, %ecx
    pushl %ecx           # fs

    movw %gs, %cx
    movzwl %cx, %ecx
    pushl %ecx           # gs

    # --- 3. 切换为内核数据段 ---
    movl $0x10, %ecx
    movw %cx, %ds
    movw %cx, %es
    movw %cx, %fs
    movw %cx, %gs

    # --- 4. 调用 C 层的 do_irq_handler ---
    # 栈布局（用户态中断）：
    #   [0-28]   edi/esi/ebp/old_esp/ebx/edx/ecx/eax (pusha)
    #   [32-47]  gs/fs/es/ds
    #   [48-51]  err
    #   [52-55]  trapno
    #   [56-59]  eip
    #   [60-63]  cs
    #   [64-67]  eflags
    #   [68-71]  esp (CPU压入)
    #   [72-75]  ss (CPU压入)
    pushl %esp
    call do_irq_handler
    addl $4, %esp

    # ⚠️⚠️⚠️ 关键修复：调用 interrupt_exit 之前，ESP 已经指向正确的 trapframe 起始位置
    #     不需要调整 ESP！因为：
    #     1. pusha 压入了 8 个通用寄存器（32 字节）
    #     2. 然后压入了 4 个段寄存器（16 字节）
    #     3. CPU 压入了 err/trapno/eip/cs/eflags/esp/ss（28 字节）
    #     4. 调用 do_irq_handler 时压入了 ESP 指针（4 字节），返回后已清理
    #     5. 现在 ESP 指向 trapframe 的起始位置（edi），完美！

    # --- 5. 调用 interrupt_exit ---
    call interrupt_exit

    # 不会到达这里
    int $0xFF

# ================================
# 内核态中断处理路径
# ================================
alltraps_kernel:
    # ⚠️ 调试：暂时禁用调试打印（会导致除零错误）
    # pushl %eax
    # pushl %ecx
    # pushl %edx
    # movl $0xDEAD0000, %eax  # 标记：内核态中断
    # call debug_print_trap_entry
    # popl %edx
    # popl %ecx
    # popl %eax

    # --- 1. 保存段寄存器 ---
    # 使用 ECX 作为临时寄存器（保持一致性）
    movw %ds, %cx
    movzwl %cx, %ecx
    pushl %ecx

    movw %es, %cx
    movzwl %cx, %ecx
    pushl %ecx

    movw %fs, %cx
    movzwl %cx, %ecx
    pushl %ecx

    movw %gs, %cx
    movzwl %cx, %ecx
    pushl %ecx

    # --- 2. 切换为内核数据段 ---
    movl $0x10, %ecx
    movw %cx, %ds
    movw %cx, %es
    movw %cx, %fs
    movw %cx, %gs

    # --- 3. 保存通用寄存器 ---
    # pusha 顺序: EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    # 栈布局(低地址): EDI, ESI, EBP, old_esp, EBX, EDX, ECX, EAX
    pusha

    # ⚠️ 关键修复：内核态中断时，CPU没有压入SS/ESP
    #     我们需要在调用 do_irq_handler 之前压入 dummy 值
    #     CPU压入顺序：先压SS（高地址），后压ESP（低地址/栈顶）
    #     pushl指令先减ESP再写入，所以压入顺序必须是：先压SS，后压ESP
    pushl $0              # SS dummy (先压，会在高地址)
    pushl $0              # ESP dummy (后压，会在低地址/栈顶)

    # --- 4. 调用 C 层的 do_irq_handler ---
    # 栈布局（内核态中断，手动压入SS/ESP）：
    #   [0-28]   edi/esi/ebp/old_esp/ebx/edx/ecx/eax (pusha)
    #   [32-47]  gs/fs/es/ds
    #   [48-51]  err
    #   [52-55]  trapno
    #   [56-59]  eip
    #   [60-63]  cs
    #   [64-67]  eflags
    #   [68-71]  0 (dummy ESP)
    #   [72-75]  0 (dummy SS)
    pushl %esp
    call do_irq_handler
    addl $4, %esp

    # ⚠️ 关键：在调用 interrupt_exit 之前，弹出手动压入的 SS/ESP
    addl $8, %esp          # 清理 dummy SS/ESP

    # ⚠️⚠️⚠️ 关键修复：内核态中断不能使用 interrupt_exit！
    # interrupt_exit 会执行 iret，但内核态中断应该直接恢复寄存器并 ret
    # 原因：内核态中断返回不需要 iret（没有特权级切换），直接恢复寄存器即可

    # --- 5. 恢复通用寄存器 (popa 的逆序) ---
    popa                     # 恢复 EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI

    # --- 6. 恢复段寄存器 (后进先出) ---
    popl %ebx
    movw %bx, %gs
    popl %ebx
    movw %bx, %fs
    popl %ebx
    movw %bx, %es
    popl %ebx
    movw %bx, %ds

    # --- 7. 直接返回 (使用 iret) ---
    # ⚠️ 内核态中断：CPU只压入 EFLAGS, CS, EIP
    # iret 会自动恢复这3个值
    iret


# ================================
# trapret: 用于 alltraps_user 的返回路径
# ================================
    .globl trapret
trapret:
    # --- 1. 恢复通用寄存器 ---
    popl %edi
    popl %esi
    popl %ebp
    addl $4, %esp         # skip oesp
    popl %ebx
    popl %edx
    popl %ecx
    popl %eax

    # --- 2. 恢复段寄存器 ---
    popl %ebx
    movw %bx, %gs
    popl %ebx
    movw %bx, %fs
    popl %ebx
    movw %bx, %es
    popl %ebx
    movw %bx, %ds

    # --- 3. 跳过 trapno + errcode ---
    addl $8, %esp

    # --- 4. 检查特权级 ---
    movl 4(%esp), %eax
    testl $3, %eax
    jz trapret_kernel

trapret_user:
    iret

trapret_kernel:
    iret
