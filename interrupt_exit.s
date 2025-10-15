.text
.global interrupt_exit
interrupt_exit:

    #; 对应 push eax，调用结束恢复栈
    #add $4,%esp

    #; 调用信号处理函数
    #call default_signal_handler #task_signal

    #; 恢复下文寄存器信息
    popa
    pop %gs
    pop %fs
    pop %es
    pop %ds

    #; 对应 push %1
    #; 对应 error code 或 push magic
    add $8,%esp
    
    iret
