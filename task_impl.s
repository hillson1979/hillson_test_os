# 32位任务结构体偏移量
.set TASK_ESP,         0
.set TASK_ESP0,        4
.set TASK_HAS_SIGNAL,  8
.set TASK_CR3,         12
.set TASK_USP,         16
.set TASK_SIG_HANDLER, 20
.set TASK_IDLE_FLAGS,  24
.set TASK_INTR_DEPTH,  28  # 注意这里的正确偏移量
.set TASK_PID,         32
.set TASK_PPID,        36
.set TASK_UID,         40
.set TASK_GID,         44
.set TASK_STATE,       48
.set TASK_NICE,        52
.set TASK_VRUNTIME,    96
#.set TASK_IFRAME,      108     
# 用户态段选择子定义
.set USER_CS, 0x23       # 用户代码段选择子
.set USER_DS, 0x2B       # 用户数据段选择子

# TSS 结构定义
.set TSS_ESP0_OFFSET, 4
.set TSS_SIZE, 108       # TSS结构大小 (108字节)

# 外部符号声明
.extern do_irq_handler    # C语言实现的中断处理函数
.extern deliver_signal    # C语言实现的信号投递函数

# switch_to 实现
.type switch_to, @function
.global switch_to
switch_to:
    # 参数: 
    #   %eax = 当前进程的 thread_struct 指针
    #   %edx = 下一个进程的 thread_struct 指针
    
    # 1. 保存当前进程上下文
    pushfl                  # 保存标志寄存器
    pushl %cs               # 保存代码段
    pushl $.L1              # 保存返回地址
    pushl %ebp
    pushl %edi
    pushl %esi
    pushl %ebx
    
    movl %eax,current
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
    #movl TASK_ESP0(%edx), %eax
    #movl %eax, (tss + TSS_ESP0_OFFSET)

    # 4. 检查是否需要处理信号
    movl current, %ebx
    cmpl $0, TASK_HAS_SIGNAL(%ebx)
    jnz handle_signal_path
    
    # 5. 恢复下一个进程的上下文
.L1:
    popl %ebx
    popl %esi
    popl %edi
    popl %ebp
    addl $4, %esp         # 跳过返回地址
    addl $4, %esp         # 跳过 CS
    popfl                 # 恢复标志寄存器
    
    # 6. 设置 TSS.esp0 为当前进程的内核栈顶
    
    
    iret

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

# 中断入口处理
.type interrupt_handler, @function
.global interrupt_handler
interrupt_handler:
    # 检查是否来自用户态
    movl 4(%esp), %eax    # 获取 CS
    andl $3, %eax
    jz .Lkernel_intr      # 如果来自内核态，跳转
    
    # 来自用户态的中断
    # 切换到当前进程的内核栈
    movl current, %ebx
    movl TASK_ESP0(%ebx), %esp
    
    # 保存用户态上下文
    pushl %ss
    pushl %esp
    pushfl
    pushl %cs
    pushl $.Luser_ret     # 返回地址
    
    jmp .Lcommon_intr

.Lkernel_intr:
    # 来自内核态的中断 - 检查是否嵌套中断
    movl current, %ebx
    incl TASK_INTR_DEPTH(%ebx)  # 修正：使用任务结构体中的中断深度
    
    # 如果已经是嵌套中断，直接使用当前栈
    cmpl $1, TASK_INTR_DEPTH(%ebx)  # 修正：使用正确的偏移量
    jg .Lcommon_intr
    
    # 第一次中断，切换到中断栈
    movl $interrupt_stack, %esp

.Lcommon_intr:
    # 保存通用寄存器
    pushl %eax
    pushl %ebx
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %edi
    pushl %ebp
    
    # 调用 C 中断处理函数
    call do_irq_handler
    
    # 检查是否需要调度
    cmpl $0, need_resched
    jz .Lno_schedule
    
    # 需要调度，调用调度器
    call schedule

.global .Lno_schedule
.Lno_schedule:
    # 恢复寄存器
    popl %ebp
    popl %edi
    popl %esi
    popl %edx
    popl %ecx
    popl %ebx
    popl %eax
    
    # 检查中断深度
    movl current, %ebx
    decl TASK_INTR_DEPTH(%ebx)  # 修正：使用任务结构体中的中断深度
    jnz .Lnested_ret      # 如果是嵌套中断，特殊返回
    
    # 普通中断返回
    iret

.Lnested_ret:
    # 嵌套中断返回，不需要修改栈
    addl $4, %esp         # 跳过错误码（如果有）
    ret

.Luser_ret:
    # 用户态中断返回路径
    # 检查是否有待处理信号
    movl current, %ebx
    cmpl $0, TASK_HAS_SIGNAL(%ebx)
    jz .Lnormal_ret
    
    # 有信号需要处理
    call deliver_signal

.Lnormal_ret:
    # 正常返回到用户态
    iret

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


# 数据段
.data
.align 16
interrupt_stack: .space 4096 # 4KB 中断栈
tmp_stack:      .space 4096 # 4KB 临时栈

.global current
current:        .long 0    # 当前进程指针
.global need_resched
need_resched:   .long 0    # 需要重新调度标志

# 定义TSS结构
.global tss
tss:
    .fill TSS_SIZE, 1, 0   # 初始化TSS结构为0

