#!/bin/bash
# 视频播放器测试脚本

# 配置
VIDEO_FILE=${1:-"test.mp4"}  # 默认使用 test.mp4
OS_IP=${2:-"192.168.0.15"}      # 默认OS IP
VIDEO_PORT=1234              # UDP端口
WIDTH=320
HEIGHT=240

echo "======================================"
echo "  Video Player Test Script"
echo "======================================"
echo "Video file: $VIDEO_FILE"
echo "Target OS: $OS_IP:$VIDEO_PORT"
echo "Resolution: ${WIDTH}x${HEIGHT}"
echo ""
echo "Starting FFmpeg stream..."
echo "Press Ctrl+C to stop"
echo ""

# 检查ffmpeg是否安装
if ! command -v ffmpeg &> /dev/null; then
    echo "Error: ffmpeg not found!"
    echo "Install with: sudo apt install ffmpeg"
    exit 1
fi

# 检查视频文件
if [ ! -f "$VIDEO_FILE" ]; then
    echo "Error: Video file '$VIDEO_FILE' not found!"
    echo ""
    echo "Usage: $0 [video_file] [os_ip]"
    echo "Example: $0 my_video.mp4 10.0.2.15"
    exit 1
fi

# 方法1: 基础MJPEG推流
echo "Method 1: Basic MJPEG over UDP"
ffmpeg -re -i "$VIDEO_FILE" \
    -vf scale=${WIDTH}:${HEIGHT} \
    -f mjpeg \
    udp://$OS_IP:$VIDEO_PORT

# 如果上面的方法失败，可以尝试以下替代方案：

# 方法2: 使用硬件加速（如果支持）
# ffmpeg -hwaccel qsv -i "$VIDEO_FILE" \
#     -c:v mjpeg_qsv \
#     -vf scale=${WIDTH}:${HEIGHT} \
#     -f mjpeg \
#     udp://$OS_IP:$VIDEO_PORT

# 方法3: 降低帧率和比特率
# ffmpeg -re -i "$VIDEO_FILE" \
#     -vf scale=${WIDTH}:${HEIGHT},fps=15 \
#     -q:v 5 \
#     -f mjpeg \
#     udp://$OS_IP:$VIDEO_PORT

# 方法4: TCP推流（更可靠）
# ffmpeg -re -i "$VIDEO_FILE" \
#     -vf scale=${WIDTH}:${HEIGHT} \
#     -f mjpeg \
#     tcp://$OS_IP:$VIDEO_PORT
