#!/usr/bin/env python3
"""
纯 UDP 测试 - 只打印收到的数据，不模拟按键
"""
import socket

UDP_IP = "0.0.0.0"
UDP_PORT = 8888

print("=" * 50)
print("UDP 测试模式 - 只打印收到的数据")
print(f"监听 {UDP_IP}:{UDP_PORT}")
print("=" * 50)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))
sock.settimeout(5.0)

print("\n等待数据...\n")

try:
    while True:
        try:
            data, addr = sock.recvfrom(1024)
            print(f"[收到] {data.decode('utf-8')} (from {addr[0]}:{addr[1]})")
        except socket.timeout:
            print(".", end="", flush=True)
except KeyboardInterrupt:
    print("\n\n测试结束")
finally:
    sock.close()
