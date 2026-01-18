# trap_entry.S
# 汇编实现所有中断入口 alltraps
# 用于 i386，兼容 GCC 内核编译 -m32 模式
# 提供符号 alltraps 供 C 函数 trap_init / set_gate 使用
#
# 设计原则（xv6 标准内核实现）：
# 1. 统一的 alltraps 入口，不分用户/内核路径
# 2. 先压段寄存器，后压 pusha，让栈布局完全匹配 struct trapframe
# 3. ESP 自然指向 trapframe 起点（edi），不做手动偏移计算
# 4. 让 CPU 自己决定 iret 是否需要特权级切换

    .section .text
    .align 4
    .globl alltraps
alltraps:
    # --- 1. 保存段寄存器（先压，在低地址） ---
    # ⚠️ 必须先压段寄存器，遵循 xv6 标准
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    # --- 2. 保存通用寄存器（后压，在高地址） ---
    # pusha 压入顺序: EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    # 栈布局（从低地址到高地址）：
    #   [ds][es][fs][gs][edi][esi][ebp][oesp][ebx][edx][ecx][eax]
    #   ↑                                                              ↑
    # ESP指向这里（trapframe起点，edi）                            当前栈顶
    pusha

    # --- 3. 切换为内核数据段 ---
    movl $0x10, %eax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # --- 4. 调用 C 层的 do_irq_handler ---
    # ⚠️ 关键：ESP 自然指向 trapframe 的起点（edi）
    #     直接传递 ESP，不需要任何偏移计算！
    pushl %esp
    call do_irq_handler
    addl $4, %esp

    # --- 4.5 检查 need_resched 标志，实现调度 ---
    # ⚠️⚠️⚠️ 关键修复：系统调用（如 yield）会设置 need_resched=1
    #      必须在这里检查并调用调度器，否则 yield 不起作用
    pushl %esp
    call check_and_schedule
    addl $4, %esp

    # --- 5. 恢复通用寄存器 ---
    popa

    # --- 6. 恢复段寄存器 ---
    popl %gs
    popl %fs
    popl %es
    popl %ds

    # --- 7. 清理 err + trapno ---
    addl $8, %esp

    # --- 8. 返回 ---
    # ⚠️ 让 CPU 自己决定是否特权级切换
    #     如果从用户态中断：CPU 会自动切换栈
    #     如果从内核态中断：CPU 只恢复 EIP/CS/EFLAGS
    iret
