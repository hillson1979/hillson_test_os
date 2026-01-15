# 32位任务结构体偏移量
.set TASK_ESP,         0
.set TASK_ESP0,        4
.set TASK_HAS_SIGNAL,  8
.set TASK_CR3,         12
.set TASK_USP,         16
.set TASK_SIG_HANDLER, 20
.set TASK_IDLE_FLAGS,  24
# ⚠️ TASK_INTR_DEPTH 已删除（Linux 不使用 per-task 中断深度）
.set TASK_PID,         32
.set TASK_PPID,        36
.set TASK_UID,         40
.set TASK_GID,         44
.set TASK_STATE,       48
.set TASK_NICE,        52
.set TASK_VRUNTIME,    96
#.set TASK_IFRAME,      108     
# 用户态段选择子定义
.set USER_CS, 0x1B       # 用户代码段选择子 (RPL=3, TI=0, index=3)
.set USER_DS, 0x23       # 用户数据段选择子 (RPL=3, TI=0, index=4)

# TSS 结构定义
.set TSS_ESP0_OFFSET, 4
.set TSS_SIZE, 108       # TSS结构大小 (108字节)

# 外部符号声明
.extern deliver_signal    # C语言实现的信号投递函数
.extern do_irq_handler    # C语言实现的中断处理函数 (在 trap_entry.S 中调用)

# switch_to 实现
.type switch_to, @function
.global switch_to
switch_to:
    # 参数（通过栈传递，cdecl 调用约定）:
    #   4(%esp) = prev (当前进程的 task_t 指针)
    #   8(%esp) = next (下一个进程的 task_t 指针)

    # ⚠️⚠️⚠️ 关键设计原则：switch_to 只能使用 ret，不能使用 iret！
    # 原因：switch_to 被schedule()调用，不是从中断处理调用
    # 栈上没有 CPU 自动压入的中断帧，iret 会读取错误的栈数据

    # 1. 保存当前进程上下文
    pushl %ebp
    pushl %edi
    pushl %esi
    pushl %ebx

    # 从栈上读取参数
    movl 20(%esp), %eax      # eax = prev
    movl 24(%esp), %edx      # edx = next

    # ⚠️⚠️⚠️ 更新全局 current 指针（汇编代码需要）
    movl %edx, current

    # 保存当前栈指针到当前进程的 thread_struct
    movl %esp, TASK_ESP(%eax)

    # 2. 切换地址空间 - 这是关键！
    movl TASK_CR3(%edx), %ebx    # 获取新进程的页目录物理地址
    movl %cr3, %ecx              # 获取当前CR3
    cmpl %ecx, %ebx              # 比较是否相同
    je .Lsame_address_space      # 如果相同，跳过CR3设置

    # 切换地址空间
    movl %ebx, %cr3              # 加载新进程的页目录

.Lsame_address_space:

    # 3. 切换到下一个进程的栈
    movl TASK_ESP(%edx), %esp

    # 3.5 关键修复:更新TSS.esp0为当前任务的内核栈顶
    # 这样从中断/syscall从用户态进入内核时,CPU会自动切换到正确的内核栈
    movl TASK_ESP0(%edx), %eax      # 获取新任务的esp0
    movl %eax, tss + TSS_ESP0_OFFSET # 更新TSS.esp0

    # ================================
    # Linux 模型关键修复：
    # ================================
    # switch_to 只负责切换内核栈,不关心是内核任务还是用户任务
    # 统一流程:
    #   1. 保存当前寄存器到栈
    #   2. 保存 ESP 到 prev->esp
    #   3. 切换到 next->esp
    #   4. 恢复寄存器
    #   5. ret 到调用者(schedule)
    #
    # 如果 next 是用户任务:
    #   - 它的内核栈上有 trapframe(上次中断时压入的)
    #   - schedule 返回后,interrupt_exit 会恢复这个 trapframe
    #   - 这是唯一执行 iret 的地方!
    #
    # ⚠️ 绝不在 switch_to 中:
    #   - 跳转到 interrupt_exit
    #   - 执行 iret
    #   - 弹出 trapframe

    # 4. ⚠️⚠️⚠️ 检查是否需要处理信号(仅内核任务)
    #    统一返回路径，不在汇编中判断任务类型
    movl current, %ebx
    cmpl $0, TASK_HAS_SIGNAL(%ebx)
    jnz handle_signal_path

    # 5. 恢复下一个进程的上下文(统一路径)
.L1:
    popl %ebx
    popl %esi
    popl %edi
    popl %ebp

    # ⚠️⚠️⚠️ 统一使用 ret 返回到 schedule
    # switch_to 只负责切栈，不判断任务类型
    # 由 schedule() 决定下一步是返回 interrupt_exit 还是其他
    ret

# 信号处理路径
handle_signal_path:
    # 保存当前寄存器状态
    pushl %eax
    pushl %edx
    
    # 调用信号处理设置函数
    call setup_signal_handler
    
    popl %edx
    popl %eax
    
    # 继续正常恢复流程
    jmp .L1

# ⚠️⚠️⚠️ 注意：interrupt_handler 已删除
# 原因：
#   1. IDT 使用的是 alltraps (在 trap_entry.S 中)
#   2. interrupt_handler 从未被调用（grep 无结果）
#   3. 避免混淆：统一使用 trap_entry.S 的实现
#
# 正确的中断流程：
#   vectors.S → alltraps → do_irq_handler → interrupt_exit → iret
#   (不在 task_impl.s 中重复实现)

# 信号处理设置
.type setup_signal_handler, @function
setup_signal_handler:
    # 获取当前进程
    movl current, %ebx
    
    # 构建信号处理栈帧
    movl TASK_USP(%ebx), %esp  # 切换到用户栈
    
    # 压入信号处理所需信息
    pushl $USER_DS        # SS
    pushl TASK_USP(%ebx)  # ESP
    pushfl                # EFLAGS
    pushl $USER_CS        # CS
    pushl TASK_SIG_HANDLER(%ebx) # EIP (信号处理函数)
    
    # 设置段寄存器为用户段
    movw $USER_DS, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    
    # 清除信号标志
    movl $0, TASK_HAS_SIGNAL(%ebx)
    
    ret


# trapframe 偏移量（匹配 interrupt.h 中的 struct trapframe）
# ⚠️ 关键修复：struct trapframe 的实际布局（按C结构体定义）
# struct从offset 0开始：edi(0), esi(4), ebp(8), oesp(12), ebx(16), edx(20), ecx(24), eax(28)
#                      ds(32), es(36), fs(40), gs(44), trapno(48), err(52)
#                      eip(56), cs(60), eflags(64), esp(68), ss(72)
.set TF_EDI,         0     # ⚠️ 修正: edi = offset 0 (pusha起始位置)
.set TF_ESI,         4     # ⚠️ 修正: esi = offset 4
.set TF_EBP,         8     # ⚠️ 修正: ebp = offset 8
.set TF_OESP,        12    # ⚠️ 修正: oesp = offset 12
.set TF_EBX,         16    # ⚠️ 修正: ebx = offset 16
.set TF_EDX,         20    # ⚠️ 修正: edx = offset 20
.set TF_ECX,         24    # ⚠️ 修正: ecx = offset 24
.set TF_EAX,         28    # ⚠️ 修正: eax = offset 28
.set TF_DS,          32    # ⚠️ 修正: ds = offset 32 (C结构体定义)
.set TF_ES,          36    # ⚠️ 修正: es = offset 36
.set TF_FS,          40    # ⚠️ 修正: fs = offset 40
.set TF_GS,          44    # ⚠️ 修正: gs = offset 44
.set TF_EIP,         56
.set TF_CS,          60
.set TF_EFLAGS,      64
.set TF_ESP,         68    # 用户ESP
.set TF_SS,          72    # 用户SS

# task_t 偏移量（根据 include/task.h 的结构体定义计算）
# ⚠️ 关键修复：重新计算偏移量
# 从task.h的结构体定义手动计算：
#   - esp (0-3)
#   - esp0 (4-7)
#   - has_signal (8-11)
#   - cr3 (12-15)
#   - user_stack (16-19)
#   - signal_handler (20-23)
#   - idle_flags (24-27)
#   - intr_depth (28-31)
#   - pid (32-35)
#   - ppid (36-39)
#   - uid (40-43)
#   - gid (44-47)
#   - state (48-51)
#   - nice (52-55)
#   - start_time (56-59)
#   - waitpid (60-63)
#   - cpu (64-67)
#   - directory (68-71)
#   - name (72-75)
#   - size (76-79)
#   - csd (80-83)
#   - load_weight (84-87)
#   - entry (88-91)
#   - time_slice (92-95)
#   - vruntime (96-103) ⚠️ 8 bytes!
#   - sched_node (104-107)
#   - sleep (104-115) ⚠️ sizeof(struct haybed) = 12 bytes
#   - mm (116-119)
#   - prev (120-123)
#   - next (124-127)
#   - check_idle (128-131)
#   - idle_context (132-135)
#   - sig_handler (136-139)
#   - signal_mask (140-143)
#   - pending_signals (144-147)
#   - tf (148-151) ⚠️ 修正：删除 intr_depth 后，偏移量变为 148
#   - task_total_count (152-155)
#   - pde (156-159)
#   - kstack (160-163)
#   - iret_frame (164-183) ⚠️ 新增：uint32_t iret_frame[5] = 20字节
.set TASK_TF,        148   # ⚠️ 修正：删除 intr_depth 后，tf = offset 148
.set TASK_PDE,       156   # ⚠️ 修正：删除 intr_depth 后，pde = offset 156
.set TASK_KSTACK,    160   # ⚠️ 修正：删除 intr_depth 后，kstack = offset 160
.set TASK_IRET_FRAME, 164  # ⚠️ 新增：删除 intr_depth 后，iret_frame = offset 164

# 外部符号声明
.extern th_u
.extern debug_print_enter
.extern debug_print_tf_ptr
.extern debug_print_tf_values

# task_to_user_mode_with_task - 第一次进入用户态的专用函数
# 参数: eax = task指针 (用户任务的task_t*)
#
# ⚠️⚠️⚠️ 设计原则（必须遵守）：
# 1. 这个函数只用于"第一次进入用户态"
# 2. 绝不能使用 interrupt_exit（interrupt_exit 只用于 CPU 自动中断路径）
# 3. 必须使用预分配的 task->tf (trapframe) 来恢复上下文
# 4. 简化栈操作，避免复杂的多层压栈
#
# 参考：Linux 的 ret_from_fork, xv6 的 trapret, BSD 的 userret
.type task_to_user_mode_with_task, @function
.global task_to_user_mode_with_task
task_to_user_mode_with_task:
    cli                         # 关中断
    movl %eax, %ecx              # ecx = task 指针
    movw $0x10, %ax             # 内核数据段
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    movl TASK_TF(%ecx), %ebx     # ebx = task->tf
    cmpl $0, %ebx
    je 1f

    # 复制 trapframe 到栈上
    pushl %ecx               # 保存 task 指针 (+4)
    pushl %ebx               # 保存 task->tf 指针 (+4)
    movl $76, %eax
    subl %eax, %esp          # ESP -= 76 (分配空间)
    movl %esp, %edi
    movl 76(%esp), %esi      # ESI = task->tf (从栈上获取)
    pushl %ecx               # 保存 task 指针（rep movsl 会破坏 ECX）(+4)
    movl $19, %ecx
    cld
    rep movsl                # 复制 76 字节
    popl %ecx                # 恢复 task 指针 (-4)

    # 栈布局现在是（从ESP往高地址）：
    # [0-75: 76字节 trapframe 副本]
    # [76-79: task->tf 指针]
    # [80-83: task 指针]
    # ESP指向 trapframe 副本的开始 (offset 0)

    # 恢复 task 指针到 ECX
    movl 80(%esp), %ecx      # 从偏移80获取 task 指针

    # 切换 CR3
    movl TASK_CR3(%ecx), %edx
    movl TASK_ESP0(%ecx), %eax
    movl %eax, tss + TSS_ESP0_OFFSET
    movl %edx, %cr3

    # 恢复通用寄存器 (popa 从 ESP 恢复 32 字节)
    popa                      # ESP += 32, 指向 gs
    popl %eax; movw %ax, %gs  # ESP += 4, 指向 fs
    popl %eax; movw %ax, %fs  # ESP += 4, 指向 es
    popl %eax; movw %ax, %es  # ESP += 4, 指向 ds
    popl %eax; movw %ax, %ds  # ESP += 4, 指向 trapno

    addl $8, %esp             # ESP += 8, 跳过 trapno/err, 指向 eip

    # 栈平衡计算：
    # 压栈：4(task) + 4(tf) + 76(分配) + 4(task) = 88 字节
    # 弹栈：4(popl ecx) + 32(popa) + 16(4个popl) + 8(addl) = 60 字节
    # 剩余栈空间：88 - 60 = 28 字节
    #
    # 当前ESP指向 trapframe 副本中 offset 56 (eip)
    # iret 需要：eip, cs, eflags, esp, ss (20字节)
    # 这些数据已经在正确的位置了！

    iret

1:
    cli
    hlt
    jmp 1b

interrupt_stack: .space 4096
tmp_stack:      .space 4096

# ⚠️⚠️⚠️ 全局 current 指针（汇编代码使用）
# C 代码必须在 schedule() 中更新这个值！
.global current
current:        .long 0    # 当前进程指针（单CPU简化）
.global need_resched
need_resched:   .long 0    # 需要重新调度标志


# 定义TSS结构
.global tss
tss:
    .fill TSS_SIZE, 1, 0   # 初始化TSS结构为0

