# UHCI USB 鼠标驱动分析与修复

## UHCI 关键概念

### 1. 帧列表 (Frame List)
- UHCI 使用 1024 个 32 位条目组成的帧列表
- 每 1ms 处理一个帧 (Frame)
- 每个条目指向一个 Queue Head (QH) 或 Transfer Descriptor (TD)
- 条目的低 2 位表示类型：01=TD, 10=QH, 00=无效

### 2. Queue Head (QH) 结构
```
QH:
  link_ptr      (DWORD 0)  - 下一个 QH/TD
  element_ptr   (DWORD 1)  - 第一个 TD
```

### 3. Transfer Descriptor (TD) 结构
```
TD:
  link_ptr      (DWORD 0)  - 下一个 TD
  ctrl_status   (DWORD 1)  - 控制/状态
  token         (DWORD 2)  - 令牌
  buffer        (DWORD 3)  - 缓冲区地址
```

### 4. TD ctrl_status 位（重要！）
```
Bit 31-30: Error Status (00=成功)
Bit 29:    Short Packet Detect (SPD)
Bit 28-27: Reserved
Bit 26:    Low Speed (LS) - 1=低速设备
Bit 25:    Isochronous Select (IOS)
Bit 24:    Interrupt on Complete (IOC) - 完成时触发中断
Bit 23:    ACTIVE - 1=TD 正在处理（HC 会在完成后清除）
Bit 22-21: Error Counter (CERR) - 重试次数（通常设为 3）
Bit 20-19: Reserved
Bit 18-16: SPD Control
Bit 15-11: Reserved
Bit 10-0:  Actual Length (ACTLEN) - 实际传输长度 (N-1)
```

**关键：ACT 位 = Bit 23 = 0x00800000**

---

## 问题分析与修复历史

### 问题 1: 报告长度不匹配 (已修复)
**问题描述**: 使用 8 字节而非标准 3 字节 Boot Mouse 报告
**影响**: UHCI 控制器报告 TD 错误 (0x6080000)
**修复**: 改回 3 字节报告长度
**位置**: `usb_hcd.c:1169`

```c
// 修复前
const int len = 8;

// 修复后
const int len = 3; // Boot mouse report is exactly 3 bytes
```

---

### 问题 2: Ping-Pong TD 缓冲区冲突 (已修复)
**问题描述**: 两个 TD 共用同一个 DMA 缓冲区
**影响**: 数据竞争，传输不稳定
**修复**: 改用单 TD 简单方案
**位置**: `usb_hcd.c:usb_mouse_periodic_init`

**分析**:
- Ping-Pong 方案需要两个独立的 DMA 缓冲区
- 两个 TD 同时写入同一缓冲区会导致数据损坏
- 对于 10ms 轮询的鼠标，单 TD 方案足够

**修复后的实现**:
```c
// 单 TD 方案
td->link_ptr = UHCI_LINK_TERMINATE;  // 终止，不循环
qh->element_ptr = td_phys;             // QH 直接指向 TD
```

---

### 问题 3: 端点号提取 (已修复)
**问题描述**: 端点地址包含方向位 (0x81)
**影响**: 控制器无法识别正确的端点
**修复**: 只取低 4 位 `ep & 0x0F`
**位置**: `usb_hcd.c:1170, 1483`

```c
// 修复: 提取端点号（去除方向位）
uint8_t ep_num = ep & 0x0F;
```

---

### 问题 4: LVGL pressed 字段 (已修复)
**问题描述**: 直接传递 USB 按钮位掩码给 LVGL
**影响**: LVGL 只接受 0 或 1，不接受位掩码
**修复**: 只取左键状态，转换为 0 或 1
**位置**: `syscall.c:1837`

```c
// 修复前
event.pressed = usb_mouse_buttons;  // ❌ 位掩码 (0-7)

// 修复后
event.pressed = (usb_mouse_buttons & 0x01) ? 1 : 0;  // ✅ 仅左键 (0/1)
```

---

### 问题 5: 帧列表调度 (已修复)
**问题描述**: 只在每 10 帧插入鼠标 QH
**影响**: 鼠标响应延迟高
**修复**: 链接到所有 1024 帧
**位置**: `usb_hcd.c:usb_mouse_periodic_init`

```c
// 修复后: 所有帧都调度鼠标 QH
for (int i = 0; i < 1024; i++) {
    ctrl->frame_list[i] = (qh_phys & ~0xF) | UHCI_LINK_QH;
}
```

---

## TD 错误码分析

### 常见错误状态值

| ctrl_status | 错误 | 说明 |
|-------------|------|------|
| 0x6080000 | 0x60 | Bit 30=1, Bit 29=1 - 设备无响应 |
| 0x4080000 | 0x40 | Bit 30=1 - CRC/超时错误 |
| 0x2080000 | 0x20 | Bit 29=1 - Babble 错误 |

**0x6080000 分解**:
```
0x6080000 = 0b00000110000010000000000000000000
           ||||||||||||||||||||||||||||||||
           ||||||||||||||||||||||||||||||++-- ACTLEN=0
           |||||||||||||||||||||||||++------- Reserved
           |||||||||||||||||||||||+---------- SPD Control
           |||||||||||||||||||||+------------ Reserved
           |||||||||||||||||||+--------------- Reserved
           |||||||||||||||||+----------------- Error Counter=0
           ||||||||||||||||+------------------ Error Counter=0
           |||||||||||||||+------------------- ACTIVE=0 (已完成)
           ||||||||||||||+-------------------- IOC=1
           |||||||||||||+--------------------- IOS=0
           ||||||||||||+---------------------- LS=1 (低速)
           |||||||||||+----------------------- Reserved=0
           ||||||||||+------------------------ Reserved=0
           |||||||||+------------------------- SPD=0
           |||||||+--------------------------- Error=1
           ||||||+---------------------------- Error=1
```

---

## 正确的 TD 初始化流程

### 单 TD 中断传输 (推荐)

```c
// 1. 分配 TD
uhci_td_t *td = uhci_alloc_td(ctrl);
memset(td, 0, sizeof(*td));

// 2. 配置 Token
uint8_t ep_num = ep & 0x0F;  // 只取端点号
td->token =
    (USB_PID_IN << 24) |           // PID = IN
    (dev_addr << 8) |               // 设备地址
    (ep_num << 15) |                // 端点号
    ((len - 1) << 21) |             // 最大长度 (N-1)
    (toggle << 19);                 // DATA toggle

// 3. 配置 Control/Status
td->buffer = dma_buffer_phys;
td->ctrl_status =
    UHCI_TD_CTRL_ACT |              // Active
    UHCI_TD_CTRL_IOC |              // 中断完成
    (low_speed ? UHCI_TD_CTRL_LS : 0) |
    (3 << 27);                       // 3 次重试

// 4. 终止链接
td->link_ptr = UHCI_LINK_TERMINATE;

// 5. 链接到 QH
qh->element_ptr = td_phys;
```

---

## 重 Arm TD 流程 (Poll 中)

```c
// 1. 检查是否完成
if (td->ctrl_status & UHCI_TD_CTRL_ACT) {
    return 0;  // 仍在处理
}

// 2. 检查错误
uint32_t errors = (td->ctrl_status >> 30) & 0x3;
if (errors != 0) {
    // 出错但仍需重 arm
}

// 3. 翻转 toggle
toggle ^= 1;

// 4. 更新 token
td->token =
    (td->token & ~(1 << 19)) |  // 清除旧 toggle
    (toggle << 19);              // 设置新 toggle

// 5. 重 arm TD
td->ctrl_status =
    UHCI_TD_CTRL_ACT |
    UHCI_TD_CTRL_IOC |
    (low_speed ? UHCI_TD_CTRL_LS : 0) |
    (3 << 27);

// 6. 内存屏障
asm volatile("mfence" ::: "memory");
```

---

## 调试检查清单

当鼠标不工作时，按顺序检查：

1. [ ] 设备枚举成功？
   ```
   [USB] Device enumerated successfully
   ```

2. [ ] 鼠标初始化成功？
   ```
   [USB Mouse] Periodic IN scheduled
   ```

3. [ ] 看到 POLL 输出？
   ```
   [USB Mouse] POLL #1: btn=0 x=0 y=0 len=3
   ```

4. [ ] 没有 TD error？
   ```
   [USB Mouse] TD error: 0x...  ❌ 有问题
   ```

5. [ ] 系统调用返回事件？
   ```
   [SYS_GUI_INPUT_READ] MOUSE: x=... y=... btn=...
   ```

6. [ ] LVGL 收到输入？
   ```
   [LVGL] Button clicked
   ```

---

## 参考文档

- [USB_HID_MOUSE_SPEC.md](USB_HID_MOUSE_SPEC.md) - HID 鼠标规范
- [USB_MOUSE_FIX_SUMMARY.md](USB_MOUSE_FIX_SUMMARY.md) - 修复总结
- Intel UHCI Design Guide
- USB HID Specification 1.11

