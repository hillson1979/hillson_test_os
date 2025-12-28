# simple_test.S - 最简单的用户程序测试
.section .text
.global _start

_start:
    # 直接调用 sys_write 系统调用
    mov $1, %eax          # 系统调用号 1 = SYS_PRINTF
    mov $msg, %ebx        # 字符串地址
    mov $0, %ecx          # 第二个参数（未使用）
    mov $0, %edx          # 第三个参数（未使用）
    int $0x80             # 触发系统调用

    # 无限循环
loop:
    jmp loop

.section .rodata
msg:
    .asciz "Hello from user mode!\n"
