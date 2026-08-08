#!/usr/bin/env python3
"""
UDP 服务器 - 监听 9999 端口
用于测试 OS 发送的 UDP 包
"""

import socket
import sys
from datetime import datetime

# 创建 UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# 绑定到所有接口的 9999 端口
server_address = ('0.0.0.0', 9999)
sock.bind(server_address)

print(f"🎯 UDP Server listening on {server_address[0]}:{server_address[1]}")
print(f"   Local IP: 192.168.0.145")
print(f"   Waiting for packets from OS (192.168.0.15)...")
print(f"   Press Ctrl+C to stop\n")
print("="*60)

try:
    while True:
        # 接收数据
        data, client_address = sock.recvfrom(4096)

        # 解码消息
        try:
            message = data.decode('utf-8').strip()
        except:
            message = data.hex()

        # 打印接收到的信息
        ts = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        print(f"\n📨 [{ts}] [RX] From {client_address[0]}:{client_address[1]}")
        print(f"   Length: {len(data)} bytes")
        print(f"   Data: {message}")

        print("-"*60)

except KeyboardInterrupt:
    print("\n\n✋ Server stopped by user")
finally:
    sock.close()
    print("Socket closed")
