# USB 鼠标驱动完整指南

## 目录
1. [概述](#概述)
2. [硬件架构](#硬件架构)
3. [USB HID Boot Protocol](#usb-hid-boot-protocol)
4. [UHCI 控制器](#uhci-控制器)
5. [驱动实现](#驱动实现)
6. [系统调用接口](#系统调用接口)
7. [LVGL 集成](#lvgl-集成)
8. [调试指南](#调试指南)

---

## 概述

本驱动实现了 USB HID Boot Protocol 鼠标，通过 UHCI (Universal Host Controller Interface) 主机控制器进行通信。

### 主要特性
- 支持 USB 1.1 低速设备
- Boot Protocol 3 字节报告
- 中断传输方式
- 相对坐标定位
- 3 键支持（左/右/中）

---

## 硬件架构

### QEMU 配置
```
-device piix3-usb-uhci    # UHCI USB 控制器
-device usb-mouse          # USB 鼠标设备
```

### 系统架构
```
┌─────────────────────────────────────────────────────────┐
│                    用户空间 (User Space)                  │
│  ┌──────────────┐      ┌────────────────────────────┐  │
│  │  lvglanet.c  │      │      LVGL 图形库           │  │
│  │   (测试程序) │◄────►│   (输入设备驱动)          │  │
│  └──────┬───────┘      └───────────────┬────────────┘  │
└─────────┼────────────────────────────────┼───────────────┘
          │                                │
          │ 系统调用 (int 0x80)           │
┌─────────┼────────────────────────────────┼───────────────┐
│         ▼                                ▼               │
│  ┌──────────────────┐        ┌──────────────────┐      │
│  │   syscall.c      │        │   usb_hcd.c      │      │
│  │  (系统调用处理)  │◄──────►│  (UHCI 驱动)     │      │
│  └────────┬─────────┘        └────────┬─────────┘      │
│           │                             │                  │
│           ▼                             ▼                  │
│  ┌──────────────────┐        ┌──────────────────┐      │
│  │   usb.c          │        │   usb_mouse.c    │      │
│  │  (USB 核心)      │◄──────►│  (鼠标驱动)      │      │
│  └──────────────────┘        └──────────────────┘      │
└──────────────────────────────────────────────────────────┘
                    │
                    ▼
         ┌────────────────────┐
         │   硬件 (UHCI)      │
         └────────────────────┘
```

---

## USB HID Boot Protocol

### 3 字节报告格式

Boot Protocol 模式下，鼠标使用固定 3 字节报告：

| 字节 | 位 | 字段 | 说明 |
|------|----|------|------|
| Byte 0 | 0 | Button 1 | 左键 (1=按下, 0=释放) |
| Byte 0 | 1 | Button 2 | 右键 (1=按下, 0=释放) |
| Byte 0 | 2 | Button 3 | 中键 (1=按下, 0=释放) |
| Byte 0 | 3-7 | Reserved | 保留 (0) |
| Byte 1 | 0-7 | X | X 轴相对位移 (有符号 8 位) |
| Byte 2 | 0-7 | Y | Y 轴相对位移 (有符号 8 位) |

### 数据结构 (usb_mouse.h)
```c
typedef struct {
    uint8_t buttons;      // Bit 0: Left, Bit 1: Right, Bit 2: Middle
    int8_t  x;            // X displacement
    int8_t  y;            // Y displacement
} usb_mouse_report_t;
```

### 位移范围
- X/Y: -128 到 +127
- 正值: 右/下
- 负值: 左/上

---

## UHCI 控制器

### 寄存器映射

UHCI 控制器使用 I/O 端口映射：

| 偏移 | 寄存器 | 说明 |
|------|--------|------|
| 0x00 | USBCMD | 命令寄存器 |
| 0x02 | USBSTS | 状态寄存器 |
| 0x04 | USBINTR | 中断使能 |
| 0x06 | FRNUM | 帧号 |
| 0x08 | FLBASEADD | 帧列表基地址 |
| 0x0C | SOFMOD | 起始帧修改 |
| 0x10 | PORTSC1 | 端口 1 状态/控制 |
| 0x12 | PORTSC2 | 端口 2 状态/控制 |

### 帧列表 (Frame List)

- 1024 个条目，每个 4 字节
- 每 1ms 处理一个帧
- 每个条目指向 QH 或 TD

```c
// 帧列表条目格式
Bits 31-4: 物理地址
Bits 3-2:  保留
Bit 1:     QH (1) / TD (0)
Bit 0:     Terminate (1=结束)
```

### Queue Head (QH) 结构

```c
typedef struct {
    uint32_t link_ptr;      // 指向下一个 QH/TD
    uint32_t element_ptr;   // 指向第一个 TD
} uhci_qh_t;
```

### Transfer Descriptor (TD) 结构

```c
typedef struct {
    uint32_t link_ptr;      // 指向下一个 TD
    uint32_t ctrl_status;   // 控制/状态
    uint32_t token;         // 令牌
    uint32_t buffer;        // 缓冲区地址
} uhci_td_t;
```

#### TD ctrl_status 字段

| 位 | 名称 | 说明 |
|----|------|------|
| 31-30 | Error Status | 00=成功, 01=CRC/超时, 10=Babble, 11=无响应 |
| 29 | SPD | Short Packet Detect |
| 26 | LS | Low Speed (1=低速) |
| 24 | IOC | Interrupt on Complete |
| 23 | ACT | Active (1=正在处理) |
| 22-21 | CERR | Error Counter (重试次数) |
| 10-0 | ACTLEN | Actual Length (N-1) |

#### TD token 字段

| 位 | 名称 | 说明 |
|----|------|------|
| 31-27 | PID | Packet ID |
| 26-22 | Device Address | 设备地址 |
| 21-19 | Endpoint | 端点号 |
| 18-19 | Data Toggle | DATA0/DATA1 |
| 10-21 | Max Length | 最大长度 (N-1) |

---

## 驱动实现

### 初始化流程

```
usb_init()
  ├─ usb_hcd_init()
  │   ├─ 复位 UHCI 控制器
  │   ├─ 分配帧列表 (4KB)
  │   ├─ 分配 QH 池
  │   ├─ 分配 TD 池
  │   └─ 启动控制器
  │
  └─ usb_hcd_scan_ports()
      └─ 检测端口状态
          └─ 如有设备 → usb_enumerate_device()
              ├─ 获取设备描述符
              ├─ 设置地址
              ├─ 设置配置
              └─ 检测 HID 设备
                  └─ usb_mouse_init()
                      └─ usb_mouse_periodic_init()
                          ├─ 分配 DMA 缓冲区
                          ├─ 初始化 TD
                          ├─ 初始化 QH
                          └─ 链接到帧列表
```

### usb_mouse_periodic_init() 详解

```c
int usb_mouse_periodic_init(int controller_id,
                            uint8_t dev_addr,
                            uint8_t ep,
                            int low_speed)
{
    // 1. 初始化状态
    mouse_periodic.toggle = 0;  // 从 DATA0 开始
    mouse_periodic.is_low_speed = low_speed;

    // 2. 分配 DMA 缓冲区 (3 字节)
    mouse_periodic.dma_buffer = dma_alloc_coherent(3, &phys);

    // 3. 分配并初始化 TD
    uhci_td_t *td = uhci_alloc_td(ctrl);

    // 提取端点号 (去除方向位)
    uint8_t ep_num = ep & 0x0F;

    td->token =
        (USB_PID_IN << 24) |           // PID = IN
        (dev_addr << 8) |               // 设备地址
        (ep_num << 15) |                // 端点号
        (2 << 21) |                     // 最大长度 = 3 (N-1)
        (0 << 19);                      // DATA0

    td->buffer = phys;
    td->ctrl_status =
        UHCI_TD_CTRL_ACT |              // Active
        UHCI_TD_CTRL_IOC |              // 中断完成
        (low_speed ? UHCI_TD_CTRL_LS : 0) |
        (3 << 27);                       // 3 次重试

    td->link_ptr = UHCI_LINK_TERMINATE;

    // 4. 分配并初始化 QH
    uhci_qh_t *qh = uhci_alloc_qh(ctrl);
    qh->element_ptr = td_phys;
    qh->link_ptr = old_link;

    // 5. 链接到所有帧
    for (int i = 0; i < 1024; i++) {
        ctrl->frame_list[i] = qh_phys | UHCI_LINK_QH;
    }

    mouse_periodic.active = 1;
    return 0;
}
```

### usb_mouse_periodic_poll() 详解

```c
int usb_mouse_periodic_poll(uint8_t *report)
{
    uhci_td_t *td = mouse_periodic.td[0];

    // 1. 检查是否仍在处理
    if (td->ctrl_status & UHCI_TD_CTRL_ACT) {
        return 0;  // 还没完成
    }

    // 2. 检查错误
    uint32_t errors = (td->ctrl_status >> 30) & 0x3;
    if (errors != 0) {
        // 出错但仍需重 arm
    }

    // 3. 检查实际长度
    int actlen = (td->ctrl_status & ACTLEN_MASK) + 1;
    if (actlen == 3) {
        memcpy(report, mouse_periodic.dma_buffer, 3);
    }

    // 4. 翻转 toggle 位
    mouse_periodic.toggle ^= 1;

    // 5. 更新 token
    td->token =
        (td->token & ~(1 << 19)) |
        (mouse_periodic.toggle << 19);

    // 6. 重 arm TD
    td->ctrl_status =
        UHCI_TD_CTRL_ACT |
        UHCI_TD_CTRL_IOC |
        (mouse_periodic.is_low_speed ? UHCI_TD_CTRL_LS : 0) |
        (3 << 27);

    // 7. 内存屏障
    asm volatile("mfence" ::: "memory");

    return actlen;
}
```

### DATA Toggle 管理

- 初始: DATA0
- 每次成功传输后翻转
- 出错时保持不变

```
传输 1: DATA0 → 成功 → toggle = 1
传输 2: DATA1 → 成功 → toggle = 0
传输 3: DATA0 → 失败 → toggle = 0 (不变)
传输 4: DATA0 → 成功 → toggle = 1
```

---

## 系统调用接口

### SYS_GUI_INPUT_READ (72)

读取 GUI 输入事件。

**参数:**
- EBX: 指向 `input_event_t` 的用户指针
- ECX: 事件类型 (2=鼠标)

**返回值:**
- EAX: 1=有事件, 0=无事件, -1=错误

**数据结构:**
```c
typedef struct {
    uint32_t type;      // 1=键盘, 2=鼠标
    int x;             // 鼠标 X
    int y;             // 鼠标 Y
    uint32_t pressed;  // 按钮状态 (0/1, 仅左键)
} input_event_t;
```

**关键修复:** `pressed` 字段只传递 0 或 1，不传递位掩码
```c
// ✅ 正确
event.pressed = (usb_mouse_buttons & 0x01) ? 1 : 0;

// ❌ 错误
event.pressed = usb_mouse_buttons;  // LVGL 不接受位掩码
```

### SYS_USB_MOUSE_POLL

直接轮询鼠标数据。

**参数:**
- EBX: 指向 `usb_mouse_report_t` 的用户指针

**返回值:**
- EAX: 1=有数据, 0=无数据, -1=错误

---

## LVGL 集成

### 输入设备驱动注册

```c
// 初始化 LVGL
lvgl_display_init();

// 注册鼠标输入设备
static lv_indev_drv_t indev_drv;
lv_indev_drv_init(&indev_drv);
indev_drv.type = LV_INDEV_TYPE_POINTER;
indev_drv.read_cb = lvgl_mouse_read_cb;
lv_indev_drv_register(&indev_drv);
```

### 读取回调

```c
static void lvgl_mouse_read_cb(lv_indev_drv_t *drv,
                                lv_indev_data_t *data)
{
    // 读取鼠标事件
    read_mouse();

    // 设置坐标
    data->point.x = mouse_x;
    data->point.y = mouse_y;

    // 设置状态
    data->state = (mouse_pressed & 0x01)
                 ? LV_INDEV_STATE_PR    // 按下
                 : LV_INDEV_STATE_REL;  // 释放
}
```

### 按钮事件

```c
static void btn_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED) {
        // 按下
    } else if (code == LV_EVENT_RELEASED) {
        // 释放
    } else if (code == LV_EVENT_CLICKED) {
        // 点击
    }
}
```

---

## 调试指南

### 调试输出检查清单

1. **设备枚举**
   ```
   [USB] Device enumerated successfully
   ```

2. **鼠标初始化**
   ```
   [USB Mouse] Periodic IN scheduled
   ```

3. **数据轮询**
   ```
   [USB Mouse] POLL #1: btn=0 x=0 y=0 len=3
   ```

4. **系统调用**
   ```
   [SYS_GUI_INPUT_READ] MOUSE: x=512 y=384 btn=0
   ```

5. **LVGL 事件**
   ```
   [LVGL] Button clicked
   ```

### 常见错误

#### TD error: 0x6080000
- **原因**: 报告长度不匹配
- **修复**: 使用 3 字节而非 8 字节

#### LVGL 按钮不响应
- **原因**: pressed 字段传递位掩码
- **修复**: 只传递 0 或 1

#### TD 一直 ACTIVE
- **原因**: QH 未正确链接到帧列表
- **修复**: 确保所有 1024 帧都链接

---

## 文件索引

| 文件 | 说明 |
|------|------|
| [include/usb.h](include/usb.h) | USB 核心定义 |
| [include/usb_mouse.h](include/usb_mouse.h) | 鼠标驱动接口 |
| [include/usb_hcd.h](include/usb_hcd.h) | 主机控制器接口 |
| [driver/usb.c](driver/usb.c) | USB 核心实现 |
| [driver/usb_hcd.c](driver/usb_hcd.c) | UHCI 驱动实现 |
| [syscall.c](syscall.c) | 系统调用处理 |
| [test/lvglanet.c](test/lvglanet.c) | LVGL 测试程序 |
| [USB_HID_MOUSE_SPEC.md](USB_HID_MOUSE_SPEC.md) | HID 规范 |
| [USB_UHCI_MOUSE_ANALYSIS.md](USB_UHCI_MOUSE_ANALYSIS.md) | UHCI 分析 |
| [USB_MOUSE_FIX_SUMMARY.md](USB_MOUSE_FIX_SUMMARY.md) | 修复总结 |

