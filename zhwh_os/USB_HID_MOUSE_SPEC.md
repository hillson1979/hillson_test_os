# USB HID Boot Mouse 驱动规范文档

## 概述

本文档描述本操作系统中 USB HID Boot Protocol 鼠标的驱动实现。

## 1. USB HID Boot Mouse 报告格式

### 1.1 3 字节 Boot 协议报告

Boot Protocol 下，USB 鼠标使用固定 3 字节的报告格式：

| 字节 | 字段 | 说明 |
|------|------|------|
| Byte 0 | Buttons | 按钮状态位掩码 |
| Byte 1 | X | X 轴相对移动（有符号 8 位整数） |
| Byte 2 | Y | Y 轴相对移动（有符号 8 位整数） |

#### Byte 0 - Buttons 位掩码：

| 位 | 含义 |
|----|------|
| Bit 0 | 左键状态（1=按下，0=释放） |
| Bit 1 | 右键状态（1=按下，0=释放） |
| Bit 2 | 中键状态（1=按下，0=释放） |
| Bits 3-7 | 保留（0） |

#### Byte 1-2 - X/Y 位移：

- 范围：-128 到 +127
- 正值表示向右/向下移动
- 负值表示向左/向上移动

---

## 2. UHCI 中断传输配置

### 2.1 端点配置

鼠标使用中断 IN 端点，典型配置：

```
端点地址：0x81 (IN 端点 1)
最大包大小：8 字节
传输类型：Interrupt
轮询间隔：10ms (或由 UHCI 帧调度决定)
```

### 2.2 传输描述符 (TD) 配置

```c
// Token 字段配置
td->token =
    (USB_PID_IN << 24) |           // PID = IN (0x69)
    (dev_addr << 8) |               // 设备地址
    (ep_num << 15) |                // 端点号 (0-15, 不含方向位)
    (max_len - 1 << 21) |           // 最大长度 (N-1 编码)
    (toggle << 19);                 // DATA toggle 位 (0=DATA0, 1=DATA1)

// Control/Status 字段配置
td->ctrl_status =
    UHCI_TD_CTRL_ACT |              // Active 位 (必须设置)
    UHCI_TD_CTRL_IOC |              // 中断完成时触发
    (low_speed ? UHCI_TD_CTRL_LS : 0) |  // 低速设备标志
    (3 << 27);                       // 错误计数器 (3 次重试)
```

### 2.3 DATA Toggle 位管理

- 中断端点初始使用 DATA0
- 每次成功传输后 toggle 位翻转
- 如果传输失败，保持原 toggle 位重试

---

## 3. 帧列表调度

### 3.1 UHCI 帧列表

- 1024 个帧，每个帧 1ms
- 鼠标 QH 应该链接到所有帧或每隔 8/10 帧
- 本实现：链接到所有 1024 帧

```c
// 将鼠标 QH 插入所有帧
for (int i = 0; i < 1024; i++) {
    ctrl->frame_list[i] = qh_phys | UHCI_LINK_QH;
}
```

---

## 4. 系统调用接口

### 4.1 SYS_GUI_INPUT_READ (72)

读取 GUI 输入事件（包括鼠标）。

**参数：**
- EBX: 指向 `input_event_t` 结构体的用户指针
- ECX: 事件类型（2=鼠标）

**返回值：**
- EAX: 1 = 有事件，0 = 无事件，-1 = 错误

**input_event_t 结构：**
```c
typedef struct {
    uint32_t type;      // 1=键盘, 2=鼠标
    int x;             // 鼠标 X 或键码
    int y;             // 鼠标 Y 或保留
    uint32_t pressed;  // 鼠标按钮状态 (0/1, 仅左键)
} input_event_t;
```

**注意：** `pressed` 字段只传递左键状态（0 或 1），LVGL 不接受位掩码。

---

## 5. LVGL 集成

### 5.1 输入设备驱动

```c
static void lvgl_mouse_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    data->point.x = mouse_x;
    data->point.y = mouse_y;
    data->state = (mouse_pressed & 0x01)
                 ? LV_INDEV_STATE_PR    // 按下
                 : LV_INDEV_STATE_REL;  // 释放
}
```

### 5.2 LVGL 按钮事件

LVGL 支持以下按钮事件：
- `LV_EVENT_PRESSED`: 按钮按下
- `LV_EVENT_RELEASED`: 按钮释放
- `LV_EVENT_CLICKED`: 完整的点击动作

---

## 6. 关键修复记录

### 6.1 pressed 字段位掩码问题

**问题：** 直接将 USB 按钮位掩码传递给 LVGL
**修复：** 只取左键状态，转换为 0 或 1
**位置：** `syscall.c:1837`

```c
// 修复前
event.pressed = usb_mouse_buttons;  // ❌ 位掩码

// 修复后
event.pressed = (usb_mouse_buttons & 0x01) ? 1 : 0;  // ✅ 仅左键
```

### 6.2 报告长度问题

**问题：** 使用 8 字节而非 3 字节
**修复：** 改回 3 字节 Boot Mouse 报告
**位置：** `usb_hcd.c:1169`

### 6.3 端点号提取

**问题：** 端点地址包含方向位 (0x81)
**修复：** 只取低 4 位 `ep & 0x0F`
**位置：** `usb_hcd.c:1170, 1483`

### 6.4 Ping-Pong TD 问题

**问题：** 两个 TD 共用同一个 DMA 缓冲区导致冲突
**修复：** 改用单 TD 简单方案
**位置：** `usb_hcd.c:usb_mouse_periodic_init`

---

## 7. 调试输出

驱动包含以下调试输出：

```
[USB Mouse] POLL #n: btn=X x=Y y=Z len=3
[SYS_GUI_INPUT_READ] MOUSE: x=X y=Y btn=B
[LVGL] Button clicked - turned purple
```

---

## 8. 参考

- USB HID Specification, Version 1.11
- Universal Host Controller Interface (UHCI) Design Guide
- Intel 82371AB (PIIX4) UHCI Controller Datasheet
