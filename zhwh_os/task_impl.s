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
.set TASK_HAS_RUN_USER, 52  # has_run_user 标志
.set TASK_NICE,        56
.set TASK_VRUNTIME,    96
.set TASK_IFRAME,      152     # tf 字段偏移量（根据 GDB 内存 dump 验证：offset 0x98 = 0xC02A3114）

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

    # ⚠️⚠️⚠️ 关键修复：在 push 任何寄存器之前，先从栈上读取参数！
    # 原因：某些 GCC 编译器可能使用寄存器传递参数（fastcall）
    # 所以我们必须从栈上读取，而不是依赖寄存器
    movl 4(%esp), %eax      # eax = prev (从栈上读取)
    movl 8(%esp), %esi      # esi = next (从栈上读取) ⚠️ 用 esi 保存 next！

    # ⚠️⚠️⚠️ 关键设计原则：switch_to 只能使用 ret，不能使用 iret！
    # 原因：switch_to 被schedule()调用，不是从中断处理调用
    # 栈上没有 CPU 自动压入的中断帧，iret 会读取错误的栈数据

    # 1. 保存当前进程上下文
    pushl %ebp
    pushl %edi
    pushl %esi              # ⚠️ 保存 esi（next 指针）
    pushl %ebx

    # ⚠️⚠️⚠️ 现在 next 指针在栈上，需要时恢复到 esi
    # (由于 push 了 4 个寄存器，栈偏移变成了 +16)

    # ⚠️⚠️⚠️ 恢复 next 指针到 esi（从栈上）
    movl 12(%esp), %esi     # esi = next（从保存的位置恢复）

    # ⚠️⚠️⚠️ 更新全局 current 指针（汇编代码需要）
    movl %esi, current

    # 保存当前栈指针到当前进程的 thread_struct
    movl %esp, TASK_ESP(%eax)

    # 2. 切换地址空间 - 这是关键！
    movl TASK_CR3(%esi), %ebx    # 获取新进程的页目录物理地址 ⚠️ 用 esi
    movl %cr3, %ecx              # 获取当前CR3
    cmpl %ecx, %ebx              # 比较是否相同
    je .Lsame_address_space      # 如果相同，跳过CR3设置

    # 切换地址空间
    movl %ebx, %cr3              # 加载新进程的页目录

.Lsame_address_space:

    # 3. 切换到下一个进程的栈
    movl TASK_ESP(%esi), %esp       # ⚠️ 用 esi

    # 3.5 关键修复:更新TSS.esp0为当前任务的内核栈顶
    # 这样从中断/syscall从用户态进入内核时,CPU会自动切换到正确的内核栈
    movl TASK_ESP0(%esi), %eax      # 获取新任务的esp0 ⚠️ 用 esi
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
# ⚠️ 关键修复：添加 has_run_user 后重新计算偏移量
# 从task.h的结构体定义手动计算：
#   - esp (0-3)
#   - esp0 (4-7)
#   - has_signal (8-11)
#   - cr3 (12-15)
#   - user_stack (16-19)
#   - signal_handler (20-23)
#   - idle_flags (24-27)
#   - intr_depth (28-31) ⚠️ 已删除
#   - pid (32-35)
#   - ppid (36-39)
#   - uid (40-43)
#   - gid (44-47)
#   - state (48-51)
#   - has_run_user (52-55) ⚠️ 新增！
#   - nice (56-59) ⚠️ +4
#   - start_time (60-63) ⚠️ +4
#   - waitpid (64-67) ⚠️ +4
#   - cpu (68-71) ⚠️ +4
#   - directory (72-75) ⚠️ +4
#   - name (76-79) ⚠️ +4
#   - size (80-83) ⚠️ +4
#   - csd (84-87) ⚠️ +4
#   - load_weight (88-91) ⚠️ +4
#   - entry (92-95) ⚠️ +4
#   - time_slice (96-99) ⚠️ +4
#   - vruntime (100-107) ⚠️ +4, 8 bytes!
#   - sched_node (108-111) ⚠️ +4
#   - sleep (108-119) ⚠️ +4, sizeof(struct haybed) = 12 bytes
#   - mm (120-123) ⚠️ +4
#   - prev (124-127) ⚠️ +4
#   - next (128-131) ⚠️ +4
#   - check_idle (132-135) ⚠️ +4
#   - idle_context (136-139) ⚠️ +4
#   - sig_handler (140-143) ⚠️ +4
#   - signal_mask (144-147) ⚠️ +4
#   - pending_signals (148-151) ⚠️ +4
#   - tf (152-155) ⚠️ +4
#   - task_total_count (156-159) ⚠️ +4
#   - pde (160-163) ⚠️ +4
#   - kstack (164-167) ⚠️ +4
#   - iret_frame (168-183) ⚠️ +4, uint32_t iret_frame[5] = 20字节
.set TASK_PDE,       160   # ⚠️ 添加 has_run_user 后，pde = offset 160
.set TASK_KSTACK,    164   # ⚠️ 添加 has_run_user 后，kstack = offset 164
.set TASK_IRET_FRAME, 168  # ⚠️ 添加 has_run_user 后，iret_frame = offset 168

# 外部符号声明
.extern th_u
.extern debug_print_enter
.extern debug_print_tf_ptr
.extern debug_print_tf_values
.extern current_task           # Linux/xv6 风格：全局 current_task 数组

# task_to_user_mode_with_task - 第一次进入用户态的专用函数
# ⚠️⚠️⚠️ 设计原则（必须遵守）：
# 1. 这个函数只用于"第一次进入用户态"
# 2. 绝不能使用 interrupt_exit（interrupt_exit 只用于 CPU 自动中断路径）
# 3. 必须使用预分配的 task->tf (trapframe) 来恢复上下文
# 4. 简化逻辑：直接通过 EBX 传递 task 指针，避免依赖 current_task[]
#
# 参考：Linux 的 ret_from_fork, xv6 的 trapret, BSD 的 userret
#
# ⚠️⚠️⚠️ 调用约定：
#   输入：栈上传递参数 [ESP+4] = task 指针
#   输出：无（执行 iret 进入用户态，永不返回）
.type task_to_user_mode_with_task, @function
.global task_to_user_mode_with_task
task_to_user_mode_with_task:
    cli                         # 关中断

    # ⚠️ 从栈上读取 task 指针参数（C 调用约定）
    movl 4(%esp), %ebx          # EBX = task 指针

    # 验证 EBX 的值
    pushl %ebx
    pushl $task_to_user_mode_ebx_msg
    call printf
    addl $8, %esp

    # 获取 task->tf 指针
    movl TASK_IFRAME(%ebx), %ecx     # ecx = task->tf
    cmpl $0, %ecx
    je 1f

    # 切换到 task->tf 所指的栈
    movl %ecx, %esp             # ESP = task->tf

    # 切换 CR3
    movl TASK_CR3(%ebx), %edx
    movl TASK_ESP0(%ebx), %eax
    movl %eax, tss + TSS_ESP0_OFFSET
    movl %edx, %cr3

    # ⚠️⚠️⚠️ 现在 ESP 指向 trapframe，按照 struct trapframe 布局恢复寄存器
    # struct trapframe 布局（include/interrupt.h:121-159）：
    #   offset 0-28: pusha 压入的通用寄存器
    #     [0:edi, 4:esi, 8:ebp, 12:oesp, 16:ebx, 20:edx, 24:ecx, 28:eax]
    #   offset 32-44: 段寄存器
    #     [32:ds, 36:es, 40:fs, 44:gs]
    #   offset 48-52: 中断信息
    #     [48:trapno, 52:err]
    #   offset 56-72: CPU 硬件压入的值
    #     [56:eip, 60:cs, 64:eflags, 68:esp, 72:ss]

    # 恢复通用寄存器（pusha 的逆序）
    popl %edi                 # offset 0
    popl %esi                 # offset 4
    popl %ebp                 # offset 8
    addl $4, %esp             # 跳过 oesp (offset 12)
    popl %ebx                 # offset 16（用户态 EBX）
    popl %edx                 # offset 20
    popl %ecx                 # offset 24
    popl %eax                 # offset 28

    # 恢复段寄存器
    popl %ds                  # offset 32
    popl %es                  # offset 36
    popl %fs                  # offset 40
    popl %gs                  # offset 44

    # 跳过 trapno 和 err（offset 48-52）
    addl $8, %esp

    # ⚠️⚠️⚠️ 执行 iret，恢复用户态
    # iret 会从栈上弹出：EIP, CS, EFLAGS, ESP, SS
    iret

1:
    # task->tf 为 NULL 的错误处理
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

# 调试消息字符串
.section .rodata
task_to_user_mode_debug_msg:
    .string "[task_to_user_mode_with_task] ENTRY: EAX=0x%x (should be 0xC02A2160)\n"
task_tf_debug_msg:
    .string "[task_to_user_mode_with_task] task->tf (EBX) = 0x%x\n"
task_to_user_mode_ebx_addr_msg:
    .string "[task_to_user_mode_with_task] &current_task = 0x%x\n"
task_to_user_mode_ebx_msg:
    .string "[task_to_user_mode_with_task] EBX (current_task[0]) = 0x%x\n"

