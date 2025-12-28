# 用户进程启动中断13错误分析

## 当前状态

**问题**: `start_task_user()` 执行时报中断13错误 (General Protection Fault)

## 已尝试的修复

1. ✅ 注释掉 `test_kernel_user_memory_isolation()` - 无效
2. ✅ 注释掉 kmalloc 测试代码 - 无效  
3. ✅ 修复 CR3 加载类型转换 - 无效
4. ✅ 回退 userboot.c 和 task.c 的修改 - 无效

## 关键发现

- 用户反馈: "一直到start_task_user,gdb都是正常的"
- 说明问题出在 `start_task_user()` **函数内部**

## start_task_user() 执行流程

```c
void start_task_user(struct task_t* th, task_entry_callback_t entry_user) {
    start_task(th, entry_user);    // 添加到调度队列
    if (th->entry != NULL) {
        entry_user(th);             // 调用 user_task_main(th)
    }
}
```

然后 `user_task_main()` 执行:
```c
void user_task_main(struct task_t* th) {
    printf("User task started\n");
    // ... 循环 ...
    task_prepare_pde();      // 准备页目录
    task_to_user_mode();     // 切换到用户模式 ← 问题很可能在这里
}
```

## 可能的问题点

### 1. task_prepare_pde() 问题
- `th_u->pde = (uint32_t*)pde.phys;` - 物理地址存储
- `copy_kernel_mappings_to_pd(th_u->pde)` - 复制内核映射
- `map_page(th_u->pde, ...)` - 映射用户内存

### 2. task_to_user_mode() 问题
- 设置段寄存器 (ds, es, fs, gs, ss, cs)
- 设置TSS (ss0, esp0)
- **切换CR3** ← 已修复类型转换
- 跳转到 `interrupt_exit` 执行 iret

### 3. interrupt_exit 问题
```asm
interrupt_exit:
    popa                    # 恢复通用寄存器
    pop %gs, %fs, %es, %ds  # 恢复段寄存器
    add $8, %esp            # 跳过 trapno, err
    iret                    # 返回用户空间 ← 在这里出错?
```

## 中断13的可能原因

1. **段选择子错误** - 段描述符表配置不正确
2. **栈指针错误** - esp或ss指向无效内存
3. **EIP错误** - 用户程序入口点无效
4. **页表错误** - 用户空间未正确映射
5. **特权级违反** - CPL/DPL不匹配

## 需要检查的关键数据

1. **trapframe内容** - 在iret前检查所有寄存器值
2. **CR3值** - 确认页表基址正确
3. **用户页表** - 确认ELF段和栈已映射
4. **GDT内容** - 确认段描述符正确
5. **EIP值** - 确认ELF入口点 0x08000000 有效

## 调试建议

使用GDB在以下位置设置断点:
1. `task_to_user_mode` 入口
2. CR3加载前后
3. `interrupt_exit` 入口
4. iret指令前

检查寄存器状态:
- EIP = 0x08000000 (ELF入口)
- ESP = 0xBFFFF000 (用户栈顶)
- CS = 0x1B (用户代码段)
- DS/ES/FS/GS/SS = 0x23 (用户数据段)
- CR3 = 用户页目录物理地址
