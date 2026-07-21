# Java Virtual Machine Port for HillsonOS

## 可行性分析

HillsonOS 已经具备移植轻量级 JVM 的核心基础设施：

| 需求 | HillsonOS 支持 | 说明 |
|------|---------------|------|
| 内存分配 | ✅ buddy/slab/kmalloc/malloc | 用户态 malloc/free 可用 |
| 文件 I/O | ✅ VFS + ramfs | open/read/write/close 系统调用 |
| 线程/任务 | ✅ 抢占式多任务 | 可映射 Java Thread 到 OS task |
| 网络 | ✅ TCP/IP 协议栈 | socket API 可用 |
| 图形 | ✅ Framebuffer + LVGL + Qt | AWT/Swing 可映射到 LVGL |
| 用户态 ELF | ✅ ELF loader | 可加载 JVM 可执行文件 |
| C 编译器 | ✅ GCC (i386) | 编译 JVM C 源码 |
| 控制台 I/O | ✅ 键盘 + 终端 | stdin/stdout/stderr |

## 限制与挑战

| 挑战 | 应对方案 |
|------|---------|
| 无完整 libc | compat/ 提供标准 C 库替代（compat.h + 映射宏） |
| 无 MMU 隔离 | 所有 Java 代码在同一地址空间运行 |
| 无动态链接 | 静态链接 classpath |
| 32 位地址空间 | 堆空间受限 (~4-64MB), 标记-清除 GC |
| 无 pthread | 单线程模式 / compat/pthread.h 桩 |
| 无信号机制 | 空实现 signal 函数 |
| 无 mmap | sys_mman.h 映射 mmap→malloc |

## 双轨策略

### 方案 A: JamVM 2.0 移植 (`jamvm/`)

完整的轻量级 JVM (~200KB stripped, 全 JVM 规范兼容)。

**状态**: 兼容层已完成，可开始编译调试。

**结构**:
- `jamvm/compat/` — 标准 C 库兼容层 (16个头文件，替代 stdio/stdlib/string/pthread/signal/mman 等)
- `jamvm/os/hillson/` — HillsonOS 操作系统移植层 + i386 FPU 初始化
- `jamvm/classpath/` — 简化 classlib 接口（classlib-defs, classlib, excep, symbol）
- `jamvm/jamvm_src/` — JamVM 2.0 上游源码 (jserv/jamvm)
- `jamvm/Makefile` — 交叉编译构建系统

**编译**:
```bash
cd zhwh_os/java/jamvm
make          # 编译 jamvm.elf
```

**子目录详情**:
```
jamvm/
├── config.h              — 构建配置（替代 autotools configure）
├── arch.h                — 架构选择（→ arch/i386.h）
├── main.c                — JVM 入口点
├── Makefile              — 构建系统
├── compat/               — 标准 C 库兼容层
│   ├── all.h             —   统一说明
│   ├── compat.h/c        —   核心兼容函数（printf, malloc, memcpy, strlen 等）
│   ├── stdio.h           —   printf/fprintf/sprintf 映射
│   ├── stdlib.h          —   malloc/free/exit/atoi 映射
│   ├── string.h          —   memcpy/strlen/strcmp 映射
│   ├── pthread.h         —   单线程桩实现
│   ├── signal.h          —   空信号实现
│   ├── unistd.h          —   sysconf/getpid 桩
│   ├── sys_mman.h        —   mmap→malloc 映射
│   ├── time_compat.h     —   clock_gettime/nanosleep 桩
│   ├── errno.h           —   errno 变量
│   ├── dlfcn.h           —   动态库桩
│   ├── sched.h           —   sched_yield 桩
│   ├── sys_time.h        —   gettimeofday 桩
│   ├── sys_sysctl.h      —   get_nprocs 桩
│   ├── inttypes.h        —   整数类型定义
│   └── limits.h          —   整数范围定义
├── os/hillson/           — HillsonOS 移植层
│   ├── os.c              —   内存/库/路径函数
│   └── i386/
│       ├── init.c        —   FPU 双精度初始化
│       └── dll_md.c      —   动态库桩
├── classpath/            — 简化 classlib 接口
│   ├── classlib.h        —   Thread/Class/Reflect/DLL/JNI/Frame 桩
│   ├── classlib-defs.h   —   CLASSLIB 宏定义
│   ├── classlib-excep.h  —   异常枚举
│   └── classlib-symbol.h —   符号表
└── jamvm_src/            — JamVM 2.0 上游源码 (git submodule)
```

### 方案 B: 自研最小 JVM (`jvm/`)

从零实现的字节码解释器，适合学习和快速原型。

**状态**: 骨架完成，包含 class 文件解析器、部分字节码解释器、标记-清除 GC。

## 编译和运行

```bash
# 编译自研 JVM
cd zhwh_os/java
make          # → jvm.elf

# 编译 JamVM 2.0 移植
cd zhwh_os/java/jamvm
make          # → jamvm.elf

# 编译 Java 测试程序
make test     # 需要 javac
```

在 HillsonOS 中运行:
```
jvm.elf HelloWorld
jamvm.elf HelloWorld
```

## 路线图

### 第一阶段: JamVM 编译通过
- [x] JamVM 源码克隆
- [x] 标准 C 库兼容层
- [x] OS 移植层
- [x] classlib 桩函数
- [x] Makefile 构建系统
- [ ] 编译调试 — 解决编译错误
- [ ] 链接成功 — 生成 jamvm.elf

### 第二阶段: 基本运行
- [ ] HelloWorld.class 运行
- [ ] System.out.println 实现
- [ ] 基本 java.lang 类实现

### 第三阶段: 增强
- [ ] 多线程支持
- [ ] JNI 支持
- [ ] 网络 API (java.net)
- [ ] 文件 I/O (java.io)

### 第四阶段: 集成
- [ ] 图形支持 (映射到 LVGL)
- [ ] 完整 classpath
- [ ] 性能优化

## 参考资料

- JamVM 官网: https://jamvm.sourceforge.net/
- 上游源码: https://github.com/jserv/jamvm
- JVM 规范: https://docs.oracle.com/javase/specs/jvms/se8/html/
- GNU Classpath: https://www.gnu.org/software/classpath/
