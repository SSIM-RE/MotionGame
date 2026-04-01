# MotionGame 技术信息

> 最后更新: 2026-03-30

---

## 硬件配置

### ESP32-C3
- 架构: RISC-V
- 特性: WiFi + Bluetooth 5.0 LE

### MPU6050
- 类型: 6轴 IMU（3轴陀螺仪 + 3轴加速度计）
- 接口: I2C
- 地址: 0x68

### LCD 128x160
- 接口: SPI
- 驱动: ST7735

### 反馈设备
- 无源蜂鸣器: 声音反馈
- 震动电机: 触觉反馈

---

## 固件架构

### 目录结构
```
firmware\MotionGame\
├── MotionGame.ino      # 主程序
└── (其他源文件)
```

### 核心模块
| 模块 | 说明 |
|------|------|
| MPU6050 | 读取姿态数据，计算倾斜角度 |
| InputSystem | 体感输入解析（倾斜/晃动检测） |
| GameEngine | 游戏循环管理 |
| DisplaySystem | 双缓冲 LCD 显示 |
| AudioSystem | 蜂鸣器音效 |
| HapticSystem | 震动反馈 |
| CloudService | 云端接口（预留） |

---

## 体感识别

### 姿态计算
- 使用 DMP（Digital Motion Processor）获取四元数
- 转换为欧拉角（Roll/Pitch/Yaw）
- 倾斜角度用于方向控制

### 动作检测
| 动作 | 检测方式 |
|------|----------|
| 倾斜 | Pitch/Roll 角度超过阈值 |
| 晃动 | 加速度变化率超过阈值 |
| 点击 | 短时间大加速度峰值 |

---

## 开发环境

### 工具链
- Arduino IDE 或 PlatformIO
- ESP-IDF
- Arduino ESP32 Core

### 烧录配置
- Board: ESP32C3 Dev Module
- Baudrate: 921600
- Port: COMx

---

## 技术笔记

### 双缓冲显示
- 原理: 在内存中构建完整帧，再一次性发送到 LCD
- 避免部分刷屏导致的闪烁

### 体感校准
- 开机时静止校准零偏
- 动态补偿温度漂移
