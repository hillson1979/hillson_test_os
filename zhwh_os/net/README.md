# 网卡驱动说明

## 概述

本 OS 实现了 RTL8139 网卡驱动，支持基本的网络功能：

- 以太网帧发送和接收
- ARP 协议（基础框架）
- IP 协议
- ICMP 协议（Ping）
- UDP 协议（基础框架）
- TCP 协议（基础框架）

## 文件结构

```
net/
├── core.c          # 网络协议栈核心
├── loopback.c      # 回环网络设备（用于测试）
├── rtl8139.c       # RTL8139 网卡驱动
├── net_test.c      # 网络测试程序
└── README.md       # 本文件

include/
├── net.h           # 网络协议栈头文件
└── rtl8139.h       # RTL8139 驱动头文件
```

## RTL8139 驱动特性

### 支持的功能

- PCI 设备探测和初始化
- DMA 发送和接收
- 中断处理
- 自动 MAC 地址读取
- 广播和多播包接收

### 硬件要求

RTL8139 驱动支持以下硬件：

- RealTek RTL8139 系列
- 常见于 QEMU 虚拟机（默认网卡）

QEMU 启动参数：
```bash
qemu-system-i386 -netdev user,id=net0 -device rtl8139,netdev=net0 ...
```

## API 接口

### 网络设备结构

```c
typedef struct net_device {
    char name[16];              // 设备名称
    uint8_t mac_addr[ETH_ALEN]; // MAC 地址
    uint32_t ip_addr;           // IP 地址
    uint32_t netmask;           // 子网掩码
    uint32_t gateway;           // 网关
    uint16_t mtu;               // 最大传输单元
    void *priv;                 // 私有数据

    // 驱动操作
    int (*send)(struct net_device *dev, uint8_t *data, uint32_t len);
    int (*recv)(struct net_device *dev, uint8_t *data, uint32_t len);
    int (*ioctl)(struct net_device *dev, int cmd, void *arg);
} net_device_t;
```

### 主要函数

#### 网络初始化

```c
void net_init(void);
int rtl8139_init(void);
int loopback_init(void);
```

#### 数据包发送/接收

```c
int net_tx_packet(net_device_t *dev, uint8_t *data, uint32_t len);
int net_rx_packet(net_device_t *dev, uint8_t *data, uint32_t len);
```

#### 网络配置

```c
int net_set_ipaddr(uint32_t ip);
int net_set_netmask(uint32_t netmask);
int net_set_gateway(uint32_t gateway);
```

## 使用示例

### 发送 Ping 请求

```c
// 获取默认网络设备
net_device_t *dev = net_device_get_default();

// 设置 IP 地址
net_set_ipaddr(0xC0A80101);  // 192.168.1.1

// 构造 ICMP Echo Request
uint8_t frame[128];
eth_hdr_t *eth = (eth_hdr_t *)frame;
// ... 填充以太网头部 ...

ip_hdr_t *ip = (ip_hdr_t *)(frame + ETH_HDR_LEN);
// ... 填充 IP 头部 ...

icmp_hdr_t *icmp = (icmp_hdr_t *)(frame + ETH_HDR_LEN + 20);
// ... 填充 ICMP 头部 ...

// 发送
net_tx_packet(dev, frame, total_len);
```

## 测试

### 编译

```bash
cd zhwh_os
make clean
make
```

### 运行

使用 QEMU 运行：

```bash
qemu-system-i386 -m 512M \
    -netdev user,id=net0,hostfwd=tcp::2222-:22 \
    -device rtl8139,netdev=net0 \
    -cdrom os.iso
```

### 预期输出

```
[net] Initializing network stack...
[net] Network stack initialized
[net] Local IP: 192.168.1.1
[loopback] Initializing loopback device...
[net] Registered device: lo
[net]   MAC: 00:00:00:00:00:01
[loopback] Loopback device ready
[rtl8139] RTL8139 driver init
[rtl8139] Found 2 PCI devices
[rtl8139] Found RTL8139 device!
[rtl8139]   Bus: 0, Device: 3, Function: 0
[rtl8139] I/O base: 0xC000
[rtl8139] IRQ: 11
[rtl8139] Reset complete
[rtl8139] RX buffer: 0xC1000000
[rtl8139] TX buffer 0: 0xC2000000
[rtl8139] TX buffer 1: 0xC2001000
[rtl8139] TX buffer 2: 0xC2002000
[rtl8139] TX buffer 3: 0xC2003000
[rtl8139] MAC: 52:54:00:12:34:56
[rtl8139] RTL8139 initialized
[net] Registered device: eth0
[net]   MAC: 52:54:00:12:34:56
Network initialized
```

## 技术细节

### RTL8139 寄存器

| 寄存器 | 地址 | 描述 |
|--------|------|------|
| IDR0-IDR5 | 0x00-0x05 | MAC 地址 |
| CMD | 0x37 | 命令寄存器 |
| TXBUF0-TXBUF3 | 0x20-0x2C | 发送缓冲区地址 |
| RXBUF | 0x30 | 接收缓冲区地址 |
| CAPR | 0x38 | 当前接收缓冲区指针 |
| IMR/ISR | 0x3C/0x3E | 中断掩码/状态 |

### 内存映射

- 发送缓冲区：4 × 2048 字节
- 接收缓冲区：8192 字节（循环缓冲区）

### 中断处理

驱动支持以下中断：

- TX_OK：发送完成
- RX_OK：接收完成
- TX_ERR：发送错误
- RX_ERR：接收错误

## 已知限制

1. **ARP 未完全实现**：目前只有框架，需要完成 ARP 请求和应答
2. **ICMP Echo Reply 未实现**：只发送，不回复
3. **UDP/TCP 未实现**：只有基本的数据结构
4. **内存管理**：使用简单的 kmalloc，没有 DMA 专用内存池
5. **中断处理**：中断处理程序还未集成到 IDT

## 下一步计划

- [ ] 完成 ARP 协议实现
- [ ] 实现 ICMP Echo Reply
- [ ] 添加 UDP 支持
- [ ] 添加 TCP 基础支持
- [ ] 实现 DHCP 客户端
- [ ] 添加网络套接字接口
- [ ] 支持更多网卡型号（如 E1000）

## 参考资料

- [RTL8139 Datasheet](https://www.realtek.com/en/products/communications-network-ics/item/rtl8139b)
- [OSDev RTL8139](https://wiki.osdev.org/RTL8139)
- [QEMU Network Documentation](https://wiki.qemu.org/Documentation/Networking)

## 许可证

本驱动是 Hillson OS 的一部分，遵循项目许可证。
