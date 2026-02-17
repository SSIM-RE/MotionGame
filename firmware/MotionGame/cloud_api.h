// cloud_api.h - 云端接口（预留）
#ifndef CLOUD_API_H
#define CLOUD_API_H

#include <stdint.h>

// ⭐ 云端功能开关
// 单机版设为 0，联网版设为 1
#define CLOUD_ENABLED 0

// 玩家数据结构
typedef struct {
    char device_id[32];     // 设备唯一标识
    uint32_t score;         // 当前分数
    uint32_t high_score;    // 历史最高
    uint64_t timestamp;     // 时间戳（Unix 时间戳）
    uint8_t game_id;        // 游戏 ID
} PlayerData_t;

// ⭐ 核心接口函数
// 单机版返回成功但不实际联网，联网版实现真实功能

/**
 * @brief 初始化云端模块
 * @return 0=成功，其他=失败
 */
int Cloud_Init(void);

/**
 * @brief 上传分数到云端
 * @param data 玩家数据
 * @return 0=成功，其他=失败
 */
int Cloud_UploadScore(PlayerData_t* data);

/**
 * @brief 从云端获取排行榜
 * @param top_scores 存储排行榜的数组
 * @param count 要获取的数量
 * @return 0=成功，其他=失败
 */
int Cloud_DownloadRanking(uint32_t* top_scores, int count);

/**
 * @brief 同步 NTP 时间
 * @return 0=成功，其他=失败
 */
int Cloud_SyncTime(void);

// ⭐ 辅助函数

/**
 * @brief 获取设备唯一 ID
 * @return 设备 ID 字符串
 */
const char* Cloud_GetDeviceID(void);

/**
 * @brief 检查云端连接状态
 * @return 1=已连接，0=未连接
 */
int Cloud_IsConnected(void);

/**
 * @brief 获取本地高分（单机版使用）
 * @param game_id 游戏 ID
 * @return 高分值
 */
uint32_t Cloud_GetLocalHighScore(uint8_t game_id);

/**
 * @brief 保存本地高分（单机版使用）
 * @param game_id 游戏 ID
 * @param score 分数
 */
void Cloud_SaveLocalHighScore(uint8_t game_id, uint32_t score);

#endif // CLOUD_API_H
