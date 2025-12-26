# 操作系统开发文档

## 1. 项目概述

这是一个简单的x86 32位操作系统，能够在计算机上运行。该操作系统实现了基本的内核功能，包括任务管理、内存管理、用户模式切换和系统调用等。

### 主要特性

- 支持多任务调度
- 实现了内核态和用户态的切换
- 提供了基本的系统调用接口
- 支持ELF格式用户程序的加载和执行
- 包含完整的构建和调试环境

## 2. 项目结构

```
hillson_test_os/
├── include/            # 头文件目录
│   ├── mm/            # 内存管理相关头文件
│   ├── pci/           # PCI设备相关头文件
│   └── x86/           # x86架构相关头文件
├── test/              # 用户程序测试目录
│   └── test.c         # 用户程序示例
├── linker/            # 链接器脚本
├── kernel.c           # 内核入口点
├── task.c             # 任务管理实现
├── userboot.c         # 用户程序加载器
├── mm.c               # 内存管理
├── interrupt.c        # 中断处理
├── printf.c           # 输出函数
├── Makefile           # 构建脚本
├── create_iso.sh      # ISO镜像创建脚本
├── kernel.iso         # 可引导的ISO镜像
└── debug.gdb          # GDB调试脚本
```

## 3. 核心功能实现

### 3.1 内核启动流程

内核启动流程定义在`kernel.c`文件中：

1. 初始化VGA显示
2. 初始化内存映射
3. 初始化PCI设备、LAPIC和IOAPIC
4. 初始化段寄存器和中断描述符表
5. 创建内核任务和用户任务
6. 启动调度器循环

### 3.2 任务管理

任务管理主要在`task.c`文件中实现：

#### 任务创建

```c
task_t* init_task(bool with_ustack)
task_t* task_load(const char* fullpath, pid_t parent_pid, bool with_ustack)
```

#### 任务启动

```c
void start_task_user(struct task_t* th, task_entry_callback_t entry_user)
void start_task_kernel(struct task_t* th, task_entry_callback_t entry_kernel)
```

#### 用户模式切换

```c
void task_to_user_mode()
```

该函数负责将任务从内核模式切换到用户模式，包括：
- 设置用户段寄存器（CS=0x1b, DS=0x23）
- 配置TSS（任务状态段）
- 切换到用户页表
- 通过`iret`指令返回用户空间

### 3.3 用户程序加载

用户程序加载在`userboot.c`中实现：

```c
int load_module_to_user(uint32_t *pd_user)
```

该函数负责：
- 解析ELF格式的用户程序
- 将程序段映射到用户虚拟地址空间
- 设置用户程序的入口地址和栈指针

### 3.4 系统调用

系统调用通过中断`int 0x80`实现，用户程序可以通过`sys_printf`等函数调用内核服务。

## 4. 构建和运行指南

### 4.1 构建内核

使用Makefile构建内核：

```bash
make
```

### 4.2 创建ISO镜像

使用脚本创建可引导的ISO镜像：

```bash
./create_iso.sh
```

### 4.3 运行操作系统

使用QEMU运行操作系统：

```bash
qemu-system-i386 -drive format=raw,file=kernel.iso
```

## 5. 调试方法

### 5.1 使用GDB调试

项目提供了多个调试脚本：

- `debug.gdb`: 基础调试脚本
- `detailed_debug.sh`: 详细调试脚本
- `complete_debug.sh`: 完整调试流程

使用方法：

1. 启动QEMU并等待GDB连接：
   ```bash
   qemu-system-i386 -drive format=raw,file=kernel.iso -s -S
   ```

2. 运行GDB调试脚本：
   ```bash
   gdb -x debug.gdb
   ```

### 5.2 关键断点设置

在调试时，可以设置以下关键断点：

- `start_task_user`: 用户任务启动
- `task_to_user_mode`: 模式切换点
- `syscall_dispatch`: 系统调用处理
- `_start`: 用户程序入口点

## 6. 系统调用接口

### 6.1 已实现的系统调用

| 系统调用 | 功能 | 参数 | 返回值 |
|---------|------|------|--------|
| `sys_printf` | 格式化输出 | `const char *fmt, ...` | 输出的字符数 |
| `sys_exit` | 程序退出 | `int code` | 无 |

### 6.2 用户程序调用示例

```c
extern int sys_printf(const char *fmt);
extern void sys_exit(int code);

void _start() {
    sys_printf("Hello from user program!\n");
    //sys_exit(0);
    for(;;);  // 永不返回
}
```

## 7. 核心代码分析

### 7.1 用户模式切换实现

```c
void task_to_user_mode()
{
    task_t *task = th_u;
    struct trapframe *tf = task->tf;

    // 设置用户段寄存器
    tf->ds = (SEG_UDATA << 3) | DPL_USER;
    tf->es = (SEG_UDATA << 3) | DPL_USER;
    tf->fs = (SEG_UDATA << 3) | DPL_USER;
    tf->ss = (SEG_UDATA << 3) | DPL_USER;
    tf->cs = (SEG_UCODE << 3) | DPL_USER;

    // 设置中断标志
    tf->eflags = FL_IF;

    // 设置TSS
    struct cpu *c = &cpus[logical_cpu_id()];
    c->ts.ss0 = SEG_KDATA << 3;
    c->ts.esp0 = (uint32_t)th_u->kstack;
    ltr(SEG_TSS << 3);

    // 切换页表
    asm volatile ("movl %0, %%cr3" :: "r"(task->pde) : "memory");

    // 切换到用户空间
    asm volatile (
        "cli\n\t"
        "movl %0, %%esp\n\t"
        "jmp interrupt_exit\n\t"
        :: "r"(tf) : "memory"
    );
}
```

### 7.2 用户程序加载实现

```c
int load_module_to_user(uint32_t *pd_user) {
    // 检查multiboot信息
    if (!(multiboot_info->flags & (1 << 3)))
        return -1;

    // 获取模块信息
    multiboot_module_t *mods = (multiboot_module_t *)phys_to_virt(multiboot_info->mods_addr);
    uint32_t mod_start = mods[0].mod_start;
    uint32_t mod_end   = mods[0].mod_end;

    // 解析ELF文件
    Elf32_Ehdr *eh = (Elf32_Ehdr *)phys_to_virt(mod_start);
    
    // 检查ELF魔数
    if (eh->e_ident[0] != 0x7F || eh->e_ident[1] != 'E' || eh->e_ident[2] != 'L' || eh->e_ident[3] != 'F') {
        printf("[load_module_to_user] Not a valid ELF file!\n");
        return -2;
    }

    // 加载程序段
    Elf32_Phdr *ph = (Elf32_Phdr *)phys_to_virt(mod_start + eh->e_phoff);
    for (int i = 0; i < eh->e_phnum; i++, ph++) {
        if (ph->p_type != PT_LOAD)
            continue;

        uint32_t va = ph->p_vaddr;
        uint32_t pa = mod_start + ph->p_offset;
        uint32_t memsz = ph->p_memsz;

        // 映射程序段到用户虚拟地址空间
        for (uint32_t off = 0; off < memsz; off += PAGE_SIZE) {
            uint32_t dst_va = va + off;
            uint32_t dst_pa = pa + off;
            map_page(pd_user, dst_va, dst_pa, USER_PTE_FLAGS);
        }
    }

    // 设置用户程序入口和栈
    task_t *task = th_u;
    struct trapframe *tf = task->tf;
    tf->eip = eh->e_entry;
    
    // 创建用户栈
    uint32_t stack_pa = pmm_alloc_page();
    map_page(pd_user, VIRT_USER_STACK_TOP - PAGE_SIZE, stack_pa, USER_PTE_FLAGS);
    tf->esp = VIRT_USER_STACK_TOP;

    return 0;
}
```

### 7.3 系统调用处理实现

系统调用处理实现位于`syscall.c`文件中，通过中断`int 0x80`触发：

```c
enum {
    SYS_PRINTF = 1,
    SYS_EXIT,
};

void syscall_dispatch(struct trapframe *tf) {
    uint32_t num = tf->eax;
    uint32_t arg1 = tf->ebx;
    uint32_t arg2 = tf->ecx;
    uint32_t arg3 = tf->edx;

    switch (num) {
        case SYS_PRINTF:
            printf("[user] %s", (char*)arg1);
            tf->eax = 0;
            break;
        case SYS_EXIT:
            printf("[user] exit code=%d\n", arg1);
            tf->eax = 0;
            break;
        default:
            printf("[syscall] unknown num=%d\n", num);
            break;
    }
}
```

### 7.4 中断处理与系统调用路由

中断处理实现位于`interrupt.c`文件中，系统调用中断`T_SYSCALL`会被路由到`syscall_dispatch`函数：

```c
// 中断处理主函数
void do_irq_handler(struct trapframe *tf) {
    // 系统调用中断处理
    if(tf->trapno == T_SYSCALL){
        syscall_dispatch(tf);
        return;
     }

    // 其他中断类型处理...
    switch (tf->trapno) {
        case 0:  // 除法错误
            handle_divide_error(tf);
            break;
        case 14: // 页错误
            handle_page_fault(tf);
            break;
        // ... 其他中断 ...
        default:
            printf("Unhandled interrupt: trapno=%d\n", tf->trapno);
            // 外部中断需要发送EOI
            if (tf->trapno >= 32 && tf->trapno <= 47) {
                send_eoi(tf->trapno - 32);
            }
            break;
    }
}
```

## 8. 常见问题及解决方案

### 8.1 用户程序无法输出消息

**问题现象**：用户程序执行后，屏幕上没有显示"Hello from user program!"消息

**可能原因**：
- 系统调用实现有问题
- 用户模式切换失败
- 页表映射不正确
- ELF文件加载失败

**解决方案**：
1. 检查`syscall_dispatch`函数的实现
2. 确保`task_to_user_mode`函数正确设置了段寄存器和页表
3. 验证`load_module_to_user`函数正确解析和映射了ELF文件
4. 使用GDB调试，设置断点跟踪执行流程

### 8.2 QEMU无法启动

**问题现象**：使用`qemu-system-i386 -kernel kernel.bin`命令无法启动

**解决方案**：使用正确的ISO镜像启动命令：
```bash
qemu-system-i386 -drive format=raw,file=kernel.iso
```

## 9. 未来改进方向

1. **完善内存管理**：实现更高效的内存分配算法
2. **增加文件系统支持**：添加基本的文件系统功能
3. **网络支持**：实现网络协议栈
4. **设备驱动**：增加更多设备的驱动程序
5. **多处理器支持**：完善SMP（对称多处理）支持
6. **POSIX兼容性**：提高系统调用的POSIX兼容性

## 10. 参考文献

- [x86汇编语言](https://en.wikibooks.org/wiki/X86_Assembly)
- [ELF文件格式](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format)
- [操作系统设计与实现](https://en.wikipedia.org/wiki/Operating_System_Design_and_Implementation)
- [QEMU用户手册](https://www.qemu.org/docs/master/)
- [GDB调试指南](https://www.gnu.org/software/gdb/documentation/)