# trap_entry.S
# 汇编实现所有中断入口 alltraps
# 用于 i386，兼容 GCC 内核编译 -m32 模式
# 提供符号 alltraps 供 C 函数 trap_init / set_gate 使用

    .section .text
    .align 4
    .globl alltraps
alltraps:
    # --- 1. 保存段寄存器 ---
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    # --- 2. 保存通用寄存器 ---
    pusha                # edi, esi, ebp, esp(dummy), ebx, edx, ecx, eax

    # --- 3. 切换为内核数据段 ---
    movl $0x10, %ecx     # 0x10 = 内核数据段选择子 (GDT 第2项)
    movw %cx, %ds
    movw %cx, %es
    movw %cx, %fs
    movw %cx, %gs

    # --- 4. 调用 C 层的 do_irq_handler(struct trapframe *tf) ---
    # 栈布局从ESP往高地址:
    #   [0] EAX, [4] ECX, [8] EDX, [12] EBX, [16] OESP, [20] EBP, [24] ESI, [28] EDI
    #   [32] GS, [36] FS, [40] ES, [44] DS, [48] trapno, [52] err, [56+] 硬件压入的值
    # struct trapframe从eax开始定义,传递ESP作为指针
    pushl %esp           # 直接传递ESP指针
    call do_irq_handler
    addl $4, %esp         # 清理参数

    # --- 5. 恢复寄存器 ---
    popa
    popl %gs
    popl %fs
    popl %es
    popl %ds

    # --- 6. 丢弃 trapno 和 errcode ---
    addl $8, %esp

    # --- 7. 返回到被中断现场 ---
    iret

    # --- trapret 函数：从内核空间返回到用户空间 ---
    .globl trapret
    .type trapret, @function
trapret:
    # --- 1. 恢复段寄存器 ---
    popl %ds
    popl %es
    popl %fs
    popl %gs

    # --- 2. 恢复通用寄存器 ---
    popa

    # --- 3. 丢弃 trapno 和 errcode ---
    addl $8, %esp

    # --- 4. 返回到用户空间 ---
    iret
