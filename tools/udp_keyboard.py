#!/usr/bin/env python3
"""
MotionGame WiFi Controller - PC 端接收器
监听 UDP 端口，模拟键盘按键

依赖安装:
    pip install pyautogui keyboard

运行:
    python udp_keyboard.py
"""

import socket
import threading
import time
import sys
import os

# 尝试导入，失败时给出提示
try:
    import pyautogui
    PYAUTOGUI_AVAILABLE = True
except ImportError:
    PYAUTOGUI_AVAILABLE = False
    print("[警告] pyautogui 未安装，按键模拟将不可用")
    print("安装命令: pip install pyautogui")

try:
    import keyboard
    KEYBOARD_AVAILABLE = True
except ImportError:
    KEYBOARD_AVAILABLE = False
    print("[警告] keyboard 未安装，按键模拟将不可用")
    print("安装命令: pip install keyboard")


# ==================== 配置 ====================
UDP_IP = "0.0.0.0"           # 监听所有接口
UDP_PORT = 8888              # 与 ESP32 发送端口一致
BUFFER_SIZE = 1024

# 按键映射
KEY_MAP = {
    "UP":    "up",
    "DOWN":  "down",
    "LEFT":  "left",
    "RIGHT": "right",
    "A":     "a",
}


# ==================== 按键控制 ====================
class KeyboardController:
    """按键控制器"""
    
    def __init__(self):
        self.active_keys = set()
        self._lock = threading.Lock()
    
    def press(self, key_name):
        """按下按键"""
        with self._lock:
            if key_name in self.active_keys:
                return  # 已经在按
            
            self.active_keys.add(key_name)
        
        if KEYBOARD_AVAILABLE:
            try:
                key = KEY_MAP.get(key_name, key_name.lower())
                keyboard.press(key)
                print(f"[按键] Press: {key_name}")
            except Exception as e:
                print(f"[错误] 按键按下失败: {e}")
    
    def release(self, key_name):
        """释放按键"""
        with self._lock:
            if key_name not in self.active_keys:
                return  # 没有在按
            
            self.active_keys.discard(key_name)
        
        if KEYBOARD_AVAILABLE:
            try:
                key = KEY_MAP.get(key_name, key_name.lower())
                keyboard.release(key)
                print(f"[按键] Release: {key_name}")
            except Exception as e:
                print(f"[错误] 按键释放失败: {e}")
    
    def release_all(self):
        """释放所有按键"""
        with self._lock:
            keys_to_release = list(self.active_keys)
            self.active_keys.clear()
        
        if KEYBOARD_AVAILABLE:
            for key_name in keys_to_release:
                try:
                    key = KEY_MAP.get(key_name, key_name.lower())
                    keyboard.release(key)
                    print(f"[按键] Release: {key_name}")
                except Exception as e:
                    print(f"[错误] 按键释放失败: {e}")


# ==================== UDP 服务器 ====================
class UDPServer:
    """UDP 服务器"""
    
    def __init__(self, controller: KeyboardController):
        self.controller = controller
        self.running = False
        self.socket = None
    
    def start(self):
        """启动服务器"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.socket.bind((UDP_IP, UDP_PORT))
            self.socket.settimeout(0.5)  # 允许定期检查 running 标志
            
            self.running = True
            print(f"[UDP] 服务器已启动，监听 {UDP_IP}:{UDP_PORT}")
            print("=" * 50)
            print("等待 ESP32 连接...")
            print("按 Ctrl+C 退出")
            print("=" * 50)
            
            self._receive_loop()
            
        except OSError as e:
            if e.errno == 10048:  # Address already in use
                print(f"[错误] 端口 {UDP_PORT} 已被占用")
                print("请关闭其他使用此端口的程序，或修改 UDP_PORT 配置")
            else:
                print(f"[错误] 无法绑定端口: {e}")
            self.running = False
            
        except Exception as e:
            print(f"[错误] 服务器异常: {e}")
            self.running = False
    
    def _receive_loop(self):
        """接收数据循环"""
        while self.running:
            try:
                data, addr = self.socket.recvfrom(BUFFER_SIZE)
                self._handle_message(data.decode('utf-8').strip(), addr)
                
            except socket.timeout:
                continue
            except Exception as e:
                if self.running:
                    print(f"[错误] 接收数据异常: {e}")
    
    def _handle_message(self, msg: str, addr):
        """处理接收到的消息"""
        print(f"[收到] {msg} (from {addr[0]}:{addr[1]})")
        
        if not msg:
            return
        
        # 解析消息: PRESS:UP, RELEASE:DOWN, RELEASE:ALL
        if msg == "RELEASE:ALL":
            self.controller.release_all()
            return
        
        parts = msg.split(":", 1)
        if len(parts) != 2:
            print(f"[警告] 无法解析消息: {msg}")
            return
        
        action, key = parts[0].upper(), parts[1].upper()
        
        if action == "PRESS":
            self.controller.press(key)
        elif action == "RELEASE":
            self.controller.release(key)
        else:
            print(f"[警告] 未知动作: {action}")
    
    def stop(self):
        """停止服务器"""
        print("[UDP] 正在停止服务器...")
        self.running = False
        
        # 释放所有按键
        self.controller.release_all()
        
        if self.socket:
            try:
                self.socket.close()
            except:
                pass
        
        print("[UDP] 服务器已停止")


# ==================== 主程序 ====================
def main():
    print("=" * 50)
    print("  MotionGame WiFi Controller - PC 端接收器")
    print("=" * 50)
    
    # 检查依赖
    if not KEYBOARD_AVAILABLE:
        print("\n[错误] keyboard 库是必需的")
        print("请运行: pip install keyboard")
        sys.exit(1)
    
    # 初始化
    controller = KeyboardController()
    server = UDPServer(controller)
    
    # 设置退出处理
    def cleanup():
        print("\n[主程序] 清理退出...")
        server.stop()
    
    try:
        server.start()
    except KeyboardInterrupt:
        print("\n[主程序] 收到 Ctrl+C")
        cleanup()
    
    print("[主程序] 程序结束")


if __name__ == "__main__":
    main()
