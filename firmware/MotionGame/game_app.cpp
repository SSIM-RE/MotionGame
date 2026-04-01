// game_app.cpp - 游戏统一入口
#include "game_app.h"
#include "game_snake.h"
#include "game_balance.h"
#include "game_race.h"
#include "display_driver.h"
#include "motion_service.h"
#include "cloud_api.h"
#include <Arduino.h>

// 当前运行的游戏ID
static GameID_t current_game = GAME_NONE;
static bool game_is_running = false;
static bool game_is_paused = false;

// 统一接口：根据游戏ID调用对应游戏
static void (*game_init_fn)(void) = NULL;
static SystemState_t (*game_update_fn)(void) = NULL;
static int (*game_score_fn)(void) = NULL;

/* ================= 接口函数 ================= */
void GameApp_Init(GameID_t game_id) {
    current_game = game_id;
    game_is_running = true;
    game_is_paused = false;
    
    // 根据游戏ID选择对应的函数
    switch (game_id) {
        case GAME_SNAKE:
            GameSnake_Init();
            game_init_fn = NULL;
            game_update_fn = GameSnake_Update;
            game_score_fn = GameSnake_GetScore;
            break;
            
        case GAME_BALANCE:
            GameBalance_Init();
            game_init_fn = NULL;
            game_update_fn = GameBalance_Update;
            game_score_fn = GameBalance_GetScore;
            break;
            
        case GAME_RACE:
            GameRace_Init();
            game_init_fn = NULL;
            game_update_fn = GameRace_Update;
            game_score_fn = GameRace_GetScore;
            break;
            
        // 其他游戏未实现
        default:
            game_is_running = false;
            game_update_fn = NULL;
            game_score_fn = NULL;
            break;
    }
    
    Serial.print("GameApp Init: ");
    Serial.println(game_id);
}

SystemState_t GameApp_Update(void) {
    if (!game_is_running || game_update_fn == NULL) {
        return SYS_NONE;
    }
    
    // 调用当前游戏的Update
    return game_update_fn();
}

void GameApp_Pause(void) {
    game_is_paused = true;
}

void GameApp_Resume(void) {
    game_is_paused = false;
}

int GameApp_GetScore(void) {
    if (game_score_fn != NULL) {
        return game_score_fn();
    }
    return 0;
}

int GameApp_GetHighScore(void) {
    // TODO: 从 EEPROM 或 Flash 读取最高分
    return 0;
}

void GameApp_Reset(void) {
    if (current_game != GAME_NONE) {
        GameApp_Init(current_game);
    }
}