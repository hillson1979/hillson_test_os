# simple_start.S - 最简单的用户程序，用于测试 task_to_user_mode_with_task
# 这个程序只做一件事：无限循环输出 'A'

.section .text
.global _start
_start:
    # 无限循环：输出字符 'A'
    movl $65, %eax    # sys_call number for putchar (假设)
    movl $'A', %ebx   # character to print
    int $0x80         # system call

    jmp _start        # 无限循环
