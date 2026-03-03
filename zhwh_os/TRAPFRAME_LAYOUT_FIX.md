# Trapframe 布局修复总结

## 问题根源

之前的 trapframe 结构定义与实际栈布局不匹配，导致：
- EIP=0, CS=0 等错误值
- 除零错误（实际是页面错误被误识别）
- offset 一直在变化（16→8→32→16...），无法稳定

## 正确的栈布局

### alltraps 的压栈顺序（trap_entry.S）

```asm
alltraps:
    pushl %ds    # offset 0（最先压入，最低地址）
    pushl %es    # offset 4
    pushl %fs    # offset 8
    pushl %gs    # offset 12
    pusha        # offset 16-44 (pusha 压入 8 个通用寄存器)
    # ESP 现在指向 offset 16 (EDI)
```

### 关键理解

1. **栈向下生长**：先压入的数据在**低地址**
2. **DS 先压入**：DS 在 offset 0（最低地址）
3. **pusha 后压入**：通用寄存器在 offset 16-44（较高地址）
4. **ESP 自然指向**：pusha 之后的 ESP 指向 offset 16（EDI）

## 修复后的 struct trapframe 布局

### include/interrupt.h (lines 80-112)

```c
struct trapframe {
  // ⚠️ alltraps 压入的段寄存器（先压，在低地址）
  uint32_t ds;      // offset 0（alltraps最先压入，低地址)
  uint32_t es;      // offset 4
  uint32_t fs;      // offset 8
  uint32_t gs;      // offset 12

  // ⚠️ pusha 压入的通用寄存器（后压，在高地址）
  uint32_t edi;     // offset 16
  uint32_t esi;     // offset 20
  uint32_t ebp;     // offset 24
  uint32_t oesp;    // offset 28 (pusha压入的原始ESP，无用)
  uint32_t ebx;     // offset 32
  uint32_t edx;     // offset 36
  uint32_t ecx;     // offset 40
  uint32_t eax;     // offset 44

  // vectors.S 压入的值 (8字节)
  uint32_t trapno;   // offset 48
  uint32_t err;      // offset 52

  // CPU 硬件压入的值 (12字节)
  uint32_t eip;      // offset 56
  uint32_t cs;       // offset 60
  uint32_t eflags;   // offset 64

  // 仅在特权级改变时压入 (8字节)
  uint32_t esp;      // offset 68
  uint32_t ss;       // offset 72
} __attribute__((packed));
```

## 修复的文件

### 1. include/interrupt.h
- ✅ 修正 struct trapfield 布局（DS 在 offset 0，不是 offset 32）
- ✅ 修正 TRAPFRAME_* 宏定义

### 2. task_impl.s
- ✅ 修正 TF_* 偏移量定义（lines 196-217）
- ✅ 修正 task_to_user_mode_with_task 的寄存器恢复顺序（lines 333-354）
  ```asm
  # 按照正确顺序恢复：DS→ES→FS→GS，然后 EAX→EDI
  popl %ds          # offset 0
  popl %es          # offset 4
  popl %fs          # offset 8
  popl %gs          # offset 12
  popl %eax         # offset 44
  popl %ecx         # offset 40
  popl %edx         # offset 36
  popl %ebx         # offset 32
  addl $4, %esp     # 跳过 oesp (offset 28)
  popl %ebp         # offset 24
  popl %esi         # offset 20
  popl %edi         # offset 16
  addl $8, %esp     # 跳过 trapno (48) 和 err (52)
  iret              # 恢复 EIP, CS, EFLAGS, ESP, SS
  ```

### 3. interrupt_exit.s
- ✅ 修正 TRAPFRAME_* 偏移量定义（lines 13-40）
- ✅ 修正 interrupt_exit 的恢复逻辑（lines 47-206）
  - 先检查特权级（CS 在 offset 56）
  - 用户态路径：恢复段寄存器 + 通用寄存器
  - 内核态路径：跳过段寄存器，只恢复通用寄存器

## 关键修复点

### 1. 段寄存器顺序
- ❌ 错误：GS/FS/ES/DS（假设 pusha 在段寄存器之前）
- ✅ 正确：DS/ES/FS/GS（alltraps 先压段寄存器，DS 在最低地址）

### 2. 寄存器恢复顺序
- 必须按照压栈的**逆序**恢复
- 先恢复段寄存器（offset 0-12）
- 再恢复通用寄存器（offset 16-44，从 EAX 开始逆序）

### 3. ESP 调整
- ❌ 错误做法：不断调整 task->tf 的初始化偏移（8→16→24...）
- ✅ 正确做法：修正 struct 定义，让偏移量固定为 0

## 验证方法

修复后，trapframe 的各个字段应该显示正确值：
- EIP：用户程序的实际入口地址（如 0x1000）
- CS：用户代码段选择子（如 0x1B）
- DS/ES/FS/GS：用户数据段选择子（如 0x23）
- 不是 0 或乱码

## 参考

- alltraps 实现：zhwh_os/trap_entry.S:15-30
- trapframe 定义：zhwh_os/include/interrupt.h:80-112
- task_to_user_mode_with_task：zhwh_os/task_impl.s:286-358
- interrupt_exit：zhwh_os/interrupt_exit.s:43-206
