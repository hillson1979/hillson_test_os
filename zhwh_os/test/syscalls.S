# 用户态系统调用包装函数
# 这些函数从中断返回时不会破坏任何调用者保存的寄存器（除了EAX返回值）

# SYS_PUTCHAR系统调用 (参数: char c)
.global syscall_putchar
syscall_putchar:
    movl $8, %eax          # 系统调用号 SYS_PUTCHAR
    movl 4(%esp), %ebx     # 参数1: 字符c
    int $0x80              # 触发系统调用
    ret

# SYS_YIELD系统调用 (无参数)
.global syscall_yield
syscall_yield:
    movl $3, %eax          # 系统调用号 SYS_YIELD
    int $0x80              # 触发系统调用
    ret

# SYS_EXIT系统调用 (参数: int code)
.global syscall_exit
syscall_exit:
    movl $2, %eax          # 系统调用号 SYS_EXIT
    movl 4(%esp), %ebx     # 参数1: 退出码
    int $0x80              # 触发系统调用
    # 这个函数不应该返回，但为了安全起见
    ret

# SYS_FORK系统调用 (无参数，返回pid)
.global syscall_fork
syscall_fork:
    movl $11, %eax         # 系统调用号 SYS_FORK
    int $0x80              # 触发系统调用
    # 返回值在EAX中：父进程返回子进程PID，子进程返回0
    ret

# SYS_WRITE系统调用 (参数: int fd, const char *buf, int len)
.global syscall_write
syscall_write:
    movl $10, %eax         # 系统调用号 SYS_WRITE
    movl 4(%esp), %ebx     # 参数1: fd
    movl 8(%esp), %ecx     # 参数2: buf
    movl 12(%esp), %edx    # 参数3: len
    int $0x80              # 触发系统调用
    # 返回值在EAX中：写入的字节数
    ret
