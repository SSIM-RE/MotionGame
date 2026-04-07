#ifndef GAME_RACE_H
#define GAME_RACE_H

#include "system_state.h"
#include <stdint.h>
#include <stdbool.h>

/* ================= 游戏显示配置 ================= */
#define RACE_SCREEN_W         128
#define RACE_SCREEN_H         160

/* ================= 赛道物理配置 ================= */
#define RACE_SEGMENT_COUNT    60       
#define RACE_DRAW_DISTANCE    40       
#define RACE_SEGMENT_LENGTH   200.0f   
#define RACE_ROAD_WIDTH       2000.0f  

// 动态双色天空调色板
#define SKY_NIGHT_TOP   0x0000  
#define SKY_NIGHT_BOT   0x000B  
#define SKY_RISE_TOP    0x1010  
#define SKY_RISE_BOT    0xE3E9  
#define SKY_NOON_TOP    0x0217  
#define SKY_NOON_BOT    0x867F  
#define SKY_SET_TOP     0x500A  
#define SKY_SET_BOT     0xFBA0  

#define RACE_COLOR_GRASS1     0x04A0
#define RACE_COLOR_GRASS2     0x05E0
#define RACE_COLOR_ROAD1      0x6B4D
#define RACE_COLOR_ROAD2      0x7BEF
#define RACE_COLOR_RUMBLE1    0xF800
#define RACE_COLOR_RUMBLE2    0xFFFF
#define RACE_COLOR_LINE       0xFFFF

// ★ 新增：景物类型定义
#define OBJ_NONE         0
#define OBJ_TREE_PIN     1
#define OBJ_PALM         2
#define OBJ_SIGN         3
#define OBJ_ROCK         4

typedef struct {
    float y;           
    float curve;       
    // ★ 新增：赛道段关联的公告板物体信息
    int8_t object_type;  
    int8_t object_side;  // -1 for left, 1 for right
    float object_scale;  
} RaceSegment_t;

typedef struct {
    RaceSegment_t segments[RACE_SEGMENT_COUNT]; 
    
    // 摄像机与玩家状态 (纯粹的 X/Z 轴体系)
    float cam_z;           
    float speed;           
    float accel;           
    float player_x;        
    float total_distance;  
    uint32_t global_idx;   
    
    // IMU 动态基准系统
    bool waiting_to_start;  
    float base_roll;        
    float base_pitch;       
    uint32_t state_timer;   
    
    // 环境与时间系统
    float time_of_day;         
    uint16_t current_sky_top;  
    uint16_t current_sky_bot;  
    float sky_offset;      // ★ BojanSof 模型：天空的 X 轴偏移量
    
    // 地形生成器
    uint8_t gen_phase;     
    int gen_count;
    int gen_total;
    float cur_curve, next_curve;
    float cur_hill, next_hill;
    
    // 动态参数
    float max_speed;       
    float cam_height;      
    float fov;             
    float steering_sensitivity; 
    
    uint32_t last_update;
    bool initialized;
} RaceGame_t;

void GameRace_Init(void);
SystemState_t GameRace_Update(void);
void GameRace_HandleSerial(void);
int GameRace_GetScore(void);

#endif