# syscalls.S - 系统调用包装函数
# 使用AT&T语法（GCC默认）

    .section .text
    .intel_syntax noprefix

# sys_putchar(c) - 输出一个字符
# 参数: [ebp+8]
    .globl sys_putchar
sys_putchar:
    push ebp
    mov ebp, esp
    mov eax, 8            # SYS_PUTCHAR
    mov ebx, [ebp+8]      # 获取字符参数
    int 0x80
    pop ebp
    ret

# sys_yield() - 让出CPU
    .globl sys_yield
sys_yield:
    mov eax, 3            # SYS_YIELD
    int 0x80
    ret

# sys_exit(code) - 退出进程
# 参数: [ebp+8]
    .globl sys_exit
sys_exit:
    push ebp
    mov ebp, esp
    mov eax, 2            # SYS_EXIT
    mov ebx, [ebp+8]      # 获取退出码
    int 0x80
    # 永远不会返回
1:  hlt
    jmp 1b

# sys_fork() - 创建子进程
    .globl sys_fork
sys_fork:
    mov eax, 11           # SYS_FORK
    int 0x80
    ret
