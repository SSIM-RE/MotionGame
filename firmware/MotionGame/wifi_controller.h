#ifndef WIFI_CONTROLLER_H
#define WIFI_CONTROLLER_H

#include "system_state.h"
#include <stdint.h>

/* ================= WiFi 配置 ================= */
#define WIFI_SSID           "ChinaNet-15051"
#define WIFI_PASSWORD       "15520649287"
#define WIFI_CONTROLLER_IP  "192.168.101.100"
#define WIFI_TARGET_IP      "192.168.101.2"
#define WIFI_TARGET_PORT    8888
#define WIFI_LOCAL_PORT     8889

/* ================= 体感阈值 ================= */
#define TILT_THRESHOLD      30.0f   // 倾斜阈值（度），降低灵敏度

/* ================= 按键枚举 ================= */
typedef enum {
    KEY_NONE = 0,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_A
} ControllerKey_t;

/* ================= 状态 ================= */
typedef enum {
    WIFI_CTRL_CONNECTING,
    WIFI_CTRL_CONNECTED,
    WIFI_CTRL_CALIBRATING,
    WIFI_CTRL_RUNNING,
    WIFI_CTRL_ERROR
} WifiControllerState_t;

/* ================= API ================= */

// 初始化 WiFi 控制模式
void WifiController_Init(void);

// 更新（主循环调用）
void WifiController_Update(void);

// 退出控制模式
void WifiController_Exit(void);

// 获取当前状态
WifiControllerState_t WifiController_GetState(void);

// 获取校准状态
bool WifiController_IsCalibrated(void);

#endif // WIFI_CONTROLLER_H
