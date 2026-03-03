# 自动化工作流使用指南

## 快速开始

### 1. 查看下一个任务
```bash
cd zhwh_os/harness
./next_task.sh
```
这会显示：
- 当前进度 (X/45 features passing)
- 下一个要实现的功能
- 相关文件列表
- 工作命令

### 2. 自动化工作流
```bash
./auto_workflow.sh
```
或者 Windows 下：
```cmd
harness\auto_workflow.bat
```

## 工作流程

### 完全自动模式
```bash
./auto_workflow.sh
```
脚本会自动：
1. 选择下一个优先级最高的 failing 功能
2. 分析当前实现
3. 生成实现计划
4. 等待你手动编写代码
5. 构建内核
6. 运行测试
7. 询问是否更新状态
8. 询问是否提交更改

### 指定功能模式
```bash
./auto_workflow.sh --feature SI-002
```
工作在特定的功能上。

### 跳过构建/测试
```bash
./auto_workflow.sh --skip-build --skip-test
```
如果你已经手动构建/测试过。

## 人工确认点

脚本会在以下点暂停，等待你的确认：

### 1. 代码实现阶段
```
⚠ Please review the plan and implement the required changes.
Press Enter to continue when ready, or Ctrl+C to abort
```

此时你应该：
1. 查看生成的计划：`harness/sessions/current_plan.md`
2. 编辑相关文件
3. 实现功能
4. 按 Enter 继续

### 2. 状态更新阶段
```
Do you want to mark this feature as PASSING? (y/n)
```

如果测试通过，输入 `y` 更新状态。

### 3. 提交阶段
```
Do you want to commit these changes? (y/n)
Enter commit message (or press Enter for default):
```

查看更改后决定是否提交。

## 构建和测试命令

### 完整构建
```bash
cd zhwh_os
make clean && make gen-offsets && make
./zh.sh
```

### Windows WSL 构建
```bash
wsl bash -c "cd /mnt/f/hillson_test_os/zhwh_os && make clean && make && ./zh.sh"
```

### 运行 QEMU 测试
```bash
qemu-system-i386 -cdrom os.iso -vga std -display sdl -serial file:test_output.txt
```

## 状态跟踪

### 查看当前进度
```bash
cat harness/feature_list.json | grep -A 2 '"total_features"'
```

### 查看某个功能的详情
```bash
grep -A 10 '"id": "SI-001"' harness/feature_list.json
```

### 查看会话历史
```bash
ls -la harness/sessions/
cat harness/sessions/YYYYMMDD_HHMMSS/manifest.json
```

## 工作流程示例

```bash
# 1. 查看下一个任务
cd zhwh_os/harness
./next_task.sh

# 输出示例：
# Progress: 5/45 features passing (11%)
# Next Task: TM-003
# Name: Context Switch
# ...

# 2. 运行自动化工作流
./auto_workflow.sh

# 脚本会：
# - 选择 TM-003
# - 显示要修改的文件
# - 等待你实现代码
# - 自动构建和测试
# - 询问更新状态
# - 询问提交

# 3. 如果你想手动控制，可以使用单独的命令：
./harness/init_session.sh  # 初始化会话
./harness/test_feature.sh TM-003  # 测试功能
./harness/update_progress.sh TM-003 passing "实现上下文切换"  # 更新并提交
```

## 手动控制流程

如果你想完全手动控制：

```bash
# 1. 初始化会话
./harness/init_session.sh

# 2. 选择并实现功能
vim task.c  # 或其他相关文件

# 3. 构建
make clean && make

# 4. 创建 ISO
./zh.sh

# 5. 测试
qemu-system-i386 -cdrom os.iso -vga std -display sdl

# 6. 如果测试通过，更新状态
./harness/update_progress.sh TM-003 passing "实现上下文切换"

# 7. 或者如果失败
./harness/update_progress.sh TM-003 failing "需要修复寄存器保存问题"
```

## 故障排除

### 构建失败
检查 `build.log` 文件
```bash
cat build.log | tail -50
```

### 测试失败
检查会话目录中的测试日志
```bash
cat harness/sessions/YYYYMMDD_HHMMSS/test_TM-003.log
```

### Git 问题
查看当前状态
```bash
git status
git log --oneline -5
```

## 文件结构

```
harness/
├── feature_list.json          # 功能列表 (45个功能)
├── auto_workflow.sh           # 自动化工作流脚本
├── auto_workflow.bat          # Windows 批处理包装器
├── next_task.sh              # 快速任务选择器
├── init_session.sh           # 会话初始化
├── test_feature.sh           # 功能测试
├── update_progress.sh        # 进度更新
├── BUILD_AND_TEST.sh         # 构建测试脚本
├── README.md                 # 本文档
└── sessions/                 # 会话历史
    └── YYYYMMDD_HHMMSS/
        ├── manifest.json
        ├── progress.log
        ├── test_*.log
        └── boot.log
```

## 最佳实践

1. **每次一个功能**: 使用 `auto_workflow.sh` 专注于一个功能
2. **测试优先**: 只有在测试通过后才标记为 passing
3. **小步快跑**: 频繁提交，每个功能一个提交
4. **查看进度**: 定期运行 `next_task.sh` 查看进度
5. **人工确认**: 在关键点（状态更新、提交）人工确认

## 下一步

运行 `./next_task.sh` 开始工作！
