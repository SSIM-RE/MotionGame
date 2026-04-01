// game_app.h - 游戏统一入口
#ifndef GAME_APP_H
#define GAME_APP_H

#include "system_state.h"

/* ================= 游戏控制函数 ================= */
void GameApp_Init(GameID_t game_id);
SystemState_t GameApp_Update(void);
void GameApp_Pause(void);
void GameApp_Resume(void);
int GameApp_GetScore(void);
int GameApp_GetHighScore(void);
void GameApp_Reset(void);

/* ================= 游戏选择 ================= */
// 使用 game_select_app.h 中的界面

#endif