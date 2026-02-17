// cloud_api.cpp - 云端接口实现（单机版）
#include "cloud_api.h"
#include <Arduino.h>
#include <string.h>

// 本地高分存储（后续可改用 SPIFFS/EEPROM）
static uint32_t local_high_scores[10] = {0};

// 设备 ID（使用 ESP32 MAC 地址）
static char device_id[32] = "UNKNOWN";

/**
 * @brief 初始化设备 ID
 */
static void init_device_id(void) {
    // 使用 ESP32 的 MAC 地址作为设备 ID
    uint8_t mac[6];
    // esp_read_mac(mac, ESP_MAC_WIFI_STA);  // 需要 ESP-IDF
    // 临时使用随机 ID
    snprintf(device_id, sizeof(device_id), "MOTION_GAME_%04X", 
             (unsigned int)(millis() & 0xFFFF));
}

int Cloud_Init(void) {
    #if CLOUD_ENABLED
        // TODO: 初始化 WiFi
        // WiFi.begin(ssid, password);
        // while (WiFi.status() != WL_CONNECTED) { delay(500); }
        
        // TODO: 初始化 MQTT/HTTP 客户端
        Serial.println("Cloud: WiFi + MQTT initialized");
    #else
        Serial.println("Cloud: Disabled (single-player mode)");
    #endif
    
    init_device_id();
    return 0;
}

int Cloud_UploadScore(PlayerData_t* data) {
    if (!data) return -1;
    
    #if CLOUD_ENABLED
        // TODO: 通过 MQTT/HTTP 上传分数
        // mqtt.publish("motiongame/score", json_data);
        
        Serial.printf("Cloud: Uploading score %d for device %s\n", 
                      data->score, data->device_id);
        return 0;
    #else
        // 单机版：只打印，不实际上传
        Serial.printf("[Cloud Disabled] Score: %d (not uploaded)\n", data->score);
        
        // 但可以保存本地高分
        if (data->score > local_high_scores[data->game_id]) {
            local_high_scores[data->game_id] = data->score;
            Serial.printf("New High Score: %d\n", data->score);
        }
        
        return 0;
    #endif
}

int Cloud_DownloadRanking(uint32_t* top_scores, int count) {
    if (!top_scores || count <= 0) return -1;
    
    #if CLOUD_ENABLED
        // TODO: 从云端下载排行榜
        // mqtt.subscribe("motiongame/ranking");
        
        Serial.println("Cloud: Downloading ranking...");
        return 0;
    #else
        // 单机版：返回本地高分
        Serial.println("[Cloud Disabled] Using local high scores");
        for (int i = 0; i < count && i < 10; i++) {
            top_scores[i] = local_high_scores[i];
        }
        return 0;
    #endif
}

int Cloud_SyncTime(void) {
    #if CLOUD_ENABLED
        // TODO: 使用 NTP 同步时间
        // configTime(gmtOffset, daylightOffset, ntpServer);
        
        Serial.println("Cloud: Syncing time via NTP...");
        return 0;
    #else
        // 单机版：使用 millis() 计时
        Serial.println("[Cloud Disabled] Using millis() for timing");
        return 0;
    #endif
}

const char* Cloud_GetDeviceID(void) {
    return device_id;
}

int Cloud_IsConnected(void) {
    #if CLOUD_ENABLED
        // TODO: 检查 WiFi 连接状态
        // return (WiFi.status() == WL_CONNECTED) ? 1 : 0;
        return 1;
    #else
        return 0;
    #endif
}

uint32_t Cloud_GetLocalHighScore(uint8_t game_id) {
    if (game_id >= 10) return 0;
    return local_high_scores[game_id];
}

void Cloud_SaveLocalHighScore(uint8_t game_id, uint32_t score) {
    if (game_id >= 10) return;
    
    if (score > local_high_scores[game_id]) {
        local_high_scores[game_id] = score;
        Serial.printf("High Score saved for game %d: %d\n", game_id, score);
    }
}
