// game_balance.h - 平衡球游戏
#ifndef GAME_BALANCE_H
#define GAME_BALANCE_H

#include "system_state.h"
#include <stdint.h>

// 游戏配置
#define BALANCE_ARENA_WIDTH   110   // 场地宽度（像素）
#define BALANCE_ARENA_HEIGHT  110   // 场地高度（像素）
#define BALL_RADIUS           5     // 球半径（像素）
#define GOAL_SIZE             16    // 终点区域大小

// 物理参数
#define BALL_SPEED_BASE       3.0f  // 基础速度
#define FRICTION              0.96f // 摩擦力

// 游戏状态
typedef struct {
    float ball_x, ball_y;           // 球位置（相对于场地左上角）
    float ball_vx, ball_vy;         // 球速度
    float goal_x, goal_y;           // 终点位置（相对于场地左上角）
    uint32_t score;                 // 当前分数
    uint32_t start_time;            // 开始时间
    uint8_t state;                  // 0=playing, 1=won
    uint8_t level;                  // 关卡数
} BalanceGame_t;

// 接口函数
void GameBalance_Init(void);
SystemState_t GameBalance_Update(void);
void GameBalance_Draw(void);
int GameBalance_GetScore(void);

#endif
