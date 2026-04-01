#ifndef GAME_BALANCE_H
#define GAME_BALANCE_H

#include "system_state.h"
#include <stdint.h>

#define BALANCE_ARENA_WIDTH   110
#define BALANCE_ARENA_HEIGHT  110
#define BALL_RADIUS           5
#define GOAL_SIZE             16
#define BALL_SPEED_BASE       3.0f
#define FRICTION              0.96f

typedef struct {
    float ball_x, ball_y;
    float ball_vx, ball_vy;
    float goal_x, goal_y;
    uint32_t score;
    uint32_t start_time;
    uint8_t state;
    uint8_t level;
} BalanceGame_t;

void GameBalance_Init(void);
SystemState_t GameBalance_Update(void);
void GameBalance_Draw(void);
int GameBalance_GetScore(void);

#endif