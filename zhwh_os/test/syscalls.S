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

# SYS_NET_PING系统调用 (参数: const char *ip_addr, const char *dev_name)
.global syscall_net_ping
syscall_net_ping:
    movl $30, %eax         # 系统调用号 SYS_NET_PING
    movl 4(%esp), %ebx     # 参数1: ip_addr 字符串指针
    movl 8(%esp), %ecx     # 参数2: dev_name 字符串指针（可选）
    int $0x80              # 触发系统调用
    # 返回值在EAX中：0表示成功，-1表示失败
    ret

# SYS_NET_IFCONFIG系统调用 (无参数)
.global syscall_net_ifconfig
syscall_net_ifconfig:
    movl $31, %eax         # 系统调用号 SYS_NET_IFCONFIG
    int $0x80              # 触发系统调用
    # 返回值在EAX中：0表示成功，-1表示失败
    ret

# SYS_WIFI_INIT系统调用 (无参数)
.global syscall_wifi_init
syscall_wifi_init:
    movl $36, %eax         # 系统调用号 SYS_WIFI_INIT
    int $0x80              # 触发系统调用
    # 返回值在EAX中：0表示成功，-1表示失败
    ret

# SYS_WIFI_SCAN系统调用 (无参数)
.global syscall_wifi_scan
syscall_wifi_scan:
    movl $32, %eax         # 系统调用号 SYS_WIFI_SCAN
    int $0x80              # 触发系统调用
    # 返回值在EAX中：扫描到的网络数量
    ret

# SYS_WIFI_CONNECT系统调用 (参数: const char *ssid, const char *password)
.global syscall_wifi_connect
syscall_wifi_connect:
    movl $33, %eax         # 系统调用号 SYS_WIFI_CONNECT
    movl 4(%esp), %ebx     # 参数1: ssid
    movl 8(%esp), %ecx     # 参数2: password
    int $0x80              # 触发系统调用
    # 返回值在EAX中：0表示成功，-1表示失败
    ret

# SYS_WIFI_DISCONNECT系统调用 (无参数)
.global syscall_wifi_disconnect
syscall_wifi_disconnect:
    movl $34, %eax         # 系统调用号 SYS_WIFI_DISCONNECT
    int $0x80              # 触发系统调用
    # 返回值在EAX中：0表示成功，-1表示失败
    ret

# SYS_WIFI_STATUS系统调用 (无参数)
.global syscall_wifi_status
syscall_wifi_status:
    movl $35, %eax         # 系统调用号 SYS_WIFI_STATUS
    int $0x80              # 触发系统调用
    ret

# SYS_WIFI_FW_BEGIN系统调用 (参数: uint32_t size)
.global syscall_wifi_fw_begin
syscall_wifi_fw_begin:
    movl $37, %eax         # SYS_WIFI_FW_BEGIN
    movl 4(%esp), %ebx     # 参数1: size
    int $0x80
    ret

# SYS_WIFI_FW_CHUNK系统调用 (参数: const void *ptr, uint32_t len, uint32_t offset)
.global syscall_wifi_fw_chunk
syscall_wifi_fw_chunk:
    movl $38, %eax         # SYS_WIFI_FW_CHUNK
    movl 4(%esp), %ebx     # 参数1: ptr
    movl 8(%esp), %ecx     # 参数2: len
    movl 12(%esp), %edx    # 参数3: offset
    int $0x80
    ret

# SYS_WIFI_FW_END系统调用 (无参数)
.global syscall_wifi_fw_end
syscall_wifi_fw_end:
    movl $39, %eax         # SYS_WIFI_FW_END
    int $0x80
    ret

# SYS_WIFI_LOAD_FIRMWARE系统调用 (参数: const char *path, int fd)
.global syscall_wifi_load_firmware
syscall_wifi_load_firmware:
    movl $40, %eax         # 系统调用号 SYS_WIFI_LOAD_FIRMWARE
    movl 4(%esp), %ebx     # 参数1: path (NULL 如果 fd 有效)
    movl 8(%esp), %ecx     # 参数2: fd (-1 表示需要打开文件)
    int $0x80              # 触发系统调用
    # 返回值在EAX中：0表示成功，-1表示失败
    ret

# SYS_WIFI_LOAD_FIRMWARE_FROM_BUFFER系统调用 (参数: const struct user_buf *ubuf)
# 复用系统调用号 40，但传递 ubuf 指针作为 arg1
.global syscall_wifi_load_firmware_from_buffer
syscall_wifi_load_firmware_from_buffer:
    movl $40, %eax         # 复用 SYS_WIFI_LOAD_FIRMWARE 系统调用号
    movl 4(%esp), %ebx     # 参数1: ubuf 指针
    movl $-2, %ecx         # 特殊标记：-2 表示从缓冲区加载
    int $0x80              # 触发系统调用
    # 返回值在EAX中：0表示成功，-1表示失败
    ret

# SYS_EXECV系统调用 (参数: const char *path, char *const argv[])
.global syscall_execv
syscall_execv:
    movl $41, %eax         # 系统调用号 SYS_EXECV
    movl 4(%esp), %ebx     # 参数1: path
    movl 8(%esp), %ecx     # 参数2: argv
    int $0x80              # 触发系统调用
    # 返回值在EAX中：0表示成功，-1表示失败
    ret
