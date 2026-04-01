#ifndef GAME_SNAKE_H
#define GAME_SNAKE_H

#include "system_state.h"
#include <stdint.h>

/* ================= 游戏配置 ================= */
#define SNAKE_GRID_SIZE     4
#define SNAKE_MAX_LENGTH    100
#define SNAKE_MOVE_INTERVAL 200  // ms

/* ================= 游戏状态 ================= */
typedef struct {
    int x[SNAKE_MAX_LENGTH];
    int y[SNAKE_MAX_LENGTH];
    int length;
    int food_x;
    int food_y;
    int direction;        // 0:右, 1:下, 2:左, 3:上
    uint32_t last_move;    // 上次移动时间
} SnakeGame_t;

/* ================= API 函数 ================= */
void GameSnake_Init(void);
SystemState_t GameSnake_Update(void);
int GameSnake_GetScore(void);

#endif