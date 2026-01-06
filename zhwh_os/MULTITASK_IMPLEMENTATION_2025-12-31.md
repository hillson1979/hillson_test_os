# 多任务调度器实现总结

**日期**: 2025-12-31
**版本**: v1.0

---

## 📋 概述

本次实现为 Hillson Test OS 添加了基础的多任务调度功能，采用简化的轮转调度算法（Round-Robin）。

---

## ✅ 已实现的功能

### 1. 任务切换机制

**文件**: [sched.c](sched.c:352-382)

```c
static inline void switch_to(struct task_t *prev, struct task_t *next)
```

**功能**：
- 保存当前任务的内核栈指针
- 切换页表（CR3寄存器）
- 恢复下一个任务的内核栈指针
- 更新全局 `current_task` 指针
- 更新 TSS.esp0（内核栈顶）

### 2. 时钟中断调度

**文件**: [interrupt.c](interrupt.c:95-115)

**实现细节**：
- 每 10 个时钟中断触发一次调度
- 设置 `need_resched` 标志
- 在中断返回前执行调度检查

```c
#define TIME_SLICE 10  // 每10个时钟中断触发一次调度

void handle_timer_interrupt(struct trapframe *tf){
    extern uint32_t ticks;
    ticks++;
    timer_ticks++;

    if (timer_ticks >= TIME_SLICE) {
        timer_ticks = 0;
        extern int need_resched;
        need_resched = 1;
    }
}
```

### 3. 中断返回调度

**文件**: [interrupt_exit.s](interrupt_exit.s:13-20)

```assembly
#; 检查是否需要调度
cmpl $0, need_resched
je 1f
#; 需要调度，调用调度器
call schedule
#; 清除调度标志
movl $0, need_resched
1:
```

### 4. 简化的轮转调度器

**文件**: [sched.c](sched.c:264-295)

```c
static struct task_t *pick_next_task_cfs()
{
    uint8_t cpu_id = logical_cpu_id();
    struct task_t *current = current_task[cpu_id];
    struct task_t *next = NULL;

    if (!current) {
        return NULL;
    }

    // 如果当前任务正在运行，标记为就绪
    if (current->state == PS_RUNNING) {
        current->state = PS_READY;
    }

    // 简单的轮转：从任务链表中找下一个就绪任务
    next = current->next;
    while (next != NULL && next != current) {
        if (next->state == PS_READY && can_schedule(next)) {
            break;
        }
        next = next->next;
    }

    // 如果没找到其他就绪任务，保持当前任务
    if (next == current || next == NULL) {
        next = current;
    }

    return next;
}
```

### 5. yield 系统调用

**文件**: [syscall.c](syscall.c:155-161)

```c
case SYS_YIELD: {
    // 让出CPU,调度其他任务
    extern int need_resched;
    need_resched = 1;
    tf->eax = 0;
    break;
}
```

### 6. 测试程序

**文件**: [test/multitask_test.c](test/multitask_test.c)

**功能**：
- 测试 `sys_yield()` 系统调用
- 演示任务主动让出 CPU
- 单任务环境下的 yield 行为验证

---

## 🔧 核心数据结构

### 任务状态

```c
#define PS_READY     0   // 就绪态
#define PS_RUNNING   1   // 运行态
#define PS_TERMNAT   2   // 终止态
#define PS_DESTROY   4   // 销毁态
#define PS_BLOCKED   16  // 阻塞态
```

### 任务结构体

```c
typedef struct task_t {
    uint32_t esp;              // 内核栈指针
    uint32_t *pde;             // 页目录
    uint32_t *kstack;          // 内核栈顶
    pid_t pid;                 // 进程ID
    pid_t ppid;                // 父进程ID
    int state;                 // 任务状态
    struct task_t *next;       // 下一个任务（链表）
    struct task_t *prev;       // 前一个任务（链表）
    struct trapframe *tf;      // 陷阱帧
    // ... 其他字段
} task_t;
```

---

## 📊 调度流程

```
1. 时钟中断 (IRQ0) 或 sys_yield()
   ↓
2. handle_timer_interrupt() / syscall_dispatch()
   设置 need_resched = 1
   ↓
3. interrupt_exit (中断返回路径)
   检查 need_resched 标志
   ↓
4. schedule()
   - 保存当前任务上下文
   - 调用 pick_next_task_cfs() 选择下一个任务
   - 调用 switch_to() 执行切换
   ↓
5. switch_to()
   - 切换内核栈 (ESP)
   - 切换页表 (CR3)
   - 更新 TSS
   ↓
6. 返回用户空间，新任务开始运行
```

---

## 🎯 设计特点

### 简化设计决策

1. **轮转调度而非 CFS**
   - 原代码有完整的 CFS（完全公平调度器）框架
   - 简化为简单的 Round-Robin 算法
   - 易于理解和调试

2. **单 CPU 支持**
   - 使用 `current_task[cpu_id]` 数组
   - 当前只使用 CPU 0

3. **简单的任务链表**
   - 使用双向链表管理任务
   - 通过 `next/prev` 指针遍历

4. **时间片调度**
   - 每 10 个时钟中断触发调度
   - 可通过修改 `TIME_SLICE` 调整

---

## ⚠️ 当前限制

1. **只有单任务切换**
   - 没有实现 `fork()` 系统调用
   - 无法在用户程序中创建新任务
   - 测试程序只能演示 yield 功能

2. **简化的调度算法**
   - 不考虑优先级
   - 不考虑任务权重
   - 简单的轮转顺序

3. **没有进程间通信**
   - 无管道、消息队列
   - 无共享内存机制

4. **缺少进程管理**
   - 没有 `wait()`/`exit()` 的完整实现
   - 没有僵尸进程回收

---

## 📝 后续开发计划

### 阶段 2：fork 系统调用（下一步）

1. 实现 `sys_fork()`
   - 复制当前任务结构
   - 复制页表（COW 或完全复制）
   - 设置子任务的返回值

2. 实现 `sys_exec()`
   - 加载新的 ELF 程序
   - 替换当前任务的地址空间

3. 实现多任务测试程序
   - 创建父子进程
   - 演示并发执行

### 阶段 3：进程管理

1. `sys_wait()` - 等待子进程
2. `sys_exit()` - 完整的进程退出
3. 僵尸进程回收
4. 进程状态转换图

### 阶段 4：进程间通信

1. 管道（pipe）
2. 信号机制
3. 共享内存

---

## 🧪 测试方法

### 编译

```bash
# 编译内核
cd zhwh_os
make clean && make

# 编译测试程序
cd test
make clean && make
```

### 创建 ISO 镜像

```bash
cd ..
bash zh.sh
```

### 运行测试

```bash
# QEMU 测试
qemu-system-i386 -cdrom os.iso -m 4096

# 真机测试
# 将 os.iso 刻录到 USB，从 USB 启动
```

### 预期输出

```
========================================
  Multi-Task Test Program
========================================

[Main] Note: This is a SINGLE-task demo
[Main] Real multitasking requires fork/exec support
[Main] This test demonstrates the yield() syscall

[Test] Yield system call test
[Test] Each iteration will yield CPU
[Test] Note: Only ONE task exists, so yield will return to same task

[Test] Iteration 1
[Test] Iteration 2
[Test] Iteration 3

[Test] Yield test completed
[Test] PASSED

[Main] Exiting...
```

---

## 📚 技术要点

### 1. 任务切换时机

- **时钟中断**：时间片用完时强制调度
- **系统调用**：用户程序调用 `sys_yield()` 主动让出 CPU
- **中断返回**：在 `interrupt_exit` 中检查调度标志

### 2. 栈管理

- **内核栈**：每个任务有独立的内核栈（4KB）
- **用户栈**：映射到用户地址空间
- **TSS.esp0**：指向当前任务的内核栈顶

### 3. 页表切换

```c
if (prev->pde != next->pde) {
    asm volatile("movl %0, %%cr3" : : "r"(next->pde) : "memory");
}
```

只有页表不同时才切换 CR3，避免不必要的 TLB 刷新。

### 4. 调度时机

```c
// 在中断返回前检查并调度
if (need_resched) {
    schedule();
    need_resched = 0;
}
```

---

## 🐛 已知问题

1. **调度器可能有竞争条件**
   - 当前没有完整的自旋锁保护
   - 需要添加更精细的锁机制

2. **任务链表管理不完善**
   - 没有从链表中删除退出的任务
   - 可能导致内存泄漏

3. **缺少优先级支持**
   - 所有任务同等优先级
   - 无法区分实时任务和普通任务

---

## 📖 参考资料

- **xv6 调度器**: https://pdos.csail.mit.edu/6.828/2021/xv6.html
- **Linux CFS**: https://www.kernel.org/doc/html/latest/scheduler/sched-design-CFS.html
- **OS 原理**: Modern Operating Systems (4th Edition)

---

**文档维护**: Hillson
**最后更新**: 2025-12-31
**项目地址**: https://github.com/hillson1979/zhwh_os
