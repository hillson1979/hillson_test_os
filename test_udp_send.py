#!/usr/bin/env python3
"""
测试脚本：发送模拟的 JPEG 数据到 localhost:1234
用于验证网络接收功能是否正常
"""

import socket
import time

def send_test_data():
    TARGET_HOST = "192.168.0.15" #"127.0.0.1"
    TARGET_PORT = 1234

    print("="*50)
    print(f"发送测试数据到 {TARGET_HOST}:{TARGET_PORT}")
    print("="*50)

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # 简单的 JPEG 文件头（SOI + APP0）
    jpeg_header = b'\xff\xd8\xff\xe0\x00\x10JFIF\x00\x01\x01\x00\x00\x01\x00\x01\x00\x00'

    try:
        for i in range(20):
            # 构造模拟的 JPEG 数据包
            timestamp = time.time()
            payload = jpeg_header + f"TEST_FRAME_{i}_{timestamp:.2f}".encode('utf-8')
            # 填充到 1KB 大小
            payload += b'\x00' * (1024 - len(payload))

            sock.sendto(payload, (TARGET_HOST, TARGET_PORT))
            print(f"[ {i+1}/20 ] 发送 {len(payload)} 字节")

            time.sleep(0.3)  # 模拟 3fps

        print("\n✅ 测试完成！")
        print(f"如果 QEMU 中运行了 net init e1000，应该看到数据包被接收")

    except Exception as e:
        print(f"\n❌ 错误: {e}")
    finally:
        sock.close()

if __name__ == "__main__":
    send_test_data()
