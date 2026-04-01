# MotionGame 🎮

> 基于 ESP32-C3 的体感游戏设备

![Platform](https://img.shields.io/badge/platform-ESP32--C3-blue)
![Framework](https://img.shields.io/badge/framework-Arduino-b2844b)
![Status](https://img.shields.io/badge/status-Active-brightgreen)

## 项目简介

MotionGame 是一款基于 ESP32-C3 的体感游戏设备，通过 MPU6050 六轴传感器实现倾斜、摇晃等手势控制，支持多款复古风格小游戏。

## 硬件规格

| 组件 | 型号 | 说明 |
|------|------|------|
| MCU | ESP32-C3 | RISC-V 架构，160MHz |
| IMU | MPU6050 | 6轴陀螺仪+加速度计 |
| 屏幕 | ST7735 | 128×160 LCD，SPI 接口 |
| 反馈 | 蜂鸣器 + 震动马达 | 音效+触觉反馈 |
| 供电 | USB-C | 5V/500mA |

## 游戏列表

| 游戏 | 控制方式 | 说明 |
|------|----------|------|
| 🐍 贪吃蛇 | 前后倾斜 | 经典贪吃蛇，体感控制方向 |
| ⚽ 平衡球 | 前后左右倾斜 | 将球移到目标区域 |
| 🏎️ 赛车 | Roll=加速/减速 Pitch=转向 | 伪3D街机风格赛车 |

## 特性

- 🎯 **体感控制** - 倾斜、摇晃、抖动精确识别
- 🔊 **音效反馈** - 蜂鸣器多级别音效
- 📳 **震动反馈** - 触觉马达震动
- 🌅 **动态环境** - 随时间变化的天空（黄昏/夜晚/黎明）
- 🏎️ **动态视野** - 速度越快 FOV 越宽

## 项目结构

```
MotionGame/
├── firmware/MotionGame/    # Arduino 固件源码
│   ├── game_snake.cpp      # 贪吃蛇游戏
│   ├── game_balance.cpp    # 平衡球游戏
│   ├── game_race.cpp       # 赛车游戏
│   ├── motor.h/cpp         # 震动马达驱动
│   ├── buzzer.h/cpp        # 蜂鸣器驱动
│   └── output_feedback.h/cpp # 统一反馈系统
├── hardware/               # 硬件设计文件
├── docs/                   # 开发文档
└── README.md
```

## 快速开始

### 编译固件

1. 安装 [Arduino IDE](https://www.arduino.cc/en/software) + [ESP32 核心](https://github.com/espressif/arduino-esp32)
2. 打开 `firmware/MotionGame/MotionGame.ino`
3. 选择开发板 `ESP32C3 Dev Module`
4. 上传

### 串口调试命令

```
SET_FOV=120     # 设置视野角度
SET_H=2000      # 设置摄像机高度
SET_SPD=200     # 设置最大速度
```

## 控制说明

### 贪吃蛇
- 前后倾斜 → 上下移动
- 左右倾斜 → 左右移动

### 平衡球
- 前后左右倾斜 → 控制球滚动方向

### 赛车
- Roll 前倾 → 加速
- Roll 后倾 → 减速/刹车
- Pitch 左倾 → 左转
- Pitch 右倾 → 右转
- 摇晃 → 退出游戏

## 技术栈

- **硬件**: ESP32-C3, MPU6050, ST7735 LCD
- **框架**: Arduino + ESP-IDF
- **传感器**: I2C 通信，姿态解算
- **显示**: SPI 双缓冲，无闪烁

## 开发记录

- 2026-01: 硬件基础搭建完成
- 2026-02: 体感识别系统完成
- 2026-03: 贪吃蛇 + 平衡球游戏完成
- 2026-04: 赛车游戏 + 反馈系统完成

## 许可证

MIT License

## 作者

SSIM-RE
