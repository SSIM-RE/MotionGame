# MotionGame 🎮

> 基于 ESP32-C3 的体感游戏设备 + WiFi 无线键盘控制器

![Platform](https://img.shields.io/badge/platform-ESP32--C3-blue)
![Framework](https://img.shields.io/badge/framework-Arduino-b2844b)
![Status](https://img.shields.io/badge/status-Active-brightgreen)

---

## 项目简介

MotionGame 是一款基于 ESP32-C3 的体感游戏设备，集成 MPU6050 六轴传感器，支持多款体感游戏。设备同时具备 **WiFi 无线键盘控制器** 功能，可以将倾斜、摇晃手势实时映射为键盘输入，控制 PC 上的应用程序。

---

## 硬件规格

| 组件 | 型号 | 说明 |
|------|------|------|
| MCU | ESP32-C3 | RISC-V 架构，160MHz，WiFi 内置 |
| IMU | MPU6050 | 6轴陀螺仪+加速度计，I2C 接口 |
| 屏幕 | ST7735 | 128×160 LCD，SPI 接口 |
| 震动 | L298N 马达驱动 | PWM 多段波形震动反馈 |
| 声音 | 蜂鸣器 | 多音符序列音效 |
| 供电 | USB-C | 5V/500m |

---

## 🎬 功能演示
见img文件夹
### 🖥️ 系统切换

<img src="D:\projects\MotionGame\img\系统.gif" alt="系统" style="zoom:30%;" />

🖥️ 主题切换

<img src="D:\projects\MotionGame\img\主题.gif" alt="主题" style="zoom:30%;" />

### 🎮 游戏

<img src="D:\projects\MotionGame\img\游戏.gif" alt="游戏" style="zoom:33%;" />

### ⌨️ 体感控制输入

<img src="D:\projects\MotionGame\img\控制.gif" alt="控制" style="zoom:40%;" />

| 设备倾斜 | → | PC 按键 | 说明 |
|:--------:|:-:|:-------:|:-----:|
| 前倾 | → | ↑ | 向上 |
| 后倾 | → | ↓ | 向下 |
| 左倾 | → | ← | 向左 |
| 右倾 | → | → | 向右 |
| 摇晃 | → | A | 确认 |

---

## 功能模式

### 🎮 游戏模式

| 游戏 | 控制方式 | 说明 |
|------|----------|------|
| 🐍 贪吃蛇 | 前后左右倾斜 | 经典贪吃蛇，体感控制方向 |
| ⚽ 平衡球 | 前后左右倾斜 | 控制小球滚动到目标区域 |
| 🏎️ 赛车 | Roll=加速/减速 Pitch=转向 | 伪3D 赛道，动态天空系统 |

### ⌨️ WiFi 控制器模式

将设备变为无线键盘，通过 UDP 发送按键事件到 PC。

| 倾斜方向 | 映射按键 |
|----------|----------|
| 右倾 (pitch ↑) | → (右箭头) |
| 左倾 (pitch ↓) | ← (左箭头) |
| 前倾 (roll ↑) | ↑ (上箭头) |
| 后倾 (roll ↓) | ↓ (下箭头) |
| 摇晃 | A (50ms 脉冲) |

进入模式时自动锁定基准姿态。

---

## 🏎️ 赛车游戏 - 伪3D 赛道实现详解

### 参考项目

本游戏的伪3D赛道实现参考自 [BojanSof/Pseudo3D-road](https://github.com/BojanSof/Pseudo3D-road)，核心技术原理源自 [Lou's Pseudo 3d Page](http://www.extentofthejam.com/pseudo/)。

### 1. 核心概念：什么是伪3D？

伪3D（又称 2.5D）是一种通过**伪透视变换**创造三维环境错觉的技术。与真正的3D多边形渲染不同，它通过对每一行（scanline）进行独立计算来实现深度感。

**为什么用伪3D而不是真3D多边形？**
- 计算量极低，适合 ESP32 这种资源有限的 MCU
- 独特的** warp 效果**能营造出老式街机赛车游戏的**速度感和兴奋感**
- 可以轻松创建超大赛道，无需担心空间关系

### 2. 赛道渲染原理

#### 2.1 Z-Map（深度映射表）

伪3D赛道的核心是**预计算的 Z-Map**：为屏幕的每一行计算对应的深度值（距离摄像机的远近）。

```
屏幕 Y 坐标 ──────→ 深度 Z 值
─────────────────────────────────
   0 (顶部/远处)  →  最大距离
   159 (底部/近处) →  最小距离 (clip_z)
```

本游戏使用简化的线性 Z 映射：
```c
float world_z = n * RACE_SEGMENT_LENGTH - cam_z;
// n = 赛道段索引
// RACE_SEGMENT_LENGTH = 200.0f (每段物理长度)
// cam_z = 摄像机在世界中的位置
```

#### 2.2 3D 透视投影

将 3D 世界坐标投影到 2D 屏幕坐标：

```c
// 透视投影公式
float d = 1.0f / tanf((fov * 0.5f) * M_PI / 180.0f);  // 焦距
float scale = d / world_z;                              // 缩放因子
float screen_x = (RACE_SCREEN_W / 2.0f) + (RACE_SCREEN_W / 2.0f) * world_x * scale;
float screen_y = (RACE_SCREEN_H / 2.0f) - (RACE_SCREEN_H / 2.0f) * world_y * scale;
float screen_w = (RACE_SCREEN_W / 2.0f) * (ROAD_WIDTH * scale);  // 道路宽度
```

**关键点**：
- 摄像机**灭点固定在屏幕中央**
- 视角偏移完全依赖 `world_x`（道路弯曲度 + 玩家横向位置）

#### 2.3 道路段渲染（画家算法）

从近到远绘制，每一段都是梯形四边形：

```c
drawQuad(sx1, sx2, sy1, sy2,   // 上下边缘的屏幕 X 坐标
         sw1, sw2,              // 上下边缘的半宽
         color);               // 填充颜色
```

**颜色条纹系统**：
```c
// 根据距离索引决定颜色（每3段切换一次）
uint16_t grass = (abs_idx / 3) % 2 ? GRASS1 : GRASS2;  // 草地
uint16_t road  = (abs_idx / 3) % 2 ? ROAD1  : ROAD2;   // 道路
uint16_t rum   = (abs_idx / 9) % 2 ? RUMBLE1 : RUMBLE2; // 路肩
uint16_t line  = (abs_idx / 6) % 2 ? 0 : LINE_COLOR;   // 中心线
```

### 3. 弯道实现

弯道通过**累积偏移量**实现。每段的 `curve` 值累加到道路中心线上：

```c
current_dx += prev->curve;   // 累加弯道曲率
current_x += current_dx;     // 道路中心 X 偏移

// 世界坐标系 X = -(道路累加弯曲度 + 玩家横向偏移)
float world_x = -current_x - player_x;
```

### 4. 动态 FOV（视野角度）

速度越快，FOV 越宽，增强速度感：

```c
float dynamic_fov = base_fov + (speed / max_speed) * 20.0f;
// 基础 FOV=100°，最高可到 120°
```

### 5. Procedural 赛道生成

赛道是**无限生成的**，每个赛道段包含：
- `curve`: 弯道曲率 (-8.0 ~ +8.0)
- `y`: 地形高度（上下坡）
- `object_type`: 路边景物类型（树木、棕榈树、标志牌）
- `object_side`: 景物在左侧还是右侧
- `object_scale`: 景物大小缩放

生成器使用三阶段循环：
```
阶段0: 平滑过渡到弯道 (easeInOut)
阶段1: 保持弯道
阶段2: 平滑回归直线 (easeInOut)
```

### 6. 路边景物系统

景物以**公告板（Billboard）** 方式绘制，通过几何图形模拟：

| 类型 | 绘制方式 | 参数 |
|------|----------|------|
| 松树 | 三角形树冠 + 矩形树干 | 缩放比例 |
| 棕榈树 | 多层圆环几何体 | 缩放比例 |
| 路牌 | 矩形牌子 + 矩形支柱 | 侧边方向 |

景物从远到近绘制（反向画家算法），保证正确的遮挡关系。

### 7. 天空系统

动态时间系统，24小时循环：

| 时段 | 天空颜色 | 天体 |
|------|----------|------|
| 0:00 - 4:00 | 深夜蓝 | 月亮 + 星星 |
| 4:00 - 6:00 | 黎明渐变 | 日出 |
| 6:00 - 18:00 | 白天 | 太阳 |
| 18:00 - 20:00 | 黄昏渐变 | 日落 |
| 20:00 - 24:00 | 夜晚 | 月亮 + 星星 |

天空 X 轴偏移与弯道联动：
```c
sky_offset -= current_curve * speed * 0.035f;
// 过弯时天空会向弯道反方向偏移
```

---

## 🎨 主题切换系统

### 6 种预设主题

| 主题 | 背景 | 主色 | 次色 | 强调色 | 适用场景 |
|------|------|------|------|--------|----------|
| **Classic** | 黑 | 红 | 绿 | 青 | 复古街机 |
| **Dark** | 深灰 | 蓝紫 | 浅蓝 | 深青 | 夜间使用 |
| **Light** | 白 | 浅红 | 深红 | 绿 | 明亮环境 |
| **Retro** | 黑 | 绿 | 橙红 | 青 | 80年代风 |
| **Game** | 黑 | 品红 | 青 | 品红 | 游戏主机 |
| **Tech** | 黑 | 青 | 品红 | 青 | 科技感 |

### 颜色结构体

```c
typedef struct {
    uint16_t background;   // 背景色
    uint16_t foreground;   // 前景色
    uint16_t primary;      // 主色（标题）
    uint16_t secondary;   // 次色（副标题）
    uint16_t accent;      // 强调色（高亮）
    uint16_t text;        // 文本色
    uint16_t highlight;    // 选中高亮
    uint16_t border;      // 边框色
} ThemeColors_t;
```

### 主题切换操作

| 操作 | 效果 |
|------|------|
| 左右倾斜 | 切换主题预览 |
| 摇晃 | 确认选中主题，保存到 EEPROM |
| Roll 后倾 | 取消，返回菜单（不保存） |

### EEPROM 持久化

主题设置保存在 ESP32 的 **EEPROM** 地址 0，首次启动自动读取。

```c
#define THEME_EEPROM_ADDR 0
EEPROM.put(THEME_EEPROM_ADDR, current_theme);
```

---

## 项目结构

```
MotionGame/
├── firmware/MotionGame/          # Arduino 固件
│   ├── MotionGame.ino            # 主入口
│   ├── system.cpp/h              # 状态机 (菜单→游戏→控制器)
│   ├── menu_app.cpp/h             # 主菜单
│   ├── game_select_app.cpp/h      # 游戏选择
│   ├── game_snake.cpp/h          # 贪吃蛇
│   ├── game_balance.cpp/h         # 平衡球
│   ├── game_race.cpp/h           # 赛车 (伪3D) ⭐
│   │   └── 核心渲染:
│   │       ├── project()         # 3D透视投影
│   │       ├── drawQuad()        # 四边形绘制
│   │       ├── generateNextSegment() # 赛道生成
│   │       ├── render()          # 双通道渲染
│   │       └── drawSkyGradient() # 动态天空
│   ├── wifi_controller.cpp/h      # WiFi 控制器 ⭐
│   ├── motion_service.cpp/h      # 体感识别
│   ├── display_driver.cpp/h       # LCD 驱动
│   ├── output_feedback.cpp/h      # 震动+蜂鸣器统一反馈
│   ├── motor.cpp/h               # 马达驱动
│   ├── buzzer.cpp/h              # 蜂鸣器驱动
│   └── theme_app.cpp/h           # 主题切换 ⭐
├── tools/                         # PC 端工具
│   ├── udp_keyboard.py           # UDP 键盘模拟器
│   └── requirements.txt          # Python 依赖
├── hardware/                      # 硬件设计文件
└── docs/                          # 开发文档
```

---

## 快速开始

### 1. 编译固件

**环境要求：**
- [Arduino IDE](https://www.arduino.cc/en/software) 2.x
- ESP32 核心 (通过 Board Manager 安装 `esp32`)

**步骤：**
```bash
# 克隆项目
git clone <repo-url>
cd MotionGame

# 在 Arduino IDE 中打开
#    文件 → 打开 → firmware/MotionGame/MotionGame.ino

# 选择开发板
#    工具 → 开发板 → ESP32 Arduino → ESP32C3 Dev Module

# 上传
#    烧录按钮 (→)
```

### 2. 配置 WiFi 控制器

编辑 `firmware/MotionGame/wifi_controller.h`：

```cpp
#define WIFI_SSID           "你的WiFi名称"
#define WIFI_PASSWORD       "你的WiFi密码"
#define WIFI_TARGET_IP      "192.168.101.2"    // PC 的 IP 地址
#define WIFI_TARGET_PORT    8888
```

### 3. 运行 PC 端接收器

```bash
# 安装依赖
pip install keyboard

# 运行（需要管理员权限）
python tools/udp_keyboard.py
```

---

## 游戏控制说明

### 🐍 贪吃蛇

| 动作 | 控制 |
|------|------|
| 上下移动 | 前后倾斜 |
| 左右移动 | 左右倾斜 |

### ⚽ 平衡球

| 动作 | 控制 |
|------|------|
| 球滚动方向 | 倾斜方向 |
| 目标区域 | 到达终点得分 |

### 🏎️ 赛车

| 动作 | 控制 |
|------|------|
| 加速 | Roll 前倾（倾斜度决定油门深度） |
| 减速/刹车 | Roll 后倾 |
| 左转 | Pitch 左倾 |
| 右转 | Pitch 右倾 |
| 退出游戏 | 摇晃 |

**赛车特性**：
- 无限 procedural 赛道
- 动态 FOV（速度越快视野越宽）
- 动态天空（24小时循环）
- 路边随机景物
- 离心力物理反馈
- 草地减速惩罚
- 引擎震动反馈

---

## WiFi 控制器使用指南

### 连接流程

```
1. ESP32 上电 → 显示主菜单
2. 选择 "CONTROLLER" 选项
3. ESP32 自动连接 WiFi
4. WiFi 连接成功后自动锁定基准姿态
5. 立即开始发送按键
```

### 调节灵敏度

编辑 `firmware/MotionGame/wifi_controller.h`：

```cpp
#define TILT_THRESHOLD      10.0f   // 倾斜阈值（度），值越小越灵敏
```

### UDP 协议

```
PRESS:UP       # 按下 上箭头
RELEASE:UP     # 释放 上箭头
RELEASE:ALL    # 释放所有按键
```

---

## 技术特性

### 体感识别
- 倾斜检测 (TILT_LEFT/RIGHT/FORWARD/BACK)
- 摇晃检测 (MOTION_SHAKE)
- Roll 姿态检测 (ROLL_FORWARD/BACK)

### 显示系统
- ST7735 128×160 LCD
- SPI 双缓冲，无闪烁
- 6 种主题色彩，EEPROM 持久化

### 反馈系统
- 马达：多段波形震动 (L298N 驱动)
- 蜂鸣器：多音符序列 (DO RE MI...)
- 统一 API：`Output_Feedback(FEEDBACK_EAT/HIT/DIE...)`

### WiFi 控制器
- UDP 单播直连 PC
- 实时按键映射
- 摇晃自动触发 A 键 (50ms 脉冲)

---

## 开发

### 串口调试命令（赛车游戏）

| 按键 | 功能 |
|------|------|
| W/S | 增加/减少摄像机高度 |
| R/F | 增加/减少 FOV |
| ]/[ | 增加/减少最大速度 |

### 添加新游戏

1. 在 `system_state.h` 添加新 `GameID_t`
2. 创建 `game_xxx.cpp/h`
3. 在 `game_app.cpp` 注册游戏
4. 在 `game_select_app.cpp` 添加游戏项

### 添加新按键

1. 在 `wifi_controller.h` 的 `ControllerKey_t` 添加
2. 在 `keys_to_byte()` 和 `send_key_event()` 添加映射
3. 在 Python `KEY_MAP` 添加

---

## 参考资料

- [Lou's Pseudo 3d Page](http://www.extentofthejam.com/pseudo/) - 伪3D赛道核心原理
- [Code inComplete's How to build a racing game](https://codeincomplete.com/articles/javascript-racer/) - JavaScript 赛车游戏教程
- [BojanSof/Pseudo3D-road](https://github.com/BojanSof/Pseudo3D-road) - 参考实现

---

## 更新日志

| 日期 | 内容 |
|------|------|
| 2026-04 | WiFi 控制器模式，支持 PC 键盘模拟 |
| 2026-04 | 详细伪3D赛道实现文档、主题切换系统文档 |
| 2026-03 | 震动反馈系统、游戏反馈音效 |
| 2026-02 | 赛车游戏、动态天空系统 |
| 2026-01 | 贪吃蛇、平衡球游戏 |

---

## 许可证

MIT License

## 作者

SSIM-RE
