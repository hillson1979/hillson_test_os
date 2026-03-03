# 视频播放器模块 - 集成说明

## 📁 文件说明

```
video_player.h           - 头文件（API定义）
video_player.c           - 核心模块（双缓冲、LVGL集成）
video_player_net.c       - 网络接收器（UDP/TCP）
video_player_decode.c    - JPEG解码器（stb_image）
video_player_example.c   - 集成示例代码
```

## 🔧 如何集成到你的OS

### 步骤1：添加源文件到编译

修改你的Makefile或构建脚本，添加以下文件：

```makefile
# 添加到你的源文件列表
SRCS += video/video_player.c
SRCS += video/video_player_net.c
SRCS += video/video_player_decode.c
SRCS += video/video_simple_udp.c
SRCS += video/video_player_socket_compat.c
SRCS += video/video_player_os_stats.c
```

### 步骤1.5：集成UDP接收

我们提供了 `video_simple_udp.c`，它通过一个简单的钩子函数接收UDP数据。

**在你的 `net/core.c` 的 `udp_input()` 函数中添加以下代码：**

```c
// 在 udp_input() 函数开头添加
extern int video_udp_hook(uint16_t dport, uint8_t *data, uint32_t len);

int udp_input(net_device_t *dev, uint8_t *data, uint32_t len) {
    udp_hdr_t *udp = (udp_hdr_t *)data;

    if (len < sizeof(udp_hdr_t)) {
        printf("[net] UDP packet too short\n");
        return -1;
    }

    // 提取端口
    uint16_t dport = ntohs(udp->udp_dport);

    // 🔥 新增：调用视频播放器钩子
    uint8_t *udp_data = data + sizeof(udp_hdr_t);
    uint32_t udp_data_len = len - sizeof(udp_hdr_t);

    if (video_udp_hook(dport, udp_data, udp_data_len) == 0) {
        // 视频播放器已处理这个包
        return 0;
    }

    // 原有的UDP处理逻辑继续...
    printf("[net] UDP: sport=%d, dport=%d, len=%d\n",
           ntohs(udp->udp_sport), dport, ntohs(udp->udp_len));
    // ...
}
```

这样，当UDP数据包到达端口1234时，视频播放器会自动接收它。

### 步骤2：获取stb_image.h

下载 [stb_image.h](https://github.com/nothings/stb)：

```bash
wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
```

### 步骤3：在主函数中初始化

```c
#include "video_player.h"

int main(void)
{
    // ... 现有初始化 ...

    // 初始化视频播放器
    net_recv_init("0.0.0.0", 1234, 1);  // UDP, 端口1234
    video_player_init();

    // 主循环
    while (1) {
        video_player_task();   // 处理视频帧
        lv_timer_handler();    // LVGL
        // ... 其他任务 ...
    }
}
```

## 📡 PC端推流命令

### UDP推流（推荐）
```bash
ffmpeg -re -i test.mp4 -vf scale=320:240 -f mjpeg udp://<你的OS_IP>:1234
```

### TCP推流
```bash
ffmpeg -re -i test.mp4 -vf scale=320:240 -f mjpeg tcp://<你的OS_IP>:1234
```

### 摄像头实时推流
```bash
ffmpeg -f v4l2 -i /dev/video0 -vf scale=320:240 -f mjpeg udp://<你的OS_IP>:1234
```

## 🎯 配置参数

在 `video_player.h` 中修改：

```c
#define VIDEO_W     320   // 视频宽度
#define VIDEO_H     240   // 视频高度
#define MAX_JPEG    (200*1024)  // 最大JPEG帧大小
```

## 📊 运行时统计

### 基础统计
```c
const video_stats_t *stats = video_player_get_stats();
printf("FPS: %u\n", stats->fps);
printf("Frames: %u\n", stats->frames_displayed);
printf("Received: %u bytes\n", stats->bytes_received);
```

### 性能统计
```c
printf("Decode time: %u ms (avg)\n", stats->avg_decode_time);
printf("Decode time: %u ms (max)\n", stats->max_decode_time);
```

### 网络统计
```c
printf("Bandwidth: %u B/s (current)\n", stats->last_bandwidth);
printf("Bandwidth: %u B/s (average)\n", stats->avg_bandwidth);
printf("Packets: %u\n", stats->total_packets);
```

### CPU和内存统计（需要OS支持）
```c
printf("Memory: %u bytes\n", stats->memory_used);
printf("CPU: %u.%u%%\n", stats->cpu_usage / 10, stats->cpu_usage % 10);
```

### 在LVGL界面中显示
使用 `vstat` 命令：
```
> vstat
```

这会显示完整的统计信息，包括：
- 帧数和FPS
- 网络带宽
- 解码性能
- 错误计数
- 内存和CPU使用（如果OS支持）

## 📈 新增统计字段说明

video_stats_t 结构包含以下新增字段：

- **解码性能**：
  - `total_decode_time`: 总解码时间
  - `avg_decode_time`: 平均每帧解码时间
  - `max_decode_time`: 最慢帧解码时间

- **网络统计**：
  - `last_bandwidth`: 当前带宽（bytes/s）
  - `avg_bandwidth`: 平均带宽（bytes/s）
  - `total_packets`: 总接收包数

- **系统资源**（需要OS实现）：
  - `memory_used`: 当前内存使用量
  - `memory_peak`: 峰值内存使用量
  - `cpu_usage`: CPU占用率（千分比）
  - `cpu_time_used`: 累计CPU时间

## 🔧 实现OS统计接口（可选）

如果需要CPU和内存统计，在 `video_player_os_stats.c` 中实现：

```c
uint32_t get_memory_usage(void) {
    // 返回你的OS当前内存使用量
    return your_os_memory_used();
}

uint32_t get_cpu_usage(void) {
    // 返回视频任务CPU占用率（千分比0-1000）
    return your_os_cpu_usage();
}
```

## ⚠️ 注意事项

1. **stb_image.h** 需要放在include路径中
2. 确保LVGL已经正确初始化
3. UDP缓冲区可能需要调大（系统配置）
4. 根据你的OS调整网络API（如果不用BSD socket）

## 🚀 测试流程

1. 编译并运行你的OS
2. 在PC上执行FFmpeg推流命令
3. 应该能在LVGL窗口看到视频播放
