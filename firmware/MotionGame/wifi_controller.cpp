#include "wifi_controller.h"
#include "motion_service.h"
#include "motion_type.h"
#include "output_feedback.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// ============================================================
// 内部状态
// ============================================================
static WifiControllerState_t state = WIFI_CTRL_CONNECTING;
static bool calibrated = false;
static uint8_t current_keys = 0;

// 校准参数
static float base_pitch = 0.0f;
static float base_roll = 0.0f;

// WiFi UDP
static WiFiUDP udp;
static const char* ssid = WIFI_SSID;
static const char* password = WIFI_PASSWORD;

// 连接控制
static unsigned long connecting_start_time = 0;
static int reconnect_count = 0;
static bool wifi_ever_connected = false;

// 超时定义
#define WIFI_CONNECT_TIMEOUT_MS  10000   // 10秒连接超时
#define WIFI_MAX_RECONNECT      3       // 最大重试次数

// ============================================================
// 工具函数
// ============================================================
static uint8_t keys_to_byte(ControllerKey_t key) {
    switch (key) {
        case KEY_UP:    return 0x01;
        case KEY_DOWN:  return 0x02;
        case KEY_LEFT:  return 0x04;
        case KEY_RIGHT: return 0x08;
        case KEY_A:     return 0x10;
        default:        return 0x00;
    }
}

static void send_key_event(ControllerKey_t key, bool press) {
    char msg[32];
    const char* key_name;
    
    switch (key) {
        case KEY_UP:    key_name = "UP";    break;
        case KEY_DOWN:  key_name = "DOWN";  break;
        case KEY_LEFT:  key_name = "LEFT";  break;
        case KEY_RIGHT: key_name = "RIGHT"; break;
        case KEY_A:     key_name = "A";     break;
        default:        return;
    }
    
    if (press) {
        snprintf(msg, sizeof(msg), "PRESS:%s", key_name);
    } else {
        snprintf(msg, sizeof(msg), "RELEASE:%s", key_name);
    }
    
    // 直接发送到 PC 的固定 IP（单播）
    IPAddress pc_ip(192, 168, 101, 2);
    udp.beginPacket(pc_ip, WIFI_TARGET_PORT);
    udp.print(msg);
    udp.endPacket();
    
    Serial.print("[WiFiCtrl] ");
    Serial.println(msg);
}

static void send_all_keys_released(void) {
    char msg[32];
    snprintf(msg, sizeof(msg), "RELEASE:ALL");
    IPAddress pc_ip(192, 168, 101, 2);
    udp.beginPacket(pc_ip, WIFI_TARGET_PORT);
    udp.print(msg);
    udp.endPacket();
    Serial.println("[WiFiCtrl] RELEASE:ALL");
}

// ============================================================
// WiFi 连接管理
// ============================================================
static void start_wifi_connection(void) {
    connecting_start_time = millis();
    
    Serial.print("[WiFiCtrl] 正在连接 WiFi: ");
    Serial.println(ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
}

static void handle_connecting(void) {
    wl_status_t status = WiFi.status();
    
    switch (status) {
        case WL_CONNECTED: {
            wifi_ever_connected = true;
            reconnect_count = 0;
            
            Serial.print("[WiFiCtrl] WiFi连接成功! IP: ");
            Serial.println(WiFi.localIP());
            Serial.print("[WiFiCtrl] Gateway: ");
            Serial.println(WiFi.gatewayIP());
            
            // 启动 UDP
            udp.begin(WIFI_LOCAL_PORT);
            Serial.println("[WiFiCtrl] UDP已启动");
            
            // 立即完成校准
            calibrated = true;
            state = WIFI_CTRL_RUNNING;
            base_pitch = IMU_me.pitch;
            base_roll = IMU_me.roll;
            
            Output_Feedback(FEEDBACK_SUCCESS);
            Serial.print("[WiFiCtrl] 基准已锁定! base_pitch=");
            Serial.print(base_pitch, 1);
            Serial.print(" base_roll=");
            Serial.println(base_roll, 1);
            Serial.println("[WiFiCtrl] 倾斜控制方向键");
            break;
        }
        
        case WL_IDLE_STATUS:
        case WL_DISCONNECTED: {
            // 连接中或断开，检查超时
            unsigned long elapsed = millis() - connecting_start_time;
            
            if (elapsed > WIFI_CONNECT_TIMEOUT_MS) {
                reconnect_count++;
                
                if (reconnect_count >= WIFI_MAX_RECONNECT) {
                    Serial.println("[WiFiCtrl] 连接失败，已达最大重试次数!");
                    state = WIFI_CTRL_ERROR;
                    Output_Feedback(FEEDBACK_ERROR);
                } else {
                    Serial.print("[WiFiCtrl] 连接超时，正在重试 (");
                    Serial.print(reconnect_count);
                    Serial.print("/");
                    Serial.print(WIFI_MAX_RECONNECT);
                    Serial.println(")...");
                    
                    WiFi.disconnect();
                    delay(100);
                    start_wifi_connection();
                }
            }
            
            // 每秒打印一次状态
            if (elapsed % 1000 < 50) {
                Serial.print("[WiFiCtrl] 等待连接... (");
                Serial.print(elapsed / 1000);
                Serial.println("s)");
            }
            break;
        }
        
        case WL_NO_SSID_AVAIL:
            Serial.println("[WiFiCtrl] 错误: 找不到WiFi网络!");
            state = WIFI_CTRL_ERROR;
            break;
            
        case WL_CONNECT_FAILED:
            Serial.println("[WiFiCtrl] 错误: 连接失败!");
            state = WIFI_CTRL_ERROR;
            break;
            
        default:
            break;
    }
}

// ============================================================
// ============================================================
// 按键映射逻辑
// ============================================================
static ControllerKey_t map_tilt_to_key(void) {
    float adjusted_pitch = IMU_me.pitch - base_pitch;
    float adjusted_roll = IMU_me.roll - base_roll;
    
    // 方向映射（根据用户要求）
    // 右倾(pitch↑) → 右, 左倾(pitch↓) → 左
    // 前倾(roll↑) → 上, 后倾(roll↓) → 下
    
    if (adjusted_pitch > TILT_THRESHOLD) {
        return KEY_RIGHT;   // 右倾
    } else if (adjusted_pitch < -TILT_THRESHOLD) {
        return KEY_LEFT;    // 左倾
    }
    
    if (adjusted_roll > TILT_THRESHOLD) {
        return KEY_UP;      // 前倾
    } else if (adjusted_roll < -TILT_THRESHOLD) {
        return KEY_DOWN;    // 后倾
    }
    
    return KEY_NONE;
}

static void update_keys(void) {
    ControllerKey_t new_key = map_tilt_to_key();
    uint8_t new_keys = keys_to_byte(new_key);
    
    // 检测按键变化
    uint8_t changed = current_keys ^ new_keys;
    
    if (changed) {
        // 释放不再按下的键
        if ((changed & 0x01) && !(new_keys & 0x01)) send_key_event(KEY_UP, false);
        if ((changed & 0x02) && !(new_keys & 0x02)) send_key_event(KEY_DOWN, false);
        if ((changed & 0x04) && !(new_keys & 0x04)) send_key_event(KEY_LEFT, false);
        if ((changed & 0x08) && !(new_keys & 0x08)) send_key_event(KEY_RIGHT, false);
        
        // 按下新键
        if ((changed & 0x01) && (new_keys & 0x01)) send_key_event(KEY_UP, true);
        if ((changed & 0x02) && (new_keys & 0x02)) send_key_event(KEY_DOWN, true);
        if ((changed & 0x04) && (new_keys & 0x04)) send_key_event(KEY_LEFT, true);
        if ((changed & 0x08) && (new_keys & 0x08)) send_key_event(KEY_RIGHT, true);
        
        current_keys = new_keys;
    }
}

// ============================================================
// 运行中循环
// ============================================================
static void running_loop(void) {
    // 摇晃 → A 键（按下后50ms自动释放）
    static bool shake_was_triggered = false;
    static unsigned long shake_time = 0;
    
    if (IMU_me.motion == MOTION_SHAKE && !shake_was_triggered) {
        shake_was_triggered = true;
        shake_time = millis();
        send_key_event(KEY_A, true);
        Output_Feedback(FEEDBACK_CLICK);
    }
    
    // 50ms后自动释放
    if (shake_was_triggered && (millis() - shake_time >= 50)) {
        shake_was_triggered = false;
        send_key_event(KEY_A, false);
    }
    
    update_keys();
}

// ============================================================
// 公开 API
// ============================================================
void WifiController_Init(void) {
    Serial.println("[WiFiCtrl] ====================");
    Serial.println("[WiFiCtrl] WiFi控制器初始化");
    Serial.print("[WiFiCtrl] SSID: ");
    Serial.println(ssid);
    Serial.print("[WiFiCtrl] 目标PC: ");
    Serial.print(WIFI_TARGET_IP);
    Serial.print(":");
    Serial.println(WIFI_TARGET_PORT);
    Serial.println("[WiFiCtrl] ====================");
    
    state = WIFI_CTRL_CONNECTING;
    calibrated = false;
    current_keys = 0;
    reconnect_count = 0;
    wifi_ever_connected = false;
    
    // 不配置静态 IP，使用 DHCP 自动获取
    // 删除 WiFi.config() 调用
    start_wifi_connection();
}

void WifiController_Update(void) {
    switch (state) {
        case WIFI_CTRL_CONNECTING:
            handle_connecting();
            break;
            
        case WIFI_CTRL_RUNNING:
            running_loop();
            break;
            
        case WIFI_CTRL_ERROR:
            // 错误状态，等待摇晃退出
            if (IMU_me.motion == MOTION_SHAKE) {
                state = WIFI_CTRL_CONNECTING;
                reconnect_count = 0;
                start_wifi_connection();
            }
            break;
            
        default:
            break;
    }
}

void WifiController_Exit(void) {
    send_all_keys_released();
    
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
    }
    
    udp.stop();
    
    state = WIFI_CTRL_CONNECTING;
    calibrated = false;
    current_keys = 0;
    
    Serial.println("[WiFiCtrl] 已退出控制模式");
}

WifiControllerState_t WifiController_GetState(void) {
    return state;
}

bool WifiController_IsCalibrated(void) {
    return calibrated;
}
