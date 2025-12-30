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

    #; 在返回用户空间前,确保CR3指向用户页表
    #; 检查当前是否是用户任务,如果是则切换到用户页表
    #; 注意:这里需要从当前任务的task结构中获取cr3值
    #; 暂时跳过,因为需要访问current_task变量

    iret
