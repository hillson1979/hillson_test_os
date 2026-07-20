#!/usr/bin/env python3
"""
UDP 客户端测试脚本
发送 UDP 包到 192.168.0.145:9999
"""

import socket
import time

def udp_test():
    # 目标地址和端口
    TARGET_HOST = "192.168.0.145"
    TARGET_PORT = 9999

    print(f"=== UDP 客户端测试 ===")
    print(f"目标: {TARGET_HOST}:{TARGET_PORT}")
    print(f"创建 UDP socket...")

    # 创建 UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(5.0)  # 5 秒超时

    # 测试消息列表
    messages = [
        "Hello from Python!",
        "Test message 2",
        "PING",
        "Are you there?",
        "Final test message"
    ]

    try:
        for i, message in enumerate(messages, 1):
            # 发送数据
            data = message.encode('utf-8')
            print(f"\n[{i}/{len(messages)}] 发送: {message}")
            print(f"    数据长度: {len(data)} 字节")

            sock.sendto(data, (TARGET_HOST, TARGET_PORT))
            print(f"    ✓ 已发送到 {TARGET_HOST}:{TARGET_PORT}")

            # 尝试接收回复
            try:
                recv_data, addr = sock.recvfrom(4096)
                print(f"    ✓ 收到回复: {recv_data.decode('utf-8', errors='ignore')}")
                print(f"    来自: {addr[0]}:{addr[1]}")
            except socket.timeout:
                print(f"    ⏱ 无回复（超时 5 秒）")

            # 短暂延迟
            time.sleep(0.5)

        print(f"\n=== 测试完成 ===")

    except Exception as e:
        print(f"\n❌ 错误: {e}")
    finally:
        sock.close()
        print("Socket 已关闭")

if __name__ == "__main__":
    # 多次测试循环
    test_rounds = 3
    for round_num in range(1, test_rounds + 1):
        print(f"\n{'='*50}")
        print(f"第 {round_num}/{test_rounds} 轮测试")
        print('='*50)
        udp_test()
        time.sleep(1)

    print(f"\n所有测试完成!")
