# 用户态系统调用包装函数
# 这些函数从中断返回时不会破坏任何调用者保存的寄存器（除了EAX返回值）

# SYS_PUTCHAR系统调用 (参数: char c)
.global syscall_putchar
.global sys_putchar
syscall_putchar:
sys_putchar:
    movl $8, %eax          # 系统调用号 SYS_PUTCHAR
    movl 4(%esp), %ebx     # 参数1: 字符c
    int $0x80              # 触发系统调用
    ret

# SYS_GETCHAR系统调用 (无参数)
.global syscall_getchar
.global sys_getchar
syscall_getchar:
sys_getchar:
    movl $7, %eax          # 系统调用号 SYS_GETCHAR
    int $0x80              # 触发系统调用
    # 返回值在EAX中：读取的字符
    ret

# SYS_YIELD系统调用 (无参数)
.global syscall_yield
syscall_yield:
    movl $3, %eax          # 系统调用号 SYS_YIELD
    int $0x80              # 触发系统调用
    ret

# SYS_GET_MEM_STATS系统调用 (参数: void *stats)
.global syscall_get_mem_stats
.global sys_get_mem_stats
syscall_get_mem_stats:
sys_get_mem_stats:
    movl $4, %eax          # 系统调用号 SYS_GET_MEM_STATS
    movl 4(%esp), %ebx     # 参数1: stats指针
    int $0x80              # 触发系统调用
    ret

# SYS_GETCWD系统调用 (参数: char *buf, int size)
.global syscall_getcwd
.global getcwd
syscall_getcwd:
getcwd:
    movl $9, %eax          # 系统调用号 SYS_GETCWD
    movl 4(%esp), %ebx     # 参数1: buf
    movl 8(%esp), %ecx     # 参数2: size
    int $0x80              # 触发系统调用
    # 返回值在EAX中：路径长度或错误码
    ret

# SYS_EXIT系统调用 (参数: int code)
.global syscall_exit
.global sys_exit
syscall_exit:
sys_exit:
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

# SYS_OPEN系统调用 (参数: const char *pathname, int flags)
.global syscall_open
syscall_open:
    movl $20, %eax         # 系统调用号 SYS_OPEN
    movl 4(%esp), %ebx     # 参数1: pathname
    movl 8(%esp), %ecx     # 参数2: flags
    int $0x80              # 触发系统调用
    # 返回值在EAX中：文件描述符或错误码
    ret

# SYS_CLOSE系统调用 (参数: int fd)
.global syscall_close
syscall_close:
    movl $21, %eax         # 系统调用号 SYS_CLOSE
    movl 4(%esp), %ebx     # 参数1: fd
    int $0x80              # 触发系统调用
    # 返回值在EAX中：0或错误码
    ret

# SYS_READ系统调用 (参数: int fd, char *buf, int len)
.global syscall_read
syscall_read:
    movl $22, %eax         # 系统调用号 SYS_READ
    movl 4(%esp), %ebx     # 参数1: fd
    movl 8(%esp), %ecx     # 参数2: buf
    movl 12(%esp), %edx    # 参数3: len
    int $0x80              # 触发系统调用
    # 返回值在EAX中：读取的字节数
    ret

# SYS_LSEEK系统调用 (参数: int fd, int offset, int whence)
.global syscall_lseek
syscall_lseek:
    movl $23, %eax         # 系统调用号 SYS_LSEEK
    movl 4(%esp), %ebx     # 参数1: fd
    movl 8(%esp), %ecx     # 参数2: offset
    movl 12(%esp), %edx    # 参数3: whence
    int $0x80              # 触发系统调用
    # 返回值在EAX中：新的文件位置
    ret

# SYS_NET_PING系统调用 (参数: const char *ip_addr)
.global syscall_net_ping
syscall_net_ping:
    movl $30, %eax         # 系统调用号 SYS_NET_PING
    movl 4(%esp), %ebx     # 参数1: ip_addr 字符串指针
    int $0x80              # 触发系统调用
    # 返回值在EAX中：0表示成功，-1表示失败
    ret
